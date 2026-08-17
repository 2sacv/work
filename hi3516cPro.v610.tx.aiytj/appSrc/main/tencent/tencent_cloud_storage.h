/*****************************************************************************
 * Tencent is pleased to support the open source community by making IoT Video available.
 * Copyright (C) 2020 THL A29 Limited, a Tencent company. All rights reserved.
 * Licensed under the MIT License (the "License"); you may not use this file 
 * except in compliance with the License. You may obtain a copy of the License at
 * http://opensource.org/licenses/MIT
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR 
 * CONDITIONS OF ANY KIND, either express or implied. See the License for the 
 * specific language governing permissions and limitations under the License.
 *
 * @file    cloud_storage.h
 * @brief   Description cloud storage function
 * @version v1.0.0
 *
 *****************************************************************************/

/*
基本推流过程(全时云存)
iv_cs_get_balance_info //获取本地套餐信息(非必须)
    |
iv_cs_init
    |
iv_cs_push_stream_start_cb // 收到回调，开始推流
    |
while(1) { iv_cs_push_stream }
    |
iv_cs_push_stream_stop_cb // 收到回调，停止推流
    |
iv_cs_exit
*/

/*
事件推流过程
iv_cs_get_balance_info //获取本地套餐信息(非必须)
    |
iv_cs_init
    |
iv_cs_event_start/iv_cs_event_start_ext // 用户触发事件
    |
iv_cs_event_capture_picture_cb // SDK向用户请求获取事件截图
    |
iv_cs_push_stream_start_cb // 收到回调，开始推流
    |
while(1) { iv_cs_push_stream }
    |
iv_cs_event_stop/iv_cs_event_stop_ext  //用户停止事件
    ｜             
iv_cs_push_stream_stop_cb // 收到回调，停止推流
    |
iv_cs_event_picture_result_cb // 截图使用完毕，用户回收截图资源
    |
iv_cs_exit

*/

#ifdef PLATFORM_TENCENT

#ifndef __TENCENT_CLOUD_STORAGE_H__
#define __TENCENT_CLOUD_STORAGE_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cpluscplus */
#define UPLOAD_TIMEOUT          6000
#define REPLY_TIMEOUT           5000
#define CS_EVENT_START_TIME     12
#define CS_EVENT_ADD_TIME       12
#define INVALID_EVENT           32
#define MS_CS                   100
#define CS_EVENT_MAX_TIME       60*6

int get_cs_chn(int sensor_id);

int check_cs_status(void);

int event_cs_status(int chn);

int is_full_time_cs_enabled(void);

void trigger_cs_event(int alarmType, int sensor_id);

void cs_report_event_with_picture(int alarmType, int sensor_id);

int cs_init(void);

int cs_uninit(void);

#ifdef __cplusplus
}
#endif /* __cpluscplus */

#endif /* __CLOUDSTORAGE_H__ */
#endif //PLATFORM_TENCENT

