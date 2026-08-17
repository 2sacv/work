/******************************************************************************
    Copyright (C), 2008-2028, JABSCO ELECTRONIC Tech. Co., Ltd

    File Name    : jco_hwtimer.h
    Version      : 1.0
    Author       : JABSCO Video Server Software Group
    Created      : 2024-12-02
    Description  :
    History      :
                        created by tangjx   2024-12-02
******************************************************************************/

#ifndef __JABSCO_HWTIMER_H__
#define __JABSCO_HWTIMER_H__

#include <linux/io.h>

#define JCO_HWTIMER_MAXNUM  4
#define CLK_SEL_FREQ    3000000
#define HZ_PER_USEC 3

#define TIMER_LOAD     0x000 //计数初值寄存器
#define TIMER_VALUE    0x004 //当前计数值寄存器
#define TIMER_CONTROL  0x008 //Timer 控制寄存器
#define TIMER_INTCLR   0x00C //中断清除寄存器
#define TIMER_RIS      0x010 //原始中断寄存器
#define TIMER_MIS      0x014 //屏蔽后中断寄存器
#define TIMER_BGLOAD   0x018 //周期模式计数初值寄存器

#define TIMER_CONTROL_ENABLE        (1 << 7)  /* 1: enable,   0:disable. */
#define TIMER_CONTROL_MODE_PERIODIC (1 << 6)  /* 1: periodic, 0:free running. */
#define TIMER_CONTROL_MODE_IRQ      (1 << 5)  /* 1: use irq,  0:shield irq. */
#define TIMER_CONTROL_TIMESIZE      (1 << 1)  /* 1: 32bit,    0:16bit. */
#define TIMER_CONTROL_ONESHOT       (1 << 0)  /* 1: one time, 0:periodic or free running. */

struct jco_hwtimer {
    void __iomem *base;
    int dev_id;
    int irq;
    void (*cb_handle)(void *);
    void *arg;
    int timer_status;                //1 表示已open , 0 表示未初始化
    spinlock_t info_lock;
    struct work_struct timer_work;
    unsigned int handle;
};

int Start_timer(unsigned int *timer_handle);
int Stop_timer(unsigned int *timer_handle);
int Set_period(unsigned int *timer_handle, unsigned long period);

unsigned int *timer_open(unsigned int index);
int timer_set(unsigned int *timer_handle, unsigned int usec, void (*handle)(void *), void *arg);
void timer_close(unsigned int *timer_handle);

#endif // __JABSCO_HWTIMER_H__