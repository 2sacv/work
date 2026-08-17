/*
 *       Filename:  waytronic_uart.c
 *    Description:
 *        Version:  1.0
 *        Created:  03/02/2026 04:13:53 PM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  xiangyp (),
 *   Organization:
 */

#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include "jpwm.h"
#include "uart.h"
#include "debug.h"
#include "utils.h"
#include "confapi.h"
#include "ptz_ctrl.h"
#include "system_sch.h"
#include "jconfstruct.h"
#include "js_scheduler.h"
#include "waytronic_uart.h"

//唯创知音的音频处理芯片，串口交互协议如下所示：
//| head  | len  | cmd_type  | cmd       | verify | tail |
//| :---- | :--- | :-------- | :-------  | :----- | :--- |
//| 0x7F  | 0x06 | 0xFF 0x06 | 0xxx 0xxx |  0xxx  | 0xEF |
//
//1. head:     头部校验位，长度为 1 字节，固定为 0x7F
//2. len:      除去头部校验位和尾部校验位的长度，长度为 1 字节，固定为 0x06
//3. cmd_type: 命令的类型，长度为 2 字节，固定为 0xFF 0x06
//4. cmd:      具体的命令，长度为 2 字节，值不固定，需根据协议手册来定
//5. verify:   除去头部校验位、尾部校验位及其本身之外的其它成员的值加在一起的和(不考虑溢出，0~255 范围)，长度为 1 字节，值不固定，需手动计算
//6. tail:     尾部校验位，长度为 1 字节，固定为 0xEF
//
//具体拟定的协议内容，可参考 WT260105-33-K3S001V1.00 深圳捷高电子-云台-WTK6900HC-32N-刘荣坚20260105.pdf，
//或者本文件已经拟定好的 g_wtcmd_maps

#define TTY_ASR                 "/dev/ttyS0"
#define BAUDRATE_ASR            (9600)
#define BYTES_RECV_MAX          (512)
#define BYTES_PER_CMD_PACK      (8)

#define CMD_HEAD                (0x7E)
#define CMD_LEN                 (0x06)
#define CMD_SET_HIGH            (0xFF)
#define CMD_SET_LOW             (0x06)
#define CMD_END                 (0xEF)

#define BYTES_VERIFY            (5)

#define IDX_HEAD                (1)
#define IDX_CMD_HIGH            (4)
#define IDX_CMD_LOW             (5)
#define IDX_VERIFY              (IDX_HEAD + BYTES_VERIFY)

#define PCT_WHTLVL_MAX          (100)
#define PCT_WHTLVL_MIN          (0)
#define PCT_WHTLVL_ADD          (10)
#define PCT_WHTLVL_DEC          (10)

//1 / 24 000 000 约等于 41.67ns，取整为 42
#define NS_PERIOD_WAYTRONIC     (42)
//标准时钟设置为占空比取一半，高低电平各 1/2
#define DUTYCYCLE_WAYTRONIC     (NS_PERIOD_WAYTRONIC / 2)

#define MS_INTV_CLR_MULTIPLE    (20 * 1000)

typedef int (*cb_handle_wtcmd)(void *usr_data);

typedef struct {
    char    cmd_func[32];
    uint8_t cmd_high;
    uint8_t cmd_low;
    cb_handle_wtcmd cb_hdl_wtcmd;
} sWTCmdMaps;

typedef struct {
    uint8_t idx;
    uint8_t val;
} sFixedCmdMaps;

typedef struct {
    int fd;
    JSScheduler sch;
    JSTCHandle  hdl_stop_motor;
    JSRWHandle  hdl_rw;
} sWTRun;

static sWTRun g_run_wt = {
    .fd = -1,
};

static void lgtextcfg_set_whitelevel(int grade, int added_mode);
#define lgtextcfg_add_whitegrade(grade) lgtextcfg_set_whitelevel(grade, TRUE)
#define lgtextcfg_set_whitegrade(grade) lgtextcfg_set_whitelevel(grade, FALSE)

