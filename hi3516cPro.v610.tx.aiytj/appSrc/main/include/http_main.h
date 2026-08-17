/**
 * Copyright (C) by Jabsco Company
 * 
 * @File Name    : http_main.h
 * @Created Time : 2015-04-02
 * @Version      : 1.0
 * @Author       : zengy
 * @Description  : 
 */

#ifndef __HTTP_MAIN_H_
#define __HTTP_MAIN_H_
#ifdef __cplusplus
extern "C" {
#endif

int  init_http_onvif_server();

int  uninit_http_onvif_server();

int  http_signal_handler();

int  uninit_onvif_only_http();

void jco_htttpd_restart(void);

#ifdef __cplusplus
}
#endif
#endif

