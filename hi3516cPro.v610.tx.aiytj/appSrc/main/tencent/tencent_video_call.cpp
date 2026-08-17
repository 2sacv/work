/* 
 *       Filename:  tencent_video_call.cpp
 *    Description:  
 *        Version:  1.0
 *        Created:  03/19/2026 03:29:05 PM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  xiangyp (), 
 *   Organization:  
 */
#include "debug.h"
#include "utils.h"
#include "confapi.h"
#include "cmdstat.h"
#include "jconfig.h"
#include "key_scan.h"
#include "js_scheduler.h"
#include "circular_queue.h"
#include "encode_audio_queue.h"
#include "tencent_video_call.h"
#include "encode_audio_output.h"
#ifdef PLATFORM_TENCENT
#include "iv_ad.h"
#include "iv_voip.h"
#include "iv_usrex.h"
#include "tencent_model.h"
#include "tencent_server.h"
#include "tencent_event_handle.h"
#include "tencent_http_service.h"
#include "tencent_cloud_storage.h"
#endif

#define ENABLE_TOUCH_KEY
#define ENABLE_BUTTON_KEY

#define GPIO_CALL_TOUCH          (8 * 7 + 7) //GPIO7_7
#define GPIO_CALL_BUTTON         (8 * 1 + 1) //GPIO1_1
#define KEY_VIDCALL_PRESSED      (0)

#define INTV_MS_DET_KEY          (50)
#define CNT_TRIG_MIN             (200 / INTV_MS_DET_KEY)
#define CNT_TRIG_MAX             (500 / INTV_MS_DET_KEY)

#define CODE_CANCEL_CALLING      (char *)"100" //视频通话已取消
#define CODE_REQUEST_VIDCALL     (char *)"101" //一键呼叫
#define CODE_CALL_END            (char *)"102" //正常呼叫结束
#define CODE_CALL_MISSED         (char *)"103" //视频通话未接听
#define CODE_REJECT_CALL         (char *)"104" //拒绝来电

#define MS_TIMEOUT_CALL_RESPOND  (30 * 1000)
#define MS_TIMEOUT_CALL_FINISH   (5 * 1000)
#define MS_INTV_PLAY_DUDUDU      (5 * 1000)

#define READONLY_ENABLE          (1)
#define READONLY_DISABLE         (0)

typedef int (*cbEvtTrans)(void *usr_data);

typedef struct {
    eCallStatus       stat_curr;      //当前电话使用状态
    eCallEvt          evt_call;       //触发的事件
    eCallStatus       stat_next;      //根据收到的接听者事件需要切换到的下一个状态
    cbEvtTrans        cb_trans_event; //将 curr 事件转换为 next 事件的回调函数
} sVidCallSM;

typedef struct {
    char               platform[16];     //当前套餐是微信还是 APP
#ifdef PLATFORM_TENCENT
    voip_video_info_s  voip_vid;
#endif
    struct timespec    clk_called;       //发起通话请求时的时间
    pthread_spinlock_t lock_evt;         //事件读取写入锁
    int64_t            ms_dududu;        //播放嘟嘟嘟声音的计时
    int64_t            ms_disconn;       //通话时意外断联的时间戳
    eCallStatus        stat_curr;        //当前电话使用状态
    int                wx_call_enabled;  //是否开通了微信通话功能
    int                is_ready;         //是否可以发起通话，目前仅非产测模式用于判断腾讯通话模块是否初始化完毕
    int                stat_toggled;     //外部是否切换了一键呼叫状态
    JSScheduler        sch;
    JSTCHandle         hdl;
    struct cmdstat     *ctx;
} sVidCallRun;

typedef struct {
    sVideoCallCfg vidcall;
    VideoEncS     videnc;
} sVidCallCfg;

static sVidCallCfg g_raw_vc;
static sVidCallCfg g_cfg_vc;

static sVidCallRun g_run_vc = {
    .stat_curr = E_STAT_IDLE,
};

static CircularQueue<eCallEvt, 32> g_evt_queue;

