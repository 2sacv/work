#ifdef PLATFORM_TENCENT
#include <string.h>
#include "debug.h"
#include "shm_buf.h"
#include "shm_buf_pool.h"
#include "confapi.h"
#include "encodeapi.h"
#include "encode_jpeg.h"
#include "utils.h"
#include "net_check.h"
#include "sim4g.h"
#include "js_scheduler.h"
#include "jconfig.h"
#include "jevent.h"
#include "encode_video.h"
#include "system_sch.h"
#include <pthread.h>

#include "js_rec.h"
#include "iv_usrex.h"
#include "iv_err.h"
#include "iv_av.h"
#include "iv_cs.h"
#include "iv_sys.h"
#include "qcloud_iot_export.h"
#include "encode_common.h"
#include "tencent_server.h"
#include "tencent_event_handle.h"
#include "tencent_cloud_storage.h"
#include "tencent_media_manage.h"
#include "tencent_living_stream.h"

extern JSScheduler sch_living;
tx_lv_cs_push_info_s g_cs_stream_info[MAX_STREAM_CHANNEL_NUM] = {{0, .type = STREAM_CLOUDSTORAGE},{0, .type = STREAM_CLOUDSTORAGE}};
tx_lv_cs_visitor_info_s g_cs_visitor_info[MAX_SENSOR_NUM] = {0,};


typedef struct {
    int      aserial;
    int      vserial;
    double   pre_time_s;
    double   stop_time_s;
} sCsAlarmInfo;

typedef struct {
    JSScheduler sch;
    JSTCHandle  hdl;
    int start;
    iv_cs_balance_info_s balance[MAX_SENSOR_NUM];
    int event_id[MAX_SENSOR_NUM][ALARM_END];
    int cs_channel;
    int event_running;  // 事件运行中
    int frozen_cnt;
    int event_total_cnt;
    sCsAlarmInfo alm_info;
} sCloudStorageRun;

static sCloudStorageRun run = {0, .event_id = {0}};
static sCloudStorageRun *g_run_cs = &run;
static pthread_mutex_t event_lock = PTHREAD_MUTEX_INITIALIZER;

int get_cs_chn(int sensor_id)
{
    return (MAX_SENSOR_NUM > 1) ? sensor_id + 1 : sensor_id;
}

int get_sensor_id(iv_cs_chn_e cs_chn)
{
    return (MAX_SENSOR_NUM > 1) ? cs_chn - 1 : cs_chn;
}

static void ivm_upload_alarm_event(int alarmType, int sensor_id)
{
    ivm_lock();
    g_ivm_objs.Event.m_uploadDeviceEvent.m_eventType = alarmType;
    g_ivm_objs.Event.m_uploadDeviceEvent.m_channel = sensor_id;
    strcpy(g_ivm_objs.Event.m_uploadDeviceEvent.m_payload, "humandetect");
    iv_dm_event_report("uploadDeviceEvent"); //这里填截图中的event_name
    DBG("iv_dm_event_report\n");
    ivm_unlock();

    return;
}

void cs_report_event_with_picture(int alarmType, int sensor_id)
{
    int ret = 0;
    char msg_info[16] = {0};
    char *jpeg_buf = NULL;
    int jpeg_len = LEN_JPEG;
    g_run_cs->event_running = TRUE;

    jpeg_buf = (char *)calloc(1, LEN_JPEG);
    if (NULL == jpeg_buf) {
        SYSLOG("calloc jpeg_buf failed!\n");
        g_run_cs->event_running = FALSE;
        return;
    }

    snprintf(msg_info, sizeof(msg_info)-1, "et=%d;", alarmType);
    DBG("msg_info [%s]\n", msg_info);
    if (alarmType < ALARM_CALL_CANCEL) {
        encode_snapshot_ex(jpeg_buf, &jpeg_len);
    }
    ret = iv_cs_event_directly_report((iv_cs_chn_e)get_cs_chn(sensor_id),
        alarmType, msg_info, 0, (uint8_t*)jpeg_buf, jpeg_len);
    if (ret) {
        ERR("iv_cs_event_directly_report failed, ret=%d\n", ret);
    } else {
        DBG("iv_cs_event_directly_report succ, jpeg_len=%d, sensor_id=%d\n", jpeg_len, sensor_id);
    }
    free(jpeg_buf);
    g_run_cs->event_running = FALSE;
}

