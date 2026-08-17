/**
 * Copyright (C) by Jabsco Company
 * 
 * @File Name    : onvif_main.h
 * @Created Time : 2015-06-03
 * @Version      : 1.0
 * @Author       : zengy
 * @Description  : 
 */

#ifndef _ONVIF_MAIN_H_
#define _ONVIF_MAIN_H_
#ifdef __cplusplus
extern "C" {
#endif

#define ONVIF_PRESET_MAX_COUNT  (128)
typedef struct {
    float panTiltX;
    float panTiltY;
    float zoomX;
} POSITION_S;

typedef struct {
    unsigned char bSet;
    char szName[63];
    POSITION_S position;
} PRESET_S;


void onvif_signalHandler(void);

int  onvif_init(void);

void onvif_uninit(void);

int onvif_reinit(void);

void *DoOnvifService(unsigned long ip, int port, short socket);

void onvif_ip_change_handle(int id, void *p_src, int size, void *ctx);

void onvif_refresh_profile(int id, void *p_src, int size, void *ctx);

void onvif_md_alarm_handle(int id, void *p_src, int size, void *ctx);

void onvif_md_alarm_clear_handle(int id, void *p_src, int size, void *ctx);

void onvif_vl_alarm_handle(int id, void *p_src, int size, void *ctx);

void onvif_def_alarm_handle(int id, void *p_src, int size, void *ctx);

void onvif_delay_notify_handle(int e_type);

#if defined(BRANCH_HBCF)
void onvif_video_mask_alarm_handle(void *arg);
void onvif_video_mask_alarm_clear_handle(void *arg);
#endif

PRESET_S *onvif_get_presets();
int onvif_set_preset(int nPreset, PRESET_S *pPreset);
int onvif_del_preset(int nPreset);

#ifdef __cplusplus
}
#endif
#endif

