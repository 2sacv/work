/*****************************************************************************
 * Tencent is pleased to support the open source community by making IoT Video available.
 * Copyright (C) 2020 THL A29 Limited, a Tencent company. All rights reserved.
 * Licensed under the MIT License (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://opensource.org/licenses/MIT
 * Unless required by applicable law or agreed to in writing, software distributed under the License
 *is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 *or implied. See the License for the specific language governing permissions and limitations under
 *the License.
 *
 * @file    iv_cs.h
 * @brief   Description cloud storage function of sdk
 * @version v1.0.0
 *
 *****************************************************************************/

#ifndef __IV_CS_H__
#define __IV_CS_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>

#include "iv_cm.h"

/* Defines the parameter of cloud storage init */
#define MAX_UPLOAD_INFO_NUM 10

typedef enum
{
    CS_SINGLE_CH = 0,
    CS_MULTI_CH1 = 1,
    CS_MULTI_CH2 = 2,
    CS_MULTI_CH3 = 3,
    CS_MULTI_CH4 = 4,
    CS_CH_NUM    = 5
} iv_cs_chn_e;

typedef enum
{
    CS_TYPE_NONE       = 0,
    CS_TYPE_FULL_TIME  = 1,
    CS_TYPE_EVENT      = 2,
    CS_TYPE_FREE_TRIAL = 3,
} iv_cs_type_e;

typedef struct iv_cs_balance_s {
    int cs_switch;
    int cs_days;
    iv_cs_type_e cs_type;
    int free_trial_remaining_sec;
} iv_cs_balance_info_s;

typedef enum
{
    CS_AI_TYPE_CLOSE       = (uint32_t)0,
    CS_AI_TYPE_FALL_DETECT = (uint32_t)(1 << 0),
} iv_cs_ai_type_e;

typedef enum cs_event_upload_options
{
    /* 事件上报时视频数据必选 */
    CS_EVENT_VIDEO_REQ = 0,

    /* 事件上报时图片数据必选 */
    CS_EVENT_PIC_REQ = 1,

    /* 事件上报时视频或者图片数据二选一 */
    CS_EVENT_VIDEO_OR_PIC_REQ = 2,
} iv_cs_event_report_opt_e;

typedef enum
{
    IV_CS_UPLOAD_OK  = 0,
    IV_CS_UPLOAD_ERR = 1,
} iv_cs_upload_state_e;

typedef enum
{
    IV_CS_FORMAT_TS = 0,
    IV_CS_FORMAT_FMP4 = 1,
} iv_cs_format_e;

typedef struct {
    uint64_t utc_sec_a;
    uint64_t utc_sec_b;
    uint64_t pts_ms_a;
    uint64_t pts_ms_b;
    uint32_t frame_seq_a;
    uint32_t frame_seq_b;
    uint32_t upload_size;
    uint32_t total_size;
    iv_cs_upload_state_e state;
} iv_cs_upload_slice_info_s;

typedef struct {
    iv_cs_upload_slice_info_s slice_info[MAX_UPLOAD_INFO_NUM];
    int32_t num;
} iv_cs_upload_info_s;

typedef enum
{
    CS_UP_BK_START   = 0,
    CS_UP_BK_RUNNING = 1,
    CS_UP_BK_END     = 2,
    CS_UP_BK_BUTT    = 3,
} iv_cs_up_bk_state_e;

typedef enum cs_notify_type
{
    /* av upload connect fail, notify_data is NULL */
    IV_CS_AV_UPLOAD_CONNECT_FAIL = 0,
    /* av upload connect recover success, notify_data is NULL */
    IV_CS_AV_UPLOAD_CONNECT_RECOVER = 1,
    /* av upload result msg, notify_data is not NULL */
    IV_CS_AV_UPLOAD_STATE_MSG = 2,
    /* av send buffer watermark warning, notify_data is NULL */
    IV_CS_AV_UPLOAD_WATERMARK_LOW = 3,
    /* av send buffer watermark high, stop push, notify_data is NULL */
    IV_CS_AV_UPLOAD_WATERMARK_WARN = 4,
    /* av send buffer watermark low, resume push, notify_data is NULL */
    IV_CS_AV_UPLOAD_WATERMARK_HIGH = 5,
    /* cs cos service wrong，please check network; notify_data is NULL */
    IV_CS_COS_SERVICE_WRONG = 6,
    /* butt msg */
    IV_CS_MSG_BUTT
} iv_cs_notify_msg_type_e;

typedef union {
    iv_cs_upload_info_s *av_result_info;
} iv_cs_notify_msg_data;

typedef struct {
    int32_t result_code;   /* result 0 is success, other is fail */
    int32_t event_id;      /* event id, 0 ~ 16 */
    uint32_t start_time_s; /* event start time(utc time) */
    uint32_t end_time_s;   /* event end time(utc time) */
} iv_cs_event_result_info;

typedef struct {
    size_t low_mark;
    size_t warn_mark;
    size_t high_mark;
    int8_t enable;
} iv_cs_congestion_ctrl_s;

