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
* @file    iv_ai.h
* @brief   Description ai function
* @version v1.0.0
*
*****************************************************************************/

#ifndef __IV_AI_H__
#define __IV_AI_H__
	
#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */
	
#include <stdint.h>

/*Defines the parameter of ai module init */
typedef struct iv_ai_init_parm_s
{
    char *work_dir;     /* cloudai working directory, such as where to get picture/video  */
    int model_id;       /* ai model id */
    void(*iv_ai_upload_cos_result_cb)(char *file_path, int err_code);
} iv_ai_init_parm_s;

/*brief: init ai module*/
void *iv_ai_init (iv_ai_init_parm_s params);

/*brief: deinit ai module */
int iv_ai_deinit(void **pp_handle);

/*brief: notify event */
int iv_ai_notify_event(void * handle, int evt_id, void *arg);

/*brief: ai task start */
int iv_ai_start(void *handle);

/*brief: ai task stop */
int iv_ai_stop(void *handle);

/*brief: add model id */
int iv_ai_add_model_id(void *handle, int model_id);

/*brief: del model id */
int iv_ai_del_mode_id(void *handle, int model_id);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __IV_AI_H__ */
