
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
 * @file    iv_ka.h
 * @brief   Description keep alive of low power of sdk
 * @version v1.0.0
 *
 *****************************************************************************/

#ifndef __IV_KA_H__
#define __IV_KA_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>

#define KEEP_ALIVE_AUTH_MESSAGE_MAX_LEN   256
#define KEEP_ALIVE_HB_MESSAGE_MAX_LEN     32
#define KEEP_ALIVE_SERVER_ADDRESS_MAX_LEN 128

typedef struct {
    //服务器域名
    char server_addr[KEEP_ALIVE_SERVER_ADDRESS_MAX_LEN];
    //服务器端口号
    uint32_t port;
    //鉴权消息
    uint8_t auth_msg[KEEP_ALIVE_AUTH_MESSAGE_MAX_LEN];
    //鉴权消息长度
    uint32_t auth_msg_len;
    //心跳消息
    uint8_t heart_beat_msg[KEEP_ALIVE_HB_MESSAGE_MAX_LEN];
    //心跳消息长度
    uint32_t heart_beat_msg_len;
    //唤醒消息
    uint8_t wake_up_msg[KEEP_ALIVE_AUTH_MESSAGE_MAX_LEN];
    //唤醒消息长度
    uint32_t wake_up_msg_len;
} iv_keep_alive_info_s;

/*brief get low power keep alive info,call this function you should check dev is online*/
int iv_get_keep_alive_info(iv_keep_alive_info_s* pkeep_alive_info, int timeout_ms);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