static const char g_evt_msg[E_EVT_CALL_COUNT + 1][20] = {
    "NONE",
    "REJECT/HANGUP",
    "PICKUP",
    "DISCON",
    "TORESP",
    "LAUNCH",
    "CANCEL",
    "TODROP"
};

static const char g_stat_msg[E_STAT_COUNT][16] = {
    "IDLE",
    "WAITHANGUP",
    "RINGING",
    "CALLING"
};

bool call_evt_enqueue(eCallEvt evt_call)
{
    int ret = 0;

    pthread_spin_lock(&g_run_vc.lock_evt);
    ret = g_evt_queue.Enqueue(evt_call);
    pthread_spin_unlock(&g_run_vc.lock_evt);

    return ret;
}

static bool call_evt_dequeue(eCallEvt *evt_call)
{
    int ret = 0;

    pthread_spin_lock(&g_run_vc.lock_evt);
    ret = g_evt_queue.Dequeue(*evt_call);
    pthread_spin_unlock(&g_run_vc.lock_evt);

    return ret;
}

static int is_wechat_callee_busy(void)
{
#ifdef PLATFORM_TENCENT
    if (get_g_sys(factest)) {
        return FALSE;
    } else {
        return iv_avt_voip_is_busy_v2();
    }
#else
    return FALSE;
#endif
}

static int wechat_request_a_call(void)
{
#ifdef PLATFORM_TENCENT
    if (get_g_sys(factest)) {
        return SUCCESS;
    } else {
        return iv_avt_voip_call_v2(IV_CM_STREAM_TYPE_VIDEO, g_cfg_vc.vidcall.openId,
                                   NULL, g_run_vc.voip_vid, 1, 0);
    }
#else
    return SUCCESS;
#endif
}

static int hang_up_wechat_call(void)
{
#ifdef PLATFORM_TENCENT
    if (get_g_sys(factest)) {
        return SUCCESS;
    } else {
        return iv_avt_voip_hang_up_v2();
    }
#else
    return SUCCESS;
#endif
}

//推送事件到 APP 消息队列
static void report_event2msg_queue(AlarmTypeE msg, int snsr_id)
{
#ifdef PLATFORM_TENCENT
    if (!get_g_sys(factest)) {
        cs_report_event_with_picture(msg, snsr_id);
    }
#endif
}

//推送消息到 APP 弹窗
static void report_event2popup_window(int chn, char *payload)
{
#ifdef PLATFORM_TENCENT
    if (!get_g_sys(factest)) {
        tencent_report_event(EVENT_VIDEOCALL, chn, payload);
    }
#endif
}

static void set_video_call_ivm_readonly(int readonly)
{
#ifdef PLATFORM_TENCENT
    if (!get_g_sys(factest)) {
        tencent_report_attr(readonly);
    }
#endif
}

static void hangup_or_cancel_video_call(AlarmTypeE msg, char *payload)
{
    report_event2msg_queue(msg, 0);

    report_event2popup_window(0, payload);

    set_video_call_ivm_readonly(READONLY_DISABLE);
}

static int update_tencent_voip_cfg(void)
{
    int ret = SUCCESS;
#ifdef PLATFORM_TENCENT
    iv_cm_venc_type_e venc = IV_CM_VENC_TYPE_BUTT;

    switch (g_cfg_vc.videnc.enc[0].codec) {
    case VENC_FORMAT_H264: {
        venc = IV_CM_VENC_TYPE_H264;
        break;
    }
    case VENC_FORMAT_H265: {
        venc = IV_CM_VENC_TYPE_H265;
        break;
    }
    case VENC_FORMAT_MJPEG: {
        venc = IV_CM_VENC_TYPE_MJPG;
        break;
    }
    default: {
        ERR("unsupported codec format %d\n", g_cfg_vc.videnc.enc[0].codec);
        ret = FAILURE;
        goto exit;
    }
    }

    g_run_vc.voip_vid.send_v_enc = venc;
    g_run_vc.voip_vid.recv_v_enc = venc;
    g_run_vc.voip_vid.recv_pixel = IV_CM_PIXEL_VARIABLE;
    g_run_vc.voip_vid.recv_aenc = IV_CM_AENC_TYPE_AAC;
    g_run_vc.voip_vid.u32Framerate = VOIP_RECV_V_FPS_15;
    g_run_vc.voip_vid.recv_v_rot = VOIP_RECV_V_ROTATE_NONE;

    DBG("venc is %d\n", venc);

    ret = SUCCESS;

exit:
#endif

    return ret;
}

