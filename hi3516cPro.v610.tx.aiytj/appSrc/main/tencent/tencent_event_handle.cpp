#ifdef PLATFORM_TENCENT
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include "debug.h"
#include "jcpService.h"
#include "utils.h"
#include "jconfig.h"
#include "jevent.h"
#include <sys/reboot.h>
#include <linux/reboot.h>
#include "g_sys.h"
#include "cJSON.h"
#include "confapi.h"
//#include "encode_main.h"
#include "jconfig.h"
#include "time_sync.h"
#include "net_check.h"
#include "time_config.h"
#include "tencent_event_handle.h"
#include "tencent_media_manage.h"
#include "tencent_param_conf.h"
#include "record_lib.h"
#include "tencent_record_play.h"
#include "tencent_server.h"
#include "tencent_cloud_storage.h"
#include "tencent_living_stream.h"
#include "iv_av.h"
#include "iv_cm.h"
#include "iv_config.h"
#include "qcloud_iot_export.h"
#include "encodeapi.h"
#include "tencent_video_call.h"

JSScheduler       g_sch_event = NULL;

typedef struct {
    int p2p_running;  // p2p 模块退出后调用 iv_avt_send_finish_stream 会导致段错误
    int p2p_timesync; // p2p 模块是否对时
} sEventRun;

static sEventRun run = {0};
static sEventRun *g_run_evt = &run;

int tencent_p2p_is_running(void)
{
    return g_run_evt->p2p_running;
}

int tencent_p2p_is_timesync(void)
{
    return g_run_evt->p2p_timesync;
}

int tencent_timestamp_reply_event_handler(const char* timestamp_ms)
{
    DBG("Ntp Server timestamp_ms:%s\n", timestamp_ms);
    static int ntp_done = FALSE;
    char cmd_buf[128] = {0};
    char resp_buf[128] = {0};
    char timestamp_s[12] = {0};

    g_run_evt->p2p_timesync = 1;
    memcpy(timestamp_s, timestamp_ms, 10);
    sprintf(cmd_buf, "timecfg -act set -time %10s", timestamp_s);

    start_timesync(atoi(timestamp_s));

    jcpcmd_sendrecv(cmd_buf, resp_buf, sizeof(resp_buf));
    DBG("resp_buf:%s\n", resp_buf);

    if (!ntp_done) {
        if (is_okey("/tmp/mmc.log")) {
            ntp_done = TRUE;
            char date_fmt[24] = { 0 };
            char buf[64] = { 0 };
            get_timestr2(atoi(timestamp_s), date_fmt, sizeof(date_fmt));
            snprintf(buf, sizeof(buf)-1, "date @%s of ntp: %s @up: %lld\n",
                                          timestamp_s, date_fmt, mono_sec());
            DumpFile("/tmp/mmc.log", buf, strlen(buf));
            UtilSystemCmd((char *)"/ipc/bin/toggle mmc log");
        }
    }

    return 0;
}

void cb_tencent_sync_utc_time(void *data)
{
    static int tick = 0;
    static time_t epo = 0;
    char time_str[64] = {0};

    /*if (!is_tencent_on_line()) {
        return;
    }*/
    // get_ntp_epoche() 失败的情况，2 分钟后立即再次同步。
    if (is_inc_mod0(tick, 60) || epo == 0) {
        SYSLOG("tick=%d, going to sync\n", tick);
    } else {
        return;
    }

    SysNtpS ntp = {0};
    conf_get_ntpcfg(&ntp);
    
    epo = get_ntp_epoche(ntp.ntpserver, ntp.ntpport);

    if (epo) {
        snprintf(time_str, sizeof(time_str) - 1, "%lld", epo);
        tencent_timestamp_reply_event_handler((const char *)time_str);
        SYSLOG("pri ntp %lld @uptime %lld\n", epo, mono_sec());
    } else {
        SYSLOG("tx ntp fail, use ali\n");
        epo = get_ntp_epoche((char *)"ntp.aliyun.com", ntp.ntpport);

        if (epo) {
            snprintf(time_str, sizeof(time_str) - 1, "%lld", epo);
            tencent_timestamp_reply_event_handler((const char *)time_str);
            SYSLOG("pri ntp %lld @uptime %lld\n", epo, mono_sec());
        }
    }

    return;
}

static void buf_free_cb(uint8_t *addr, size_t size)
{
    if (addr) {
        free(addr);
    }
}

