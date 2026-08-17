/* 
 *       Filename:  encode_audio_queue.c
 *    Description:  
 *        Version:  1.0
 *        Created:  11/03/2022 05:12:07 PM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  xiangyp (), 
 *   Organization:  
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

#include "debug.h"
#include "utils.h"
#include "sp_dec.h"
#include "confapi.h"
#include "interf_dec.h"
#include "jconfstruct.h"
#include "system_ctrl.h"
#include "update.h"
#include "encode_audio_queue.h"
#include "encode_audio_output.h"
#include "factory_db.h"
#include "circular_queue.h"

#define AMR_FILE_HEAD               "#!AMR\n"
#define AMR_FILE_HEAD_SIZE          strlen(AMR_FILE_HEAD)
#define FRAME_HEAD_SIZE             (1)
#define MAX_DECOMPRESSION_RATIO     (20)

typedef struct {
    size_t  len;
    uint8_t *buf;
} sAudioRawBuffer;

typedef struct {
    AUDIO_PROMPT name;
    const char *path;
    eAudioType type;
    int duration;
    int is_force;   //true: 静默模式不会被屏蔽 false:静默模式会被屏蔽
} AudioLocalS;

static AudioLocalS g_audio_info[] = {
    {AUDIO_AIRLINK_MODE         , "/ipc/etc/amr/01.amr"        , AUDIO_TYPE_SYSTEM , 0, 0},  //欢迎，设备已经启动   2.56s
    {AUDIO_KEY_RESETING         , "/ipc/etc/amr/02.amr"        , AUDIO_TYPE_SYSTEM , 0, 1},  //设备复位成功       1s
    {AUDIO_EXEC_RESETING        , "/ipc/etc/amr/03.amr"        , AUDIO_TYPE_SYSTEM , 0, 1},  //正在启动(RESET)，请稍候     1.4s                     //按复位键
    {AUDIO_IP_SUCESS            , "/ipc/etc/amr/04.amr"        , AUDIO_TYPE_NET    , 0, 0},  //WIFI配置成功，正在连接网络，请稍候    2.72s      //wifi连接成功
    {AUDIO_BURNING_SUCC         , "/ipc/etc/ffw/12.amr"        , AUDIO_TYPE_FACTORY, 0, 1},  //烧写成功
    {AUDIO_BURNING_FAIL         , "/ipc/etc/ffw/13.amr"        , AUDIO_TYPE_FACTORY, 0, 1},  //烧写失败
    {AUDIO_RUNINGOUT_ID         , "/ipc/etc/ffw/14.amr"        , AUDIO_TYPE_FACTORY, 0, 1},  //SD卡用完
    {AUDIO_PASSWORD_ERR         , "/ipc/etc/amr/15.amr"        , AUDIO_TYPE_NET    , 0, 1},  //Wifi密码错误 请重新输入                       //密码错误
    {AUDIO_SETUP_WIFI           , "/ipc/etc/amr/16.amr"        , AUDIO_TYPE_NET    , 0, 0},  //请打开手机进行WiFi设置         //等待配网
    {AUDIO_DI_DI                , "/ipc/etc/ffw/di.amr"        , AUDIO_TYPE_NET    , 0, 0},  //滴
    {AUDIO_RECV_PWD             , "/ipc/etc/amr/recv.amr"      , AUDIO_TYPE_NET    , 0, 0},  //收到密码，等待连接
    {AUDIO_CON_FAIL             , "/ipc/etc/amr/con_fail.amr"  , AUDIO_TYPE_NET    , 0, 1},  //连接失败，请检查路由器设置
    {AUDIO_ALARM_ALARM          , "/opt/custom/motionaudio.amr", AUDIO_TYPE_ALARM  , 0, 1},  //报警声音
    {AUDIO_ALARM_DOG            , "/opt/custom/dog.amr"        , AUDIO_TYPE_ALARM  , 0, 1},  //报警声音
    {AUDIO_ALARM_OTHER          , "/opt/custom/other.amr"      , AUDIO_TYPE_ALARM  , 0, 1},  //报警声音
    {AUDIO_ALARM_CUSTOM         , "/opt/custom/custom.amr"     , AUDIO_TYPE_ALARM  , 0, 1},  //自定义报警声音
    {AUDIO_AUTH_SUCCESS         , "/ipc/etc/amr/07.amr"        , AUDIO_TYPE_NET    , 0, 0},  //网络连接成功
    {AUDIO_FACTORY_UPGRADE      , "/ipc/etc/amr/up.amr"        , AUDIO_TYPE_UPGRADE, 0, 1},  //升级中请等待
    {AUDIO_FACTORY_TEST         , "/ipc/etc/ffw/10.amr"        , AUDIO_TYPE_FACTORY, 0, 1},  //测试模式

    {AUDIO_SETUP_4G             , "/ipc/etc/amr/33.amr"        , AUDIO_TYPE_4G     , 0, 0},  //设备已启动，正在配置4G网络
    {AUDIO_CHECK_SUCCESS_4G     , "/ipc/etc/amr/34.amr"        , AUDIO_TYPE_4G     , 0, 0},  //4G模块检测成功
    {AUDIO_SIM_CHECK_SUCCESS_4G , "/ipc/etc/amr/35.amr"        , AUDIO_TYPE_4G     , 0, 0},  //SIM卡检测成功，正在连接网络，请稍候
    {AUDIO_CHECK_FAIL_4G        , "/ipc/etc/amr/36.amr"        , AUDIO_TYPE_4G     , 0, 1},  //4G模块检测失败
    {AUDIO_SIM_CHECK_FAIL_4G    , "/ipc/etc/amr/37.amr"        , AUDIO_TYPE_4G     , 0, 1},  //SIM卡检测失败，请检查SIM卡
    {AUDIO_SIM_CHECK_ICCID_4G   , "/ipc/etc/amr/38.amr"        , AUDIO_TYPE_4G     , 0, 1},  //不能识别的4G卡，请使用原厂专用的物联卡
    {AUDIO_CSQ_WEAK_4G          , "/ipc/etc/amr/csq_0.amr"     , AUDIO_TYPE_4G     , 0, 1},  //信号强度偏弱，请调整安装位置或天线
    {AUDIO_CSQ_STRONG_4G        , "/ipc/etc/amr/csq_1.amr"     , AUDIO_TYPE_4G     , 0, 0},  //信号强度正常
    {AUDIO_UPGRAD_SUCCESS_REBOOT, "/ipc/etc/amr/39.amr"        , AUDIO_TYPE_UPGRADE, 0, 1},  //设备升级成功，正在重启，请等待
    {AUDIO_UPGRADING_NO_OFF     , "/ipc/etc/amr/40.amr"        , AUDIO_TYPE_UPGRADE, 0, 1},  //设备正在升级，请不要断开设备电源
    {AUDIO_SIM4G_CON_FAIL       , "/ipc/etc/amr/41.amr"        , AUDIO_TYPE_4G     , 0, 1},  //4G网络连接失败，请检查网络环境
    {AUDIO_AGING_TEST           , "/tmp/test.amr"              , AUDIO_TYPE_FACTORY, 0, 0},  //正在进行老化测试
    {AUDIO_SIM4G_SIGLE_NORMAL   , "/ipc/etc/amr/42.amr"        , AUDIO_TYPE_4G     , 0, 1},  //信号强度 正常
    {AUDIO_SIM4G_SIGLE_WEEK     , "/ipc/etc/amr/43.amr"        , AUDIO_TYPE_4G     , 0, 1},  //信号强度偏弱，请调整安装位置或天线
    {AUDIO_SIM4G_SCAN_QRCODE    , "/ipc/etc/amr/44.amr"        , AUDIO_TYPE_4G     , 0, 0},  //请扫码添加
    {AUDIO_SIM4G_BIND_SUCCESS   , "/ipc/etc/amr/45.amr"        , AUDIO_TYPE_4G     , 0, 0},  //设备添加成功
    {AUDIO_SIM4G_DI_DEV_BINDING , "/ipc/etc/amr/46.amr"        , AUDIO_TYPE_4G     , 0, 0},  //正在添加设备

    {AUDIO_DIG_1                , "/ipc/etc/amr/dig/1.amr"     , AUDIO_TYPE_SYSTEM , 0, 0},  //一
    {AUDIO_DIG_2                , "/ipc/etc/amr/dig/2.amr"     , AUDIO_TYPE_SYSTEM , 0, 0},  //二
    {AUDIO_DIG_3                , "/ipc/etc/amr/dig/3.amr"     , AUDIO_TYPE_SYSTEM , 0, 0},  //三
    {AUDIO_DIG_4                , "/ipc/etc/amr/dig/4.amr"     , AUDIO_TYPE_SYSTEM , 0, 0},  //四
    {AUDIO_DIG_5                , "/ipc/etc/amr/dig/5.amr"     , AUDIO_TYPE_SYSTEM , 0, 0},  //五
    {AUDIO_DIG_6                , "/ipc/etc/amr/dig/6.amr"     , AUDIO_TYPE_SYSTEM , 0, 0},  //六
    {AUDIO_DIG_7                , "/ipc/etc/amr/dig/7.amr"     , AUDIO_TYPE_SYSTEM , 0, 0},  //七
    {AUDIO_DIG_8                , "/ipc/etc/amr/dig/8.amr"     , AUDIO_TYPE_SYSTEM , 0, 0},  //八
    {AUDIO_DIG_9                , "/ipc/etc/amr/dig/9.amr"     , AUDIO_TYPE_SYSTEM , 0, 0},  //九
    {AUDIO_DIG_0                , "/ipc/etc/amr/dig/0.amr"     , AUDIO_TYPE_SYSTEM , 0, 0},  //零

    /*一键呼叫的提示音*/
    {AUDIO_CALL_UP              , "/ipc/etc/amr/47.amr"        , AUDIO_TYPE_VIDCALL, 0, 1},  //嘟嘟嘟...
    {AUDIO_CALL_OFF             , "/ipc/etc/amr/48.amr"        , AUDIO_TYPE_VIDCALL, 0, 1},  //一键呼叫已取消，请稍后再拨
    {AUDIO_CALL_NOT_AVAILABLE   , "/ipc/etc/amr/49.amr"        , AUDIO_TYPE_VIDCALL, 0, 1},  //您所呼叫的用户暂时无法接通，请稍后再拨
    {AUDIO_CALL_FINISH          , "/ipc/etc/amr/50.amr"        , AUDIO_TYPE_VIDCALL, 0, 1},  //通话完成，已挂断
    {AUDIO_CALL_BUSY            , "/ipc/etc/amr/51.amr"        , AUDIO_TYPE_VIDCALL, 0, 1},  //您所呼叫的用户正忙，请稍后再拨
    {AUDIO_CALL_DROP            , "/ipc/etc/amr/52.amr"        , AUDIO_TYPE_VIDCALL, 0, 1},  //通话中断，已挂断

    /*ASR*/
    {AUDIO_ASR_LM_HERE          , "/ipc/etc/amr/asr_01.amr"    , AUDIO_TYPE_ASR,     0, 1},  //我在的主人
    {AUDIO_ASR_OPEN_DEV         , "/ipc/etc/amr/asr_02.amr"    , AUDIO_TYPE_ASR,     0, 1},  //设备已打开
    {AUDIO_ASR_OFF_DEV          , "/ipc/etc/amr/asr_03.amr"    , AUDIO_TYPE_ASR,     0, 1},  //设备已关闭

    {AUDIO_VOLUM_INC            , "/ipc/etc/amr/asr_04.amr"    , AUDIO_TYPE_ASR,     0, 1},  //音量已调大
    {AUDIO_VOLUM_DEC            , "/ipc/etc/amr/asr_05.amr"    , AUDIO_TYPE_ASR,     0, 1},  //音量已调小
    {AUDIO_VOLUM_OFF            , "/ipc/etc/amr/asr_06.amr"    , AUDIO_TYPE_ASR,     0, 1},  //音量已关闭
    {AUDIO_VOLUM_MAX            , "/ipc/etc/amr/asr_07.amr"    , AUDIO_TYPE_ASR,     0, 1},  //音量调到最大

    {AUDIO_LIGHT_ON             , "/ipc/etc/amr/asr_08.amr"    , AUDIO_TYPE_ASR,     0, 1},  //灯光已打开
    {AUDIO_LIGHT_OFF            , "/ipc/etc/amr/asr_09.amr"    , AUDIO_TYPE_ASR,     0, 1},  //灯光已关闭
    {AUDIO_LIGHT_INC            , "/ipc/etc/amr/asr_10.amr"    , AUDIO_TYPE_ASR,     0, 1},  //灯光已调亮
    {AUDIO_LIGHT_DEC            , "/ipc/etc/amr/asr_11.amr"    , AUDIO_TYPE_ASR,     0, 1},  //灯光已调暗

    {AUDIO_TURN_LEFT            , "/ipc/etc/amr/asr_12.amr"    , AUDIO_TYPE_ASR,     0, 1},  //好的左转
    {AUDIO_TURN_RIGHT           , "/ipc/etc/amr/asr_13.amr"    , AUDIO_TYPE_ASR,     0, 1},  //好的右转
    {AUDIO_TURN_UP              , "/ipc/etc/amr/asr_14.amr"    , AUDIO_TYPE_ASR,     0, 1},  //好的向上
    {AUDIO_TURN_DOWN            , "/ipc/etc/amr/asr_15.amr"    , AUDIO_TYPE_ASR,     0, 1},  //好的向下

    {AUDIO_TURN_MAX_LEFT        , "/ipc/etc/amr/asr_16.amr"    , AUDIO_TYPE_ASR,     0, 1},  //已转至最左边
    {AUDIO_TURN_MAX_RIGHT       , "/ipc/etc/amr/asr_17.amr"    , AUDIO_TYPE_ASR,     0, 1},  //已转至最右边
    {AUDIO_TURN_MAX_UP          , "/ipc/etc/amr/asr_18.amr"    , AUDIO_TYPE_ASR,     0, 1},  //已转至最上边
    {AUDIO_TURN_MAX_DOWN        , "/ipc/etc/amr/asr_19.amr"    , AUDIO_TYPE_ASR,     0, 1},  //已转至最下边

    {AUDIO_CALL_WAIT            , "/ipc/etc/amr/asr_20.amr"    , AUDIO_TYPE_ASR,     0, 1},  //正在呼叫请稍等
    {AUDIO_CALL_HANGUP          , "/ipc/etc/amr/asr_21.amr"    , AUDIO_TYPE_ASR,     0, 1},  //已挂断通话
    {AUDIO_PTZ_CALIBRATE        , "/ipc/etc/amr/asr_22.amr"    , AUDIO_TYPE_ASR,     0, 1},  //云台已校准

    {AUDIO_ASR_WAKEUP_AGAIN     , "/ipc/etc/amr/asr_23.amr"    , AUDIO_TYPE_ASR,     0, 1},  //主人需要再唤醒我吧
    {AUDIO_LIGHT_MAX            , "/ipc/etc/amr/asr_24.amr"    , AUDIO_TYPE_ASR,     0, 1},  //亮度已调至最 
    {AUDIO_VOLUME_MAX           , "/ipc/etc/amr/asr_25.amr"    , AUDIO_TYPE_ASR,     0, 1},  //音量已调至最 
    {AUDIO_NIGHT_LIGHT_OFF      , "/ipc/etc/amr/asr_26.amr"    , AUDIO_TYPE_ASR,     0, 1},  //已关闭小夜灯
    {AUDIO_ASR_OK               , "/ipc/etc/amr/asr_27.amr"    , AUDIO_TYPE_ASR,     0, 1},  //好的
    {AUDIO_VOLUM_MUTE           , "/ipc/etc/amr/asr_28.amr"    , AUDIO_TYPE_ASR,     0, 1},  //已静音
};

