/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    :
 * Created Time : 2014-04-01
 * Version      : 1.0
 * Author       : luoshunfa
 * Description  :
 */

#ifndef _RECORD_WATCH_h_
#define _RECORD_WATCH_h_

#ifdef __cplusplus
	extern "C" {
#endif

#include <pthread.h>

#include "js_rec.h"
#include "jconfstruct.h"
    
enum {
    CMD_RECORD_CFG     = 1 << 0,
    CMD_VIDEO_CFG      = 1 << 1,
    CMD_AUDIO_CFG      = 1 << 2,
    CMD_TIME_CHANGE    = 1 << 3,
    CMD_VIDMASK_CHANGE = 1 << 4,
    CMD_ENCODE_CHANGE  = 1 << 5,

    CMD_NEED_STOP      = 1 << 6,
};

struct record_cfg{
    RecordCtrlS rec_reccfg;
    VideoEncS   rec_encodecfg;
    AudioCfgS   rec_audiocfg;
    VideoMaskS  vid_maskcfg;
};

struct record_run {
    pthread_mutex_t lock;
    JSScheduler sch_rec;
    JSScheduler sch_stop;
    JSTCHandle hdl_loop;
    int      record_init;
    
    JSRecHandle * currec_handle;
    JRecParamInfo rec_paraminfo;
    int need_alarmrec;
    int need_manualrec;
    int need_stoprec;
    struct cmdstat *p_ctx;
};

typedef enum {
    JREC_TYPE_ALARM,
    JREC_TYPE_SCHEDULE,
    JREC_TYPE_MANUAL,
    JREC_TYPE_NUMS,
} eJRecType;

#define ALARM_RECORD_SEC 120
#define DEF_RECORD_TIME     (900)

int init_record_watch(void);
int uninit_record_watch(void);
int repair_record_all(void);
int is_need_delete_old_files(void);
int repair_last_record(const char *path);
int scan_record_all_tmp_mp4(void);
void record_sync_cur_mp4info();
int record_get_cur_recfile_path(char *path);
#ifdef __cplusplus
}
#endif	
#endif

