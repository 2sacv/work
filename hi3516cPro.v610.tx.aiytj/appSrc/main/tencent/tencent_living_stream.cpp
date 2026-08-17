#ifdef PLATFORM_TENCENT
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "g711.h"
#include "utils.h"
#include "confapi.h"
#include "shm_buf.h"
#include "shm_buf_pool.h"
#include "sim4g.h"
#include "encodeapi.h"
#include "jconfig.h"
#include "conf_list.h"
#include "jevent.h"

#include "net_qrcode.h"
#include "jcpService.h"
#include "system_sch.h"  

#include "cJSON.h"
#include "watch_rstkey.h"
#include "airlink.h"
#include "net_check.h"
#include "ble_services.h"

#include "iv_cm.h"
#include "iv_cs.h"
#include "iv_err.h"
#include "tencent_media_manage.h"
#include "tencent_server.h"
#include "tencent_living_stream.h"
#include "tencent_cloud_storage.h"
#include "tencent_event_handle.h"
#include "tencent_video_call.h"

JSScheduler sch_living = NULL;
static JSTCHandle hdl_report = NULL;  
tx_lv_cs_visitor_info_s g_live_visitor_info[MAX_CONNECT_NUM][MAX_SENSOR_NUM][MAX_VENC_NUM] = {0,};
tx_lv_cs_push_info_s g_push_stream_info[MAX_STREAM_CHANNEL_NUM] = {0, .type = STREAM_LIVING};
extern tx_lv_cs_visitor_info_s g_cs_visitor_info[MAX_SENSOR_NUM];

int tencent_living_running_count(void)
{
    int run_cnt = 0;
    int i = 0, j = 0, k = 0;
    for (i = 0; i < TENCENT_REPLAY_MAX_CHN; i++) {
        if (is_replay_chn_running(i)) {
            run_cnt++;
        }
    }

    run_cnt = run_cnt/MAX_SENSOR_NUM;

    for (i = 0; i < MAX_CONNECT_NUM; i++) {
        for (j = 0; j < MAX_SENSOR_NUM; j++) {
            for (k = 0; k < MAX_VENC_NUM; k++) {
                if (TRUE == g_live_visitor_info[i][j][k].running) {
                    run_cnt++;
                    break;
                }
            }

            if (k < MAX_VENC_NUM) {
                break;
            }
        }
    }

    return run_cnt;
}

/*
 *  net_status字段说明 0:有线，1:Wi-Fi，2:4G
 */
static int network_status()
{
    if (net_link_status("eth0") == TRUE) {
        return 0;
    }

    if (get_g_sys(usb_wifi)) {
        return 1;
    }

    return 2;
}

void tencent_report_property()
{
    static int net_dbm = 0;
    DevConfS dev_conf = {0,};

    conf_get_devconf_cfg(&dev_conf);
    if (get_g_sys(usb_wifi)) {
        if (net_dbm == 0) { 
            net_dbm = get_wifi_quality();
        }

    } else {
        Sim4g sim4g_info = {0,};
        sim4g_get_stat(&sim4g_info);
        net_dbm = sim4g_info.dbm;
    }
    
    tencent_live_report_property("{\"%s\":%d, \"%s\":%d, \"%s\":%d, \"%s\":%d}", 
            LIVE_REPORT_VITCOUNT, tencent_living_running_count(),
            LIVE_REPORT_NET_STATUS, network_status(), LIVE_REPORT_NET_DBM, net_dbm,
            LIVE_REPORT_QUALITY, dev_conf.definition);

    return;
}

void tencent_set_congst_low(int i, int j)
{
    for (int k = 0; k < MAX_VENC_NUM; k++) {
        g_live_visitor_info[i][j][k].congst_low = true;
        g_live_visitor_info[i][j][k].congst_high = false;
        g_live_visitor_info[i][j][k].congst_warn = false;
    }
}

/*
 * 只发 I frame
 **/
void tencent_set_congst_warn(int i, int j)
{
    for (int k = 0; k < MAX_VENC_NUM; k++) {
        g_live_visitor_info[i][j][k].congst_warn = true;
    }
}

/*
 * 停止发送所有
 **/