static int cb_handle_cmd_wakeup(void *usr_data)
{
    int ret = SUCCESS;

    pri_asr(LVL_DBG, "%s\n", __func__);
    encode_audio_queue_push_amr(AUDIO_ASR_LM_HERE, TRUE); //我在的主人

    return ret;
}

static int cb_handle_cmd_sleep(void *usr_data)
{
    int ret = SUCCESS;

    pri_asr(LVL_DBG, "%s\n", __func__);

    return ret;
}

static int cb_handle_cmd_open_camera(void *usr_data)
{
    int ret = SUCCESS;

    pri_asr(LVL_DBG, "%s\n", __func__);
    encode_audio_queue_push_amr(AUDIO_ASR_OPEN_DEV, TRUE); //好的，设备已打开

    return ret;
}

static int cb_handle_cmd_close_camera(void *usr_data)
{
    int ret = SUCCESS;

    pri_asr(LVL_DBG, "%s\n", __func__);
    encode_audio_queue_push_amr(AUDIO_ASR_OFF_DEV, TRUE); //好的，设备已关闭

    return ret;
}

void audiocfg_set_outvolume(int grade)
{
    AudioCfgS audcfg = {0};

    conf_get_audiocfg(&audcfg);

    audcfg.outvolume = RANGE(grade, PCT_VOLUME_MIN, PCT_VOLUME_MAX);
    audcfg.talkvolume = RANGE(grade, PCT_VOLUME_MIN, PCT_VOLUME_MAX);
    DBG("set out volume to %d, talk volume to %d\n",
        audcfg.outvolume, audcfg.talkvolume);

    conf_set_audiocfg(audcfg);

    //encode_audio_queue_push_amr(AUDIO_ASR_OK, TRUE);
}

void audiocfg_add_outvolume(int grade)
{
    AudioCfgS audcfg = {0};

    conf_get_audiocfg(&audcfg);

    audcfg.outvolume += grade;
    audcfg.talkvolume += grade;
    audcfg.outvolume = RANGE(audcfg.outvolume, PCT_VOLUME_MIN, PCT_VOLUME_MAX);
    audcfg.talkvolume = RANGE(audcfg.talkvolume, PCT_VOLUME_MIN, PCT_VOLUME_MAX);
    DBG("add out volume to %d, talk volume to %d\n",
        audcfg.outvolume, audcfg.talkvolume);

    conf_set_audiocfg(audcfg);

    //// 调小音量
    //if (grade < 0) {
    //    if (PCT_VOLUME_MIN == audcfg.outvolume) {
    //        encode_audio_queue_push_amr(AUDIO_VOLUM_MUTE, TRUE);  // 已静音
    //    } else {
    //        encode_audio_queue_push_amr(AUDIO_VOLUM_DEC, TRUE);   // 音量已调小
    //    }
    //// 调大音量
    //} else if (grade > 0) {
    //    // 到 100% 音量，提示“音量已调到最大”
    //    if (PCT_VOLUME_MAX == audcfg.outvolume) {
    //        encode_audio_queue_push_amr(AUDIO_VOLUME_MAX, TRUE);  // 音量已调至最大
    //    } else {
    //        encode_audio_queue_push_amr(AUDIO_VOLUM_INC, TRUE);   // 音量已调大
    //    }
    //}
}

static void lgtextcfg_set_whitelevel(int grade, int added_mode)
{
    LightExtCfg lgtextcfg = {0};

    conf_get_lightext_cfg(&lgtextcfg);

    if (added_mode) {
        lgtextcfg.nightled += grade;
    } else {
        lgtextcfg.nightled = grade;
    }

    lgtextcfg.nightled = RANGE(lgtextcfg.nightled, PCT_WHTLVL_MIN, PCT_WHTLVL_MAX);

    conf_set_lightext_cfg(lgtextcfg);

    if (grade < 0) {
        if (PCT_WHTLVL_MIN == lgtextcfg.nightled) {
            encode_audio_queue_push_amr(AUDIO_NIGHT_LIGHT_OFF, TRUE); // 已关闭小夜灯
        } else {
            encode_audio_queue_push_amr(AUDIO_LIGHT_DEC, TRUE);       // 灯光已调暗
        }
    } else if (grade > 0) {
        if (PCT_WHTLVL_MAX == lgtextcfg.nightled) {
            encode_audio_queue_push_amr(AUDIO_LIGHT_MAX, TRUE);       // 亮度已调至最大
        } else {
            encode_audio_queue_push_amr(AUDIO_LIGHT_INC, TRUE);       // 灯光已调亮
        }
    }
}

