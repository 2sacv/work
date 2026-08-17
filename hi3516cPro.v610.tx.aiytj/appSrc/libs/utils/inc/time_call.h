/*
 * Copyright (C) by Jabsco Company
 * 
 * File Name    : time_call.h
 * Created Time : 2014-02-26
 * Version      : 1.0
 * Author       : luoshunfa
 * Description  : 
 */


#ifndef _time_call_H_
#define _time_call_H_
#ifdef __cplusplus
 extern "C"{
#endif


/* notice: timecall function must be non block,
 * or it casue other function cann't be call
 * in time.
 */
typedef void (*JTimeCallFunc)(void *data);
typedef void * JTimeEngine;

int		create_time_engine(JTimeEngine *engine);
void 	release_time_engine(JTimeEngine *engine);

/* notice: 
 * timeinterval: secends next time call.
 */
int 	register_time_func(JTimeEngine engine, char *desc, int timeinterval, JTimeCallFunc func, void *data);
void 	unregister_timel_func(JTimeEngine engine, JTimeCallFunc func);
 
#ifdef __cplusplus
 }
#endif
#endif

