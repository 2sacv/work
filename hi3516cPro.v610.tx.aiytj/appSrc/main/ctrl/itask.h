/*
 *       Filename:  itask.h
 *    Description:  
 *        Version:  1.0
 *        Created:  01/24/2021 05:30:47 PM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  zhangjian (), 
 *   Organization:  
 */

#ifndef _ITASK_H
#define _ITASK_H
#ifdef __cplusplus 
extern "C" {
#endif

#include <pthread.h>
#include "js_scheduler.h"
#include "debug.h"
#include "jevent.h"     // task 必有 attach_event
#include "jconfig.h"    // task 必有 attach_config 

enum ITASK {
    /* CODEC */
    ITASK_VIDEO     = 0,
    ITASK_AUDIO_IN  = 1,
    ITASK_AUDIO_OUT = 2,
    ITASK_JPEG      = 3,
    ITASK_ISP       = 4,
    ITASK_OSD       = 5,
    /* MISC */
    ITASK_ROI       = 6,
    ITASK_MASK      = 7,
    ITASK_LAMP      = 8,
    /* ALARM */
    ITASK_ALARM_MD  = 9,
    ITASK_ALARM_HD  = 10,
    ITASK_ALARM_VG  = 11,
    ITASK_ALARM_FACE  = 12,
    ITASK_ALARM_VP  = 13,
    ITASK_ALARM_PASSENG,
    ITASK_ALARM_FACE_SNAP,
    ITASK_PASSENGER_DB,
    ITASK_NV12TOJPEG,
    /* NET */
    ITASK_NTP,
	ITASK_SEARCH,
    ITASK_WDT,
    ITASK_WATCH_FRAME,
    ITASK_TMREBOOT,
    ITASK_BUTT,
    ITASK_AGING,
    ITASK_COUNT,
};

/*
 * itask mean INDEPENDENT TASK
 * 参考 git stage 的设计，用 VAR 及 VAR_stage，在只加一把锁的情况下，保证线程安全。
 * VAR_stage 是要加锁才能访问的变量， VAR 则是 loop 内访问的变量。
 */
struct itask {
    pthread_mutex_t mutex;
    const char *name;
    int  cmd_stage;
    int  cmd_self;          // delay operator
    int  stat_stage;
    int  stat;
    void (*copystage2loop)(void *);
    void (*stop_routine)(void *);
    union {
        void *data;
        void **dmap;        // usage to see main_void_p()
    };
    JSTCHandle  hdl_loop;
	JSRWHandle  hdl_sock[2];
	JSTCHandle  hdl_frozen;
    JSScheduler sch;
};

enum STAT {
    STAT_TICKING        = 1 << 10,      //
    STAT_TICKACK        = 1 << 11,      //
    STAT_FROZEN         = 1 << 22,      //
    STAT_CONFIGURING    = 1 << 23,      // 只有一个事件时直接使用，可用 get_command()
    STAT_GENERAL        = 1 << 24,
    STAT_PAUSE          = 1 << 28,
    STAT_STOPPED        = 1 << 29,
    STAT_ALL_COMMAND    = 0x00FFFFFF,   // 低 24 位用于多事件的状态值
    STAT_ALL_STATBIT    = 0xFF000000,   // 高 8 位用于通用状态，
};

extern struct itask g_tasks[ITASK_COUNT];

int  get_command(struct itask *task);
void set_command(struct itask *task, int bit);
void clr_command(struct itask *task, int bit);

int  pop_stat(struct itask *task, int bit);
int  get_stat(struct itask *task, int bit);
void set_stat(struct itask *task, int bit);
void clr_stat(struct itask *task, int bit);

void itask_tick_sync();
void itask_tick_ack();
void itask_stop_sync();
void itask_stop_ack();

void set_frozen(struct itask *task, int ms);

#define chk_frozen(task, cmd, clr) do {                             \
    if (pop_stat(task, STAT_FROZEN)) {                              \
        set_command(task, cmd & (~clr));                            \
        _io_itask("freezing %s, cmd:%x\n", task->name, cmd&(~clr)); \
        return;                                                     \
    }                                                               \
} while(0)

#define chk_running(task, cmd) do {             \
    /* update stopping */                       \
    if (pop_stat(task, STAT_PAUSE)) {           \
        if (!pop_stat(task, STAT_STOPPED)) {    \
            if (task->stop_routine) {           \
                task->stop_routine(task);       \
            }                                   \
        }                                       \
        set_stat(task, STAT_STOPPED);           \
        return;                                 \
    }                                           \
    /* tick from jcp   */                       \
    if (pop_stat(task, STAT_TICKING)) {         \
        set_stat(task, STAT_TICKACK);           \
    }                                           \
} while(0)

#ifdef __cplusplus
}
#endif
#endif