static void cb_stop_motor(void *usr_data)
{
	PelcoCmd pelco_ctrl = {0};

    pelco_ctrl.type = 1;
    pelco_ctrl.cmd = 9;

    peclo_cmd_enqueue(&pelco_ctrl);
}

static void move_motor_through_cmd(eCmdType1 cmd, int auto_stop)
{
    static struct timespec clk_prev = {0};
    static eCmdType1 cmd_prev = E_MOVE_NONE;
    static size_t cnt_multiple = 1;

	PelcoCmd pelco_ctrl = {0};
    MotorStatus status = {0};
    motor_t motorcfg = {0};

    get_motor_status(&status);
    conf_get_motorcfg(&motorcfg);

    int hor = status.steps[PAN_MOTOR];
    int ver = status.steps[TILT_MOTOR];

    if (NULL != g_run_wt.hdl_stop_motor) {
        WAR("last pelco ctrl not finished, ignore this time\n");
        goto exit;
    }

    if (ms_since_previous2(&clk_prev) >= MS_INTV_CLR_MULTIPLE) {
        cnt_multiple = 1;
    }

    switch (cmd) {
    case E_MOVE_UP:
        if (0 == ver) {
            encode_audio_queue_push_amr(AUDIO_TURN_MAX_UP, TRUE);   // 已转至最上边
            goto exit;
        } else {
            encode_audio_queue_push_amr(AUDIO_TURN_UP, TRUE);       // 好的向上
        }

        pelco_ctrl.data2 = 32;
        break;
    case E_MOVE_DOWN: {
        if (motorcfg.v_maxstep == ver) {
            encode_audio_queue_push_amr(AUDIO_TURN_MAX_DOWN, TRUE); // 已转至最下边
            goto exit;
        } else {
            encode_audio_queue_push_amr(AUDIO_TURN_DOWN, TRUE);     // 好的向下
        }

        pelco_ctrl.data2 = 32;
        break;
    }
    case E_MOVE_LEFT:
        if (0 == hor) {
            encode_audio_queue_push_amr(AUDIO_TURN_MAX_LEFT, TRUE); // 已转至最左边
            goto exit;
        } else {
            encode_audio_queue_push_amr(AUDIO_TURN_LEFT, TRUE);     // 好的左转
        }

        pelco_ctrl.data1 = 32;
        break;
    case E_MOVE_RIGHT: {
        if (motorcfg.h_maxstep == hor) {
            encode_audio_queue_push_amr(AUDIO_TURN_MAX_RIGHT, TRUE);// 已转至最右边
            goto exit;
        } else {
            encode_audio_queue_push_amr(AUDIO_TURN_RIGHT, TRUE);    // 好的右转
        }

        pelco_ctrl.data1 = 32;
        break;
    }
    default: {
        ERR("bad cmd %d\n", cmd);
        goto exit;
    }
    }

    pelco_ctrl.type = 1;
    pelco_ctrl.cmd = cmd;

    //用户多次想要往同一个方向转，策略为越转角度越大
    if (cmd_prev == cmd) {
        cnt_multiple++;
    } else {
        cnt_multiple = 1;
        cmd_prev = cmd;
    }

    peclo_cmd_enqueue(&pelco_ctrl);

    if (auto_stop) {
        js_create_once(g_run_wt.hdl_stop_motor, g_run_wt.sch, cnt_multiple * 500,
                       cb_stop_motor, NULL);
    }

exit:

    return;
}

static int cb_handle_cmd_turn_up_volume(void *usr_data)
{
    pri_asr(LVL_DBG, "%s\n", __func__);

    audiocfg_add_outvolume(PCT_VOLUME_ADD);

    return SUCCESS;
}