static int g_each_fomat_size[] = {13, 14, 16, 18, 20, 21, 27, 32};

static CircularQueue<int, 32> g_queue_audio;
static sAudioRawBuffer g_aud_raw = {0};
static pthread_spinlock_t g_lock_amr;
static sDecoderInterface *g_decoder = NULL;

static bool amr_type_enqueue_front(int amr_type)
{
    int ret = 0;

    pthread_spin_lock(&g_lock_amr);
    ret = g_queue_audio.EnqueueFront(amr_type);
    pthread_spin_unlock(&g_lock_amr);

    return ret;
}

static bool amr_type_enqueue(int amr_type)
{
    int ret = 0;

    pthread_spin_lock(&g_lock_amr);
    ret = g_queue_audio.Enqueue(amr_type);
    pthread_spin_unlock(&g_lock_amr);

    return ret;
}

static bool amr_type_dequeue(int *amr_type)
{
    int ret = 0;

    pthread_spin_lock(&g_lock_amr);
    ret = g_queue_audio.Dequeue(*amr_type);
    pthread_spin_unlock(&g_lock_amr);

    return ret;
}

static bool amr_type_clear(void)
{
    int ret = 0;

    pthread_spin_lock(&g_lock_amr);
    g_queue_audio.ClearQueue();
    pthread_spin_unlock(&g_lock_amr);

    return ret;
}

