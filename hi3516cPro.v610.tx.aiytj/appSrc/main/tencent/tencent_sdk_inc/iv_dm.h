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
 *
 * @file    iv_dm.h
 * @brief   Description data model module function of sdk
 * @version v1.0.0
 *
 *****************************************************************************/

#ifndef __IV_DM_H__
#define __IV_DM_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>

/********************************Macro Definition****************************/

typedef enum
{
    REPORT_TIMEOUT  = -2,  // property report timeout
    REPORT_REJECTED = -1,  // property report rejected
    REPORT_ACCEPTED = 0    // property report accepted
} iv_dm_report_result_e;

/*Defines the parameter of data model init */
typedef struct iv_dm_init_parm_s {
    /*brief: the callback function of device online */
    int (*iv_dm_env_init_cb)(void);
} iv_dm_init_parm_s;

/***************************** Macro Definition end **************************/

/******************************* API declaration *****************************/
/*brief: data model module init*/
int iv_dm_init(const iv_dm_init_parm_s *pstInitParm);

/*brief: data model module exit*/
int iv_dm_exit(void);

/*
 * @brief 事件上报
 * @param event_name:   事件的名称 ,
 *
 * @retval  IV_ERR_NONE : success
 * @retval  error code  : failure
 */
int iv_dm_event_report(const char *event_name);

/**
 * @brief 上报用户自定义事件消息，可以触发腾讯连连小程序强提醒
 * 后台需添加 _sys_camera_event 系统物模型，此接口暂无客户使用，后续开发或使用需重新验证
 * 
 * @param evt_msg 消息字符串
 * @return int 
 */
int iv_dm_camera_event_report(const char *evt_msg);

typedef int (*ivm_property_report_cb)(void *param, iv_dm_report_result_e result_code);

/*brief: 属性上报*/
int iv_dm_property_report(const char *key, ivm_property_report_cb cb, void *param);

/*brief: 属性同步，该接口只能在单一线程中调用，返回值为0表示同步成功 */
int iv_dm_property_sync(int timeout_ms);

/**
 * @brief get the status of first time synchronize data module
 *
 * @return int: 0 synchronize not finished or failed, 1 success
 */
int iv_dm_init_sync_status(void);

/**
 * @brief 发送消息(通知)，不同ID对应场景具体参见产品文档
 *
 * @return int: 该接口为异步方式，返回0表示入队成功，不代表实际发送结果
 */
int iv_msg_send_notice(int msg_id, uint64_t utc_time_sec);

/**
 * @brief 发送消息(通知)扩展接口，支持带JSON消息体。不同ID对应场景具体参见产品文档
 *
 * @param msg_id:   消息ID
 * @param msg_data:    JSON消息体
 * @param msg_data_len:   消息体长度
 * @param utc_time_sec:  消息发生时utc时间，单位秒
 * 
 * @return int: 该接口为异步方式，返回0表示入队成功，不代表实际发送结果
 */
int iv_msg_send_notice_ext(int msg_id, char *msg_data, int msg_data_len, uint64_t utc_time_sec);

/***************************** API declaration end ***************************/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __IV_DM_H__ */
