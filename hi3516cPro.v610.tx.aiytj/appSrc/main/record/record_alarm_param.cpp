/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    :
 * Created Time : 2024-06-27
 * Version      : 1.0
 * Author       : tangjx
 * Description  :
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <dirent.h>

#include "recordapi.h"
#include "g_log.h"
#include "debug.h"
#include "utils.h"
#include "shm_buf.h"
#include "record_main.h"
#include "encodeapi.h"
#include "record_lib.h"
#include "record_file_manage.h"
#include "record_watch.h"
#include "record_alarm_param.h"
#include "record_disk.h"

#include "jconfig.h"
#include "confapi.h"
#include "libmp4.h"
#include "js_rec.h"
#include "js_scheduler.h"
#include "system_ctrl.h"
#include "jevent.h"

static RecAlarmParam_t g_recalarm_param = {0};

void set_recalarm_flag(int alarm_record, int time_record)
{
    time_t curTime = 0;
    struct tm curTm_s = {0};
    struct tm *curTm = &curTm_s;
    int alarm_time = 0;
    int minutes_num = 0;

    int alarm_minutes = 0;
    int curday = 0;
    int recalarm = FALSE;
    curTime = time(NULL);
    localtime_r(&curTime, &curTm_s);

    do{
        curday = (curTm->tm_year + 1900) * 10000 + (curTm->tm_mon + 1) * 100 + curTm->tm_mday;
        if (g_recalarm_param.startday != curday) {
            g_recalarm_param.is_cross_day = TRUE;
        }

        if((TRUE == g_recalarm_param.is_cross_day || !g_recalarm_param.is_init) &&
            (TRUE == time_record || TRUE == alarm_record)) {
            g_recalarm_param.is_cross_day = FALSE;
            if(TRUE == time_record) {
                recalarm = TRUE;
            } else {
                recalarm = FALSE;
            }
            init_recalarm_param(recalarm);
        }

        if(TRUE != alarm_record || !g_recalarm_param.is_init) {
            break;
        }

        minutes_num = curTm->tm_hour * 60 + curTm->tm_min;

         for(alarm_time = 0; alarm_time < (MINS_ALARM_RECORD+alarm_minutes); alarm_time++) {
            DBG("minutes_num:%d\n", minutes_num);
            if(minutes_num <= MINS_OF_1DAY) {
                   g_recalarm_param.recalarm_flag[minutes_num % MINS_OF_1DAY] = 'A';
            }
            minutes_num = minutes_num + 1;
        }

        int fd = open(g_recalarm_param.recalarm_file, O_RDWR);
        if (fd < 0) {
            ERR("%s is open fail: %s\n",
                g_recalarm_param.recalarm_file, strerror(errno));
            if (!is_okey(g_recalarm_param.recalarm_file)) { // 文件不存在，重新创建
                g_recalarm_param.is_init = FALSE;
            }
            break;
        }

        write(fd, g_recalarm_param.recalarm_flag ,MINS_OF_1DAY + 1);
        close(fd);
        sync();
    } while(0);

}

void get_recalarm_flag(int day, char *flag_buf, int buflen)
{
    int ret = 0;
    char filePath[256] = {0,};
    char devPath[128] = {0,};
    char *p = filePath;

    do{
        ret = storage_get_mmcpath(devPath);
        if(ret < 0){
            break;
        }

        p += sprintf(p, "%s/IPCamera/%d/%s", devPath, day, REC_ALARM_FILE);

        int fd = open(filePath, O_RDWR);
        if (fd < 0) {
            ERR("%s is open fail: %s\n", filePath, strerror(errno));
            break;
        }

        read(fd, flag_buf, buflen);
        close(fd);
    } while (0);
}

void get_curday_recalarm_flag(char *flagbuf, int buflen)
{
    if (flagbuf == NULL || g_recalarm_param.recalarm_flag == NULL) {
        return;
    }

    memcpy(flagbuf, g_recalarm_param.recalarm_flag, buflen);

    return;
}

int init_recalarm_param(int recalarm)
{
    int ret = 0;
    time_t curTime = 0;
    struct tm curTm_s = {0};
    struct tm *curTm = &curTm_s;
    char filePath[256] = {0,};
    char devPath[128] = "/mnt";
    char curDevPath[128] = {0,};
    char *p = filePath;
    int is_new_file = FALSE;
    RecordCtrlS cfg = {0};

    do{
        ret = conf_get_recordcfg(&cfg);
        if(ret < 0) {
            ret = FAILURE;
            break;
        }

        if((TimeJudge((unsigned int *)&cfg.timestrategy) == 0) && (TRUE == recalarm)) {
            ret = FAILURE;
            break;
        }

        if (storage_get_mmcpath(curDevPath) < 0){
            ret = FAILURE;
            break;
        }

        p += sprintf(p, "%s/IPCamera", devPath);
        ret = generate_record_dir(filePath);
        if(ret < 0) {
            ERR( "mkdir %s error: %s!\n", filePath, strerror(errno));
            break;
        }
        curTime = time(NULL);
        localtime_r(&curTime, &curTm_s);
        p += sprintf(p, "/%d%02d%02d", curTm->tm_year + 1900,
                     curTm->tm_mon + 1, curTm->tm_mday);
        ret = generate_record_dir(filePath);
        if(ret < 0) {
            ERR( "mkdir %s error: %s!\n", filePath, strerror(errno));
            break;
        }

        p += sprintf(p, "/%s", REC_ALARM_FILE);
        g_recalarm_param.startday = (curTm->tm_year + 1900) * 10000 + (curTm->tm_mon + 1) * 100 + curTm->tm_mday;
        memcpy(g_recalarm_param.recalarm_file, filePath, strlen(filePath));
        if (!is_okey(filePath)) {
            is_new_file = TRUE;
        }

        int fd = open(filePath, O_RDWR|O_CREAT,S_IRWXU);
        if (fd < 0) {
            ret = FAILURE;
            ERR("%s is open fail: %s\n", filePath, strerror(errno));
            break;
        }

        if (is_new_file) { // 初始化 1440 个标志位
            memset(g_recalarm_param.recalarm_flag, 'N', MINS_OF_1DAY);
            g_recalarm_param.recalarm_flag[MINS_OF_1DAY] = '\0';
            write(fd, g_recalarm_param.recalarm_flag, MINS_OF_1DAY + 1);
        } else {
            read(fd, g_recalarm_param.recalarm_flag ,MINS_OF_1DAY + 1);
        }
        close(fd);
        memset(g_recalarm_param.second_day_recflag, 'N', MINS_OF_1DAY);
        g_recalarm_param.second_day_recflag[MINS_OF_1DAY] = '\0';
        g_recalarm_param.is_cross_day = FALSE;
        g_recalarm_param.is_init = TRUE;
    } while(0);
    return ret;
}

void uinit_recalarm_flag(void)
{
    g_recalarm_param.is_init = FALSE;
    g_recalarm_param.need_save = FALSE;
    g_recalarm_param.is_cross_day = FALSE;
    memset(g_recalarm_param.recalarm_flag, 'N', MINS_OF_1DAY);
    g_recalarm_param.recalarm_flag[MINS_OF_1DAY] = '\0';
    memset(g_recalarm_param.second_day_recflag, 'N', MINS_OF_1DAY);
    g_recalarm_param.second_day_recflag[MINS_OF_1DAY] = '\0';
}