static int cb_handle_cmd_turn_down_volume(void *usr_data)
{
    pri_asr(LVL_DBG, "%s\n", __func__);

    audiocfg_add_outvolume(-PCT_VOLUME_DEC);

    return SUCCESS;
}

static int cb_handle_cmd_mute(void *usr_data)
{
    pri_asr(LVL_DBG, "%s\n", __func__);

    audiocfg_add_outvolume(-PCT_VOLUME_MAX);

    return SUCCESS;
}

static int cb_handle_cmd_turn2max_volume(void *usr_data)
{
    int ret = SUCCESS;

    pri_asr(LVL_DBG, "%s\n", __func__);

    audiocfg_add_outvolume(PCT_VOLUME_MAX);

    return ret;
}

static int cb_handle_cmd_turn_on_light(void *usr_data)
{
    int ret = SUCCESS;

    pri_asr(LVL_DBG, "%s\n", __func__);

    lgtextcfg_add_whitegrade(PCT_WHTLVL_MAX);

    return ret;
}

static int cb_handle_cmd_turn_off_light(void *usr_data)
{
    int ret = SUCCESS;

    pri_asr(LVL_DBG, "%s\n", __func__);

    lgtextcfg_add_whitegrade(-PCT_WHTLVL_MAX);

    return ret;
}

static int cb_handle_cmd_turn_up_light(void *usr_data)
{
    int ret = SUCCESS;

    pri_asr(LVL_DBG, "%s\n", __func__);

    lgtextcfg_add_whitegrade(PCT_WHTLVL_ADD);

    return ret;
}

static int cb_handle_cmd_turn_down_light(void *usr_data)
{
    int ret = SUCCESS;

    pri_asr(LVL_DBG, "%s\n", __func__);

    lgtextcfg_add_whitegrade(-PCT_WHTLVL_DEC);

    return ret;
}

static int cb_handle_cmd_turn2min_light(void *usr_data)
{
    int ret = SUCCESS;

    pri_asr(LVL_DBG, "%s\n", __func__);

    lgtextcfg_set_whitegrade(PCT_WHTLVL_ADD);

    return ret;
}

static int cb_handle_cmd_turn_left_ptz(void *usr_data)
{
    int ret = SUCCESS;

    pri_asr(LVL_DBG, "%s\n", __func__);

    move_motor_through_cmd(E_MOVE_LEFT, TRUE);

    return ret;
}

static int cb_handle_cmd_turn_right_ptz(void *usr_data)
{
    int ret = SUCCESS;

    pri_asr(LVL_DBG, "%s\n", __func__);

    move_motor_through_cmd(E_MOVE_RIGHT, TRUE);

    return ret;
}

static int cb_handle_cmd_turn_up_ptz(void *usr_data)
{
    int ret = SUCCESS;

    pri_asr(LVL_DBG, "%s\n", __func__);

    move_motor_through_cmd(E_MOVE_UP, TRUE);

    return ret;
}

static int cb_handle_cmd_turn_down_ptz(void *usr_data)
{
    int ret = SUCCESS;

    pri_asr(LVL_DBG, "%s\n", __func__);

    move_motor_through_cmd(E_MOVE_DOWN, TRUE);

    return ret;
}

static int cb_handle_cmd_turn2max_left_ptz(void *usr_data)
{
    int ret = SUCCESS;

    pri_asr(LVL_DBG, "%s\n", __func__);

    move_motor_through_cmd(E_MOVE_LEFT, FALSE);

    return ret;
}

static int cb_handle_cmd_turn2max_right_ptz(void *usr_data)
{
    int ret = SUCCESS;

    pri_asr(LVL_DBG, "%s\n", __func__);

    move_motor_through_cmd(E_MOVE_RIGHT, FALSE);

    return ret;
}

static int cb_handle_cmd_turn2max_up_ptz(void *usr_data)
{
    int ret = SUCCESS;

    pri_asr(LVL_DBG, "%s\n", __func__);

    move_motor_through_cmd(E_MOVE_UP, FALSE);

    return ret;
}