/*
* 用于获取当前设备的云存的套餐信息
* 当 `timeout_s`大于0时，该接口为阻塞获取信息，直接从后台获取最新套餐信息，失败返回错误码`IV_ERR_CS_QUERY_SERVICE_TIMEOUT`，成功返回`IV_ERR_NONE`和套餐信息
* 当 `timeout_s`等于0时，该接口为非阻塞接口，直接返回`SDK`内的套餐缓存信息
* 当云存的套餐在后台发生变化后，正常情况下会及时更新到设备并缓存再SDK内，通过本接口直接查询SDK内缓存套餐信息效率更高
* 接口可在设备上线后云存`SDK`未初始化前进行调用来获取套餐信息，云存SDK未初始化时采用轮询方式调用该接口，建议只在第一次调用时采用阻塞方式，后续的调用采用非阻塞方式，否则会引起带宽资源浪费
*/
int check_cs_status(void)
{
    int cs_chn = 0;

    for (int i = 0; i < MAX_SENSOR_NUM; i++) {
        cs_chn = get_cs_chn(i);
        iv_cs_get_balance_info((iv_cs_chn_e)cs_chn, &g_run_cs->balance[i], 5);
        DBG("cs balance info [cs_chn:%d, cs_switch:%d, cs_days:%d, cs_type:%d," \
            "free_trial_remaining_sec:%d]\n", cs_chn, g_run_cs->balance[i].cs_switch,
            g_run_cs->balance[i].cs_days, g_run_cs->balance[i].cs_type,
            g_run_cs->balance[i].free_trial_remaining_sec);
    }

    return TRUE;
}

int event_cs_status(int chn)
{
    int cs_chn = get_cs_chn(chn);
    iv_cs_get_balance_info((iv_cs_chn_e)cs_chn, &g_run_cs->balance[chn], 0);
    if (g_run_cs->balance[chn].cs_switch == TRUE 
        && g_run_cs->balance[chn].cs_type == CS_TYPE_EVENT) {
        return TRUE;
    }

    return FALSE;
}

// 检查全时云存储服务是否已开通
// 返回: TRUE-已开通 FALSE-未开通
int is_full_time_cs_enabled(void)
{
    int cs_chn = 0;
    iv_cs_balance_info_s cs_balance[MAX_SENSOR_NUM] = {0};

    for (int i = 0; i < MAX_SENSOR_NUM; i++) {
        cs_chn = get_cs_chn(i);
        iv_cs_get_balance_info((iv_cs_chn_e)cs_chn, &cs_balance[i], 0);
        DBG("cs balance info [cs_chn:%d, cs_switch:%d, cs_days:%d, cs_type:%d, free_trial_remaining_sec:%d] \n", \
            cs_chn, cs_balance[i].cs_switch, cs_balance[i].cs_days, cs_balance[i].cs_type, cs_balance[i].free_trial_remaining_sec);
    }
    for (int i = 0; i < MAX_SENSOR_NUM; i++) {
        if (cs_balance[i].cs_switch == 1 && cs_balance[i].cs_type == CS_TYPE_FULL_TIME) {
            return TRUE;
        }
    }

    return FALSE;
}

static void report_alarm_picture(int alarmType, int sensor_id)
{
    int cs_chn = 0;
    int ret = SUCCESS;
    static int alm_stat[ALARM_END] = {0};
    static struct timespec ts[MAX_SENSOR_NUM][ALARM_END] = {0};
    AlarmInfocfg alarm_info = {0};
    conf_get_alarminfo_cfg(&alarm_info);

    if (ts[sensor_id][alarmType].tv_sec == 0 && ts[sensor_id][alarmType].tv_nsec == 0) {
        ts[sensor_id][alarmType].tv_sec -= (alarm_info.interval * 1000);
    }

    //获取本地套餐信息
    for (int i = 0; i < MAX_SENSOR_NUM; i++) {
        cs_chn = get_cs_chn(i);
        ret = iv_cs_get_balance_info((iv_cs_chn_e)cs_chn, &g_run_cs->balance[i], 0);
        if (ret || 0 == g_run_cs->balance[i].cs_switch) {
            DBG("get cs balance info [cs_chn:%d, ret:%d, cs_switch:%d]\n", cs_chn, ret, g_run_cs->balance[i].cs_switch);
            continue;
        }
    }

    if (1 == g_run_cs->balance[sensor_id].cs_switch) {
        trigger_cs_event(alarmType, sensor_id);
        return;
    }

    if (!ms_clock_is_timeup(&ts[sensor_id][alarmType], alarm_info.interval * 1000) && alm_stat[alarmType]) {
        return ;
    }

    alm_stat[alarmType] = TRUE;

    ivm_upload_alarm_event(alarmType, sensor_id);
    cs_report_event_with_picture(alarmType, sensor_id);

    return;
}

static int get_cs_quality(tx_lv_cs_visitor_info_s* p_cs_visitor_info)
{
    int video_channel = 0;
    RecordCtrlS record_info = {0,};
    conf_get_recordcfg(&record_info);

    if (record_info.rec_type == VIDEO_CHN_MAIN) {
        p_cs_visitor_info->video_res_type = IV_AVT_VIDEO_RES_SD;
    } else {
        p_cs_visitor_info->video_res_type = IV_AVT_VIDEO_RES_FL;
    }
    video_channel = tencent_get_video_channel(p_cs_visitor_info->sensor_id, p_cs_visitor_info->video_res_type, 1);
    return video_channel;
}

