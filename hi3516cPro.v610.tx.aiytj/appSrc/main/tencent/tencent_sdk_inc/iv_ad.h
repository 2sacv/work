/*****************************************************************************
 * Tencent is pleased to support the open source community by making IoT Video available.
 * Copyright (C) 2020 THL A29 Limited, a Tencent company. All rights reserved.
 * Licensed under the MIT License (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://opensource.org/licenses/MIT
 * Unless required by applicable law or agreed to in writing, software distributed under the License is
 * distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @file    iv_ad.h
 * @brief   Description add device function of sdk
 * @version v1.0.0
 *
 *****************************************************************************/

#ifndef __IV_AD_H__
#define __IV_AD_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* brief: send binding infomation to server */
int iv_ad_send_bind_info(char *pToken, int time_out);

int iv_ad_unbind_device_request(int time_out);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __IV_AD_H__ */