static void update_video_call_cfg(void)
{
    if (g_cfg_vc.vidcall.openStatus && strlen(g_cfg_vc.vidcall.openId) > 0) {
        g_run_vc.wx_call_enabled = TRUE;
        strncpy(g_run_vc.platform, "wechat", sizeof(g_run_vc.platform) - 1);
    } else {
        g_run_vc.wx_call_enabled = FALSE;
        strncpy(g_run_vc.platform, "app", sizeof(g_run_vc.platform) - 1);
    }

    DBG("wechat call %s\n", g_run_vc.wx_call_enabled ? "enable" : "disable");
}

void wechat_call_evt_enqueue(int cmd)
{
    DBG("wechat callee respond cmd %d\n", cmd);

#ifdef PLATFORM_TENCENT
    switch (cmd) {
    case IV_AVT_COMMAND_CALL_REJECT: {
        call_evt_enqueue(E_EVT_CALL_REJECT);
        break;
    }
    case IV_AVT_COMMAND_CALL_ANSWER: {
        call_evt_enqueue(E_EVT_CALL_PICKUP);
        break;
    }
    case IV_AVT_COMMAND_CALL_HANG_UP: {
        call_evt_enqueue(E_EVT_CALL_HANGUP);
        break;
    }
    default: {
        goto exit;
    }
    }
#endif

exit:

    return;
}

static void cb_videocall_cfg(int id, void *p_src, int size, void *ctx)
{
    CPY2CMDCFG(CMD_VC_VIDCALLCFG, &g_raw_vc.vidcall, p_src, size);
}

static void cb_video_cfg(int id, void *p_src, int size, void *ctx)
{
    CPY2CMDCFG(CMD_VC_VIDEOCFG, &g_raw_vc.videnc, p_src, size);
}

void toggle_video_call(void)
{
    if (NULL != g_run_vc.ctx) {
        struct cmdstat *ctx = g_run_vc.ctx;
        CPY2CMD(CMD_VC_DO_VIDCALL);
    }
}

static void diff_cfg2cmd(void *ctx)
{
    struct cmdstat *p_cmd = (struct cmdstat *)ctx;

    if (p_cmd->cmd_stage) {
        if (p_cmd->cmd_stage & CMD_VC_VIDCALLCFG) {
            DBG("----- video call -----\n");
            memcpy(&g_cfg_vc.vidcall, &g_raw_vc.vidcall, sizeof(g_cfg_vc.vidcall));
        }

        if (p_cmd->cmd_stage & CMD_VC_VIDEOCFG) {
            DBG("----- video -----\n");
            memcpy(&g_cfg_vc.videnc, &g_raw_vc.videnc, sizeof(g_cfg_vc.videnc));
        }
    };
}

static void wait_for_pickup(void)
{
    //产测模式不进行超时判断
    //接听方响应超时，挂断电话，播放"您所呼叫的用户暂时无法接通，请稍后再拨"
    if (!get_g_sys(factest) &&
        ms_since_previous(&g_run_vc.clk_called) >= MS_TIMEOUT_CALL_RESPOND) {
        call_evt_enqueue(E_EVT_CALL_TORESP);
    } else {
        //每隔 MS_INTV_PLAY_DUDUDU 播放一次"嘟嘟嘟"
        int64_t ms_curr = mono_msec();
        if (ms_curr - g_run_vc.ms_dududu >= MS_INTV_PLAY_DUDUDU) {
            DBG("play dududu\n");

            encode_audio_queue_push_amr(AUDIO_CALL_UP, FALSE);
            g_run_vc.ms_dududu = ms_curr;
        }
    }

    return;
}

