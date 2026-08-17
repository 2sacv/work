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
 * @file    iv_sys.h
 * @brief   Description system function of sdk
 * @version v1.0.0
 *
 *****************************************************************************/

#ifndef __IV_SYS_H__
#define __IV_SYS_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#define IV_SYS_FILE_PATH_MAX_LEN 128
/********************************Macro Definition****************************/

/*Defines the certificate type*/
typedef enum
{
    IV_SYS_CERTIFICATE_TYPE_CERT = 0, /*cert file certificate*/
    IV_SYS_CERTIFICATE_TYPE_KEY  = 1, /*key file certificate*/

    IV_SYS_CERTIFICATE_TYPE_BUTT
} iv_sys_certificate_type_e;

typedef enum
{
    IV_SYS_RECONNECT_STATUS  = 0,
    IV_SYS_DISCONNECT_STATUS = 1,

    IV_SYS_INVALID_STATUS
} iv_sys_offline_status_type_e;

/**
 * SDK log print/upload level
 */
typedef enum
{
    IV_eLOG_DISABLE = 0,
    IV_eLOG_ERROR   = 1,
    IV_eLOG_WARN    = 2,
    IV_eLOG_INFO    = 3,
    IV_eLOG_DEBUG   = 4
} iv_sys_log_level_type_e;

typedef struct {
    char dev_cert_file[IV_SYS_FILE_PATH_MAX_LEN];
    char dev_pkey_file[IV_SYS_FILE_PATH_MAX_LEN];
} iv_sys_cert_info;

typedef struct {
    /*product id*/
    const char *product_id;
    /*device name*/
    const char *device_name;
    /* region */
    const char *region;
    /* product sercert, dynamic register need */
    const char *product_secret;
    /*CERTIFICATE_TYPE_CERT:cert path; CERTIFICATE_TYPE_KEY:key*/
    union {
        iv_sys_cert_info *device_cert;
        const char *device_key;
    };
} iv_sys_device_info;

typedef struct iv_sys_os_impl {
    /* implementation of heap memory allocation */
    void *(*hal_os_malloc)(size_t size);
    void (*hal_os_free)(void *ptr);
    /* standard output */
    void (*hal_os_printf)(const char *format, va_list ap);
    /* a unit of time, in millisecond, from 1970-1-1:00:00:00 to the present */
    uint64_t (*hal_os_gettime_ms)(void);
    /* a unit of time, in seconds, from 1970-1-1:00:00:00 to the present */
    uint32_t (*hal_os_gettime_second)(void);
    /* sleep implementation, millisecond unit */
    void (*hal_os_sleepms)(uint32_t ms);
    /* mac address of the network device that the device is in use */
    void (*hal_os_getmac)(uint8_t *mac, size_t mac_len);
    /* root partition size, in kB*/
    size_t (*hal_os_get_disksize)(void);
    /* memory size, in kB*/
    size_t (*hal_os_get_memsize)(void);
    /* returns the platform type, such as Linux, RTOS...*/
    const char *(*hal_os_get_platform)(void);
    /* Create a file: if the file does not exist, create it in read-write mode.
     If the file exists, open it and return the handle.*/
    int (*hal_os_create_file)(const char *pathname);
    /* read the file and return the bytes read, fail:return -1*/
    size_t (*hal_os_read_file)(int fd, void *buf, size_t count);
    /* write to the file and return the bytes written, fail:return -1 */
    size_t (*hal_os_write_file)(int fd, const void *buf, size_t count);
    /* close the file handle, fail:return -1, success:return 0*/
    int (*hal_os_close_file)(int fd);

    /*file system api*/
    void *(*hal_os_fopen)(const char *filename, const char *mode);
    size_t (*hal_os_fread)(void *ptr, size_t size, size_t nmemb, void *fp);
    size_t (*hal_os_fwrite)(const void *ptr, size_t size, size_t nmemb, void *fp);
    size_t (*hal_os_filesize)(void *fp);
    int (*hal_os_fclose)(void *fp);
    /* random data generation */
    long int (*hal_os_random)(void);
} iv_sys_os_impl_t;

/*Defines the parameter of system init */
typedef struct iv_sys_init_parm_s {
    /*read-write directory, can not save after power off*/
    char sys_cache_path[IV_SYS_FILE_PATH_MAX_LEN];
    /*read-write directory, can save after power off*/
    char sys_store_path[IV_SYS_FILE_PATH_MAX_LEN];
    /*read-write directory, can save device_info.json*/
    const char *dev_info_path;
    /*device information*/
    iv_sys_device_info *device_info;
    // timeout value (unit: ms) mqtt communication command, suggest value: (5 * 1000)ms
    uint32_t command_timeout;
    //  keep alive time interval in millisecond,suggest value: (240 * 1000)ms
    uint32_t keep_alive_ms;
    //  mqtt ping packet send interval, must lower than keep_alive_ms
    uint32_t mqtt_ping_interval_ms;
    // flag of auto reconnection, 1 is enable and recommended
    uint8_t auto_connect_enable;
    /*mqtt recv buffer max size, default: 2048 */
    uint32_t mqtt_recv_buf_max_size;
    /*mqtt write buffer max size, default:2048 */
    uint32_t mqtt_write_buf_max_size;
    /*max channel number, 0 : device is IPC; other:device is NVR, means max ipc number*/
    uint32_t max_channel_num;
    /*internal communicate use http or https, default 0 means use https*/
    uint32_t comm_use_http;
    /*for p2p proxy mode, set to 1, default 0 means no proxy*/
    uint32_t p2p_use_proxy;
    /*brief: the callback function of device online */
    void (*iv_sys_online_cb)(uint64_t u64NetDateTime);
    /*brief: the callback function of device offline */
    void (*iv_sys_offline_cb)(iv_sys_offline_status_type_e status);
    /*brief: the callback function of module status,
      module_status: bit 0: cs status, bit 1: ai status */
    void (*iv_func_module_status_cb)(uint32_t module_status);
    /*brief: read data from slave moudle*/
    int (*iv_sys_slave_read_cb)(uint8_t *data, uint32_t len);
    /*brief: write data to slave moudle*/
    int (*iv_sys_slave_write_cb)(uint8_t *data, uint32_t len);
    /*brief: the the callback function of hal  */
    iv_sys_os_impl_t os_impl_cb;
} iv_sys_init_parm_s;

/********************************Macro Definition end*************************/

/********************************API declaration******************************/
/*brief: init iv system*/
int iv_sys_init(const iv_sys_init_parm_s *pstInitParm);

/*brief: exit iv system */
int iv_sys_exit(void);

/*brief: get system certificate type*/
iv_sys_certificate_type_e iv_sys_get_certificate_type(void);

/*brief: get system time, Param: ms*/
int iv_sys_get_time(uint64_t *milliseconds);

/*brief: set system time, Param: ms*/
int iv_sys_set_time(uint64_t milliseconds);

/*brief: sync system time, Param: ms*/
// int iv_sys_sync_time(uint64_t *Param);

/*brief: set log print level*/
void iv_sys_set_log_level(iv_sys_log_level_type_e level);

/*brief: set log upload level (NOT SUPPORT ANYMORE!!)*/
int iv_sys_set_upload_level(iv_sys_log_level_type_e level);

/*@brief: Do dynamic register/create device */
int iv_sys_dyn_reg_device(iv_sys_device_info *psys_devInfo, char *dev_psk, int dev_psk_len);

/*brief: report offline message（已废弃）*/
int iv_sys_active_offline(void);

/*brief: report customer log*/
int iv_sys_customer_log_report(char *msg);

/********************************API declaration end******************************/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __IV_SYS_H__ */
