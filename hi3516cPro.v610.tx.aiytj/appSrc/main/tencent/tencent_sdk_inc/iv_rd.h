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
 * @file    iv_rd.h
 * @brief   Description record function of sdk
 * @version v1.0.0
 *
 *****************************************************************************/

#ifndef __IV_RD_H__
#define __IV_RD_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>

#include <time.h>

#include "iv_cm.h"

/**
 * @brief record file pack type
 */
typedef enum
{
    IV_RD_PACK_TYPE_NONE, /* not pack record file */
    IV_RD_PACK_TYPE_TIME, /* pack record file by fix time duration */
    IV_RD_PACK_TYPE_SIZE, /* pack record file by fix size */

    IV_RD_PACK_TYPE_BUTT /*invalid pack type */
} iv_rd_pack_type_e;

/*Defines the parameter of record init */
typedef struct iv_rd_init_parm_s {
    char s8SdcardPath[64];                     /*the sdcard path that use to record*/
    iv_cm_aenc_sample_rate_e eAudioSampleRate; /*the sample rate of audio*/
    iv_cm_venc_type_e eVencType;               /*venc type*/
    iv_rd_pack_type_e ePackType;
    union {
        uint32_t u32PackTimeSec;  /* seconds of pack record file */
        uint32_t u32PackFileSize; /* byte file size of pack record file */
    } uPackLen;
} iv_rd_init_parm_s;

/*brief: init iv record*/
int iv_rd_init(iv_rd_init_parm_s *pstInitParm);

/*brief: exit iv record*/
int iv_rd_exit(void);

/*brief: start record*/
int iv_rd_record_start(char *ps8RecordType, iv_cm_av_data_info_s *format);

/*brief: stop record*/
int iv_rd_record_stop(void);

/*brief: send record stream*/
int iv_rd_push_stream(iv_cm_stream_type_e eStreamType, void *pStream);

#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif /*__IV_RD_H__*/
