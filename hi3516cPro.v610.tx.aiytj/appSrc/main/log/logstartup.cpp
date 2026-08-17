/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : main.cpp
 * @Created Time : 2013-10-15
 * @Version      : 1.0
 * @Author       : zengy
 * @Description  :
 */

#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include "debug.h"
#include "commonlog.h"

#include "logapi.h"

#define TMPLOG      "/tmp/applog"
#define REALLOG     "/opt/log/applog"

#define MAX_ITEMNUM  20

static JCommonLog *log = NULL;

void* init_server_log()
{
    log = JCommonLog::createNew(NULL, REALLOG, TMPLOG, MAX_LOG_SIZE, 0, MAX_DEL_COUNT);
    log->log_init();

    return NULL;
}

void* init_client_log_sync(void *data)
{
    log->log_init_client_sync(data);

    return NULL;
}

void* uninit_server_log()
{
    log->reclaim();
	
    return NULL;
}

int log_record(int type, int subtype, int level, const char *module, const char *fmt, ...)
{
    if(0 > type || 0 > subtype || 0 > level || NULL == module || NULL == fmt) {
        return -1;
    }
	
	if(NULL == log) {
		ERR("Log service is not init!\n");
		return -1;
	}

    LogRecordInfoS info = {0};
    info.type = type;
    info.subtype = subtype;
    info.level = level;
    if (module != NULL) {
        strcpy(info.module, module);
    }

    va_list ap;

    bzero(&ap, sizeof(va_list));

    va_start(ap, fmt);
    vsnprintf(info.msg, sizeof(info.msg), fmt, ap);
    va_end(ap);

	return log->add_record((void*)&info, NULL);
}

int log_query(LogQueryInfoS *info, char *buf, int buflen)
{
    if (NULL == info || NULL == buf || 0 == buflen) {
        ERR("info is NULL!\n");
        return -1;
    }

	if(NULL == log) {
		ERR("Log service is not init!\n");
		return -1;
	}

    struct tm tim = {0};

    if (strptime(info->starttime, "%Y-%m-%d %H:%M:%S", &tim) == NULL) {
        ERR("time format wrong\n");
        return -1;
    }

    if (strptime(info->endtime, "%Y-%m-%d %H:%M:%S", &tim) == NULL) {
        ERR("time format wrong\n");
        return -1;
    }

    if (strcmp(info->starttime, info->endtime) > 0) {
        ERR("starttime > endtime\n");
        return -1;
    }

    if ((info->itemindex) <= 0) {
        info->itemindex = 1;
    }

    if ((info->itemnum) < 0) {
        info->itemnum = MAX_ITEMNUM;
    }

    int respLen = 0;
	
	int ret = log->query_log((void*)info, (void*)buf, buflen, (int *)&respLen);
	
    return ret;
}

int log_sync()
{
	log->sync_log_file();

	return 0;
}

