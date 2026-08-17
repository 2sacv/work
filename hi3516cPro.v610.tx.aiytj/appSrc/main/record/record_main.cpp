/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : recordMain.cpp
 * @Created Time : 2014-03-04
 * @Version      : 1.0
 * @Author       : zengy
 * @Description  :
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

#include "debug.h"
#include "record_disk.h"
#include "record_watch.h"
#include "record_main.h"
#include "system_ctrl.h"
#include "record_file_manage.h"
#include "time_config.h"
#include "conf_list.h"
#include "record_delay.h"
#include "g_sys.h"
#include "record_alarm_param.h"
#include "time_sync.h"
#include "pthread_manage.h"

int g_timeset_before_setrecordtime_flag = 0;

int record_sync_time_from_last_recordfile(void)
{
    int ret = SUCCESS;
    char datetime[32] = {0};

    struct tm thetm = {0};
    time_t synctime = 0;
    int year, month, day, hour, min, sec;
    int timelen;
    char tmpbuf[8] = {0};

    //TzoneS timeZone = {0};

    do{
        if(g_timeset_before_setrecordtime_flag){
            DBG("g_timeset_before_setrecordtime_flag has been set!\n");
            break;
        }

        ret = get_lastest_record_datetime(datetime, sizeof(datetime));
        if(ret < 0)
            break;

        //like 20220909-175707-0335
        DBG("LAST record time is :%s\n", datetime);

        tmpbuf[0] = datetime[0];
        tmpbuf[1] = datetime[1];
        tmpbuf[2] = datetime[2];
        tmpbuf[3] = datetime[3];
        tmpbuf[4] = '\0';
        year = atoi(tmpbuf);

        tmpbuf[0] = datetime[4];
        tmpbuf[1] = datetime[5];
        tmpbuf[2] = '\0';
        month = atoi(tmpbuf);

        tmpbuf[0] = datetime[6];
        tmpbuf[1] = datetime[7];
        tmpbuf[2] = '\0';
        day = atoi(tmpbuf);

        tmpbuf[0] = datetime[9];
        tmpbuf[1] = datetime[10];
        tmpbuf[2] = '\0';
        hour = atoi(tmpbuf);

        tmpbuf[0] = datetime[11];
        tmpbuf[1] = datetime[12];
        tmpbuf[2] = '\0';
        min = atoi(tmpbuf);

        tmpbuf[0] = datetime[13];
        tmpbuf[1] = datetime[14];
        tmpbuf[2] = '\0';
        sec = atoi(tmpbuf);

        tmpbuf[0] = datetime[16];
        tmpbuf[1] = datetime[17];
        tmpbuf[2] = datetime[18];
        tmpbuf[3] = datetime[19];
        tmpbuf[4] = '\0';
        timelen = atoi(tmpbuf);
        DBG("year: %d, mon: %d, mday: %d, hour: %d, min: %d, sec: %d timelen:%d\n", year, month, day, hour, min, sec, timelen);

        timelen += 20; //add the time secends for reboot time, not add too big

        sec += timelen;
        min += sec / 60;
        sec = sec % 60;
        hour += min / 60;
        min = min % 60;

        DBG("year: %d, mon: %d, mday: %d, hour: %d, min: %d, sec: %d\n", year, month, day, hour, min, sec);

        thetm.tm_sec = sec;
        thetm.tm_min = min;
        thetm.tm_hour = hour;
        thetm.tm_mday = day;
        thetm.tm_mon = month - 1;
        thetm.tm_year = year - 1900;

        //get_config(handleTimeZoneCfg, timeZone);
        //synctime = mktime(&thetm) - timeZone.timeoffset ;
        synctime = mktime(&thetm);
        DBG("synctime:%lld\n", synctime);
        if (check_timesync(synctime) == SUCCESS) {
            /*对时之前进行规则核对，阿里没对过时才进行对时*/
            dump_system_time(synctime);
        }

        g_timeset_before_setrecordtime_flag = true;
    }while(0);

    return ret;
}

int record_module_uninit(void)
{
    uninit_record_watch();
    uninit_record_disk();
    uninit_delay_handle();
    DBG( "record exit\n");

    return 0;
}

void *record_module_init_cb(void *data)
{
    SYSLOG("start init record server\n");

    init_record_disk();

    scan_record_all_tmp_mp4();

    repair_record_all();

    init_delay_handle();
    record_sync_time_from_last_recordfile();

    init_recalarm_param(TRUE);

    if(get_g_sys(factest)) {
        SYSLOG("\n\n\n___ Warning: skip record bcz FACTORY_DB_FILE ___\n\n\n");
    } else {
        init_record_watch();
    }

    SYSLOG("record server init success\n");

    return NULL;
}

int record_module_init(void)
{
    int i = 0;

    // 录像模块会扫描 sd 卡的临时文件，碰到有的坏卡会卡住，所以单独创建一个线程初始化录像模块
    if (0 == pthread_namecreate("init_record", record_module_init_cb, NULL)) {
        SYSLOG("create record server thread fail\n");
        return -1;
    }

    // 最多等待 5s，录像模块还没初始化完成就继续向下走
    while (!g_timeset_before_setrecordtime_flag && i++ < 5) {
        usleep(1 * 1000 * 1000);
    }

    return 0;
}