/*监控过程中，事件通知接口，用户在该回调中可以根据事件类型做相应的操作*/
void tencent_talk_notify_process(iv_avt_event_e event, uint32_t visitor, uint32_t channel,
                            iv_avt_video_res_type_e video_res_type)
{
    if (IV_AVT_EVENT_P2P_PEER_READY == event) {
        //Log_d("avt peername is %s", iv_avt_get_peerinfo());
    }
    DBG("visitor %d channel %d stream %d event id %d\n", visitor, channel, video_res_type, event);

    switch (event) {
        case IV_AVT_EVENT_P2P_PEER_READY:{//P2P初始化完成通知
            g_run_evt->p2p_running = TRUE;
            break;
        }
        case IV_AVT_EVENT_P2P_PEER_CONNECT_FAIL:{//P2P连接stun服务器失败
            g_run_evt->p2p_running = FALSE;
            break;
        }
        case IV_AVT_EVENT_P2P_PEER_ERROR:{//检测网络错误
            g_run_evt->p2p_running = FALSE;
            break;
        }
        case IV_AVT_EVENT_P2P_PEER_ADDR_CHANGED:{//P2P地址方式变化
            break;
        }
        case IV_AVT_EVENT_P2P_WATERMARK_LOW:{//P2P缓存数据低于最低值通知
            if (video_res_type <= IV_AVT_VIDEO_RES_HD) {
                tencent_set_congst_low(visitor, channel);
            }
            break;
        }
        case IV_AVT_EVENT_P2P_WATERMARK_WARN:{//P2P缓存数据超过报警值通知
            if (video_res_type <= IV_AVT_VIDEO_RES_HD) {
                tencent_set_congst_warn(visitor, channel);
            }
            break;
        }
        case IV_AVT_EVENT_P2P_WATERMARK_HIGH:{//P2P缓存数据超过最大值通知
            if (video_res_type <= IV_AVT_VIDEO_RES_HD) {
                tencent_set_congst_high(visitor, channel);
            }
            break;
        }
        case IV_AVT_EVENT_P2P_LOCAL_NET_READY:{//P2P局域网完成
            break;
        }
        case IV_AVT_EVENT_P2P_BUTT:{//无效事件
            break;
        }
        default:
            break;
    }
}

int exec_shell(char *cmd, char *resp, int resp_len)
{
    int nr = -1;
    DBG("shell--------\n");
    if (0 == memcmp(cmd + strlen(cmd) - 1, "&", strlen("&"))) {
        UtilSystemCmd(cmd);
        nr = snprintf(resp, resp_len-1, "[%s] cmd exec ok\n", cmd);
    } else {
        nr = ReadCmdResult(cmd, resp, resp_len-1);
        if (nr == FAILURE) {
            nr = snprintf(resp, resp_len-1, "[%s] vpopen fail\n", cmd);
        }
    }
    DBG("req_cmd:%s\n", cmd);
    resp[nr] = '\0';

    return (nr+1);
}

int user_data_json_parse(char *src, char *resp_buf, int resp_len)
{
    // 解析JSON字符串
    cJSON *root = cJSON_Parse(src);
    if (root == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            ERR("解析错误: %s\n", error_ptr);
        }
        return -1;
    }

    do {
        cJSON *jcp_item = cJSON_GetObjectItemCaseSensitive(root, "jcp");
        if (jcp_item != NULL && cJSON_IsString(jcp_item)) {
            jcpcmd_sendrecv(jcp_item->valuestring, resp_buf, JCP_MAX_LEN+1);
            break;
        }

        cJSON *sh_item = cJSON_GetObjectItemCaseSensitive(root, "shell");
        if (sh_item != NULL && cJSON_IsString(sh_item)) {
            exec_shell(sh_item->valuestring, resp_buf, resp_len);
            break;
        }
    } while (0);

    // 释放内存
    cJSON_Delete(root);
    return 0;

}

int parse_quality_json(char *src, int *dst_quality, int *sender)
{
    cJSON *root = NULL;
    char *str = strstr(src,"{");
    do {
        if(str == NULL) {
            break;
        }

        root = cJSON_Parse(str);
        cJSON *json_quality = cJSON_GetObjectItem(root, "quality");
        if (json_quality == NULL) {
            ERR("quality json is NULL\n");
            break;
        }
        *dst_quality = json_quality->valueint;

        cJSON *json_sender = cJSON_GetObjectItem(root, "sender");
        if (json_sender == NULL) {
            ERR("sender json is NULL\n");
            break;
        }
        *sender = json_sender->valueint;
    } while(0);

    // 释放内存
    cJSON_Delete(root);

    return 0;
}