static void wait_for_hang_up(void)
{
    //超时时间：tencent_server.cpp->command_timeout
    //收到 E_STAT_WAITHANGUP 后 MS_TIMEOUT_CALL_FINISH 没有收到 E_EVT_CALL_HANGUP
    //播放"通话中断，已挂断"
    if (g_run_vc.ms_disconn > 0) {
        if (mono_msec() - g_run_vc.ms_disconn > MS_TIMEOUT_CALL_FINISH) {
            call_evt_enqueue(E_EVT_CALL_TODROP);
        }
    }

    return;
}

static void detect_key_status(eCallStatus *stat_curr)
{
    int key_evt_trigged = FALSE;

#ifdef ENABLE_TOUCH_KEY
    static key_config_t cfg_touch = {
        .event = KEY_EVENT_LONG_PRESS,
        .param = {
            .gpio           = GPIO_CALL_TOUCH,
            .active_level   = KEY_VIDCALL_PRESSED,
            .debounce_ms    = 200,
            .long_press_ms  = 500,
        },
    };
    static key_sta_t sta_touch = {0};

    sta_touch.dbg_on = get_g_log(vidcall);

    if (key_triggered(&cfg_touch, &sta_touch)) {
        key_evt_trigged = TRUE;
    }
#endif

#ifdef ENABLE_BUTTON_KEY
    static key_config_t cfg_butt = {
        .event = KEY_EVENT_SHORT_PRESS,
        .param = {
            .gpio           = GPIO_CALL_BUTTON,
            .active_level   = KEY_VIDCALL_PRESSED,
            .debounce_ms    = 100,
            .long_press_ms  = 600,
        },
    };

    static key_sta_t sta_butt = {0};

    sta_butt.dbg_on = get_g_log(vidcall);

    if (key_triggered(&cfg_butt, &sta_butt)) {
        key_evt_trigged = TRUE;
    }
#endif

    if (g_run_vc.stat_toggled) {
        g_run_vc.stat_toggled = FALSE;
        key_evt_trigged = TRUE;
    }

    if (key_evt_trigged) {
        if (E_STAT_IDLE == *stat_curr) {
            call_evt_enqueue(E_EVT_CALL_LAUNCH);
        } else {
            call_evt_enqueue(E_EVT_CALL_CANCEL);
        }
    }

    return;
}

static void cb_hang_up_video_call_silently(void *usr_data)
{
    if (g_run_vc.wx_call_enabled) {
        if (E_STAT_IDLE != g_run_vc.stat_curr) {
            hang_up_wechat_call();
        }
    } else {
        if (E_STAT_IDLE != g_run_vc.stat_curr) {
            hangup_or_cancel_video_call(ALARM_CALL_CANCEL, CODE_CANCEL_CALLING);
        }
    }

    g_run_vc.ms_disconn = 0;
    g_run_vc.stat_curr = E_STAT_IDLE;
}

static void hang_up_video_call_silently(void)
{
    js_run_function(g_run_vc.sch, cb_hang_up_video_call_silently, NULL, 1);
}

static int cb_launch_a_call(void *ctx)
{
    DBG("%s launch a video call\n", g_run_vc.platform);

    ms_clock_reset(&g_run_vc.clk_called);
    g_run_vc.ms_dududu = mono_msec();
    encode_audio_queue_push_amr(AUDIO_CALL_UP, TRUE);

    if (g_run_vc.wx_call_enabled) {
        if (is_wechat_callee_busy()) {
            SYSLOG("wechat call is busy or no user\n");
            call_evt_enqueue(E_EVT_CALL_CANCEL);
        } else {
            wechat_request_a_call();
        }
    } else {
        report_event2msg_queue(ALARM_CALLING, 0);
        set_video_call_ivm_readonly(READONLY_ENABLE);
        report_event2popup_window(0, CODE_REQUEST_VIDCALL);
    }

    return SUCCESS;
}