static int get_audio_info_idx(AUDIO_PROMPT name, int *idx)
{
    return_val_if_fail(NULL != idx, FAILURE);

    int ret = FAILURE, cnt = 0, audio_num = 0;

    audio_num = sizeof(g_audio_info) / sizeof(g_audio_info[0]);

    for (cnt = 0; cnt < audio_num; cnt++) {
        if (g_audio_info[cnt].name == name) {
            *idx = cnt;
            ret = SUCCESS;
        }
    }

    if (FAILURE == ret) {
        ERR("no audio data matches\n");
    }

    return ret;
}

int get_amr_play_duration_ms_by_path(const char *path, int *duration)
{
    return_val_if_fail(NULL != path && NULL != duration, FAILURE);

    struct stat file_info = {0};
    int ret = 0, read_offset = 0, amr_format = 0, calc_dur = 0, read_bytes = 0;
    FILE *file = NULL;
    char buf[32] = {0};

    do {
        if (FAILURE == stat(path, &file_info)) {
            ERR("find %s error\n", path);
            ret = FAILURE;
            break;
        }

        file = fopen(path, "rb");
        if (NULL == file) {
            ERR("fopen %s failed\n", path);
            ret = FAILURE;
            break;
        }

        fread(buf, sizeof(unsigned char), strlen(AMR_FILE_HEAD), file);
        if (0 != strncmp((const char *)buf, AMR_FILE_HEAD, strlen(AMR_FILE_HEAD))) {
            ERR("fread error or file type is not amr\n");
            ret = FAILURE;
            break;
        }

        read_offset += AMR_FILE_HEAD_SIZE;

        while (read_offset < file_info.st_size) {
            ret = fread(buf, sizeof(unsigned char), 1, file);
            if (0 == ret) {
                dbg_audio("fread end\n");
                break;
            }

            amr_format = (buf[0] >> 3) & 0x000F;
            read_bytes = g_each_fomat_size[amr_format] - FRAME_HEAD_SIZE;

            ret = fread(&buf[1], sizeof(char), read_bytes, file);
            if (0 == ret) {
                ERR("fread error\n");
                ret = FAILURE;
                break;
            }

            calc_dur += 20;
        }
    } while(0);

    if (SUCCESS == ret) {
        *duration = calc_dur;
    } else {
        *duration = 0;
    }

    if (file != NULL) {
        fclose(file);
        file = NULL;
    }

    DBG("%s calc display duration:%d\n", path, *duration);

    return ret;
}