void av_talk_recv_user_data(uint32_t visitor, uint32_t channel, char *src, uint32_t src_len, iv_cm_memory_s *dst)
{
    DevConfS dev_conf = {0,};
    tx_info_s tx_info = {0,};
    char resp_buf[JCP_MAX_LEN] = {0};

    conf_get_devconf_cfg(&dev_conf);
    dbg_tencent("visitor %d channel %d src = %s src_len = %hu\n", visitor, channel, src, src_len);
    do {
        dst->buf = (uint8_t *)malloc(JCP_MAX_LEN + 1);
        if (strstr(src, "{\"quality\"")) {
            int quality = 0;
            int sender = 0;
            parse_quality_json(src, &quality, &sender);
            tx_info.sender = sender; 
            tx_info.sensor_id = channel;
            tx_info.visitor = visitor;
            tx_info.video_res_type = (iv_avt_video_res_type_e)quality;
            if (sender != 1) {
                dev_conf.definition = quality;
                conf_set_devconf_cfg(dev_conf);
            }
            send_conf_data(JEvent_Quality_Change, &tx_info, sizeof(tx_info));
            strncpy(resp_buf, "success", sizeof(resp_buf));
            DBG("v %d src[%d]: %s sz: %hu quality: %d\n", visitor, channel, src, src_len, quality);
        } else {
            int64_t ms_start = 0;

            if (get_g_run(jcp, RUN_JCP_P2P_TIME)) {
                ms_start = mono_msec();
            }
            user_data_json_parse(src, resp_buf, sizeof(resp_buf));
            if (get_g_run(jcp, RUN_JCP_P2P_TIME)) {
                printf("____________ %s spend %.3fs\n", __func__, (mono_msec()-ms_start)/1000.0);
            }
        }
    } while(0);

    if (dst->buf) {
        memcpy(dst->buf, resp_buf, (strlen(resp_buf) + 1));
        dst->size        = strlen(resp_buf) + 1;
        dst->buf_free_fn = buf_free_cb;
    }

    return;
}

/*接收命令*/
int tencent_talk_command_proc(iv_avt_command_type_e command, uint32_t visitor, uint32_t channel,
                         iv_avt_video_res_type_e video_res_type, void *args)
{
    /* 请注意所有的回调里面都不要有耗时或阻塞的操作，否则会影响主线程性能 */
    int ret = 0;
    DBG("%s command:%d visitor:%d chn:%d Reso:%d\n", 
        __func__, command, visitor, channel, video_res_type);
    switch (command) {
        case IV_AVT_COMMAND_USR_DATA: {
            iv_avt_usr_data_parm_s *usr_data = (iv_avt_usr_data_parm_s *)args;
            av_talk_recv_user_data(visitor, channel, usr_data->src, usr_data->src_len, &usr_data->dst);

            break;
        }
        case IV_AVT_COMMAND_REQ_STREAM: {//请求流信令
            iv_avt_req_stream_param_s *req_param = (iv_avt_req_stream_param_s *)args;
            DBG("req stream param %d", req_param->request_type);
            if (IV_AVT_REQUEST_SEND_STREAM == req_param->request_type) {
                //直播
                req_param->request_result = IV_AVT_DEV_ACCEPT;
            } else {
                //对讲
                req_param->request_result = IV_AVT_DEV_ACCEPT;
            }
            break;
        }
        case IV_AVT_COMMAND_PLAYBACK_SEEK: {//移动播放位置
            iv_cm_pb_seek_s *pb_seek = (iv_cm_pb_seek_s *)args;
            //DBG("playback seek time:%d\n", (int)pb_seek->seek_time_ms);

            record_cmd_param_s param = {0};
            param.common.record_id = channel + visitor * MAX_SENSOR_NUM;
            param.common.cmd_type = STORAGE_RECORD_SEEK;
            param.seek.timestamp_ms = pb_seek->seek_time_ms;
            tencent_record_cmd_handle(param);
            break;
        }
        case IV_AVT_COMMAND_PLAYBACK_PAUSE: {//暂停回放
            DBG("IV_AVT_COMMAND_PLAYBACK_PAUSE\n");
            record_cmd_param_s param = {0};
            param.common.record_id = channel + visitor * MAX_SENSOR_NUM;
            param.common.cmd_type = STORAGE_RECORD_PAUSE;
            tencent_record_cmd_handle(param);
            break;
        }
        case IV_AVT_COMMAND_PLAYBACK_RESUME: {//继续回放
            DBG("IV_AVT_COMMAND_PLAYBACK_UNSUME\n");
            record_cmd_param_s param = {0};
            param.common.record_id = channel + visitor * MAX_SENSOR_NUM;
            param.common.cmd_type = STORAGE_RECORD_UNPAUSE;
            tencent_record_cmd_handle(param);
            break;
        }
        case IV_AVT_COMMAND_PLAYBACK_SPEED: {//设置播放速度
            iv_cm_pb_speed_s *pb_speed = (iv_cm_pb_speed_s *)args;
            DBG("playback speed time_ms:%d\n", pb_speed->time_ms);
            record_cmd_param_s param = {0};
            param.common.record_id = channel + visitor * MAX_SENSOR_NUM;
            param.common.cmd_type = STORAGE_RECORD_SET_PARAM;
            param.set_param.speed = pb_speed->time_ms;
            if (param.set_param.speed == REPLAY_SPEED_4X) {
                param.set_param.key_only = 1;
            } else {
                param.set_param.key_only = 0; 
            }
            tencent_record_cmd_handle(param);
            break;
        }
        case IV_AVT_COMMAND_CALL_ANSWER:
        case IV_AVT_COMMAND_CALL_HANG_UP:
        case IV_AVT_COMMAND_CALL_REJECT:
        case IV_AVT_COMMAND_CALL_CANCEL:
        case IV_AVT_COMMAND_CALL_BUSY:
        case IV_AVT_COMMAND_CALL_TIMEOUT: {
            wechat_call_evt_enqueue(command);
            break;
        }
        default:
            break;
    }
    return ret;
}