void tencent_set_congst_high(int i, int j)
{
    for (int k = 0; k < MAX_VENC_NUM; k++) {
        g_live_visitor_info[i][j][k].congst_high = true;
    }
}

int tencent_get_video_channel(int sensor_id, iv_avt_video_res_type_e video_res_type, int sender)
{
    int video_channel = SHM_BUF_MAIN;

    if (sender) {
        if (IV_AVT_VIDEO_RES_FL == video_res_type) {
            video_channel = SHM_BUF_SUB;
        } else {
            video_channel = SHM_BUF_MAIN;
        }
    }
    return video_channel;
}

int tencent_get_video_info(int idx, VideoInfoS *info)
{
    int times = 0;
    eShmMediaType mediatype = SHM_MEDIA_UNKOWN;

    do {
        times++;
        if (shm_buf_get_media_info(get_shm_buf_pool(idx), &mediatype, &info->bps, &info->fps, &info->width, &info->height)) {
            ERR("%s get media info error, shm_buf_get_media_info%s\n", "\033[1;31m", "\033[0m");
            usleep(20*1000);
            continue;
        }

        if (0 == info->fps) {
            ERR("info->fps error, shm_buf_get_media_info\n");
            usleep(20*1000);
            continue;
        }

        if (SHM_MEDIA_VIDEO_MJPEG == mediatype) {
            info->mediatype = MJPEG;
            DBG("info->mediatype: MJPEG\n");
        } else if (SHM_MEDIA_VIDEO_H265 == mediatype) {
            info->mediatype = H265;
            DBG("info->mediatype: H265\n");
        } else {
            info->mediatype = H264;
            DBG("info->mediatype: H264\n");
        }

        break;
    }while(times < 10);

    times = 0;
    info->idx = idx;

    if (SHM_MEDIA_VIDEO_H265 == mediatype) {
        do {
            times++;
            shm_buf_get_vps_sps_pps(get_shm_buf_pool(idx), info->vpsdata, &info->vpssize, info->spsdata, 
            &info->spssize, info->ppsdata, &info->ppssize);

            if (info->vpssize <= 0 || info->spssize <= 0 || info->ppssize <= 0) {
                ERR("H265 info->fps error, shm_buf_get_media_info\n");
                usleep(20*1000);
                continue;
            }
            break;
        } while (times < 10);
    } else {
        do {
            times++;
            shm_buf_get_sps_pps(get_shm_buf_pool(idx), info->spsdata, &info->spssize, 
                    info->ppsdata, &info->ppssize);
            if (info->spssize <= 0 || info->ppssize <= 0) {
                ERR("info->fps error, shm_buf_get_media_info\n");
                usleep(20*1000);
                continue;
            }
            break;
        } while (times < 10);
    }

    return 0;
}

void tencent_live_stream_init(tx_lv_cs_push_info_s *lv_cs_push_info)
{
    AudioCfgS audiocfg = {0};
    VideoInfoS video_info = {0};

    conf_get_audiocfg(&audiocfg);

    lv_cs_push_info->video.timestamp_ms = 0;
    lv_cs_push_info->video.frame_serial = -1;
    tencent_get_video_info(lv_cs_push_info->video.video_channel, &video_info);
    lv_cs_push_info->video.info = video_info;

    lv_cs_push_info->audio.timestamp_ms = 0;
    lv_cs_push_info->audio.frame_serial = 0;
    lv_cs_push_info->audio.audio_frame_serial = 0;
    lv_cs_push_info->audio.enable = (AudioEnableE)audiocfg.inenable;

    return;
}

