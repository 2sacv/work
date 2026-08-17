/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    :
 * Created Time : 2024-02-23
 * Version      : 1.0
 * Author       : luoshunfa
 * Description  :
 */


#ifndef _JS_SCHEDULER_H_
#define _JS_SCHEDULER_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <inttypes.h>
#include <stdint.h>

#ifdef _WIN32
# define JS_EXTERN __declspec(dllexport)
#else
# define JS_EXTERN /* nothing */
#endif

#if defined(_WIN32)
#define strcasecmp _stricmp
#define strncasecmp  _strnicmp
#endif

#if defined(_MSC_VER) && _MSC_VER < 1900
#define snprintf _snprintf
#endif

#if defined(ANDROID)
#include <android/log.h>

#define js_log(fmt, args ...) do {                                                                            \
    __android_log_print(ANDROID_LOG_DEBUG, __FILE__, "<js_log> [%s:%d]:" fmt, __PRETTY_FUNCTION__, __LINE__, ##args);  \
 } while (0)

#elif defined(_WIN32)

#if 1
#define js_log(fmt, ...) do {                                                 \
    fprintf(stderr, "<js_log> [%s:%d]:"fmt, __FILE__, __LINE__, __VA_ARGS__); \
 } while (0)
#else
#define js_log(fmt, ...) do { \
        char strBuffer[1024] = { 0 }; \
        _snprintf(strBuffer, sizeof(strBuffer)-1, "<js_log> [%s:%d]"fmt, __FILE__, __LINE__, ##__VA_ARGS__); \
        OutputDebugStringA(strBuffer); \
    }while(0)
#endif

#else
///*
#define js_log(fmt, args ...) do {                                            \
    fprintf(stderr, "<js_log> [%s:%d]:" fmt, __FILE__, __LINE__, ##args);     \
 } while (0)
//*/

//#define js_log(fmt, args ...) do {} while (0)

#endif

typedef void * JSScheduler;

typedef void * JSTCHandle;
typedef void (*JSTCFunc)(void *userdata);

#define JS_READABLE     0x0001
#define JS_WRITABLE     0x0004
typedef void * JSRWHandle;
typedef void (*JSRWFunc)(int fd, int events, void *userdata);

typedef void (*JSRunFunc)(void *userdata);

JS_EXTERN JSScheduler js_create_scheduler(char *schername);
JS_EXTERN int js_delete_scheduler(JSScheduler scher);
JS_EXTERN int js_scheduler_dbgonoff(JSScheduler scher, int dbgonoff);
JS_EXTERN int js_scheduler_is_blocked(JSScheduler scher);
JS_EXTERN int js_scheduler_blocked_nums(int dectectms);
JS_EXTERN void js_dump_schedulers_callback(void);
JS_EXTERN int js_scheduler_set_skip_blocked_detect(JSScheduler scher, int skipblockeddetect);


/* 1. first call after firstmillisecends tims
 * 2. if repeatmillisecends > 0 , will call every repeatmillisecends tims
 * 3. if repeatmillisecends == 0, just call once, will stop after first call
 * 4. should call delete for every create timer
 */
 /* use _r api to replace this api*/
 /////////////////////////////////////////////////////////////////////////////////////////
JS_EXTERN JSTCHandle js_create_timer(JSScheduler scher, int firstmillisecends, int repeatmillisecends, JSTCFunc cb, void *userdata);
JS_EXTERN int js_delete_timer(JSTCHandle handle);
JS_EXTERN int js_modify_timer_time(JSTCHandle handle, int repeatmillisecends);

JS_EXTERN JSRWHandle js_create_reader(JSScheduler scher, int fd, int event, JSRWFunc cb, void *userdata);
JS_EXTERN int js_delete_reader(JSRWHandle handle);
JS_EXTERN int js_modify_reader_event(JSRWHandle handle, int event);
/////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////
/*
* new api, thread safe for time handle and read handle
* JSTCHandle g_time_handle = NULL;  
* js_create_timer_r(scheduler, 1*1000, 1*1000, callback, usrdata, &g_time_handle);
* js_delete_timer_r(&g_time_handle);

* JSRWHandle g_read_handle = NULL;
* js_create_reader_r(scheduler, fd, event, callback, userdata, &g_read_handle);
* js_delete_reader_r(&g_read_handle);
*/
/*
* note: when repeatmillisecends = 0, after run cb, will delete *phandle, and set *phanle = 0, 	
* important: The memory pointed by phanle cann't be released before cb!!!
*/
JS_EXTERN int js_create_timer_r(JSScheduler scher, int firstmillisecends, int repeatmillisecends,
                                JSTCFunc cb, void *userdata, JSTCHandle * phandle);
JS_EXTERN int js_delete_timer_r(JSTCHandle * phandle);
JS_EXTERN int js_modify_timer_time_r(JSTCHandle * phandle, int repeatmillisecends);

JS_EXTERN int js_create_reader_r(JSScheduler scher, int fd, int event,
                                 JSRWFunc cb, void *userdata, JSRWHandle * phandle);
JS_EXTERN int js_delete_reader_r(JSRWHandle * phandle);
JS_EXTERN int js_modify_reader_event_r(JSRWHandle * phandle, int event);
/////////////////////////////////////////////////////////////////////////////////////////

/*
 * syncflag:
 * 0: 在scher里调用js_run_function 则立即执行cb; 在其他线程调用则推到scher队列里，立即返回。scher下次调度循环里执行CB�?
 * 1: 在scher里调用js_run_function 则立即执行cb; 在其他线程调用则推到scher队列里，并等待scher调度执行cb，然后才返回�?
 * 2: 直接推到scher队列里，立即返回。scher下次调度循环里执行CB�?
 */ 
JS_EXTERN int js_run_function(JSScheduler scher, JSRunFunc cb, void *userdata, int syncflag);

JS_EXTERN void js_sleep(int microsecends);
JS_EXTERN uint64_t js_hrtime(void);

#ifdef __cplusplus
}
#endif
#endif


