#ifndef _SYSTEM_SCH_H
#define _SYSTEM_SCH_H

#ifdef __cplusplus
extern "C" {
#endif

#include "js_scheduler.h"

/*
 * 1. 由于线程安全的原因，一次运行的任务，不能在回调 routine 中调用 js_delete_timer_r()
 * 2. js_delete_timer_r()会在下次运行js_create_once()时执行
 * 3. 如果js_create_once()在整个生命周期只运行一次，则会有一个 JSTCHandle 的内存泄漏，不过这没有关系。
 * 4. 保证 js_create_once(routine) 只在一个线程内运行，可以不用加锁
      static JSTCHandle hdl_;
 */
#define js_create_once(handle, sch, delay_ms, routine, arg) do {    \
    js_delete_timer_r(&handle);                                 \
    js_create_timer_r(sch, delay_ms, 0, routine, arg, &handle);       \
} while(0)

extern JSScheduler sch_sock, sch_fast, sch_slow, sch_disk;

/*
 * sch debug
 **/
int start_test_sch(void);

#ifdef __cplusplus
}
#endif
#endif
