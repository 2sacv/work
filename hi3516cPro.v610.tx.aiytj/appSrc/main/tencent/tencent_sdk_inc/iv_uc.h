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
* @file    iv_uc.h
* @brief   Description user cmd function of sdk
* @version v1.0.0
*
*****************************************************************************/

#ifndef __IV_UC_H__
#define __IV_UC_H__
	
#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#include "stdint.h"

/* Defines the parameter of user cmd init */
typedef struct iv_uc_init_parm_s
{
	/* recieve cmd message callback */
    void (*iv_uc_recv_msg_cb)(void *pUcData, uint32_t u32UcDataLen);
}iv_uc_init_parm_s;

/* brief: init user cmd module */
int iv_uc_init(iv_uc_init_parm_s *pstInitParm);

/* brief: exit user cmd module */
int iv_uc_exit(void);

/* brief: send user cmd message */
uint32_t iv_uc_send_msg(void *pUcData, uint32_t u32UcDataLen);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* __IV_UC_H__ */

