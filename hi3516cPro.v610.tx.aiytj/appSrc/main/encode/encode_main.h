/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : encode_main.h
 * @Created Time : 2024-11-25
 * @Version      : 1.0
 * @Author       : tangjx
 * @Description  :
*/

#ifndef __ENCODE_MAIN_H__
#define __ENCODE_MAIN_H__
#ifdef __cplusplus
extern "C" {
#endif

#define MAX_SENSOR_NUM 1

#include "encodeapi.h"
#include "shm_buf_pool.h"
#include "ot_common_venc.h"

int init_encode_server();
int uninit_encode_background();
int uninit_encode_wait();
int uninit_encode_done();

#ifdef __cplusplus
}
#endif
#endif