static void _read_video_noblock(void *userdata, tSBFrame* tFrame)
{
    int err_code = 0;

    iv_cm_venc_stream_s v_stream   = {0};
    iv_cm_venc_pack_s venc_packet = {0};
    tx_lv_cs_push_info_s *live = (tx_lv_cs_push_info_s *)userdata;
    VideoFrameInfoS* video = &live->video;
    if (SHM_ERR_NOT_READY == tFrame->error) {
        //ERR("SHM_ERR_NOT_READY \n");
    } else if (SHM_ERR_OVER_WRITE == tFrame->error) {
        video->frame_serial = -1;
        ERR("SHM_ERR_OVER_WRITE \n");
    } else if (SHM_ERR_BUF_TOO_SMALL == tFrame->error) {
        ERR("SHM_ERR_BUF_TOO_SMALL \n");
    } else if (SHM_ERR_SUCCESS != tFrame->error) {
        video->frame_serial = -1;
        DBG("Realtime: VIDEO CACHE_ERR RET:%d\n", tFrame->error);
    } 

    if (SHM_ERR_SUCCESS != tFrame->error) {
        return;
    }

    dbg_tencent("VIDEO: frame_serial=%d, frame_timestamp=%lf\n", tFrame->frame_serial, tFrame->frame_timestamp);
    video->frame_serial = tFrame->frame_serial;
    video->timestamp_ms = ((uint64_t)(tFrame->frame_timestamp*1000));
    venc_packet.pu8Addr = (uint8_t *)tFrame->framedata;
    venc_packet.u32Len  = tFrame->frame_size;
    venc_packet.u64PTS  = video->timestamp_ms;
    venc_packet.u32Seq  = video->frame_serial;
    venc_packet.eFrameType = (SHM_FRAME_VIDEO_I == tFrame->frame_type) ?
        IV_CM_FRAME_TYPE_I : IV_CM_FRAME_TYPE_P;

    v_stream.u32PackCount = 1;
    v_stream.pstVencPack[0] = &venc_packet;
    video->frame_serial++;
    static time_t congst_start_sec = 0;
    switch(live->type) {
    case STREAM_LIVING:
    for (int i = 0; i < MAX_CONNECT_NUM; i++) {
        for (int j = 0; j < MAX_SENSOR_NUM; j++) {
            for (int k = 0; k < MAX_VENC_NUM; k++) {
                tx_lv_cs_visitor_info_s *visitor = &g_live_visitor_info[i][j][k];
                if (!(video->video_channel == visitor->video_channel && visitor->running)) {
                    continue;
                }

                if (visitor->congst_high) {
                    time_t now = mono_sec();
                    if (congst_start_sec == 0) {
                        congst_start_sec = now;
                    }
                    if ((now - congst_start_sec) >= 10) {
                        ERR("high skip SVR[%d-%d-%d] persist 10s, reset tencent\n", j, i, k);
                        send_event(JEvent_TencentReset);
                        congst_start_sec = 0;
                        visitor->iframe_requested = 0;
                    } else {
                        printf("high skip SVR[%d-%d-%d], waiting %lds\n",
                               j, i, k, (long)(now - congst_start_sec));
                        /* 标记已进入HIGH，恢复时用于检测状态跳变 */
                        visitor->iframe_requested = 1;
                    }
                    continue;
                } else {
                    if (congst_start_sec != 0 || visitor->iframe_requested) {
                        /* congst_high恢复，强制请求I帧（仅在恢复时请求，避免重复产出） */
                        visitor->waiting_key_frame = 1;
                        visitor->request_key_frame = 1;
                        visitor->iframe_requested = 0;
                        printf("high recover SVR[%d-%d-%d], force request I-frame\n", j, i, k);
                    }
                    congst_start_sec = 0;
                }

                if (visitor->waiting_key_frame) {
                    if (SHM_FRAME_VIDEO_I == tFrame->frame_type) {
                        visitor->waiting_key_frame = 0;
                        visitor->request_key_frame = 0;
                    } else {
                        if (visitor->request_key_frame != 0) {
                            encode_immediate_iframe((CH_FS_E)live->video.video_channel);
                            visitor->request_key_frame = 0;
                        }
                        continue;
                    }
                }
                err_code = iv_avt_send_stream(visitor->visitor, visitor->sensor_id,
                        visitor->quality, IV_AVT_STREAM_TYPE_VIDEO, &v_stream);
                if (err_code != 0) {
                    switch (err_code) {
                    case IV_ERR_AVT_NEED_IDR_FRAME:
                        // 立即请求I帧
                        visitor->waiting_key_frame = 1;
                        visitor->request_key_frame = 1;
                        break;
                    case IV_ERR_AVT_REQ_CHN_BUSY:
                    case IV_ERR_AVT_SEND_DATA_TIMEOUT:
                        // 等待I帧再发送
                        visitor->waiting_key_frame = 1;
                        break;
                    default:
                        visitor->waiting_key_frame = 1;
                        break;
                    }
                    ERR("SVR[%d-%d-%d] send video stream fail:%d\n", j, i, k, err_code);
                }

                if (get_g_run(tencent, RUN_P2P_STATUS)) {
                    iv_p2p_send_stats_s stream_send_status  = {0};
                    if (0 == iv_avt_get_send_stream_status(
                            visitor->visitor,
                            visitor->sensor_id,
                            visitor->quality,
                            &stream_send_status) && (SHM_FRAME_VIDEO_I == tFrame->frame_type)) {
                        printf("|%s:%d| visitor %d chn %d Qual %d "
                            "ibps: %8.2fK, Bps: %6.2fK, "
                            "sum: %6.2fMB link_mode.1p2p %u\n",
                            __func__, __LINE__,
                            visitor->visitor,
                            visitor->sensor_id,
                            visitor->quality,
                            stream_send_status.inst_net_rate/1024.0,
                            stream_send_status.ave_sent_rate/1024.0,
                            stream_send_status.sum_sent_acked/1024/1024.0,
                            stream_send_status.link_mode);
                    }
                }
            }
        }
    }
    break;
    case STREAM_CLOUDSTORAGE:
    for (int j = 0; j < MAX_SENSOR_NUM; j++) {
        tx_lv_cs_visitor_info_s *cs_viewer = &g_cs_visitor_info[j];
        if (video->video_channel == cs_viewer->video_channel && cs_viewer->running) {
           if (cs_viewer->waiting_key_frame) {
                if (SHM_FRAME_VIDEO_I == tFrame->frame_type) {
                    cs_viewer->waiting_key_frame = 0;
                    cs_viewer->request_key_frame = 0;
                } else {
                    if (cs_viewer->request_key_frame != 0) {
                        encode_immediate_iframe((CH_FS_E)live->video.video_channel);
                        cs_viewer->request_key_frame = 0;
                    }
                    continue;
                }
            }
            err_code = iv_cs_push_stream((iv_cs_chn_e)(get_cs_chn(cs_viewer->sensor_id)),
                    IV_CM_STREAM_TYPE_VIDEO, &venc_packet);
            if (err_code != 0) {
                switch (err_code) {
                case IV_ERR_CS_NEED_IDR_FRAME:
                    // 立即请求I帧
                    cs_viewer->waiting_key_frame = 1;
                    cs_viewer->request_key_frame = 1;
                    break;
                case IV_ERR_CS_HTTP_DATA_FAIL:
                case IV_ERR_CS_PRM_MALLOC_FAIL:
                    // 等待I帧再发送
                    cs_viewer->waiting_key_frame = 1;
                    break;
                default:
                    cs_viewer->waiting_key_frame = 1;
                    break;
                }
                ERR("s[%d] send cs video stream fail:%d\n", j, err_code);
            }
        }
    }
    break;
    default:
        ERR("push video stream type is unkownd!\n");
    }

    return;
}