static int cb_cancel_call_normal(void *ctx)
{
    DBG("%s cancel before callee respond\n", g_run_vc.platform);

    encode_audio_queue_push_amr(AUDIO_CALL_OFF, FALSE);

    if (g_run_vc.wx_call_enabled) {
        hang_up_wechat_call();
    } else {
        hangup_or_cancel_video_call(ALARM_CALL_CANCEL, CODE_CANCEL_CALLING);
    }

    return SUCCESS;
}

static int cb_cancel_call_reject(void *ctx)
{
    DBG("%s cancel after callee reject\n", g_run_vc.platform);

    encode_audio_queue_push_amr(AUDIO_CALL_BUSY, FALSE);

    if (g_run_vc.wx_call_enabled) {
        hang_up_wechat_call();
    } else {
        hangup_or_cancel_video_call(ALARM_CALL_REJECT, CODE_REJECT_CALL);
    }

    return SUCCESS;
}

static int cb_cancel_call_timeout(void *ctx)
{
    DBG("%s cancel after callee timeout\n", g_run_vc.platform);

    encode_audio_queue_push_amr(AUDIO_CALL_NOT_AVAILABLE, FALSE);

    if (g_run_vc.wx_call_enabled) {
        hang_up_wechat_call();
    } else {
        hangup_or_cancel_video_call(ALARM_CALL_MISSED, CODE_CALL_MISSED);
    }

    return SUCCESS;
}

static int cb_hangup_call_normal(void *ctx)
{
    DBG("%s callee hangup when calling\n", g_run_vc.platform);

    encode_audio_queue_push_amr(AUDIO_CALL_FINISH, FALSE);

    if (g_run_vc.wx_call_enabled) {
        hang_up_wechat_call();
    } else {
        hangup_or_cancel_video_call(ALARM_CALL_END, CODE_CALL_END);
    }

    return SUCCESS;
}

static int cb_record_call_disctime(void *ctx)
{
    DBG("%s callee may lost connect\n", g_run_vc.platform);

    g_run_vc.ms_disconn = mono_msec();

    return SUCCESS;
}

static int cb_hangup_call_by_caller(void *ctx)
{
    DBG("%s caller hangup when calling\n", g_run_vc.platform);

    encode_audio_queue_push_amr(AUDIO_CALL_FINISH, FALSE);

    if (g_run_vc.wx_call_enabled) {
        hang_up_wechat_call();
    } else {
        hangup_or_cancel_video_call(ALARM_CALL_CANCEL, CODE_CANCEL_CALLING);
    }

    return SUCCESS;
}

static int cb_hangup_call_exception(void *ctx)
{
    DBG("%s callee lost connect\n", g_run_vc.platform);

    encode_audio_queue_push_amr(AUDIO_CALL_DROP, FALSE);

    if (g_run_vc.wx_call_enabled) {
        hang_up_wechat_call();
    } else {
        hangup_or_cancel_video_call(ALARM_CALL_CANCEL, CODE_CANCEL_CALLING);
    }

    return SUCCESS;
}