/*
 * 云存生成策略:
 * 1. 全天云存: 按通道开通云存,开通后即生效
 * 2. 事件云存: 多目设备,一次报警会录制事件云存
*/
void trigger_cs_event(int alarmType, int sensor_id)
{
    int ret = 0;
    int cs_chn = 0;
    int video_channel = 0;
    tSBFrame frm_info = {SHM_ERR_FAILDE,};
    uint32_t ev_time_s[MAX_SENSOR_NUM] = {0};
    char msg_info[64] = {0};

    // 已经在推云存，且相同报警事件不重复调用 iv_cs_event_start
    if (g_run_cs->frozen_cnt > 0 && g_run_cs->event_id[sensor_id][alarmType] != 0) {
        DBG("Already pushing cloud storage, skip. frozen_cnt:%d\n", g_run_cs->frozen_cnt);
        return;
    }

    // 事件物模型
    ivm_upload_alarm_event(alarmType, sensor_id);

    DBG("trigger_cs_event, alarmType:%d\n", alarmType);
    g_run_cs->event_running = TRUE;

    pthread_mutex_lock(&event_lock);
    for (int i = 0; i < MAX_SENSOR_NUM; i++) {
        if (1 == g_run_cs->balance[i].cs_switch) {
            g_run_cs->event_id[i][alarmType] = (i == sensor_id) ? alarmType : INVALID_EVENT;
            cs_chn = get_cs_chn(i);
            snprintf(msg_info, sizeof(msg_info)-1, "et=%d;", g_run_cs->event_id[i][alarmType]);

            tx_lv_cs_visitor_info_s* p_cs_visitor_info = &g_cs_visitor_info[sensor_id];
            // 续传非相同报警事件的云存，无需预录制
            if (g_run_cs->frozen_cnt <= 0) {
                video_channel = get_cs_quality(p_cs_visitor_info);
                tencent_get_video_channel(sensor_id, p_cs_visitor_info->video_res_type, 1);
                ret = handle_alarm_pre_record(get_shm_buf_pool(video_channel), &frm_info, (time_t *)&ev_time_s[i]);
                g_run_cs->alm_info.pre_time_s = get_usec_of_day()/1000000.0 - ev_time_s[i];
                // 保证上一段云存录像结束时间小于预录制开始时间点2s,防止云存拼接
                if (ret != SUCCESS ||  g_run_cs->alm_info.pre_time_s - g_run_cs->alm_info.stop_time_s <= 2) {
                    g_run_cs->alm_info.pre_time_s = 0;
                    g_run_cs->alm_info.vserial = -1;
                    g_run_cs->alm_info.aserial = 0;
                } else {
                    g_run_cs->alm_info.aserial = get_serial_by_vts(get_shm_buf_pool(SHM_BUF_AUDIO_AAC), frm_info.frame_timestamp);
                    g_run_cs->alm_info.vserial = frm_info.frame_serial;
                }
            } else {
                    g_run_cs->alm_info.pre_time_s = 0;
                    g_run_cs->alm_info.vserial = 0;
                    g_run_cs->alm_info.aserial = 0;
            }
            ret = iv_cs_event_start_ext((iv_cs_chn_e)cs_chn, (int32_t)g_run_cs->event_id[i][alarmType], 
                                        msg_info, g_run_cs->alm_info.pre_time_s+ev_time_s[i], g_run_cs->alm_info.pre_time_s);
            if (ret != 0) {
                ERR("iv_cs_event_start_ext fail ret:%d\n", ret);
            }

            g_run_cs->frozen_cnt = CS_EVENT_START_TIME*1000/MS_CS;
        }
        DBG("cs balance info [chn:%d, cs_switch:%d, cs_days:%d, cs_type:%d, free_trial_remaining_sec:%d] \n", \
            cs_chn, g_run_cs->balance[i].cs_switch, g_run_cs->balance[i].cs_days, g_run_cs->balance[i].cs_type, g_run_cs->balance[i].free_trial_remaining_sec);
    }

    pthread_mutex_unlock(&event_lock);
    g_run_cs->event_running = FALSE;

    return;
}

