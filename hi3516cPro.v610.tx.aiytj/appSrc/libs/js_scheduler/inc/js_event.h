/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    :
 * Created Time : 2022-10-15
 * Version      : 1.0
 * Author       : luoshunfa
 * Description  :
 */


#ifndef _JS_EVENT_H_
#define _JS_EVENT_H_
#ifdef __cplusplus
extern "C" {
#endif

typedef int JSEventType;
typedef void (*JSEventCBFunc)(JSEventType event, void *msgdata, int datasize, void *userdata);
typedef void * JSEventManager;

JSEventManager * js_event_manager_new(void);
int js_event_manager_release(JSEventManager *p_em);
void js_event_manager_dump_current_callback(JSEventManager *p_em);
void js_event_manager_set_debug(JSEventManager *p_em, int enable);

int js_event_register_type(JSEventManager * p_em, JSEventType event);
int js_event_attach(JSEventManager *p_em, JSEventType event, JSEventCBFunc callback, void *userdata);
int js_event_attach_async(JSEventManager * p_em,        JSEventType event, JSEventCBFunc callback, void *userdata);
int js_event_detach(JSEventManager * p_em, JSEventType event, JSEventCBFunc callback, void *userdata);

void js_event_send(JSEventManager * p_em, JSEventType event, void *msgdata, int datasize);

#if defined(ANDROID)
#include <android/log.h>
	
#define je_log(fmt, args ...) do {                                                                            \
		__android_log_print(ANDROID_LOG_DEBUG, __FILE__, "[%s:%d]:" fmt, __PRETTY_FUNCTION__, __LINE__, ##args);  \
	 } while (0)
	
#elif defined(_WIN32)
#define je_log(fmt, ...) do {                                                 \
		fprintf(stderr, "<js_log> [%s:%d]:"fmt, __FILE__, __LINE__, __VA_ARGS__); \
	 } while (0)
	
#else
#define je_log(fmt, args ...) do {                                            \
		fprintf(stderr, "<js_log> [%s:%d]:" fmt, __FILE__, __LINE__, ##args);	   \
	 } while (0)
	
#endif

#ifdef __cplusplus
}
#endif
#endif



