/* 
 *       Filename:  itask.c
 *    Description:  
 *        Version:  1.0
 *        Created:  01/24/2021 05:25:33 PM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  zhangjian (), 
 *   Organization:  
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h> // mutex
#include "debug.h"
#include "itask.h"
#include "system_sch.h"

/* task list */
struct itask g_tasks[ITASK_COUNT] = {
    {PTHREAD_MUTEX_INITIALIZER,}, 
};

/* 所有事件(config+alarm)一次性获取，在 loop() 中串行执行 */
int get_command(struct itask *task)
{
    int cmd;
    pthread_mutex_lock(&(task->mutex));
    task->stat = task->stat_stage;
    cmd = (task->cmd_stage | task->cmd_self);
    task->cmd_stage = task->cmd_self = 0;
    if (task->copystage2loop != NULL) {
        task->copystage2loop((void*)task);
    }
    pthread_mutex_unlock(&(task->mutex));

    return cmd;
}

void set_command(struct itask *task, int bit)
{
    task->cmd_self |= bit;
}

void clr_command(struct itask *task, int bit)
{
    task->cmd_self &= ~bit;
}

/* pop, sch 内部使用，不加锁，确保在一次 task 过程中，任意时间 stat 的原子性 */
int pop_stat(struct itask *task, int bit)
{
    return (task->stat & bit);
}

/* get,set,clr: sch 外部读，或写，加锁 */
int get_stat(struct itask *task, int bit)
{
    int stat_stage;
    pthread_mutex_lock(&(task->mutex));
    stat_stage = (task->stat_stage & bit);
    pthread_mutex_unlock(&(task->mutex));
    return stat_stage;
}

void set_stat(struct itask *task, int bit)
{
    pthread_mutex_lock(&(task->mutex));
    task->stat_stage |= bit;
    pthread_mutex_unlock(&(task->mutex));
    return;
}

void clr_stat(struct itask *task, int bit)
{
    pthread_mutex_lock(&(task->mutex));
    task->stat_stage &= ~bit;
    pthread_mutex_unlock(&(task->mutex));
    return;
}

void set_frozen(struct itask *task, int ms)
{
    set_stat(task, STAT_FROZEN);

    void clr_frozen(void *ctx)
    {
        struct itask *task = ctx;
        clr_stat(task, STAT_FROZEN);
        COLOR_G("clear STAT_FROZEN from %s\n", task->name);
    }

    js_create_once(task->hdl_frozen, task->sch, ms, clr_frozen, task);
}

/*
 * ack, sync 参考 TCP 3 次握手的设计:
 * SYN(synchronous建立联机) ACK(acknowledgement 确认) 
 * 每次 ack 前，先清理掉状态 STAT_ackING
 */
void itask_tick_sync()
{
    int i;
    for (i = 0; i < ARRAY_SIZE(g_tasks); i++) {
        struct itask *task = &g_tasks[i];
        DBG("i:%02d tick %s\n", i, task->name);
        clr_stat(task, STAT_TICKING);
        set_stat(task, STAT_TICKING);
    }
}

/*
 * 遇到 step 较长的 loop 如 ntp, 可能需要多次 sync 才能得到状态。
 */
void itask_tick_ack()
{
    int i;
    for (i = 0; i < ARRAY_SIZE(g_tasks); i++) {
        struct itask *task = &g_tasks[i];
        if (!get_stat(task, STAT_TICKACK)) {
            WAR("i:%02d no ACK from itask %s", i, task->name);
        }
    }
}

/*
 * 主要是释放 CPU 资源。
 * 内存资源，则通过 stop_routine() 进行释放
 * 不同于 STAT_ackING，STAT_STOPPED 是不清理的
 * 升级时，通知 itask 分层 stop，避免应用资源的依赖。
 * 此函数，在 notify_from_server(JEvent_UpdateBegin) 之后调用，独立的线程服务，更加上层
 **/
void itask_stop_sync()
{
    struct itask *task;

    task = &g_tasks[ITASK_TMREBOOT]  ; set_stat(task, STAT_PAUSE);
    /* 网络相关 */
    task = &g_tasks[ITASK_SEARCH]    ; set_stat(task, STAT_PAUSE);
    task = &g_tasks[ITASK_NTP]       ; set_stat(task, STAT_PAUSE);

    /* 告警检测 */
    task = &g_tasks[ITASK_ALARM_MD]  ; set_stat(task, STAT_PAUSE);
    task = &g_tasks[ITASK_ALARM_HD]  ; set_stat(task, STAT_PAUSE);
    task = &g_tasks[ITASK_ALARM_VG]  ; set_stat(task, STAT_PAUSE);
    task = &g_tasks[ITASK_ALARM_FACE]  ; set_stat(task, STAT_PAUSE);
    task = &g_tasks[ITASK_ALARM_VP]  ; set_stat(task, STAT_PAUSE);
    task = &g_tasks[ITASK_ALARM_PASSENG]  ; set_stat(task, STAT_PAUSE);
    task = &g_tasks[ITASK_ALARM_FACE_SNAP]  ; set_stat(task, STAT_PAUSE);
    task = &g_tasks[ITASK_NV12TOJPEG]  ; set_stat(task, STAT_PAUSE);
    task = &g_tasks[ITASK_PASSENGER_DB]  ; set_stat(task, STAT_PAUSE);

    /* 灯光控制 */
    task = &g_tasks[ITASK_LAMP]      ; set_stat(task, STAT_PAUSE);

    /* ROI MASK */
    task = &g_tasks[ITASK_ROI ]      ; set_stat(task, STAT_PAUSE);
    task = &g_tasks[ITASK_MASK]      ; set_stat(task, STAT_PAUSE);

    /* 人形 1/3 才能完全退出 */
    usleep(500*1000)                 ;

    /* 音频视频 */
    task = &g_tasks[ITASK_AUDIO_OUT] ; set_stat(task, STAT_PAUSE);
    task = &g_tasks[ITASK_AUDIO_IN ] ; set_stat(task, STAT_PAUSE);
    task = &g_tasks[ITASK_OSD]       ; set_stat(task, STAT_PAUSE);
    task = &g_tasks[ITASK_JPEG]      ; set_stat(task, STAT_PAUSE);

    /* shm_cache uninit .5s */ 
    usleep(500*1000)                 ;
    task = &g_tasks[ITASK_VIDEO]     ; set_stat(task, STAT_PAUSE);
    task = &g_tasks[ITASK_ISP]       ; set_stat(task, STAT_PAUSE);
}

/*
 * 遇到 step 较长的 loop 如 ntp, 可能需要多次 sync 才能得到状态。
 */
void itask_stop_ack()
{
    int i;
    for (i = 0; i < ARRAY_SIZE(g_tasks); i++) {
        struct itask *task = &g_tasks[i];
        if (!get_stat(task, STAT_STOPPED)) {
            WAR("i:%02d running itask %s", i, task->name);
        }
    }
}