void tencent_push_live_video(void *userdata)
{
    tx_lv_cs_push_info_s *p_push_info = (tx_lv_cs_push_info_s *)userdata;
    shm_buf_read_frame_add_vspps(get_shm_buf_pool(p_push_info->video.video_channel), p_push_info->video.frame_serial, _read_video_noblock, p_push_info);

    return;
}

static void _read_audio_noblock(void *userdata, tSBFrame* tFrame)
{
    int err_code = 0;
    tx_lv_cs_push_info_s* live = (tx_lv_cs_push_info_s*)userdata;
    iv_cm_aenc_stream_s a_stream   = {0};
    iv_cm_aenc_pack_s aenc_packet  = {0};
    AudioFrameInfoS* audio = &live->audio;
    VideoFrameInfoS* video = &live->video;

    if (SHM_ERR_SUCCESS == tFrame->error) {
        dbg_tencent("AUDIO: frame_serial=%d, audio_timestamp=%lf\n",tFrame->frame_serial, tFrame->frame_timestamp);
        audio->timestamp_ms = (uint64_t)(tFrame->frame_timestamp*1000);
        int time_inter = audio->timestamp_ms - video->timestamp_ms;
        if (abs(time_inter) > 500 && tFrame->frame_serial != 0 && live->type == STREAM_LIVING) {
            // 同步音视频时间戳 差帧=时间间隔/帧间隔(这里以 16 采样率计算)
            audio->frame_serial = tFrame->frame_serial - time_inter / AAC_FRM_DURATION_16K;

            dbg_tencent("inter = %d change audio frame serial to %d\n", time_inter, audio->frame_serial);
            return;
        }
        audio->frame_serial = tFrame->frame_serial;

        //SDK载入音频数据
        aenc_packet.pu8Addr = (uint8_t *)tFrame->framedata;
        aenc_packet.u32Len  = (uint32_t)tFrame->frame_size;
        aenc_packet.u64PTS  = audio->timestamp_ms;
        aenc_packet.u32Seq  = audio->frame_serial;
        a_stream.u32PackCount   = 1;
        a_stream.pstAencPack[0] = &aenc_packet;

        audio->frame_serial++;
        switch(live->type){
        case STREAM_LIVING:
        for (int i = 0; i < MAX_CONNECT_NUM; i++) {
            for (int j = 0; j < MAX_SENSOR_NUM; j++) {
                for (int k = 0; k < MAX_VENC_NUM; k++) {
                    tx_lv_cs_visitor_info_s *visitor = &g_live_visitor_info[i][j][k];

                    if (visitor->congst_high) {
                        continue;
                    }

                    if (video->video_channel == visitor->video_channel && visitor->running) {
                        err_code = iv_avt_send_stream(visitor->visitor, visitor->sensor_id,
                                visitor->quality, IV_AVT_STREAM_TYPE_AUDIO, &a_stream);
                        if (err_code < 0) {
                            ERR("SVR[%d-%d-%d] send audio stream fail:%d\n", j, i, k, err_code);
                            audio->frame_serial = 0;
                        }
                    }
                }
            }
        }
        break;
        case STREAM_CLOUDSTORAGE:
        for (int j = 0; j < MAX_SENSOR_NUM; j++) {
            tx_lv_cs_visitor_info_s *cs_viewer = &g_cs_visitor_info[j];
            if (video->video_channel == cs_viewer->video_channel && cs_viewer->running) {
                err_code =iv_cs_push_stream((iv_cs_chn_e)get_cs_chn(cs_viewer->sensor_id),
                        IV_CM_STREAM_TYPE_AUDIO, &aenc_packet);
                if (err_code != 0) {
                    ERR("s[%d] send cs audio stream fail:%d\n", j, err_code);
                }
            }
        }
        break;
        default:
                ERR("push audio stream type is unkownd!\n");
        }
    } else if (SHM_ERR_NOT_READY == tFrame->error) {
        //DBG("SHM_ERR_NOT_READY\n");
    } else if (SHM_ERR_OVER_WRITE == tFrame->error) {
        DBG("SHM_ERR_OVER_WRITE\n");
        audio->frame_serial = 0;
    } else {
        audio->frame_serial = 0;
        DBG("Realtime: AUDIO CACHE_ERR RET:%d\n", tFrame->error);
    }

    return;
}