/*
 * 计算函数 audio_amr_decoder_process(), duration 单位 s 并冗余 2s，保证播完整亦可防止 cpu 过高
 * 未播过，g_audio_info[idx].duration 则未初始化，使用默认的 3*1000;
 * 播过后，加 500ms
 */
int get_amr_ms_by_name(AUDIO_PROMPT name)
{
    int ms = 4*1000;

    do {
        int idx = 0;
        if (SUCCESS == get_audio_info_idx(name, &idx)) {
            if (g_audio_info[idx].duration) {
                ms = g_audio_info[idx].duration * 1000;
            }
        }
    } while(0);

    return ms;
}

int get_amr_path_from_alarm_type(AUDIO_PROMPT name, char **path)
{
    return_val_if_fail(NULL != path, FAILURE);

    int ret = FAILURE, idx = 0;

    ret = get_audio_info_idx(name, &idx);
    if (SUCCESS == ret) {
        *path = (char *)g_audio_info[idx].path;
    } else {
        *path = NULL;
    }

    return ret;
}

int audio_amr_decoder_process(const char *path, unsigned char **audio, int *len,
                              int *duration)
{
    return_val_if_fail(NULL != path && NULL != audio && NULL != len, FAILURE);

    struct stat stAmr = {0};
    int  ret = 0;
    FILE *fp = NULL;

    if (-1 == stat(path, &stAmr)) {
        ERR("decoder_process [%s] stat fail\n", path);
        return -1;
    }

    size_t len_raw_aud = stAmr.st_size * 40;
    size_t decoder_offset = 0;

    do {
        if (0 == len_raw_aud) {
            ERR("audio file size may be 0\n");
            ret = -1;
            break;
        }

        if (NULL != g_aud_raw.buf && len_raw_aud > g_aud_raw.len) {
            free(g_aud_raw.buf);
            g_aud_raw.buf = NULL;
        }

        if (NULL == g_aud_raw.buf) {
            g_aud_raw.buf = (uint8_t *)malloc(len_raw_aud);
            g_aud_raw.len = len_raw_aud;
        }

        if (NULL == g_aud_raw.buf) {
            ERR("malloc decoder buf failed!\n");
            ret = -1;
            g_aud_raw.len = 0;
            break;
        }

        fp = fopen(path, "rb");
        if (NULL == fp) {
            ERR("fopen %s fail: %s\n", path, strerror(errno));
            ret = -1;
            break;
        }

        ret = Decoder_Interface_init(&g_decoder);
        if (ret != 0 || NULL == g_decoder) {
            ERR("decoder init failed\n");
            ret = -1;
            break;
        }

        // read and verify magic number
        char szMagic[8] = {0};
        fread(szMagic, sizeof(char), strlen(AMR_FILE_HEAD), fp);

        if (strncmp(szMagic, AMR_FILE_HEAD, strlen(AMR_FILE_HEAD))) {
            ret = -1;
            break;
        }

        // find mode, read file
        enum Mode modeDec;
        short nBlockSize[16] = {12, 13, 15, 17, 19, 20, 26, 31, 5, 0, 0, 0, 0, 0, 0, 0};
        int nReadSize = 0, nFrames = 0;
        unsigned char nCodes[32] = {0};

        while (0 < fread(nCodes, sizeof (unsigned char), 1, fp)) {
            modeDec = (enum Mode)((nCodes[0] >> 3) & 0x000F);
            nReadSize = nBlockSize[modeDec];
            fread(&nCodes[1], sizeof(char), nReadSize, fp);
            
            nFrames++;
            Decoder_Interface_Decode(g_decoder, nCodes,
                                     (short *)(g_aud_raw.buf + decoder_offset), 0);
            decoder_offset += 320;
            if (len_raw_aud <= decoder_offset + 320) {
                DBG("amr buf no enough size:%lld, len_raw_aud=%d"
                    ", decoder_offset=%d, nFrames=%d\n",
                    stAmr.st_size, len_raw_aud, decoder_offset, nFrames);
                break;
            }
        }
    } while (0);

    DBG("decoder_offset:%d\n", decoder_offset);

    if (NULL != fp) {
        fclose(fp);
        fp = NULL;
    }

    if (0 == ret) {
        *audio = (unsigned char *)g_aud_raw.buf;
        *len = decoder_offset;
        /*播放时间计算=数据量(decoder_offset) / (位宽(16bit = 2byte) * 采样率(8K) * 声道数(1)) + 允许的误差(2s) */
        /*采样率这些参数需要跟sdk初始化的地方保存一致 */
        *duration = decoder_offset / (2 * 8000 * 1) + 2;
        DBG("%s success, st_size:%llu, pcm_size:%d\n", path, stAmr.st_size, decoder_offset);
        return 0;
    }

    DBG("%s FAIL\n", path);
    return -1;
}

