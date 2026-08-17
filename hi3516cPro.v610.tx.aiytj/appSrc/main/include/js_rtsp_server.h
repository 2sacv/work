/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    :
 * Created Time : 2018-09-08
 * Version      : 1.0
 * Author       : luoshunfa
 * Description  :
 */

#ifndef _RTSP_SERVER_H_
#define _RTSP_SERVER_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef void* (*JGetSBFunc)(int mediatype, int level, int channel, int isaudio);
typedef void (*JPutSBFunc)(int mediatype, int level, int channel, int isaudio);

int rtsp_server_init(void);
void rtsp_server_uninit(void);

void rtsp_server_uninit_cb(void * userdata);


#ifdef __cplusplus
}
#endif
#endif