typedef struct iv_cs_channel_params {
    iv_cs_chn_e channel_id;

    /* the max data size of a gop */
    uint32_t u32MaxGopSize;

    /* the congestion cfg of cloud storage's stream */
    iv_cs_congestion_ctrl_s congestion_cfg;

    /* set input audio/video format */
    iv_cm_av_data_info_s av_fmt;

    /* set cloud storage video mux format*/
    iv_cs_format_e cs_fmt;

    /* the options of report event */
    iv_cs_event_report_opt_e event_report_opt;
} iv_cs_channel_params_s;

typedef struct iv_cs_init_func_cb {
    /* the callback function of start to push cloud storage's stream */
    int (*iv_cs_push_stream_start_cb)(iv_cs_chn_e channel);

    /* the callback function of stop to push cloud storage's stream */
    int (*iv_cs_push_stream_stop_cb)(iv_cs_chn_e channel);

    /* the callback function of ai service notify, not use yet */
    void (*iv_cs_ai_service_notify_cb)(iv_cs_chn_e channel, unsigned int ai_server_type, unsigned long long utc_expire);

    /* 抓图回调，建议非阻塞使用，仅支持jpg格式，返回时提供图片指针和大小，如果不需抓图请填NULL和0 */
    int (*iv_cs_event_capture_picture_cb)(iv_cs_chn_e channel, int32_t event_id, uint8_t **pic, int32_t *size);

    /* 该回调会在 SDK 上传图片结束后回调，可根据该接口进行图片资源的回收处理。
        *pic 为已经上传完成的图片指针，err_code 为上传结果，0成功，其他值失败
        每张抓图都会回调一次，在对应图片回调前不应删除对应的图片资源，以免SDK使用图片资源失败。*/
    int (*iv_cs_event_picture_result_cb)(iv_cs_chn_e channel, uint8_t **pic, int32_t err_code);

    /* the callback function of cloud storage's event report result */
    int (*iv_cs_event_report_result_cb)(iv_cs_chn_e channel, iv_cs_event_result_info *pst_result_info);

    /* the callback function of cloud storage's notify msg */
    int (*iv_cs_notify_cb)(iv_cs_chn_e channel, iv_cs_notify_msg_type_e notify_msg_type,
                           iv_cs_notify_msg_data *pst_notify_data);

    /* the callback function of cloud storage's get balance info from user */
    void (*iv_cs_get_balance_cb)(iv_cs_chn_e channel, int8_t *is_vaild, iv_cs_balance_info_s *pst_balance_info,
                                 int32_t timeout_ms);

    /* the callback function of cloud storage's get stream data */
    int (*iv_cs_dump_file_cb)(iv_cs_chn_e channel, iv_cs_up_bk_state_e state, uint64_t start_ts, uint64_t end_ts,
                              char *file_name, uint8_t *buf, uint32_t len);
} iv_cs_init_func_cb_s;

typedef struct iv_cs_init_parm_s {
    /* the number of channel，max number is CS_CH_NUM*/
    uint32_t channel_num;

    iv_cs_init_func_cb_s func_cb;

    iv_cs_channel_params_s *ch_params;
} iv_cs_init_parm_s;

/* brief: cloud storage module init */
int iv_cs_init(iv_cs_init_parm_s *pstInitParm);

/* brief: cloud storage module exit */
int iv_cs_exit(void);

/* brief: cloud storage resource destroy*/
int iv_cs_resource_destroy(uint32_t timeout_ms); 

/* brief: push cloud storage's stream */
int iv_cs_push_stream(iv_cs_chn_e channel, iv_cm_stream_type_e eCsStreamType, void *pstCsStream);

/* brief: upload backup cloud storage's stream */
int iv_cs_upload_backup_video(iv_cs_chn_e channel, iv_cs_up_bk_state_e state, char *file_name, uint8_t *buf,
                              uint32_t len);

/* brief: event start */
int iv_cs_event_start(iv_cs_chn_e channel, int32_t event_id, const char *info);

/* brief: event stop */
int iv_cs_event_stop(iv_cs_chn_e channel, int32_t event_id, const char *info);

/* brief: event start ext*/
int iv_cs_event_start_ext(iv_cs_chn_e channel, int32_t event_id, const char *info,
                          uint32_t event_start_time_s, uint32_t stream_start_time_s);

/* brief: event stop ext*/
int iv_cs_event_stop_ext(iv_cs_chn_e channel, int32_t event_id, const char *info,
                         uint32_t event_stop_time_s, uint32_t stream_stop_time_s);

/* brief: event directly to report */
int iv_cs_event_directly_report(iv_cs_chn_e channel, int32_t event_id, const char *info,
                                uint32_t event_time_s, const uint8_t *pic_addr, uint32_t pic_len);

/* brief: get cs balance info */
int iv_cs_get_balance_info(iv_cs_chn_e channel, iv_cs_balance_info_s *pstBalanceInfo, uint32_t timeout_s);

/* brief: set cs upload and reply timeout, uint:ms */
int iv_cs_set_trans_time(uint32_t upload_timeout_ms, uint32_t reply_timeout_ms);

/* brief: set AI */
int iv_cs_set_ai_process(iv_cs_chn_e channel, int32_t event_id, iv_cs_ai_type_e ai_type, void *args);

/* brief: get AI */
int iv_cs_get_ai_process(iv_cs_chn_e channel, int32_t event_id, iv_cs_ai_type_e *ai_type, void *args);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __IV_CS_H__ */
