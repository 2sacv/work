#ifdef PLATFORM_TENCENT
#include "debug.h"
#include "jconfstruct.h"
#include "confapi.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "g711.h"
#include <errno.h>
#include "sim4g.h"
#include "encodeapi.h"
#include "encode_common.h"
#include "jcpService.h"
#include "shm_buf.h"
#include "conf_list.h"
#include "cJSON.h"

#include "tencent_media_manage.h"
#include "tencent_living_stream.h"
#include "tencent_talk.h"
#include "record_lib.h"
#include "tencent_record_play.h"
#include "tencent_server.h"
#include "tencent_cloud_storage.h"
#include "encode_audio_input.h"
#include "jconfig.h"
extern "C" {
int get_rtsp_connecting_nums(void);
}

tx_talk_visitor_info_s g_talk_visitor_info = {0,};
tx_download_info_s g_file_download[MAX_SENSOR_NUM][MAX_CONNECT_NUM] = {0,};   // 文件下载

static JSScheduler sch_mmng = NULL;
static JSScheduler sch_playback = NULL;

#define FRAG_THRESHOLD_ORDER 7   // 检测 512KB 的块的数量
#define MIN_HIGH_ORDER_BLOCKS 2  // 少于2个认为碎片严重

void tencent_check_drop_cache(void)
{
    int block_count = get_num_of_kbytes(FRAG_THRESHOLD_ORDER);
    if(block_count < MIN_HIGH_ORDER_BLOCKS) {
        DropCache(__func__);
        CompactMemo(__func__);
    }
}

int is_media_useable(void)
{
    if (get_g_sys(factest)) {
        return TRUE;
    }

    priv_ctrl_t pri_info = {0};
    get_config(handlePrivCtrlCfg, pri_info);
    if (pri_info.video == 1) {
        return FALSE;
    }

    if (!get_g_sys(usb_4g)) {
        return TRUE;
    }

    if (is_tencent_eth0_linked()) {
        return TRUE;
    }

    if (sim4g_video_workable()) {
        return TRUE;
    }

    return FALSE;
}

/*
 *接收对讲音频流程
 *1、iv_avt_start_recv_stream_cb //开始接收音频回调
 *2、iv_avt_recv_stream_cb //接收数据流,并进行解码播放
 *3、iv_avt_stop_recv_stream_cb //停止接收
*/
int tencent_talk_illegal_check(uint32_t visitor, uint32_t channel, iv_avt_stream_type_e stream_type, tx_talk_visitor_info_s *p_talk_visitor)
{
    int ret = 0;
    do {
        if (channel < DEV_QIU || channel > DEV_QIANG) {
            ret = -1;
            ERR("invalid channel %d\n", channel);
            break;
        }

        if (!p_talk_visitor->running) {
            ret = -1;
            ERR("talk is running:%d\n", p_talk_visitor->running);
            break;
        }

        if (p_talk_visitor->visitor != visitor) {
            ret = -1;
            ERR("talk is running, but running vistor:%u isn't equal visitor:%u\n", p_talk_visitor->visitor, visitor);
            break;
        }

        if (stream_type < IV_AVT_STREAM_TYPE_AUDIO || stream_type > IV_AVT_STREAM_TYPE_AV) {
            ERR("talk stream_type:%d illegal\n", stream_type);
            ret = -1;
            break;
        }
    } while(0);

    return ret;
}