void tencent_push_live_audio(void *userdata)
{

    tx_lv_cs_push_info_s *lv_push_info = (tx_lv_cs_push_info_s *)userdata;
    shm_buf_read_frame_ex(get_shm_buf_pool(SHM_BUF_AUDIO_AAC), 
                             lv_push_info->audio.frame_serial, _read_audio_noblock, lv_push_info);
    return;
}

static int tencent_check_silent_mode()
{
    /* first start push stream, set silent mode */
    static int is_silent_mode = 0;
    if (!is_silent_mode) {
        gpio_t gpio = {0};
        DevConfS devconf = {0};
        conf_get_gpiocfg(&gpio);
        conf_get_devconf_cfg(&devconf);
        if (gpio.ao_prompt) {
            is_silent_mode = 1;
            gpio.ao_prompt = 0;
            devconf.devicebind = 1;
            conf_set_devconf_cfg(devconf);
            stop_qrcode_server();
            ble_services_stop();
            conf_set_gpiocfg(gpio);
            SYSLOG("anti-disturb ___ done ___\n");
        }
    }

    return 0;
}

int tencent_live_report_property(const char *const json_format, ...)
{
    int ret = FAILURE;
    size_t recv_len = 0;
    va_list arg_list;
    char msg_buf[512] = {0};
    unsigned char recv_buf[128] = {0};
    int i = 0, j = 0, k = 0;

    va_start(arg_list, json_format);
    ret = vsnprintf(msg_buf, sizeof(msg_buf)-1, json_format, arg_list);
    va_end(arg_list);
    if (ret < 0) {
        ERR("tencent live report property arg_list failed, ret = %d", ret);
        return ret;
    }

    DBG("msg_buf = %s\n", msg_buf);

    for (i = 0; i < MAX_CONNECT_NUM; i++) {
        for (j = 0; j < MAX_SENSOR_NUM; j++) {
            for (k = 0; k < MAX_VENC_NUM; k++) {
                tx_lv_cs_visitor_info_s *visitor = &g_live_visitor_info[i][j][k];
                if (1 == visitor->running) {
                    memset(recv_buf, 0, sizeof(recv_buf));

                    recv_len = sizeof(recv_buf) - 1;
                    ret = iv_avt_send_command(visitor->visitor, msg_buf, 
                                        strlen(msg_buf), recv_buf, &recv_len, 100);
                    if (ret) {
                        ERR("tencent iv_avt_send_command error, visitor: %d, ret: %d\n",
                                visitor->visitor, ret);
                    }
                }
            }

            if (k < MAX_VENC_NUM) {
                break;  // visitor 相同, 只需要传一遍
            }
        }
    }

    return ret;
}

