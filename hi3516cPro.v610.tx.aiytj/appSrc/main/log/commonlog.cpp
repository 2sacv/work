/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : commonlog.cpp
 * @Created Time : 2014-02-25
 * @Version      : 1.0
 * @Author       : zengy
 * @Description  :
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <pthread.h>
#include <time.h>
#include <fcntl.h>
#include <time.h>
#include <sys/times.h>

#include "debug.h"
#include "commonlog.h"
#include "logcomm.h"
#include "utils.h"

#define MAX_MSG_BYTE 256
const char *LogMainType[] = {"ptz", "alarm", "common"};
const char *LogLevel[] = {"debug", "warning", "fatal"};
//char *LogSubType[][3] = {{"contrl", "alarm"},
//                       {"VideoLoss", "motionDec", "IO"}
//                      };

JCommonLog* JCommonLog::createNew(JSScheduler engine, const char* path, const char* tmppath,
                                  int maxsize, int maxrecord, int delnum)
{
    return new JCommonLog(engine, path, tmppath, maxsize, maxrecord, delnum);
}

JCommonLog::JCommonLog(JSScheduler engine, const char* path, const char* tmppath, int maxsize, int maxrecord, int delnum)
    : JCOLog(engine, path, tmppath, maxsize, maxrecord, delnum)
{
    strcpy(pathtemp, tmppath);
}

JCommonLog::~JCommonLog()
{

}

int JCommonLog::add_record(void *req, void* rsp)
{
    if(req == NULL) {
        ERR("Parameter error [req] is NULL!\n");
        return -1;
    }

    LogRecordInfoS *info = (LogRecordInfoS *)req;
    //DBG("type = %d, msg = %s\n", info->type, info->msg);
    if(add_log_record(info->type, info->subtype, info->level, info->module, info->msg) < 0)
        return -1;

    return 0;
}

int JCommonLog::add_log_record(int type, int subtype, int level, char *mod, char *text)
{
    if(type < 0 || subtype < 0 || level < 0 || NULL == mod || NULL == text)
        return -1;

    char szTime[20] = {0};
    char szType[12] = {0};
    char szSubType[12] = {0};
    char szLevel[12] = {0};
    char szMod[12] = {0};
    int  addnum = 0;
    int  nwrite = 0;
    char msgbuf[512] = {0};

    sscanf(LogMainType[type], "%8s", szType);
    sscanf(LogLevel[type], "%8s", szLevel);
    sscanf(mod, "%8s", szMod);
    //subtype

    time_t now;
    struct tm trm;
    time(&now);
    localtime_r(&now, &trm);
    strftime(szTime, 20, "%F %H:%M:%S", &trm);

    log_lock();
    // append a '\n' at end of line if needed, so sizeof(msgbuf)-1
    nwrite = snprintf(msgbuf, sizeof(msgbuf)-1, "%-20s|%-8s|%-8s|%-8s|%-8s|%s",
                      szTime, szType, szSubType, szLevel, szMod, text);
    if (msgbuf[nwrite-1] != '\n') {
        msgbuf[nwrite] = '\n';
        nwrite++;
    }

    Writefully(get_file_fd(), msgbuf, nwrite);
    addnum = ++get_new_add_num();
    log_unlock();

    if(addnum >= MAX_ADD_COUNT) {
        DBG("add log num >= 100,need sync log file\n");
        sync_log_file();
    }

    return 0;
}

int JCommonLog::query_log(void *req, void* resp, int resplen, int *extlen)
{
    if(req == NULL) {
        ERR("Parameter error [req] is NULL!\n");
        return -1;
    }
    LogQueryInfoS *info = (LogQueryInfoS *)req;
    //DBG("info.type = %d, info.level=%d, info.itemindex = %d, info.itemnum=%d, "
    //  "info.starttime=%s,info.endtime=%s\n", info->type, info->level,
    //  info->itemindex, info->itemnum, info->starttime, info->endtime);

    if (query_log_record(info, (char *)resp, resplen, extlen) < 0) {
        return -1;
    }

    return 0;

}

int JCommonLog::query_log_record(LogQueryInfoS *info, char *buf, int bufsize, int *extlen)
{
    FILE *fp = NULL;
    char *line = NULL;
    size_t length = 0;
    ssize_t read = 0;

    char stime[32] = {0};
    char stype[32] = {0};

    char *p = NULL;
    int len = 0;
    char tempbuf[128] = {0};
    int templen = 0;

    int line_num = 0;
    int realnum = 0;

    log_lock();

    fp = fopen(pathtemp, "r");
    if(NULL == fp) {
        ERR("open [%s] failed!\n", pathtemp);
        log_unlock();
        return -1;
    }

    while((read = getline(&line, &length, fp)) != -1) {
        p = NULL;
        memset(stime, 0, sizeof(stime));
        memset(stype, 0, sizeof(stype));

        sscanf(line, "%24[^|]|%16[^|]", stime, stype);
        if ((strcmp(stime, info->endtime) > 0) ||
            (strcmp(stime, info->starttime) < 0)) {
            continue;
        }

        if(0 != info->type) {
			p = strstr(stype, " ");
            if(p != NULL) {
                *p = '\0';
            }

            if(strcmp(LogMainType[info->type], stype)) {
                continue;
            }
        }
        p = strstr(line, "\n");
        if (NULL == p) {
            continue;
        }
        *p = '\0';
        line_num++;

        if ((line_num >= (info->itemindex)) &&
            line_num < (info->itemindex + info->itemnum)) {
            if ((len + MAX_MSG_BYTE) > bufsize) {
                DBG("(len + MAX_MSG_BYTE):%d > bufsize : %d\n",
                    (len + MAX_MSG_BYTE), bufsize);
                break;
            }
            len += sprintf(buf+len, "%s#", line);
            realnum++;
        }

    }

    templen = sprintf(tempbuf, "itemtotal=%d;itemnum=%d;itemlist=",
                      line_num, realnum);
    memmove(buf+templen, buf, len);
    memcpy(buf, tempbuf, templen);
    strcat(buf, ";");
    *extlen = strlen(buf);
    fclose(fp);
    log_unlock();
    if (line) {
        free(line);
    }

    return 0;
}
/*  */