int audio_decode_local_amr(const char *path, unsigned char **audio, int *len, int *duration)
{
    return_val_if_fail(NULL != path && NULL != audio && NULL != len, FAILURE);

    struct stat file_info = {0};
    int ret = 0, read_bytes = 0, push_offset = 0;
    int64_t start_time = 0;
    enum Mode amr_format = MR475;
    FILE *file = NULL;
    unsigned char buf[32] = {0};

    start_time = mono_msec();

    do {
        ret = Decoder_Interface_init(&g_decoder);
        if (ret != 0 || NULL == g_decoder) {
            ERR("decoder init failed\n");
            ret = -1;
            break;
        }

        if (FAILURE == stat(path, &file_info)) {
            ERR("find %s error\n", path);
            ret = FAILURE;
            break;
        }

        file = fopen(path, "rb");
        if (NULL == file) {
            ERR("fopen %s failed\n", path);
            ret = FAILURE;
            break;
        }

        fread(buf, sizeof(unsigned char), strlen(AMR_FILE_HEAD), file);
        if (0 != strncmp((const char *)buf, AMR_FILE_HEAD, strlen(AMR_FILE_HEAD))) {
            ERR("fread error or file type is not amr\n");
            ret = FAILURE;
            break;
        }

        *len = file_info.st_size * MAX_DECOMPRESSION_RATIO;
        *audio = (unsigned char *)malloc(*len);
        if (NULL == *audio) {
            ERR("audio data malloc failed\n");
            ret = FAILURE;
            break;
        }

        dbg_audio("audio data malloc len:%d\n", *len);

        memset(buf, 0, sizeof(buf));

        while (1) {
            ret = fread(buf, sizeof(unsigned char), 1, file);
            if (0 == ret) {
                dbg_audio("fread end\n");
                break;
            }

            amr_format = (enum Mode)((buf[0] >> 3) & 0x000F);
            read_bytes = g_each_fomat_size[amr_format] - FRAME_HEAD_SIZE;

            ret = fread(&buf[1], sizeof(char), read_bytes, file);
            if (0 == ret) {
                ERR("fread error\n");
                ret = FAILURE;
                break;
            }

            if (push_offset + 320 > *len) {
                ERR("malloc size is smaller than decoder size!\n");
                ret = FAILURE;
                break;
            }

            Decoder_Interface_Decode(g_decoder, buf, 
                                     (short *)((*audio) + push_offset), 0);
            push_offset += 320;
            *duration += 20;
        }

        dbg_audio("malloc:%d, used:%d, duration:%d\n", *len, push_offset, *duration);
    } while(0);

    memset(&(*audio)[push_offset], 0, *len - push_offset);

    if (NULL != file) {
        fclose(file);
        file = NULL;
    }

    if (FAILURE == ret) {
        ERR("get amr local\n");

        if (NULL != *audio) {
            free(*audio);
            *audio = NULL;
        }
        *len = 0;
        *duration = 0;
    }

    dbg_audio("decode %s takes %lld ms\n", path, mono_msec() - start_time);

    return ret;
}