static void cs_start_stream_cb(void* data)
{
    int video_channel = 0;
    int *sensor_id = (int *)data;

    tx_lv_cs_visitor_info_s* p_cs_visitor_info = &g_cs_visitor_info[*sensor_id];
    p_cs_visitor_info->sensor_id = *sensor_id;
    video_channel = get_cs_quality(p_cs_visitor_info);

    p_cs_visitor_info->running = 1;
    p_cs_visitor_info->video_channel = video_channel;
    p_cs_visitor_info->waiting_key_frame = 1;
    p_cs_visitor_info->request_key_frame = 1;
    tx_lv_cs_push_info_s *p_cs_stream_info = &g_cs_stream_info[video_channel];
    g_cs_stream_info[video_channel].video.video_channel = video_channel;
    p_cs_stream_info->cnt++;
    if (p_cs_stream_info->running) {
        ERR("cnt:%d, sensor_id:%u,video_channel:%d, has already started!\n", p_cs_stream_info->cnt, p_cs_visitor_info->sensor_id, video_channel);
        return;
    }
    tencent_check_drop_cache();
    tencent_live_stream_init(p_cs_stream_info);
    encode_immediate_iframe((CH_FS_E)p_cs_visitor_info->video_channel);
    p_cs_stream_info->video.frame_serial = g_run_cs->alm_info.vserial;
    p_cs_stream_info->audio.frame_serial = g_run_cs->alm_info.aserial;
    DBG("serial:%d\n",  g_run_cs->alm_info.vserial);
    p_cs_stream_info->running = 1;
    DBG("start_cs_stream_cb, video_channel:%d\n", p_cs_visitor_info->video_channel);
    js_create_timer_r(sch_living, 100, 40, tencent_push_live_video, (void *)p_cs_stream_info, &p_cs_stream_info->video.v_handle);
    js_create_timer_r(sch_living, 100, 30, tencent_push_live_audio, (void *)p_cs_stream_info, &p_cs_stream_info->audio.a_handle);

    return;
}

static int cs_start_cb(iv_cs_chn_e channel)
{
    DBG("cs start, channel:%d\n", channel);
    int sensor_id = get_sensor_id(channel);
    int rc = 0;
    uint64_t time = 0;
    char buf[256] = {0};

    rc = iv_sys_get_time(&time);
    if (QCLOUD_RET_SUCCESS == rc) {
        DBG("sys get system time is [%s]\n", get_timestr2(time/1000, buf, sizeof(buf)));
    } else {
        ERR("sys get system time failed!\n");
    }

    js_run_function(sch_living, cs_start_stream_cb, (void*)&sensor_id, 1);

    return 0;
}

static void cs_stop_stream_cb(void* data)
{
    int video_channel = 0;
    int *sensor_id = (int *)data;

    tx_lv_cs_visitor_info_s *p_visitor_info = &g_cs_visitor_info[*sensor_id];

    video_channel = tencent_get_video_channel(p_visitor_info->sensor_id, p_visitor_info->video_res_type, 1);
    tx_lv_cs_push_info_s *p_cs_stream_info = &g_cs_stream_info[video_channel];

    memset(p_visitor_info, 0, sizeof(tx_lv_cs_visitor_info_s));

    if (p_cs_stream_info->cnt > 0) {
        p_cs_stream_info->cnt--;
        DBG("cnt:%d video_channel:%d\n", p_cs_stream_info->cnt, video_channel);
    }

    if ((0 == p_cs_stream_info->cnt) && (p_cs_stream_info->video.v_handle)) {
        p_cs_stream_info->running = 0;
        js_delete_timer_r(&p_cs_stream_info->video.v_handle);
        js_delete_timer_r(&p_cs_stream_info->audio.a_handle);
        DBG("p_push_stream_info running is 0, delete handle\n");
    }

    return;
}

static int cs_stop_cb(iv_cs_chn_e channel)
{
    int sensor_id = get_sensor_id(channel);

    DBG("cs stop, sensor_id = %d\n", sensor_id);

    js_run_function(sch_living, cs_stop_stream_cb, (void*)&sensor_id, 1);

    return 0;
}

static void cs_event_stop_cb(void)
{
    int ret = 0;
    int cs_chn = 0;
    char msg_info[128] = {0};

    for (int i = 0; i < MAX_SENSOR_NUM; i++) {
        cs_chn = get_cs_chn(i);
        /*if (g_run_cs->balance[i].cs_type == CS_TYPE_FULL_TIME) {
            continue;
        }*/
        for (int j = ALARM_MD; j < ALARM_END; j++) {
            if (g_run_cs->event_id[i][j] == 0) {
                continue;
            }
            snprintf(msg_info, sizeof(msg_info)-1, "et=%d;", g_run_cs->event_id[i][j]);
            ret = iv_cs_event_stop_ext((iv_cs_chn_e)cs_chn, g_run_cs->event_id[i][j], msg_info, 0, 0);
            if (ret != 0) {
                ERR("cs_stop ret:%d cs_chn:%d s_cs_event_id:%d msg_info:%s cs_type:%d\n", ret, cs_chn, g_run_cs->event_id[i][j], msg_info, g_run_cs->balance[i].cs_type);
                if (g_run_cs->balance[i].cs_type == CS_TYPE_EVENT && ret == IV_ERR_CS_EVENT_IS_VALID) {  // SDK start失败,必须由设备端主动停止云存
                    cs_stop_cb((iv_cs_chn_e)cs_chn);
                }
            } else {
                DBG("iv_cs_event_stop success cs_chn:%d\n", cs_chn);
            }
            g_run_cs->alm_info.stop_time_s = get_usec_of_day()/1000000.0;
            g_run_cs->event_id[i][j] = 0;
        }
    }

    return;
}