/*对讲前接收音视频数据格式*/
int tencent_talk_start_recv_stream(uint32_t visitor, uint32_t channel, iv_avt_stream_type_e stream_type, iv_cm_av_data_info_s *p_av_data_info)
{
    DBG("%s visitor:%d chn:%d stream_type:%d\n", __func__, visitor, channel, stream_type);
    talk_set_dec_init_val(0);

    if (stream_type == IV_AVT_STREAM_TYPE_AUDIO) {
        DBG("audio: type(%d), mode(%d), bitwidth(%d), sample rate(%d) sample number(%d)\n",
                p_av_data_info->eAudioType, p_av_data_info->eAudioMode,
                p_av_data_info->eAudioBitWidth, p_av_data_info->eAudioSampleRate,
                p_av_data_info->u32SampleNumPerFrame);
    } else if (stream_type == IV_AVT_STREAM_TYPE_VIDEO) {
        DBG("video: type(%d), width(%d), height(%d), sample rate(%d)\n",
                p_av_data_info->eVideoType, p_av_data_info->u32VideoWidth,
                p_av_data_info->u32VideoHeight, p_av_data_info->u32Framerate);
    }

    int ret = 0;
    tx_talk_visitor_info_s *p_talk_visitor = &g_talk_visitor_info;

    do {
        if (channel < DEV_QIU || channel > DEV_QIANG) {
            ret = -1;
            ERR("invalid channel %d\n", channel);
            break;
        }

        if (p_talk_visitor->running) {
            ret = -1;
            ERR("talk running:%d has already started!\n", p_talk_visitor->running);
            break;
        }

        if (stream_type < IV_AVT_STREAM_TYPE_AUDIO || stream_type > IV_AVT_STREAM_TYPE_AV) {
            ERR("talk stream_type:%d illegal\n", stream_type);
            ret = -1;
            break;
        }
    } while(0);

    if (ret == 0) {
        p_talk_visitor->sensor_id = channel;
        p_talk_visitor->running = 1;
        p_talk_visitor->visitor = visitor;
    }

    return ret;
}

//接收音频数据流,并进行解码播放
int tencent_talk_recv_stream(uint32_t visitor, uint32_t channel, iv_avt_stream_type_e stream_type, void *pStream)
{
    tx_talk_visitor_info_s *p_talk_visitor = &g_talk_visitor_info;

    if (tencent_talk_illegal_check(visitor, channel, stream_type, p_talk_visitor) != 0) {
        return -1;
    }
    if (stream_type != IV_AVT_STREAM_TYPE_AUDIO) {
        return -1;
    }
    dbg_tencent("%s visitor:%d chn:%d stream_type:%d\n", __func__, visitor, channel, stream_type);
    if (talk_create_speaker_socket() != SUCCESS) {
        return -1;
    }
    iv_cm_aenc_stream_s *p_a_stream = (iv_cm_aenc_stream_s *)pStream;
    tencent_push_audio_data((char *)p_a_stream->pstAencPack[0]->pu8Addr, p_a_stream->pstAencPack[0]->u32Len);

    return 0;
}

/*停止接收音频流*/
int tencent_talk_stop_recv_stream(uint32_t visitor, uint32_t channel, iv_avt_stream_type_e stream_type)
{
    tx_talk_visitor_info_s *p_talk_visitor = &g_talk_visitor_info;

    if (tencent_talk_illegal_check(visitor, channel, stream_type, p_talk_visitor) != 0) {
        return -1;
    }

    DBG("%s visitor:%d chn:%d stream_type:%d\n", __func__, visitor, channel, stream_type);
    memset(p_talk_visitor, 0, sizeof(tx_talk_visitor_info_s));
    tencent_talk_stop();

    DBG("tencent_talk_stop_recv_stream success!\n");

    return 0;
}

void tencent_record_cmd_handle(record_cmd_param_s param)
{
    RecordPlayMngS* replay_mng = (RecordPlayMngS*)replay_get_chn(param.common.record_id);
    if (NULL == replay_mng) {
        DBG("[record] tencent_record_cmd_handle not find param\n");
        return;
    }

    RecordplayCmdS* cmd = (RecordplayCmdS*)malloc(sizeof(RecordplayCmdS));
    if (cmd == NULL) {
        ERR("malloc RecordplayCmdS fail\n");
        return;
    }

    cmd->key_only = param.set_param.key_only;
    cmd->play_status = param.common.cmd_type;
    cmd->timestamp_ms = param.seek.timestamp_ms;
    cmd->id = param.common.record_id;
    cmd->speed = param.set_param.speed;

    DBG("replay_push_cmd\n");
    js_run_function(sch_playback, replay_push_cmd, (void*)cmd, 0);

    return;
}

static int start_push_record_stream(tx_info_s *p_tx_info)
{
    // 如果处于SD卡格式化过程中则不执行
    if (get_g_stat(record, SD_REC_FORMAT) || !get_g_stat(record, SD_CD_IN)) {
        DBG("SD formating or not exist!!!\n");
        return -1;
    }
    int record_id = p_tx_info->sensor_id + p_tx_info->visitor * MAX_SENSOR_NUM;

    RecordPlayChnS* replay = (RecordPlayChnS*)replay_get_chn(record_id);
    replay->rec_info.visitor = p_tx_info->visitor;
    replay->rec_info.sensor_id = p_tx_info->sensor_id;
    replay->run.video_timestamp_ms = 0;
    replay->run.audio_timestamp_ms = 0;
    if (p_tx_info != NULL) {
        free(p_tx_info);
    }
    tencent_report_property();
    js_create_timer_r(sch_playback, 50, STREAM_PUSH_TIMER, replay_push_record, (void*)replay, &replay->mng.handle);

    return 0;
}