static int audio_get_local_amr(int idx, AudioDataS *data)
{
    return_val_if_fail(idx >= 0 && idx < ARRAY_SIZE(g_audio_info) && 
                       NULL != data, FAILURE);

    int ret = 0;
    const char *szFile = NULL;

    do {
        szFile = g_audio_info[idx].path;
        
        if (g_audio_info[idx].name == AUDIO_ALARM_OTHER){
            if (access("/opt/custom/cust_other.amr", F_OK) == 0){
                szFile = "/opt/custom/cust_other.amr";
            }
        }
        
        ret = audio_amr_decoder_process(szFile, &data->audio, &data->len, &data->duration);
        if (SUCCESS == ret) {
            g_audio_info[idx].duration = data->duration;
            g_audio_info[idx].type = data->type;
        }
    } while(0);

    return ret;
}

int encode_audio_queue_push_amr(AUDIO_PROMPT name, int fast_play)
{
    int ret = 0, idx = 0;

    do {
        ret = get_audio_info_idx(name, &idx);
        if (FAILURE == ret) {
            break;
        }

        if (!g_audio_info[idx].is_force) {
            gpio_t gpio = {0};
            conf_get_gpiocfg(&gpio);

            if (!gpio.ao_prompt) {
                break;
            }
        }

        if (TRUE == fast_play) {
            ret = amr_type_enqueue_front(idx);
            break_if_fail(TRUE == ret, ret);

            ret = encode_audio_stop_playing();
        } else {
            ret = amr_type_enqueue(idx);
        }
        if (FAILURE == ret) {
            ERR("push queue failed\n");
        }
    } while(0);

    return ret;
}

