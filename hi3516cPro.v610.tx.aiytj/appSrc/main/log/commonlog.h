/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : commonlog.h
 * @Created Time : 2014-02-25
 * @Version      : 1.0
 * @Author       : zengy
 * @Description  :
 */

#ifndef _COMMONLOG_H_
#define _COMMONLOG_H_

#include "jcolog.h"
#include "logapi.h"

#if defined(DEV_TYPE_BASE)
#define MAX_LOG_SIZE    64
#elif defined(DEV_TYPE_ENHANCED)
#define MAX_LOG_SIZE    256
#endif

#define MAX_DEL_COUNT   400
#define MAX_ADD_COUNT   100

class JCommonLog : public JCOLog
{
public:
    static JCommonLog* createNew(JSScheduler engine, const char* path, const char* tmppath,
                                 int maxsize, int maxrecord, int delnum);

	int 		  add_record(void *req, void *resp);
	int 		  query_log(void *req, void *resp, int resplen, int *extlen);

private:
    JCommonLog(JSScheduler engine, const char* path, const char* tmppath, int maxsize, int maxrecord, int delnum);
    ~JCommonLog();

    int           add_log_record(int type, int subtype, int level, char *mod, char *text);
    int           query_log_record(LogQueryInfoS *info, char *buf, int bufsize, int *extlen);

private:
	char          pathtemp[64];
};

#endif