static void start_push_record_cb(void* data)
{
    tx_info_s *p_tx_info = (tx_info_s *)data;
    start_push_record_stream(p_tx_info);

    return;
}

static int tencent_start_push_record(uint32_t visitor, uint32_t channel, iv_avt_video_res_type_e video_res_type, iv_cm_time_fragment_s *pb_time)
{
    DBG("%s visitor:%d chn:%d Reso:%d\n", __func__, visitor, channel, video_res_type);
    tx_info_s *p_tx_info = (tx_info_s*)malloc(sizeof(tx_info_s));
    if (p_tx_info == NULL) {
        ERR("malloc fail\n");
        return -1;
    }
    p_tx_info->visitor = visitor;
    p_tx_info->sensor_id = channel;
    p_tx_info->video_res_type = video_res_type;
    p_tx_info->pb_time = pb_time;
    js_run_function(sch_mmng, start_push_record_cb, p_tx_info, 1);

    return 0;
}

void tencent_get_media_info(uint32_t sensor_id, iv_avt_video_res_type_e video_res_type, iv_cm_av_data_info_s *pstAvDataInfo, int sender)
{
    int width = 0;
    int height = 0;
    int video_channel = 0;
    VideoEncS videoenc = {0};
    RecordCtrlS record_ctrl = {0};

    conf_get_videocfg(&videoenc);
    conf_get_recordcfg(&record_ctrl);

    if (sender) {
        if (video_res_type == IV_AVT_VIDEO_RES_HD ||
            video_res_type == IV_AVT_VIDEO_RES_SD) { //主码流
            video_channel = VIDEO_CHN_MAIN;
        } else {
            video_channel = VIDEO_CHN_SUB;
        }
    } else {
        video_channel = VIDEO_CHN_MAIN;
    }

    if (MAX_SENSOR_NUM > 1) {
        if (video_channel == VIDEO_CHN_MAIN) { //主码流
            encode_vencsize_to_resolution(videoenc.enc[sensor_id].vencsize, &width, &height);
            pstAvDataInfo->u32Framerate = videoenc.enc[sensor_id].fps;
            pstAvDataInfo->eVideoType = (VENC_FORMAT_H265 == videoenc.enc[sensor_id].codec)?
                IV_CM_VENC_TYPE_H265:IV_CM_VENC_TYPE_H264;
        } else {
            encode_vencsize_to_resolution(videoenc.enc[2].vencsize, &width, &height);
            pstAvDataInfo->u32Framerate = videoenc.enc[2].fps;
            pstAvDataInfo->eVideoType = (VENC_FORMAT_H265 == videoenc.enc[2].codec)?
                IV_CM_VENC_TYPE_H265:IV_CM_VENC_TYPE_H264;
        }
    } else {
        encode_vencsize_to_resolution(videoenc.enc[video_channel].vencsize, &width, &height);
        pstAvDataInfo->u32Framerate = videoenc.enc[video_channel].fps;
        pstAvDataInfo->eVideoType = (VENC_FORMAT_H265 == videoenc.enc[video_channel].codec)?
            IV_CM_VENC_TYPE_H265:IV_CM_VENC_TYPE_H264;
    }

    pstAvDataInfo->u32VideoWidth = width;
    pstAvDataInfo->u32VideoHeight = height;

    // audio
    pstAvDataInfo->eAudioType = IV_CM_AENC_TYPE_AAC;
    pstAvDataInfo->eAudioMode = IV_CM_AENC_MODE_MONO;//单声道
    pstAvDataInfo->eAudioBitWidth = IV_CM_AENC_BIT_WIDTH_16;
    pstAvDataInfo->eAudioSampleRate = IV_CM_AENC_SAMPLE_RATE_16000;
    pstAvDataInfo->u32SampleNumPerFrame = AAC_SMPL_PER_FRM_16K;

    return;
}

