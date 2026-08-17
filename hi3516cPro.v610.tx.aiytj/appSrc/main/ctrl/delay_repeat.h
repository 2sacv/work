#ifndef _DELAY_REPEAT_H
#define _DELAY_REPEAT_H
#ifdef __cplusplus
extern "C" {
#endif
    /* macro */

    /* cond act like ASSERT
     * #include "BasicUsageEnvironment.hh"
     **/
#define DELAY_REPEAT_IF_FAIL(cond, schedule, delay_secs) do {             \
    static TaskToken task = NULL;                                         \
    if (!(cond)) {                                                        \
        if (NULL == task) {                                               \
            task = schedule->scheduleDelayedTask(delay_secs*1000*1000LL,  \
                (TaskFunc*)__FUNCTION__, NULL);                           \
        } else {                                                          \
            schedule->rescheduleDelayedTask(task, delay_secs*1000*1000LL, \
                (TaskFunc*)__FUNCTION__, NULL);                           \
        }                                                                 \
        return;                                                           \
    }                                                                     \
                                                                          \
    if(task) {                                                            \
        schedule->unscheduleDelayedTask(task);                            \
    }                                                                     \
} while (0);


    /* typedef */

    /* declaration */
#ifdef __cplusplus
}
#endif
#endif