static int cs_image_cb(iv_cs_chn_e channel, int event_id, uint8_t **pic, int32_t *size)
{
    Log_d("%s chn: %d, event_id: %d\n", __func__, channel, event_id);

    int jpeg_len = LEN_JPEG;
    char *jpeg_buf = (char *)malloc(LEN_JPEG);
    if (jpeg_buf) {
        encode_snapshot_ex(jpeg_buf, &jpeg_len);
        *pic  = (uint8_t *)jpeg_buf;
        *size = jpeg_len;
        DBG("%s succ, channel: %d, event_id: %d, jpeg_buf: %p, jpeg_len: %d\n",
            __func__, channel, event_id, jpeg_buf, jpeg_len);
    } else {
        ERR("%s malloc jpeg buf fail\n", __func__);
    }

    return 0;
}

static int cs_image_result_cb(iv_cs_chn_e channel, uint8_t **pic, int32_t err_code)
{
    Log_d("%s, channel: %d, pic: %p, err_code: %d\n", __func__, channel, *pic, err_code);
    DBG("%s, channel: %d, pic: %p, err_code: %d\n", __func__, channel, *pic, err_code);
    if (*pic) {
        free(*pic);
    }

    return 0;
}

static int cs_upload_state(iv_cs_upload_info_s *info)
{
    int i = 0;
    DBG("cs upload state:\n");
    for (i = 0; i < info->num; i++) {
		if (info->slice_info[i].state != IV_CS_UPLOAD_OK) {
        	SYSLOG("state %d, size %d / %d, frame %d, %d\n",
               info->slice_info[i].state, info->slice_info[i].upload_size,
               info->slice_info[i].total_size, info->slice_info[i].frame_seq_a,
               info->slice_info[i].frame_seq_b);
		} else {
        	DBG("state %d, size %d / %d, frame %d, %d\n",
               info->slice_info[i].state, info->slice_info[i].upload_size,
               info->slice_info[i].total_size, info->slice_info[i].frame_seq_a,
               info->slice_info[i].frame_seq_b);
		}
    }
    printf("\n");
    return 0;
}

static int cs_notify_msg_cb(iv_cs_chn_e channel,
        iv_cs_notify_msg_type_e notify_msg_type, iv_cs_notify_msg_data *pst_notify_data)
{
    DBG("cloud storage notify msg type:%d, channel:%d\n", notify_msg_type, channel);

    switch (notify_msg_type) {
        case IV_CS_AV_UPLOAD_STATE_MSG:
            cs_upload_state(pst_notify_data->av_result_info);
            break;
        default:
            break;
    }

    return 0;
}

void cs_get_media_info(int sensor_id, int cs_chn, iv_cm_av_data_info_s *pstAvInfo)
{
    int width = 0;
    int height = 0;
    VideoEncS videoenc = {0};
    RecordCtrlS record_ctrl = {0};

    conf_get_videocfg(&videoenc);
    conf_get_recordcfg(&record_ctrl);

    if (MAX_SENSOR_NUM > 1) {
        if (cs_chn == VIDEO_CHN_MAIN) { //主码流
            encode_vencsize_to_resolution(videoenc.enc[sensor_id].vencsize, &width, &height);
            pstAvInfo->u32Framerate = videoenc.enc[sensor_id].fps;
            pstAvInfo->eVideoType = (VENC_FORMAT_H265 == videoenc.enc[sensor_id].codec)?IV_CM_VENC_TYPE_H265:IV_CM_VENC_TYPE_H264;
        } else {
            encode_vencsize_to_resolution(videoenc.enc[2].vencsize, &width, &height);
            pstAvInfo->u32Framerate = videoenc.enc[2].fps;
            pstAvInfo->eVideoType = (VENC_FORMAT_H265 == videoenc.enc[2].codec)?IV_CM_VENC_TYPE_H265:IV_CM_VENC_TYPE_H264;
        }
    } else {
        encode_vencsize_to_resolution(videoenc.enc[cs_chn].vencsize, &width, &height);
        pstAvInfo->u32Framerate = videoenc.enc[cs_chn].fps;
        pstAvInfo->eVideoType = (VENC_FORMAT_H265 == videoenc.enc[cs_chn].codec)?IV_CM_VENC_TYPE_H265:IV_CM_VENC_TYPE_H264;
    }

    pstAvInfo->u32VideoWidth = width;
    pstAvInfo->u32VideoHeight = height;

    // audio
    pstAvInfo->eAudioType = IV_CM_AENC_TYPE_AAC;
    pstAvInfo->eAudioMode = IV_CM_AENC_MODE_MONO;//单声道
    pstAvInfo->eAudioBitWidth = IV_CM_AENC_BIT_WIDTH_16;
    pstAvInfo->eAudioSampleRate = IV_CM_AENC_SAMPLE_RATE_16000;
    pstAvInfo->u32SampleNumPerFrame = 1024;

}