static void start_push_stream (tx_info_s *p_tx_info, int is_start)
{
    int video_channel = 0;
    tx_lv_cs_visitor_info_s *p_visitor_info = &g_live_visitor_info[p_tx_info->visitor][p_tx_info->sensor_id][p_tx_info->video_res_type];

    if (is_start == FALSE) {
        p_visitor_info = &g_live_visitor_info[p_tx_info->visitor][p_tx_info->sensor_id][p_tx_info->quality];
    }
    do {
        if (p_visitor_info->running && (p_visitor_info->visitor == p_tx_info->visitor)) {
            ERR("visitor %d sensor_id:%u,has already started!\n", p_tx_info->visitor, p_tx_info->sensor_id);
            break;
        }

        video_channel = tencent_get_video_channel(p_tx_info->sensor_id, p_tx_info->video_res_type, p_tx_info->sender);
        tx_lv_cs_push_info_s *p_push_stream_info = &g_push_stream_info[video_channel];
        p_push_stream_info->video.video_channel = p_visitor_info->video_channel = video_channel;
        if (!p_visitor_info->running) {
            p_visitor_info->running           = 1;
            p_visitor_info->visitor           = p_tx_info->visitor;
            p_visitor_info->waiting_key_frame = 1;
            p_visitor_info->request_key_frame = 1;
            p_visitor_info->sensor_id         = p_tx_info->sensor_id;
            p_visitor_info->video_res_type    = p_tx_info->video_res_type;
            p_visitor_info->quality           = (TRUE == is_start) ? p_tx_info->video_res_type : p_tx_info->quality;
            p_push_stream_info->cnt++;
        }
        // if (is_start == TRUE) {
            js_create_once(hdl_report, sch_living, 2000, (JSTCFunc)tencent_report_property, NULL); 
        // }

        if (p_push_stream_info->running) {
            DBG("cnt:%d, sensor_id:%u,video_channel:%d, has already started!\n", 
                p_push_stream_info->cnt, p_visitor_info->sensor_id, video_channel);
            break;
        }
        tencent_live_stream_init(p_push_stream_info);
        encode_immediate_iframe((CH_FS_E)p_visitor_info->video_channel);
        p_push_stream_info->running = 1;
        DBG("start_push_stream_cb, video_channel: %d cnt: %d\n",
            p_visitor_info->video_channel, p_push_stream_info->cnt);
        // loop 第一次延时时间需要大一点, 要保证 IDR 帧生成后再起线程
        js_create_timer_r(sch_living, 150, 40, tencent_push_live_video, (void *)p_push_stream_info, &p_push_stream_info->video.v_handle);
        js_create_timer_r(sch_living, 150, 40, tencent_push_live_audio, (void *)p_push_stream_info, &p_push_stream_info->audio.a_handle);
    } while(0);
}