static int cb_handle_cmd_turn2max_down_ptz(void *usr_data)
{
    int ret = SUCCESS;

    pri_asr(LVL_DBG, "%s\n", __func__);

    move_motor_through_cmd(E_MOVE_DOWN, FALSE);

    return ret;
}

static int cb_handle_cmd_call_ws(void *usr_data)
{
    int ret = SUCCESS;

    pri_asr(LVL_DBG, "%s\n", __func__);
    encode_audio_queue_push_amr(AUDIO_CALL_WAIT, TRUE); //正在呼叫中，请稍等

    return ret;
}

static int cb_handle_cmd_hang_up_ws(void *usr_data)
{
    int ret = SUCCESS;

    pri_asr(LVL_DBG, "%s\n", __func__);
    encode_audio_queue_push_amr(AUDIO_CALL_HANGUP, TRUE); //已挂断通话

    return ret;
}

static int cb_handle_cmd_calibrate_ptz(void *usr_data)
{
    int ret = SUCCESS;

    pri_asr(LVL_DBG, "%s\n", __func__);

    motor_reinit();
    encode_audio_queue_push_amr(AUDIO_PTZ_CALIBRATE, TRUE); //云台已校准

    return ret;
}

static sWTCmdMaps g_wtcmd_maps[] = {
    {"WakeUp",           0x01, 0x04, cb_handle_cmd_wakeup},
    {"Sleep",            0x00, 0xFF, cb_handle_cmd_sleep},
    {"OpenCamera",       0x01, 0x05, cb_handle_cmd_open_camera},
    {"CloseCamera",      0x01, 0x06, cb_handle_cmd_close_camera},
    {"TurnUpVolume",     0x01, 0x07, cb_handle_cmd_turn_up_volume},
    {"TurnDownVolume",   0x01, 0x08, cb_handle_cmd_turn_down_volume},
    {"Mute",             0x01, 0x09, cb_handle_cmd_mute},
    {"Turn2MaxVolume",   0x01, 0x0A, cb_handle_cmd_turn2max_volume},
    {"TurnOnLight",      0x01, 0x0B, cb_handle_cmd_turn_on_light},
    {"TurnOffLight",     0x01, 0x0C, cb_handle_cmd_turn_off_light},
    {"TurnUpLight",      0x01, 0x0D, cb_handle_cmd_turn_up_light},
    {"TurnDownLight",    0x01, 0x0E, cb_handle_cmd_turn_down_light},
    {"Turn2MaxLight",    0x01, 0x0F, cb_handle_cmd_turn_on_light},
    {"Turn2MinLight",    0x01, 0x10, cb_handle_cmd_turn2min_light},
    {"TurnLeftPTZ",      0x01, 0x11, cb_handle_cmd_turn_left_ptz},
    {"TurnRightPTZ",     0x01, 0x12, cb_handle_cmd_turn_right_ptz},
    {"TurnUpPTZ",        0x01, 0x13, cb_handle_cmd_turn_up_ptz},
    {"TurnDownPTZ",      0x01, 0x14, cb_handle_cmd_turn_down_ptz},
    {"Turn2MaxLeftPTZ",  0x01, 0x15, cb_handle_cmd_turn2max_left_ptz},
    {"Turn2MaxRightPTZ", 0x01, 0x16, cb_handle_cmd_turn2max_right_ptz},
    {"Turn2MaxUpPTZ",    0x01, 0x17, cb_handle_cmd_turn2max_up_ptz},
    {"Turn2MaxDownPTZ",  0x01, 0x18, cb_handle_cmd_turn2max_down_ptz},
    {"CallWS",           0x01, 0x19, cb_handle_cmd_call_ws},
    {"HangUpWS",         0x01, 0x1A, cb_handle_cmd_hang_up_ws},
    {"CalibratePTZ",     0x01, 0x1B, cb_handle_cmd_calibrate_ptz}
};