//一键呼叫状态机
static sVidCallSM g_vidcall_sm[] = {
    {E_STAT_IDLE,       E_EVT_CALL_LAUNCH, E_STAT_RINGING,    cb_launch_a_call        }, //"未通话"状态，  拨打方"发起"通话请求，转到"通话中"状态
    {E_STAT_RINGING,    E_EVT_CALL_PICKUP, E_STAT_CALLING,    NULL                    }, //"呼叫中"状态，  接听方"同意"通话请求，转到"通话中"状态
    {E_STAT_RINGING,    E_EVT_CALL_CANCEL, E_STAT_IDLE,       cb_cancel_call_normal   }, //"呼叫中"状态，  拨打方"取消"通话请求，转到"未通话"状态
    {E_STAT_RINGING,    E_EVT_CALL_REJECT, E_STAT_IDLE,       cb_cancel_call_reject   }, //"呼叫中"状态，  接听方"拒绝"通话请求，转到"未通话"状态
    {E_STAT_RINGING,    E_EVT_CALL_TORESP, E_STAT_IDLE,       cb_cancel_call_timeout  }, //"呼叫中"状态，  接听方"超时"无操作，  转到"未通话"状态
    {E_STAT_CALLING,    E_EVT_CALL_HANGUP, E_STAT_IDLE,       cb_hangup_call_normal   }, //"通话中"状态，  接听方"挂断"通话，    转到"未通话"状态
    {E_STAT_CALLING,    E_EVT_CALL_DISCON, E_STAT_WAITHANGUP, cb_record_call_disctime }, //"通话中"状态，  网络意外断联，        转到"等待挂断"状态
    {E_STAT_CALLING,    E_EVT_CALL_CANCEL, E_STAT_IDLE,       cb_hangup_call_by_caller}, //"通话中"状态，  拨打方"挂断"通话，    转到"未通话"状态
    {E_STAT_WAITHANGUP, E_EVT_CALL_HANGUP, E_STAT_IDLE,       cb_hangup_call_normal   }, //"等待挂断"状态，接听方"挂断"通话，    转到"未通话"状态
    {E_STAT_WAITHANGUP, E_EVT_CALL_TODROP, E_STAT_IDLE,       cb_hangup_call_exception}  //"等待挂断"状态，等待接听方"挂断"超时，转到"未通话"状态
};

static void cb_video_call_loop(void *ctx)
{
    int ret = SUCCESS, idx_sm = 0, evt_matched = FALSE;
    eCallEvt evt_call = E_EVT_CALL_NONE;

    int cmd = cmd_get_command((struct cmdstat *)ctx);
    if (cmd > 0) {
        if (cmd & CMD_VC_DO_VIDCALL) {
            g_run_vc.stat_toggled = TRUE;
        }

        if (cmd & CMD_VC_VIDCALLCFG) {
            update_video_call_cfg();
        }

        if (cmd & CMD_VC_VIDEOCFG) {
            ret = update_tencent_voip_cfg();
            if (ret < 0) {
                SYSLOG("failed to update tecent voip cfg\n");
            }
        }
    }

    if (!g_cfg_vc.vidcall.enable) {
        pri_vidcall(LVL_LOOP, "video call cfg not enable\n");
        goto exit;
    }

#ifdef PLATFORM_TENCENT
    if (!get_g_sys(factest) && !g_run_vc.is_ready) {
        pri_vidcall(LVL_LOOP, "video call is not ready\n");
        goto exit;
    }
#endif

    detect_key_status(&g_run_vc.stat_curr);

    switch (g_run_vc.stat_curr) {
    case E_STAT_RINGING: {
        wait_for_pickup();
        break;
    }
    case E_STAT_WAITHANGUP: {
        wait_for_hang_up();
        break;
    }
    default: {
        break;
    }
    }

    if (!call_evt_dequeue(&evt_call)) {
        goto exit;
    }

    for (idx_sm = 0; idx_sm < ARRAY_SIZE(g_vidcall_sm); idx_sm++) {
        if (evt_call == g_vidcall_sm[idx_sm].evt_call &&
            g_run_vc.stat_curr == g_vidcall_sm[idx_sm].stat_curr) {

            DBG("recv EVT_CALL_%s from STAT_%s to STAT_%s\n",
                g_evt_msg[evt_call + 1], g_stat_msg[g_run_vc.stat_curr],
                g_stat_msg[g_vidcall_sm[idx_sm].stat_next]);

            g_run_vc.stat_curr = g_vidcall_sm[idx_sm].stat_next;

            if (NULL != g_vidcall_sm[idx_sm].cb_trans_event) {
                g_vidcall_sm[idx_sm].cb_trans_event(ctx);
            }

            if (E_STAT_IDLE == g_run_vc.stat_curr) {
                g_run_vc.ms_disconn = 0;
            }

            evt_matched = TRUE;
            break;
        }
    }

    if (!evt_matched) {
        pri_vidcall(LVL_ERR, "invalid EVT_CALL_%s，curr STAT_%s\n",
                    g_evt_msg[evt_call + 1], g_stat_msg[g_run_vc.stat_curr]);
    }

exit:

    return;
}