int encode_audio_queue_get_amr(AudioDataS *audio)
{
    return_val_if_fail(NULL != audio, FAILURE);

    int ret = SUCCESS;
    int idx = 0;

    do {
        ret = amr_type_dequeue(&idx);
        if (FALSE == ret) {
            audio->audio = NULL;
            audio->len = 0;
            audio->duration = 0;
            audio->type = AUDIO_TYPE_NONE;
            ret = NO_AUDIO_DATA;
            break;
        }

        ret = audio_get_local_amr(idx, audio);
        break_if_fail(SUCCESS == ret, FAILURE);
    } while(0);

    return ret;
}

int encode_audio_queue_start(void)
{
    DBG("encode audio queue start\n");

    return SUCCESS;
}


void encode_audio_queue_clean(void)
{
    amr_type_clear();
}

int encode_audio_queue_stop(void)
{
    int ret = 0;

    DBG("encode audio queue stop\n");

    do {
        encode_audio_queue_clean();

        if (NULL != g_decoder) {
            Decoder_Interface_exit(&g_decoder);
        }

        if (NULL != g_aud_raw.buf) {
            free(g_aud_raw.buf);
            g_aud_raw.buf = NULL;
            g_aud_raw.len = 0;
        }
    } while(0);

    return ret;
}