static sFixedCmdMaps g_pack_maps[] = {
    {0, CMD_HEAD},
    {1, CMD_LEN},
    {2, CMD_SET_HIGH},
    {3, CMD_SET_LOW},
    {7, CMD_END}
};

static void cb_read_asr_cmd(int fd, int events, void *usr_data)
{
    static uint8_t buf_recved[BYTES_RECV_MAX] = {0};
    static size_t bytes_all_recved = 0, bytes_all_cooked = 0;

    uint8_t buf_cur_pack[BYTES_PER_CMD_PACK] = {0};
    size_t bytes_tmp_cooked = 0, bytes_raw = 0;
    int bytes_read = 0, idx_pack = 0, idx_cmd = 0, idx_verify = 0;
    int analyse_err = FALSE, found_cmd = FALSE;
    uint8_t code_calced = 0;

    do {
        bytes_read = read(fd, &buf_recved[bytes_all_recved],
                          sizeof(buf_recved) - bytes_all_recved);
        if (bytes_read < 0) {
            if (EINTR == bytes_read) {
                ms_sleep(10);
                pri_asr(LVL_DBG, "read waytronic inter error: %s\n", strerror(errno));
                continue;
            } else if (EAGAIN == bytes_read) {
                pri_asr(LVL_DBG, "read waytronic again error: %s\n", strerror(errno));
                ms_sleep(10);
                continue;
            } else {
                break;
            }
        } else if (0 == bytes_read) {
            break;
        }

        pri_asr(LVL_DBG, "read %d bytes\n", bytes_read);
        bytes_all_recved += bytes_read;
        bytes_all_recved %= sizeof(buf_recved);
    } while (bytes_read > 0);

    //算出有多少字节可解析
    calc_raw_bytes(sizeof(buf_recved), bytes_all_recved, bytes_all_cooked,
                   &bytes_raw);

    pri_asr(LVL_DBG, "all %d bytes can read\n", bytes_raw);
    for (int idx = 0; idx < ARRAY_SIZE(buf_recved); idx++) {
        if (get_g_log(asr) & LVL_DBG) {
            printf("%02x ", buf_recved[idx]);
        }
    }
    if (get_g_log(asr) & LVL_DBG) {
        printf("\n\n");
    }

    //可解析的长度满足一个包的长度(前提需包的长度固定)
    while (bytes_raw >= sizeof(buf_cur_pack)) {
        //初始化变量
        analyse_err = FALSE;
        bytes_tmp_cooked = bytes_all_cooked;

        //将一个包长度的字节，拷入到 buf_cur_pack
        src_ringbuffer_memcpy((char *)buf_cur_pack, (char *)buf_recved, &bytes_tmp_cooked,
                              sizeof(buf_recved), sizeof(buf_cur_pack));

        //校验固定的内容是否正确
        for (idx_pack = 0; idx_pack < ARRAY_SIZE(g_pack_maps); idx_pack++) {
            if (g_pack_maps[idx_pack].val != buf_cur_pack[g_pack_maps[idx_pack].idx]) {
                ERR("analyse fixed content %u err, shouled be 0x%02x, but is 0x%02x\n",
                    idx_pack, g_pack_maps[idx_pack].val, buf_cur_pack[idx_pack]);
                ERR("idx_pack: %d, idx_buf: %d\n", idx_pack, g_pack_maps[idx_pack].idx);
                analyse_err = TRUE;
                break;
            }
        }

        //校验出错，前进 1 字节后，继续往下解析
        if (analyse_err) {
            RING_BUFFER_ADD(bytes_all_cooked, 1, sizeof(buf_recved));
            bytes_raw -= 1;
            continue;
        }

        //除头尾和校验位之外的内容进行和校验
        code_calced = 0;
        for (idx_verify = IDX_HEAD; idx_verify < IDX_VERIFY;
             idx_verify++) {
            code_calced += buf_cur_pack[idx_verify];
        }

        //校验不匹配，前进 1 字节后，继续往下解析
        if (code_calced != buf_cur_pack[IDX_VERIFY]) {
            ERR("calc verify code err, should be 0x%02x, but is 0x%02x\n",
                code_calced, buf_cur_pack[IDX_VERIFY]);
            RING_BUFFER_ADD(bytes_all_cooked, 1, sizeof(buf_recved));
            bytes_raw -= 1;
            continue;
        }

        //解析相应命令并执行
        for (idx_cmd = 0; idx_cmd < ARRAY_SIZE(g_wtcmd_maps); idx_cmd++) {
            if (g_wtcmd_maps[idx_cmd].cmd_high == buf_cur_pack[IDX_CMD_HIGH] &&
                g_wtcmd_maps[idx_cmd].cmd_low == buf_cur_pack[IDX_CMD_LOW]) {
                pri_asr(LVL_DBG, "respond cmd %s\n", g_wtcmd_maps[idx_cmd].cmd_func);
                g_wtcmd_maps[idx_cmd].cb_hdl_wtcmd(NULL);
                found_cmd = TRUE;
                break;
            }
        }

        //没有匹配的 cmd，前进 1 字节后，继续往下解析
        if (!found_cmd) {
            ERR("can't recognize cmd 0x%02x 0x%02x\n",
                buf_cur_pack[IDX_CMD_HIGH], buf_cur_pack[IDX_CMD_LOW]);
            RING_BUFFER_ADD(bytes_all_cooked, 1, sizeof(buf_recved));
            bytes_raw -= 1;
            continue;
        }

        RING_BUFFER_ADD(bytes_all_cooked, sizeof(buf_cur_pack), sizeof(buf_recved));
        bytes_raw -= sizeof(buf_cur_pack);
    }
}

