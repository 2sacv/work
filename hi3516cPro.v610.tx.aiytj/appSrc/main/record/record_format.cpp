/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    : record_capture.c
 * Created Time : 2012-10-08
 * Version      : 1.0
 * Author       : dongzh
 * Description  :
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>

#include "debug.h"
#include "utils.h"
#include "system_ctrl.h"
#include "record_delay.h"
#include "record_watch.h"
#include "record_disk.h"

#include "recordapi.h"
#include "record_format.h"
#include "record_alarm_param.h"
#include "g_stat.h"
#include "g_sys.h"
#include "g_log.h"
#include "factory_db.h"
#include "js_rec.h"

extern "C" {
void close_all_record_connection(void);
}

#ifdef PLATFORM_TENCENT
#include "tencent_record_play.h"
#endif

#ifdef PLATFORM_GB
#include "gb_api.h"
#endif

extern JSScheduler          g_disk_scheduler;

int sformatstatus = 0;
static pthread_mutex_t  sformatMutex = PTHREAD_MUTEX_INITIALIZER;

int set_format_status(int status)
{
    pthread_mutex_lock(&sformatMutex);
    sformatstatus = status;
    pthread_mutex_unlock(&sformatMutex);
    return 0;
}

int get_format_status()
{
    int status = 0;
    pthread_mutex_lock(&sformatMutex);
    status = sformatstatus;
    pthread_mutex_unlock(&sformatMutex);

    return status;
}

static void format_handle_delay(void *PReq)
{
    int ret = 0;
    char szCmd[256] = {0};
    char devPath[128] = {0};
    char *devname = (char *)PReq;
    struct timeval tv_recv, tv_resp = {0}; 
    float tv_spend;

    static int s_formating_flags = 0;
    
    if(s_formating_flags){
       return;
    }

    s_formating_flags = 1;

    if(is_okey("/tmp/fmterror.txt")) {
        UtilSystemCmd((char *)"rm /tmp/fmterror.txt");
    }

    sprintf(devPath, "%s", devname);
    set_format_status(0);
    toggle_redirect(0);

    snprintf(szCmd, sizeof(szCmd), "rm %s", F_ADD_FILE);
    UtilSystemCmd(szCmd);
    memset(szCmd, 0, sizeof(szCmd));
    snprintf(szCmd, sizeof(szCmd), "rm %s", F_REMOVE_FILE);
    UtilSystemCmd(szCmd);

#ifdef PLATFORM_GB
    gb_stop_playback();
#endif

    close_all_record_connection();

    // 如果一人在看回放，另一人在格式化
#ifdef PLATFORM_TENCENT
    replay_stop_allchn();
#endif

    uninit_record_watch();

    record_storage_dev_remove(devPath);
    
    set_format_status(20);

    SYSLOG("start format SD-card\n");
    gettimeofday(&tv_recv, NULL);
    set_g_stat(record, SD_REC_FORMAT);
    UtilSystemCmd("lzbox sdcard format");
    clr_g_stat(record, SD_REC_FORMAT);
    gettimeofday(&tv_resp, NULL);
    tv_spend = tv_resp.tv_sec-tv_recv.tv_sec + (float)(tv_recv.tv_usec-tv_resp.tv_usec)/(1000*1000);
    SYSLOG("spend %.2fs to format SD-card: /opt/log/sd.format\n", tv_spend);

    do {
        if (is_mountpoint("/mnt")) {
            break;
        }
        usleep(500*1000);
    } while(++ret < 6);
    TouchFile(F_SD_AUTH);
    sync();

    if (is_mountpoint("/mnt") && is_okey("/mnt/.devid") && df_used_p("/mnt") <= 1) {
        DBG("format success\n");
        set_format_status(100);
    } else {
        UtilSystemCmd((char *)"rm /tmp/fmterror.txt");
        DBG("format error\n");
        set_format_status(-1);
    }

    set_cur_serial(-1);
    set_cur_aserial(0);
    init_record_watch();

//cleanup:
    if(devname){
        free(devname);
        devname = NULL;
    }

    s_formating_flags = 0;
}

void format_handle_req(const char *devname)
{
//  int ret = 0;
    char *pDevName = NULL;

    if(NULL == devname) {
        DBG("format_handle_req error:devname is NULL\n");
        return ;
    }

    if(!strcmp(devname,"samba") || !strcmp(devname,"nfs")) {
        DBG("format samba or nfs is not allow\n");
        return ;
    }

    pDevName = (char *)malloc(strlen(devname));
    if(NULL == pDevName) {
        DBG("format malloc error:%s\n", strerror(errno));
        return ;
    }

    sprintf(pDevName, "%s", devname);

    js_run_function(g_disk_scheduler, format_handle_delay, pDevName, 0);
/*
    ret = delay_handle_add(format_handle_delay, pDevName, NULL);
    if(ret < 0) {
        DBG("add format delay error\n");
        free(pDevName);
    }*/

    return ;
}

int record_request_format(const char *devname)
{
    if (!get_g_stat(record, SD_CD_IN)) {
        ERR("no sd card, ignore format\n");
        set_format_status(-1);
        return -1;
    }

    if (get_g_stat(record, SD_ERR_WRITE_PROTECT | SD_ERR_MMCNODE)) {
        ERR("storage get mmcpath failed, ignore format\n");
        set_format_status(-1);
        return -1;
    }

    format_handle_req(devname);

    return 0;
}

int record_get_format_status(int *isok)
{
    int status = 0;

    status = get_format_status();
    *isok = status;

    return 0;
}