struct audiomap {
    int nr_playde;      /* 已经播放的次数 */
    int nr_total;       /* 总计能够播放的次数 */
    int enable;         /* 默认无依赖=1，有依赖的默认=0，根据依赖动态更新 */
    AUDIO_PROMPT name;  /* 索引名 */
};

static struct audiomap amap[] = {
    { .nr_playde=0, .nr_total=1, .enable=FALSE, .name=AUDIO_RUNINGOUT_ID            },
    { .nr_playde=0, .nr_total=1, .enable=FALSE, .name=AUDIO_CSQ_WEAK_4G             },
    { .nr_playde=0, .nr_total=1, .enable=FALSE, .name=AUDIO_CSQ_STRONG_4G           },
    { .nr_playde=0, .nr_total=1, .enable=TRUE , .name=AUDIO_CHECK_SUCCESS_4G        },
    { .nr_playde=0, .nr_total=2, .enable=FALSE, .name=AUDIO_CHECK_FAIL_4G           },
    { .nr_playde=0, .nr_total=1, .enable=TRUE , .name=AUDIO_SIM_CHECK_SUCCESS_4G    },
    { .nr_playde=0, .nr_total=2, .enable=TRUE , .name=AUDIO_SIM_CHECK_FAIL_4G       },
    { .nr_playde=0, .nr_total=2, .enable=TRUE , .name=AUDIO_SIM_CHECK_ICCID_4G      },
    { .nr_playde=0, .nr_total=2, .enable=TRUE , .name=AUDIO_SIM4G_CON_FAIL          },
    { .nr_playde=0, .nr_total=2, .enable=TRUE , .name=AUDIO_AUTH_SUCCESS            },
    { .nr_playde=0, .nr_total=2, .enable=TRUE , .name=AUDIO_CON_FAIL                },
    { .nr_playde=0, .nr_total=2, .enable=TRUE , .name=AUDIO_SIM4G_SCAN_QRCODE       },
};

static int idx(AUDIO_PROMPT status)
{
    for (int i = 0; i < ARRAY_SIZE(amap); i++) {
        if (amap[i].name == status) {
            return i;
        }
    }

    /* 防止 idx 索引出错导致数据越界 return 0 */
    ERR("conditionally audio idx error\n");
    return 0;
}

/* 动态更新依赖开关 */
static void refresh_amap_enable(void)
{
    /* 信号强和弱只能播放一个 */
    amap[idx(AUDIO_CSQ_WEAK_4G)].enable = !amap[idx(AUDIO_CSQ_STRONG_4G)].nr_playde;
    amap[idx(AUDIO_CSQ_STRONG_4G)].enable = !amap[idx(AUDIO_CSQ_WEAK_4G)].nr_playde;

    /* 4G 模块 succ 和 fail 只能播放一个 */
    amap[idx(AUDIO_CHECK_FAIL_4G)].enable = !amap[idx(AUDIO_CHECK_SUCCESS_4G)].nr_playde;
    amap[idx(AUDIO_CHECK_SUCCESS_4G)].enable = !amap[idx(AUDIO_CHECK_FAIL_4G)].nr_playde;
}

int play_conditionally(AUDIO_PROMPT status)
{
    refresh_amap_enable();

    int i = idx(status);
    int to_play = amap[i].nr_playde < amap[i].nr_total;
    if (amap[i].enable && to_play) {
        amap[i].nr_playde++;
        DBG("play_conditionally %d, played %d\n", status, amap[i].nr_playde);
        encode_audio_queue_push_amr(status, FALSE);
    }

    return 0;
}