static int init_waytronic_clk(void)
{
    int ret = SUCCESS;

    pwm_open_export(PWM_WAYTRONIC_CLK, TRUE);
    pwm_set_period(PWM_WAYTRONIC_CLK, NS_PERIOD_WAYTRONIC);
    pwm_set_duty_cycle(PWM_WAYTRONIC_CLK, DUTYCYCLE_WAYTRONIC);
    pwm_enable_chn(PWM_WAYTRONIC_CLK, TRUE);

    return ret;
}

static int uninit_waytronic_clk(void)
{
    int ret = SUCCESS;

    pwm_enable_chn(PWM_WAYTRONIC_CLK, FALSE);
    pwm_open_export(PWM_WAYTRONIC_CLK, FALSE);

    return ret;
}

int init_waytronic_uart(void)
{
    int ret = SUCCESS;

    DBG("%s\n", __func__);

    init_waytronic_clk();

    if (NULL == g_run_wt.sch) {
        g_run_wt.sch = js_create_scheduler("sch_wt_asr");
        goto_if_fatal_err(NULL != g_run_wt.sch, exit, ret = FAILURE,
                          "failed to create sch_wt_asr");
    }

    if (g_run_wt.fd < 0) {
        ret = uart_fd_open(&g_run_wt.fd, TTY_ASR, BAUDRATE_ASR);
        goto_if_fatal_err(SUCCESS == ret, exit, ret = FAILURE,
                          "failed to open uart %s\n", TTY_ASR);
        DBG("waytronic fd is %d\n", g_run_wt.fd);
    }

    if (NULL == g_run_wt.hdl_rw) {
        js_create_reader_r(g_run_wt.sch, g_run_wt.fd, JS_READABLE, cb_read_asr_cmd,
                           NULL, &g_run_wt.hdl_rw);
        goto_if_fatal_err(NULL != g_run_wt.hdl_rw, exit, ret = FAILURE,
                          "failed create reader of uart %s\n", TTY_ASR);
    }

    ret = SUCCESS;

exit:

    if (ret != SUCCESS) {
        uninit_waytronic_uart();
    }

    return ret;
}

int uninit_waytronic_uart(void)
{
    DBG("%s\n", __func__);

    js_delete_reader_r(&g_run_wt.hdl_rw);

    js_delete_scheduler(&g_run_wt.sch);
    g_run_wt.sch = NULL;

    uninit_waytronic_clk();

    return uart_fd_close(&g_run_wt.fd);
}