static void start_push_stream_cb(void* data)
{
    tx_info_s *p_tx_info = (tx_info_s *)data;

    tencent_check_silent_mode();
    start_push_stream(p_tx_info, TRUE);

    return;
}

int tencent_start_push_stream(uint32_t visitor, uint32_t channel, iv_avt_video_res_type_e video_res_type, int sender)
{
    DBG("start push stream visitor:%d channel:%d type:%d\n", visitor, channel, video_res_type);

    tx_info_s info = {0,};
    tx_info_s *p_tx_info =  &info;
    p_tx_info->sender = sender;
    p_tx_info->visitor = visitor;
    p_tx_info->sensor_id = channel;
    p_tx_info->video_res_type = video_res_type;

    js_run_function(sch_living, start_push_stream_cb, p_tx_info, 1);

    return 0;
}

static void stop_push_stream(tx_info_s *p_tx_info, int is_stop)
{
    tx_lv_cs_visitor_info_s *p_visitor_info = &g_live_visitor_info[p_tx_info->visitor][p_tx_info->sensor_id][p_tx_info->video_res_type];
    tx_lv_cs_push_info_s *p_lv_push_info = &g_push_stream_info[p_visitor_info->video_channel];
    int running = p_visitor_info->running;

    if (is_stop == FALSE) {
        for (int k = 0; k < MAX_VENC_NUM; k++) {
            if (g_live_visitor_info[p_tx_info->visitor][p_tx_info->sensor_id][k].running == 1) {
                p_visitor_info = &g_live_visitor_info[p_tx_info->visitor][p_tx_info->sensor_id][k];
                p_lv_push_info = &g_push_stream_info[p_visitor_info->video_channel];
                break;
            }
        }
        p_tx_info->quality = p_visitor_info->quality;
        running = p_visitor_info->running;
        memset(p_visitor_info, 0, sizeof(tx_lv_cs_visitor_info_s));
    } else {
        memset(p_visitor_info, 0, sizeof(tx_lv_cs_visitor_info_s));

        int visitor = p_tx_info->visitor;
        int sensor_id = p_tx_info->sensor_id;
        for (int k = 0; k < MAX_VENC_NUM; k++) {
            g_live_visitor_info[visitor][sensor_id][k].congst_high = false;
            g_live_visitor_info[visitor][sensor_id][k].congst_warn = false;
            g_live_visitor_info[visitor][sensor_id][k].congst_low = false;
        }
        tencent_report_property();
    }

    if (p_lv_push_info->cnt > 0 && running == 1) {
        p_lv_push_info->cnt--;
    }

    DBG("stop_push_stream, video_channel: %d cnt: %d\n",
        p_visitor_info->video_channel, p_lv_push_info->cnt);
    if ((0 == p_lv_push_info->cnt) && (p_lv_push_info->video.v_handle)) {
        p_lv_push_info->running = 0;
        js_delete_timer_r(&p_lv_push_info->video.v_handle);
        js_delete_timer_r(&p_lv_push_info->audio.a_handle);
        DBG("p_lv_push_info running is 0, delete handle\n");
    }
}

static void stop_push_stream_cb(void* data)
{
    tx_info_s *p_tx_info = (tx_info_s *)data;

    stop_push_stream(p_tx_info, TRUE);

    return;
}

int tencent_stop_push_stream(uint32_t visitor, uint32_t channel, iv_avt_video_res_type_e video_res_type)
{
    DBG("stop push visitor:%d channel:%d type:%d\n", visitor, channel, video_res_type);

    tx_info_s info = {0,};
    tx_info_s *p_tx_info =  &info;

    p_tx_info->visitor = visitor;
    p_tx_info->sensor_id = channel;
    p_tx_info->video_res_type = video_res_type;
    //一键呼叫功能在通话时，APP进程被干掉需要做反应
    call_evt_enqueue(E_EVT_CALL_DISCON); 
    js_run_function(sch_living, stop_push_stream_cb, p_tx_info, 1);

    return 0;
}

