/**
 * Copyright (C) by Jabsco Company
 * 
 * @File Name    : alarmapi.h
 * @Created Time : 2014-03-11
 * @Version      : 1.0
 * @Author       : zengy
 * @Description  : 
 */

#ifndef _ALARM_API_H_
#define _ALARM_API_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "alarm_event.h"

//typ:报警类型
//channel，报警通道数
//filter   :  是否时间过滤。1 ，需要时间过滤，0， 不经过时间过滤
//desc   :   报警的一些说明。
int alarm_report(JALARM_TYPE type, int channel, int filter, const char *desc);

/*'stime' format :  2014-03-14 11:15:39  */ /* JALARM_TYPE_BEGIN indicate all type*/
int alarm_query(QueryInfoS *info, char *buf, int bufsize);

int alarm_sync_log();

#ifdef __cplusplus
}
#endif
#endif

