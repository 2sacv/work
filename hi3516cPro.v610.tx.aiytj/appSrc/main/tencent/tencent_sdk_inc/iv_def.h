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
 * @file    iv_def.h
 * @brief   Description data type and functions of object model
 * @version v1.0.0
 *
 *****************************************************************************/

#pragma once

#ifndef _IOT_VIDEO_GDM_DEF_H_
#define _IOT_VIDEO_GDM_DEF_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "iv_sys.h"

typedef int (*ivm_callback_Action)(void *);  //!< 接收到Action控制命令时的回调处理函数
typedef void (*ivm_callback_ProWritable)(const void *);  //!< 接收到ProWritable变更时的回调处理函数

/*!
        \brief	物模型环境初始化
 */
int ivm_env_init(void);

/*!
        \brief	物模型互斥加锁
 */
void ivm_lock(void);

/*!
        \brief	物模型互斥解锁
 */
void ivm_unlock(void);

/*!
 *	\brief 设置产品相关能力信息
 *	property 具体对象字符串
 *	json_val 具体对象对应的json值
 *
 *	ivm_set_productInfo( "videoCodec", "H264" );
 *	ivm_set_productInfo( "audioCodec", "AAC" );
 */
// int ivm_set_productInfo(const char *property, const void *val);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  // _IOT_VIDEO_GDM_DEF_H_