int parse_user_args_json(char *src, int *sender)
{
    cJSON *root = NULL;
    char *str = strstr(src,"{");
    do {
        if(str == NULL) {
            break;
        }

        root = cJSON_Parse(str);

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

/*
音视频数据传输流程：
1.iv_avt_get_av_enc_info_cb //获取音视频编码
2.iv_avt_start_real_play_cb //开始播放
3.while(1){iv_avt_send_stream} //推流
4.iv_avt_stop_real_play_cb  //停止推流
5.停止调用iv_avt_send_stream
*/

/*获取音视频编码
 *channel表示摄像头的ID，针对单摄像头设备，该值为0
 *
*/
void tencent_talk_get_enc_info(uint32_t visitor, uint32_t channel, iv_avt_video_res_type_e video_res_type, iv_cm_av_data_info_s *p_av_data_info, void *args)
{
    WAR("%s visitor:%d chn:%d Reso:%d\n", __func__, visitor, channel, video_res_type);
    iv_avt_req_stream_info_s *stream_info = (iv_avt_req_stream_info_s *)args;
    if (video_res_type == IV_AVT_VIDEO_RES_PB) {
        if (stream_info) {
            DBG("start palyback begin_time:%lld end_time: %lld, user_args=%s, type=%u\n",
                stream_info->pb_time->begin_time_s, stream_info->pb_time->end_time_s,
                stream_info->user_args, stream_info->pb_time->type);
        }
        int record_id = channel + visitor * MAX_SENSOR_NUM;
        // 判断当前服务已满
        if (is_replay_chn_full()) {
            DBG("The param is over the the max!!!");
            return;
        }

        // 判断当前服务是否已存在
        if (is_replay_chn_running(record_id)) {
            DBG("The param is exist!!!");
            return;
        }
        replay_init_chn(channel, record_id, stream_info->pb_time, p_av_data_info);
        DBG("eVideoType = %d\n", p_av_data_info->eVideoType);
    } else {
        int sender = 0;
        if (stream_info && stream_info->user_args) {
            parse_user_args_json(stream_info->user_args, &sender);
            DBG("stream user_args: %s\n", stream_info->user_args);
        }
        tencent_get_media_info(channel, video_res_type, p_av_data_info, sender);
    }

    return;
}

/*开始播放*/
void tencent_talk_start_real_play(uint32_t visitor, uint32_t channel, iv_avt_video_res_type_e video_res_type, void *args)
{
    int sender = 0;
    DBG("%s visitor:%d chn:%d Reso:%d\n", __func__, visitor, channel, video_res_type);
    iv_avt_req_stream_info_s *req_args = (iv_avt_req_stream_info_s *)args;
    if (visitor >= MAX_CONNECT_NUM) {   // 判断在运行的vistor个数
        ERR("connect number visitor:%u has already max value!,\n", visitor);
        return;
    }

    // if ((get_rtsp_connecting_nums() > 0 || is_full_time_cs_enabled()) && visitor >= MAX_CONNECT_NUM-1) {
    //     ERR("connect number visitor:%u has already max value!, rtsp_num:%d\n", visitor, get_rtsp_connecting_nums());
    //     return;
    // }

    if (channel < DEV_QIU || channel > DEV_QIANG) {
        ERR("channel:%u is invalid\n", channel);
        return;
    }

    if (video_res_type < IV_AVT_VIDEO_RES_FL || video_res_type > IV_AVT_VIDEO_RES_BUTT) {
        ERR("res_type:%d is invalid\n", video_res_type);
        return;
    }

    if (!args) {
        ERR("recv usr data NULL or visitor invalid\n");
        return;
    } else {
        if (req_args->requester) {
            DBG("stream requester:%s\n", req_args->requester);
        }
        if (req_args->user_args) {
            parse_user_args_json(req_args->user_args, &sender);
            DBG("stream user_args: %s\n", req_args->user_args);
        }
    }

    if (is_media_useable()) {
        tencent_check_drop_cache();
        if (video_res_type == IV_AVT_VIDEO_RES_PB) {
            iv_cm_time_fragment_s *pb_time = (iv_cm_time_fragment_s *)(req_args->pb_time);
            DBG("start palyback begin_time:%lld and end_time:%lld, type=%d\n",
                    pb_time->begin_time_s, pb_time->end_time_s, pb_time->type);
            tencent_start_push_record(visitor, channel, video_res_type, pb_time);
        } else {
            tencent_start_push_stream(visitor, channel, video_res_type, sender);
        }
    } else {
        ERR("tencent_talk_start_real_play, not useable\n");
    }

    return;
}

static void stop_record_stream_cb(void* data)
{
    int *record_id = (int *)data;

    RecordPlayChnS* replay_mng = (RecordPlayChnS*)replay_get_chn(*record_id);
    if(NULL == replay_mng) {
        return;
    }

    js_delete_timer_r(&replay_mng->mng.handle);

    replay_uninit_chn(*record_id);
    tencent_report_property();

    return;
}

void tencent_stop_push_record(uint32_t visitor, uint32_t channel, iv_avt_video_res_type_e video_res_type)
{
    int record_id = channel + visitor * MAX_SENSOR_NUM;
    js_run_function(sch_mmng, stop_record_stream_cb, (void*)&record_id, 1);

    return;
}

/*停止推流*/
void tencent_talk_stop_real_play(uint32_t visitor, uint32_t channel, iv_avt_video_res_type_e video_res_type)
{
    DBG("%s visitor:%d chn:%d Reso:%d\n", __func__, visitor, channel, video_res_type);
    if (visitor >= MAX_CONNECT_NUM) {   // 判断在运行的vistor个数
        ERR("connect number visitor:%u has already max value!,\n", visitor);
        return;
    }

    if (channel < DEV_QIU || channel > DEV_QIANG) {
        ERR("channel:%u is invalid\n", channel);
        return;
    }

    if (video_res_type < IV_AVT_VIDEO_RES_FL || video_res_type > IV_AVT_VIDEO_RES_BUTT) {
        ERR("res_type:%d is invalid\n", video_res_type);
        return;
    }

    if (is_media_useable()) {
        if (video_res_type == IV_AVT_VIDEO_RES_PB) {
            tencent_stop_push_record(visitor, channel, video_res_type);
        } else {
            tencent_stop_push_stream(visitor, channel, video_res_type);
        }
    } else {
        ERR("tencent_talk_stop_real_play, not useable\n");
    }

    return;
}

static int tencent_download_file_stop(uint32_t visitor, uint32_t channel)
{
    int rc = 0;

    if (channel >= MAX_SENSOR_NUM) {
        ERR("invalid channel %d!", channel);
        return -1;
    }

    tx_download_info_s *p_download_handle = g_file_download[channel];

    int i = 0;
    for (i = 0; i < MAX_CONNECT_NUM; i++) {
        if (p_download_handle[i].running && (p_download_handle[i].visitor == visitor)) {
            p_download_handle[i].running = 0;
            p_download_handle[i].visitor = 0;
            break;
        }
    }

    if (i >= MAX_CONNECT_NUM) {
        ERR("visitor %d channel:%u download is invalid!", visitor, channel);
        return -1;
    }

    if (p_download_handle[i].fp) {
        fclose(p_download_handle[i].fp);
        p_download_handle[i].fp = NULL;
    }

    if (p_download_handle[i].blk_buf) {
        free(p_download_handle[i].blk_buf);
        p_download_handle[i].blk_buf = NULL;
    }

    memset(&p_download_handle[i], 0, sizeof(tx_download_info_s));

    return rc;
}

static int tencent_download_file_start(uint32_t visitor, uint32_t channel, void *args)
{
    iv_cm_download_param_s *p_download_file = (iv_cm_download_param_s *)args;
    int rc        = 0;
    if (!args) {
        ERR("input parameter is NULL!");
        return -1;
    }

    if (visitor >= MAX_CONNECT_NUM) {   // 判断在运行的vistor个数
        ERR("connect number visitor:%u has already max value!,\n", visitor);
        return -1;
    }

    if (channel < DEV_QIU || channel > DEV_QIANG) {
        ERR("channel:%u is invalid\n", channel);
        return -1;
    }

    tx_download_info_s *p_download_handle = g_file_download[channel];
    DBG("OTA downlond filename [%s], file_offset [%d]\n",
            p_download_file->file_name, p_download_file->file_offset);

    int i = 0;
    for (i = 0; i < MAX_CONNECT_NUM; i++) {
        if (p_download_handle[i].running && (visitor == p_download_handle[i].visitor)) {
            tencent_download_file_stop(visitor, channel);
        }
    }

    for (i = 0; i < MAX_CONNECT_NUM; i++) {
        if (!p_download_handle[i].running) {
            p_download_handle[i].running = 1;
            p_download_handle[i].visitor = visitor;
            break;
        }
    }

    if (i >= MAX_CONNECT_NUM) {
        ERR("connect number has already max value!\n");
        return -1;
    }

    do {
        char path[128] = {0};
        snprintf(path, sizeof(path), "%s", p_download_file->file_name);
        FILE *fp = fopen(path, "rb");
        if (!fp) {
            ERR("open file %s failed!\n", path);
            rc = -1;  //关闭下载
            break;
        }

        fseek(fp, p_download_file->file_offset, SEEK_SET);
        p_download_handle[i].blk_size = 4 * 1024;//一次下载4k
        char *buf = (char *)malloc(p_download_handle[i].blk_size);
        if (!buf) {
            ERR("malloc buffer %d failed!\n", p_download_handle[i].blk_size);
            fclose(fp);
            fp = NULL;
            rc = -1;  //关闭下载
            break;
        }

        p_download_handle[i].fp      = fp;
        p_download_handle[i].blk_buf = buf;
    } while (0);

    if (rc) {
        tencent_download_file_stop(visitor, channel);
    }

    return rc;
}

static int tencent_download_file_running(uint32_t visitor, uint32_t channel, void *args)
{
    int rc = 0;

    if (channel >= MAX_SENSOR_NUM) {
        ERR("invalid channel %d!", channel);
        return -1;
    }

    iv_cm_memory_s *p_data_buf            = (iv_cm_memory_s *)args;
    tx_download_info_s *p_download_handle = g_file_download[channel];

    int i = 0;
    for (i = 0; i < MAX_CONNECT_NUM; i++) {
        if (p_download_handle[i].running && (p_download_handle[i].visitor == visitor)) {
            break;
        }
    }

    if (i >= MAX_CONNECT_NUM) {
        ERR("visitor %d download is invalid!", visitor);
        return -1;
    }


    if (feof(p_download_handle[i].fp)) {
        p_data_buf->buf         = NULL;
        p_data_buf->size        = -1;
        p_data_buf->buf_free_fn = NULL;
        return -1;
    }

    uint32_t read_len = 0;
    if (p_download_handle[i].fp && p_download_handle[i].blk_buf) {
        read_len = fread(p_download_handle[i].blk_buf, 1, p_download_handle[i].blk_size,
                         p_download_handle[i].fp);
        if (read_len <= 0) {
            ERR("read frame error!\n");
            p_data_buf->buf         = NULL;
            p_data_buf->size        = -1;
            p_data_buf->buf_free_fn = NULL;
            return -1;
        }
    }

    p_data_buf->buf         = (uint8_t *)p_download_handle[i].blk_buf;
    p_data_buf->size        = read_len;
    p_data_buf->buf_free_fn = NULL;

    return rc;
}

/*文件下载回调,在小程序请求下载设备端卡录像时调用*/
int tencent_talk_download_proc(iv_avt_download_status_e status, uint32_t visitor, uint32_t channel, void *args)
{
    int ret = 0;
    dbg_tencent("%s visitor:%d chn:%d status:%d\n", __func__, visitor, channel, status);
    switch (status) {
        case IV_AVT_DOWNLOAD_STATUS_START:
            DBG("download start status %d visitor %d channel %d \n", status, visitor, channel);
            ret = tencent_download_file_start(visitor, channel, args);
            break;
        case IV_AVT_DOWNLOAD_STATUS_RUNNING:
            ret = tencent_download_file_running(visitor, channel, args);
            break;
        case IV_AVT_DOWNLOAD_STATUS_STOP:
            DBG("download stop status %d visitor %d channel %d\n", status, visitor, channel);
            ret = tencent_download_file_stop(visitor, channel);
            break;
        default:
            ret = -1;
            break;
    }
    return ret;
}

int tencent_media_manage_init()
{
    tencent_living_init();
    sch_mmng = js_create_scheduler((char *)"p2p_media_manage");
    if(NULL == sch_mmng){
        ERR("creat tencent server sch fail\n");
        return FAILURE;
    }

    sch_playback = js_create_scheduler((char*)"tencent_playback");
    if (NULL == sch_playback) {
        ERR("create tencent playback scheduler fail\n");
        return FAILURE;
    }

    return SUCCESS;
}

int tencent_media_manage_uninit(void)
{
    tencent_living_uninit();
    destroy_speaker_socket_fd();

    js_delete_scheduler(sch_mmng);
    sch_mmng = NULL;

    js_delete_scheduler(sch_playback);
    sch_playback = NULL;

    return 0;
}
#endif