static void cb_channel_change(int id, void *p_src, int size, void *ctx)
{
    RecordCtrlS * record_info = (RecordCtrlS *)p_src;

    if (g_run_cs->cs_channel != record_info->rec_type) {
        // 切换通道,重新初始化云存
        do_cs_reset();
    }
}

static void cb_codec_reset(int id, void *p_src, int size, void *ctx)
{
    int *video_chn = (int *)p_src;

    // 在上传的云存通道上重启编码器,重新开始云存
    if (*video_chn == g_run_cs->cs_channel) {
        // 编码器重置,重新初始化云存
        do_cs_reset();
    }
}

int cs_dump_file_cb(iv_cs_chn_e channel, iv_cs_up_bk_state_e state,
        uint64_t start_ts, uint64_t end_ts, char *file_name, uint8_t *buf, uint32_t len)
{
    char f_name[64] = {0};
    const char *path = "/tmp/cs/";
    static FILE *fp_ts[CS_CH_NUM];

    if (access(path, F_OK) != 0) {
        return 0;
    }

    if (state == CS_UP_BK_START) {
        DBG("channel:%d CS_UP_BK_START \n", channel);
        /* 固定文件名，仅存一份，wb 自动覆盖 */
        sprintf(f_name, "%s%d_cs_file.ts", path, channel);
        fp_ts[channel] = fopen(f_name, "wb");
    } else if (state == CS_UP_BK_RUNNING) {
        if (fp_ts[channel]) {
            fwrite(buf, len, 1, fp_ts[channel]);
        }
    } else if (state == CS_UP_BK_END) {
        DBG("channel:%d CS_UP_BK_END, file is %s, start_ts is %llu, end_ts is %llu\n",
                channel, file_name, start_ts, end_ts);
        if (fp_ts[channel]) {
            fclose(fp_ts[channel]);
            fp_ts[channel] = NULL;
        }
    }
    return 0;
}

static void cs_calc_event_cnt(void *data)
{
    int frozen_time = (g_run_cs->frozen_cnt*MS_CS)/1000;
    
    do {
        if (frozen_time <= 0 || frozen_time > CS_EVENT_ADD_TIME) {
            break;
        }
        // 开了云存并且已经在上传云存

        g_run_cs->frozen_cnt = (CS_EVENT_ADD_TIME*1000)/MS_CS;
        DBG("frozen_cnt: %d->%d\n", frozen_time, g_run_cs->frozen_cnt);

    } while (0);

    return;
}

static void alarm_event_cb(int id, void *p_src, int size, void *ctx)
{
    int sensor_id = 0;
    if (p_src != NULL) {
        sensor_id = *(int *)p_src;
    }
    if (!platform_on_line()) {
        ERR("TENCENT CLOUD NOT ONLINE\n");
        return;
    }
    AlarmTypeE alarmType = ALARM_END;

    switch (id) {
    case JEvent_AlarmMD:
        alarmType = ALARM_MD;
        break;
    case JEvent_AlarmVgline:
        alarmType = ALARM_VGL;
        break;
    case JEvent_AlarmVgrect:
        alarmType = ALARM_VGR;
        break;
    case JEvent_Alarmhumadetect:
        alarmType = ALARM_HD;
        break;
    case JEvent_AlarmCar:
        alarmType = ALARM_CAR;
        break;
    case JEvent_AlarmPet:
        alarmType = ALARM_PET;
        break;
    case JEvent_AlarmCry:
        alarmType = ALARM_CRY;
        break;
    case JEvent_AlarmVMask:
        alarmType = ALARM_VMASK;
        break;
    case JEvent_SceneChange:
        alarmType = ALARM_SCENE;
        break;
    default:
        ERR("jevent invaild\n");
        return;
    }

    js_run_function(sch_living, cs_calc_event_cnt, NULL, 1);

    //获取本地套餐信息
    int cs_chn = get_cs_chn(sensor_id);
    iv_cs_get_balance_info((iv_cs_chn_e)cs_chn, &g_run_cs->balance[sensor_id], 0);
    // 未开通云存套餐或全天云存的定时报警不上报
    if (0 == g_run_cs->balance[sensor_id].cs_switch || 
        CS_TYPE_FULL_TIME == g_run_cs->balance[sensor_id].cs_type) {
        if (ALARM_SCENE == alarmType) {
            DBG("not report scene, cs_switch:%d, cs_type:%d\n", 
                g_run_cs->balance[sensor_id].cs_switch, g_run_cs->balance[sensor_id].cs_type);
            return;
        }
    }

    report_alarm_picture(alarmType, sensor_id);

    return;
}