void tencent_video_res_type_change(uint32_t sensor_id, iv_avt_video_res_type_e video_res_type)
{
    Appvecfg appdev = {0};
    char jcpcmd[JCP_MAX_LEN] = {0};
    char resp_buf[JCP_MAX_LEN] = {0};

    conf_get_appve_cfg(&appdev);
    // 不再区分 eth wireless，一律使用 头 3 个 --> wireless
    snprintf(jcpcmd, sizeof(jcpcmd), appdev.appve[video_res_type+sensor_id*2].appves);
    jcpcmd_sendrecv(jcpcmd, resp_buf, sizeof(resp_buf));

    return;
}

static void restart_push_stream_cb(void* data)
{
    tx_info_s *p_tx_info = (tx_info_s *)data;

    stop_push_stream(p_tx_info, FALSE);
    if (!p_tx_info->sender) {   // AMS只切换通道,不切换参数
        // 高清和超清在同一通道，只修改 fps、bps，故此处根据 video_res_type 切换
        tencent_video_res_type_change(p_tx_info->sensor_id, p_tx_info->video_res_type);
    }
    start_push_stream(p_tx_info, FALSE);

    return;
}

static void stop_live_stream(void* data)
{
    uint32_t *video_channel = (uint32_t *)data;

    tx_lv_cs_push_info_s *p_lv_push_info = &g_push_stream_info[*video_channel];
    for (int i = 0; i < MAX_CONNECT_NUM; i++) {
        for (int j = 0; j < MAX_SENSOR_NUM; j++) {
            for (int k = 0; k < MAX_VENC_NUM; k++) {
                tx_lv_cs_visitor_info_s *visitor = &g_live_visitor_info[i][j][k];
                if (visitor->video_channel == *video_channel) {
                    if (tencent_p2p_is_running() && p_lv_push_info->cnt != 0) {
                        DBG("p_lv_push_info->cnt: %d\n", p_lv_push_info->cnt);
                        iv_avt_send_finish_stream(visitor->visitor, visitor->sensor_id,
                                                  visitor->quality);
                    }
                    memset(visitor, 0, sizeof(tx_lv_cs_visitor_info_s));
                }
            }
        }
    }
    p_lv_push_info->cnt = 0;
    p_lv_push_info->running = 0;
    js_delete_timer_r(&p_lv_push_info->video.v_handle);
    js_delete_timer_r(&p_lv_push_info->audio.a_handle);

    return;
}

void tencent_live_event_cb(int id, void *p_src, int size, void *ctx)
{
    int *video_channel = (int *)p_src;
    js_run_function(sch_living, stop_live_stream, (void*)video_channel, 1);
}

void tencent_quality_change(int id, void *p_src, int size, void *ctx)
{
    tx_info_s *p_tx_info = (tx_info_s *)p_src;
    DBG("tencent_quality_change: org %d, new %d, sender %d\n", p_tx_info->quality, p_tx_info->video_res_type, p_tx_info->sender);
    js_run_function(sch_living, restart_push_stream_cb, p_tx_info, 1);
}

int tencent_living_init()
{
    sch_living = js_create_scheduler((char*)"tencent_living");
    if (NULL == sch_living) {
        ERR("create tencent living scheduler fail\n");
        return FAILURE;
    }

    return SUCCESS;
}

int tencent_living_uninit()
{
    for (int i = 0; i < MAX_CONNECT_NUM; i++) {
        for (int j = 0; j < MAX_SENSOR_NUM; j++) {
            for (int k = 0; k < MAX_VENC_NUM; k++) {
                // 清理所有visitor数据
                memset(&g_live_visitor_info[i][j][k], 0, sizeof(tx_lv_cs_visitor_info_s));
            }
        }
    }

    for (int i = 0; i < MAX_STREAM_CHANNEL_NUM; i++) {
        // 清理所有流数据
        js_delete_timer_r(&g_push_stream_info[i].video.v_handle);
        js_delete_timer_r(&g_push_stream_info[i].audio.a_handle);
        memset(&g_push_stream_info[i], 0, sizeof(tx_lv_cs_push_info_s));
    }

    js_delete_timer_r(&hdl_report);  
    js_delete_scheduler(sch_living);
    sch_living = NULL;

    return 0;
}
#endif