static void tencent_record_remove_cb(int id, void *p_src, int size, void *ctx)
{
    if (p_src == NULL) {
        return;
    }

    DBG("update record list\n");
    record_clr_cache_list(*(int *)p_src);
}

static void tencent_definition_change(int id, void *p_src, int size, void *ctx)
{
    DevConfS *dev = (DevConfS *)p_src;
    static int pre_definition = -1;

    if (p_src == NULL) {
        return;
    }

    if (pre_definition == dev->definition) {
        return;
    }
    pre_definition = dev->definition;

    for (int snr = 0; snr < MAX_SENSOR_NUM; snr++) {
        tencent_video_res_type_change(snr, (iv_avt_video_res_type_e)dev->definition);
    }
}

int tencent_register_user_event(void)
{
    DBG("tencent register user event\n");

    attach_event_async(JEvent_AlarmCabDis     , tencent_reconnect_cb    , NULL);
    attach_event_async(JEvent_AlarmCableNormal, tencent_reconnect_cb    , NULL);
    attach_event_async(JEvent_TencentReset    , tencent_reconnect_cb    , NULL);
    attach_event_async(JEvent_Tencent_Offline , tencent_reconnect_cb    , NULL);
    attach_event_async(JEvent_AlarmRMRcord    , tencent_record_remove_cb, NULL);
    attach_config(JEvent_Quality_Change       , tencent_quality_change  , NULL);
    attach_config(JEvent_DevCfg               , tencent_definition_change  , NULL);
    attach_event_async(JEvent_DevVideoCodec   , tencent_live_event_cb, NULL);
    return 0;
}

int tencent_unregister_user_event(void)
{
    DBG("tencent unregister user event\n");

    detach_event(JEvent_Tencent_Offline , tencent_reconnect_cb    , NULL);
    detach_event(JEvent_AlarmCabDis     , tencent_reconnect_cb    , NULL);
    detach_event(JEvent_AlarmCableNormal, tencent_reconnect_cb    , NULL);
    detach_event(JEvent_AlarmRMRcord    , tencent_record_remove_cb, NULL);
    detach_event(JEvent_TencentReset    , tencent_reconnect_cb    , NULL);
    detach_config(JEvent_Quality_Change , tencent_quality_change  , NULL);
    detach_config(JEvent_DevCfg         , tencent_definition_change  , NULL);
    detach_event(JEvent_DevVideoCodec   , tencent_live_event_cb, NULL);
    return 0;
}

#endif