static void cb_set_video_call_ready(void *usr_data)
{
    g_run_vc.is_ready = *((int *)usr_data);
}

void set_video_call_ready(int is_ready)
{
    if (NULL != g_run_vc.sch) {
        js_run_function(g_run_vc.sch, cb_set_video_call_ready, &is_ready, 1);
    } else {
        cb_set_video_call_ready(&is_ready);
    }
}

int get_video_call_status(void)
{
    return g_run_vc.stat_curr;
}

static int init_video_call_gpio(void)
{
    int ret = SUCCESS;

#ifdef ENABLE_TOUCH_KEY
    ret = gpio_open_export(GPIO_CALL_TOUCH);
    goto_if_fatal_err(SUCCESS == ret, exit, ret = FAILURE,
                      "failed to export vidcall gpio%d\n", GPIO_CALL_TOUCH);

	ret = gpio_open_set_direction(GPIO_CALL_TOUCH, "in");
    goto_if_fatal_err(SUCCESS == ret, exit, ret = FAILURE,
                      "failed to set vidcall gpio%d direction\n", GPIO_CALL_TOUCH);
#endif

#ifdef ENABLE_BUTTON_KEY
    ret = gpio_open_export(GPIO_CALL_BUTTON);
    goto_if_fatal_err(SUCCESS == ret, exit, ret = FAILURE,
                      "failed to export vidcall gpio%d\n", GPIO_CALL_BUTTON);

	ret = gpio_open_set_direction(GPIO_CALL_BUTTON, "in");
    goto_if_fatal_err(SUCCESS == ret, exit, ret = FAILURE,
                      "failed to set vidcall gpio%d direction\n", GPIO_CALL_BUTTON);
#endif

exit:

    return ret;
}

int init_tencent_video_call(JSScheduler sch)
{
    static struct cmdstat cmdstat_vidcall = {0};
    int ret = SUCCESS;

    DBG("%s\n", __func__);

    goto_if_fatal_err(NULL != sch, exit, ret = FAILURE,
                      "video call sch param is null\n");

    g_run_vc.sch = sch;
    cmdstat_vidcall.diff_cfg2cmd = diff_cfg2cmd;
    g_run_vc.ctx = &cmdstat_vidcall;

    ret = init_video_call_gpio();
    goto_if_fatal_err(SUCCESS == ret, exit, ret = FAILURE,
                      "failed to init video call gpio\n");

    conf_get_videocall_cfg(&g_cfg_vc.vidcall);
    conf_get_videocfg(&g_cfg_vc.videnc);

    update_video_call_cfg();

    ret = update_tencent_voip_cfg();
    goto_if_fatal_err(SUCCESS == ret, exit, ret = FAILURE,
                      "failed to udpate tencent voip cfg\n");

    if (NULL == g_run_vc.hdl) {
        attach_config(JEvent_VideoCallCfg, cb_videocall_cfg, &cmdstat_vidcall);
        attach_config(JEvent_VideoCfgChg,  cb_video_cfg,     &cmdstat_vidcall);

        js_create_timer_r(g_run_vc.sch, INTV_MS_DET_KEY, INTV_MS_DET_KEY,
                          cb_video_call_loop, &cmdstat_vidcall, &g_run_vc.hdl);
        goto_if_fatal_err(NULL != g_run_vc.hdl, exit, ret = FAILURE,
                          "failed to create sch_vidcall timer\n");
    }

exit:

    return ret;
}

void uninit_tencent_video_call(void)
{
    DBG("%s\n", __func__);

    if (NULL != g_run_vc.hdl) {
        detach_config(JEvent_VideoCallCfg, cb_videocall_cfg, g_run_vc.ctx);
        detach_config(JEvent_VideoCfgChg,  cb_video_cfg,     g_run_vc.ctx);

        js_delete_timer_r(&g_run_vc.hdl);
    }

    hang_up_video_call_silently();

    g_run_vc.sch = NULL;
}
