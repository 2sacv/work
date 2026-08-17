/**
 * Copyright (C) by Jabsco Company
 * 
 * @File Name    : alarm_log.h
 * @Created Time : 2014-03-12
 * @Version      : 1.0
 * @Author       : zengy
 * @Description  : 
 */

#ifndef _ALARM_LOG_H_
#define _ALARM_LOG_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "mxml.h"
#include "alarm_event.h"


int init_alarm_log(void *data);

int uninit_alarm_log();

int alarm_log_sync();

int alarm_add_event_log(JALARM_TYPE type, int channel, char *desc);

int alarm_event_query(QueryInfoS *info, char *buf, int bufsize, int *retlen);
 
#ifdef __cplusplus
}
#endif
#endif