static void cs_loop(void *ctx)
{
    if (g_run_cs->frozen_cnt == 0) {
        return;
    }

    pthread_mutex_lock(&event_lock);
    if (--g_run_cs->frozen_cnt == 0 || ++g_run_cs->event_total_cnt == (CS_EVENT_MAX_TIME * 1000 / MS_CS)) {
        g_run_cs->frozen_cnt = 0;
        g_run_cs->event_total_cnt = 0;
        cs_event_stop_cb();
    } else {
        dbg_tencent("[cs] skip frozen Cnt=[%d], event_total_cnt = [%d]\n",
                g_run_cs->frozen_cnt, g_run_cs->event_total_cnt); 
    }
    pthread_mutex_unlock(&event_lock);

    return;
}

int cs_init(void)
{
    DBG("cs_init\n");

    int ret     = 0;
    int cs_chn  = 0;
    RecordCtrlS record_info = {0,};
    iv_cs_init_parm_s stCsInitParm = {0};
    iv_cs_channel_params_s stChnParams[MAX_SENSOR_NUM];

    if (g_run_cs->sch == NULL) {
        g_run_cs->sch = js_create_scheduler((char*)"cs_sch");
        if (NULL == g_run_cs->sch) {
            ERR("create cs scheduler fail\n");
            return FAILURE;
        }
    }
    memset(&stCsInitParm, 0, sizeof(stCsInitParm));
    memset(stChnParams, 0, sizeof(stChnParams));
    attach_event_async(JEvent_DevVideoReport , cb_codec_reset   , NULL);
    conf_get_recordcfg(&record_info);
    g_run_cs->cs_channel = record_info.rec_type;
    stCsInitParm.channel_num = MAX_SENSOR_NUM;

    for (uint32_t i = 0; i < stCsInitParm.channel_num; i++) {
        cs_chn = get_cs_chn(i);
        cs_get_media_info(i, g_run_cs->cs_channel, &stChnParams[i].av_fmt);
        if (stChnParams[i].av_fmt.eAudioType == IV_CM_AENC_TYPE_AAC) {
            stChnParams[i].av_fmt.u32AudioCodecOption = IV_CM_AAC_TYPE_LC;
        }
        stChnParams[i].congestion_cfg.enable = 0;
        stChnParams[i].channel_id    = (iv_cs_chn_e)cs_chn;
        if (g_run_cs->cs_channel == VIDEO_CHN_MAIN) {
            stChnParams[i].u32MaxGopSize = MAX_FRAME_BYTES * 3;
        } else {
            stChnParams[i].u32MaxGopSize = MAX_FRAME_BYTES;
        }
        stChnParams[i].cs_fmt        = IV_CS_FORMAT_FMP4;
        stChnParams[i].event_report_opt = CS_EVENT_VIDEO_OR_PIC_REQ;
    }

    stCsInitParm.func_cb.iv_cs_push_stream_start_cb     = cs_start_cb;
    stCsInitParm.func_cb.iv_cs_push_stream_stop_cb      = cs_stop_cb;
    stCsInitParm.func_cb.iv_cs_event_capture_picture_cb = cs_image_cb;
    stCsInitParm.func_cb.iv_cs_event_picture_result_cb  = cs_image_result_cb;
    stCsInitParm.func_cb.iv_cs_notify_cb                = cs_notify_msg_cb;
    stCsInitParm.func_cb.iv_cs_dump_file_cb             = cs_dump_file_cb;
    stCsInitParm.ch_params = stChnParams;
    ret = iv_cs_init(&stCsInitParm);
    if (ret != SUCCESS) {
        ERR("iv_cs_init error:%d\n", ret);
        return ret;
    }
    iv_cs_set_trans_time(UPLOAD_TIMEOUT, REPLY_TIMEOUT);

    // devrecordcfg -act set -rec_type 0/1
    attach_config(JEvent_RecordCfgChg        , cb_channel_change, NULL);
    attach_event_async(JEvent_AlarmMD        , alarm_event_cb   , NULL);
    attach_event_async(JEvent_AlarmVgline    , alarm_event_cb   , NULL);
    attach_event_async(JEvent_AlarmVgrect    , alarm_event_cb   , NULL);
    attach_event_async(JEvent_Alarmhumadetect, alarm_event_cb   , NULL);
    attach_event_async(JEvent_AlarmCar       , alarm_event_cb   , NULL);
    attach_event_async(JEvent_AlarmPet       , alarm_event_cb   , NULL);
    attach_event_async(JEvent_AlarmCry       , alarm_event_cb   , NULL);
    attach_event_async(JEvent_AlarmVMask     , alarm_event_cb   , NULL);
    attach_event_async(JEvent_SceneChange    , alarm_event_cb   , NULL);

    //事件云存触发建议事件至少持续20s及以上
    js_create_timer_r(g_run_cs->sch, 200, 1*MS_CS, cs_loop, NULL, &g_run_cs->hdl);

    //获取本地套餐信息
    check_cs_status();
    g_run_cs->start = TRUE;

    return ret;
}

