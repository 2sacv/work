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
* @file    iv_ota.h
* @brief   Description ota function of sdk
* @version v1.0.0
*
*****************************************************************************/

#ifndef __IV_OTA_H__
#define __IV_OTA_H__
	
#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */
	
#include <stdint.h>

/* the max length of ota download file storage path */
#define IV_OTA_LOCAL_FILE_NAME_MAX_LEN  160
#define IV_OTA_FIRMWARE_VERSION_MAX_LEN 64

/* Defines the ota progress type */
typedef enum
{
    IV_OTA_PROGRESS_TYPE_WRITE_FLASH   = 0, /* write firmware to flash */
    IV_OTA_PROGRESS_TYPE_REBOOT        = 1, /* reboot */
    IV_OTA_PROGRESS_TYPE_SUCCESS       = 2, /* ota success */
    IV_OTA_PROGRESS_TYPE_FAIL          = 3, /* ota fail */
    IV_OTA_PROGRESS_TYPE_WRITING_FLASH = 4, /* writing firmware to flash */

    IV_OTA_PROGRESS_TYPE_BUTT
} iv_ota_progress_type_e;

/* Defines the ota's fail type */
typedef enum
{
	IV_OTA_FAIL_TYPE_OPEN_FILE			= 0,	/* fail to open firmware */
	IV_OTA_FAIL_TYPE_WRONG_SIZE			= 1,	/* the size of the transmission is not equal to the real size */
	IV_OTA_FAIL_TYPE_WRITE_FLASH		= 2,	/* fail to write flash */

	IV_OTA_FAIL_TYPE_WRITE_BUTT
}iv_ota_fail_type_e;

/* Defines the parameter of ota_thread exit status */
typedef struct iv_ota_thread_exit_status_s
{
	int mqtt_online;    /* 0:mqtt is offline,  1:mqtt is online */
}iv_ota_thread_exit_status_s;

/* Defines the parameter of user cmd init */
typedef struct iv_ota_init_parm_s
{
	/* callback for update/burning firmware after download finish */
    void (* iv_ota_firmware_update_cb)(char * firmware_name, uint32_t firmware_len);
    /* local file path for saving firmware and info files */
    char  firmware_path[IV_OTA_LOCAL_FILE_NAME_MAX_LEN];
    /* running firmware version for current device */
    char  firmware_version[IV_OTA_FIRMWARE_VERSION_MAX_LEN];
	/* callback for ota preparation, it should return 0 (means prepare OK) to make ota continue, otherwise ota will stop */
	// int (* iv_ota_prepare_cb)();
	int (* iv_ota_prepare_cb)(char *new_firmware_version, uint32_t new_firmware_size);
	void (* iv_ota_download_size_cb)(uint32_t size);
	void (* iv_ota_thread_exit_cb)(iv_ota_thread_exit_status_s ota_thread_status);
}iv_ota_init_parm_s;

/* brief: ota module init */
int iv_ota_init(iv_ota_init_parm_s *init_parm);

/* brief: ota module exit */
int iv_ota_exit(void);

/* brief: device update */
int iv_ota_update_progress(iv_ota_progress_type_e type, int progress_value);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __IV_OTA_H__ */
 
