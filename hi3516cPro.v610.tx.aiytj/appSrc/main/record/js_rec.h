/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    :
 * Created Time : 2018-09-11
 * Version      : 1.0
 * Author       : luoshunfa
 * Description  :
 */

#ifndef _JS_REC_H_
#define _JS_REC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "shm_buf.h"
#include "js_scheduler.h"

typedef void  JSRecHandle;

typedef struct {
    int    record_chn;
    int    I_frame_interval;
    time_t start_utc;
    int    record_time; /* how long secends the file to record */
    int    precord_time; /* how long secends need to pre record , the value 1-10 is valid*/
    int    need_audio_record; /* whether audio is record*/
    int    record_type; /* */
    char   record_filename[256];
} JRecParamInfo;

int get_cur_serial();
void set_cur_serial(int serial);
int get_cur_aserial();
void set_cur_aserial(int serial);

JSRecHandle* js_start_record_mp4(JSScheduler scheduler, JSScheduler scheduler_stop, JRecParamInfo *info);
void js_stop_record_mp4(void *recsess);

/*Note:this fuction must be called in recsess's scheduler*/
JRecParamInfo* js_get_record_recparaminfo(JSRecHandle *recsess);

/*Note:this fuction must be called in recsess's scheduler*/
int js_get_record_pasttime(JSRecHandle *recsess);

/*Note:this fuction must be called in recsess's scheduler*/
int js_is_record_corssday(JSRecHandle *recsess);

/*Note:this fuction must be called in recsess's scheduler*/
int js_is_record_stoped(JSRecHandle *recsess);

int js_clear_scheduler_stop(JSRecHandle *recsess);

struct timespec *js_get_record_start_time(JSRecHandle *recsess);

void js_sync_cur_mp4info(void *data);

int handle_alarm_pre_record(shm_buf_t shm_buf, tSBFrame *frm_info, time_t *time_offset);

int get_serial_by_vts(shm_buf_t abuf, double vframe_timestamp);

#ifdef __cplusplus
}
#endif

#endif