static void cs_uninit_stream_cb(void* data)
{
    int video_channel = 0;
    for(int i = 0; i < MAX_SENSOR_NUM; i++) {
        tx_lv_cs_visitor_info_s* p_cs_visitor_info = &g_cs_visitor_info[i];
        video_channel = tencent_get_video_channel(p_cs_visitor_info->sensor_id, p_cs_visitor_info->video_res_type, 1);
        tx_lv_cs_push_info_s *p_cs_stream_info = &g_cs_stream_info[video_channel];

        if (p_cs_visitor_info->running == 1) {
            memset(p_cs_visitor_info, 0, sizeof(tx_lv_cs_visitor_info_s));

            if (p_cs_stream_info->cnt > 0) {
                p_cs_stream_info->cnt--;
                DBG("cnt:%d video_channel:%d\n", p_cs_stream_info->cnt, video_channel);
            }

            if ((0 == p_cs_stream_info->cnt) && (p_cs_stream_info->video.v_handle)) {
                p_cs_stream_info->running = 0;
                js_delete_timer_r(&p_cs_stream_info->video.v_handle);
                js_delete_timer_r(&p_cs_stream_info->audio.a_handle);
                DBG("p_cs_stream_info running is 0, delete handle\n");
            }
        }
    }
}

int cs_uninit(void)
{
    int ret = SUCCESS;

    // 防止云存未初始化,或者多次反复进行反初始化
    if (g_run_cs->start== FALSE) {
        return SUCCESS;
    }

    // 反初始化云存前注销事件,防止反初始化时处理start
    detach_config(JEvent_RecordCfgChg   , cb_channel_change , NULL);
    detach_event(JEvent_DevVideoReport  , cb_codec_reset    , NULL);
    detach_event(JEvent_AlarmMD         , alarm_event_cb    , NULL);
    detach_event(JEvent_AlarmVgline     , alarm_event_cb    , NULL);
    detach_event(JEvent_AlarmVgrect     , alarm_event_cb    , NULL);
    detach_event(JEvent_Alarmhumadetect , alarm_event_cb    , NULL);
    detach_event(JEvent_AlarmCar        , alarm_event_cb    , NULL);
    detach_event(JEvent_AlarmPet        , alarm_event_cb    , NULL);
    detach_event(JEvent_AlarmCry        , alarm_event_cb    , NULL);
    detach_event(JEvent_AlarmVMask      , alarm_event_cb    , NULL);
    detach_event(JEvent_SceneChange     , alarm_event_cb     , NULL);

    /* 
     * 将iv_cs_event_start 和 iv_cs_event_stop当成整体
     * g_run_cs->event_running == 0 表示将iv_cs_event_start未运行
     * g_run_cs->frozen_cnt == 0表示iv_cs_event_stop未运行
     *
    */
    while (g_run_cs->event_running || g_run_cs->frozen_cnt) {
        DBG("wait cs event end\n");
        sleep(1);
    }

    // 停止推流
    js_run_function(sch_living, cs_uninit_stream_cb, (void*)NULL, 1);
    g_run_cs->start = FALSE;
    for (int i = 0; i < MAX_SENSOR_NUM; i++) {
        for (int j = ALARM_MD; j < ALARM_END; j++) {
            g_run_cs->event_id[i][j] = 0;
        }
    }
    g_run_cs->alm_info.aserial = 0;
    g_run_cs->alm_info.vserial = 0;
    g_run_cs->alm_info.pre_time_s = 0;
    g_run_cs->alm_info.stop_time_s = 0;
    if (g_run_cs->hdl) {
        js_delete_timer_r(&g_run_cs->hdl);
    }

    js_delete_scheduler(g_run_cs->sch);
    g_run_cs->sch = NULL;

    // 退出云存模块
    ret = iv_cs_exit();
    if (ret == FAILURE) {
        ERR("iv_cs_exit fail\n");
    }

    return ret;
}

#endif
