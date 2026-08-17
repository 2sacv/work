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
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/clocksource.h>
#include "jco_hwtimer.h"

#if 0
#define DBGPrintk(fmt, args...) do { \
    printk("\033[m""[-DBG-] [%s:%5d] " fmt, (char *)__FILE__,__LINE__,## args);    \
} while(0)
#else
#define DBGPrintk(fmt, args...) do {}while(0)
#endif

static struct jco_hwtimer *g_jco_hwtimer[JCO_HWTIMER_MAXNUM];

static int timer_set_oneshot(struct jco_hwtimer *timer)
{
    unsigned int ctrl = 0;

    ctrl = readl(timer->base + TIMER_CONTROL);
    ctrl &= ~TIMER_CONTROL_ENABLE;
    ctrl |= TIMER_CONTROL_MODE_PERIODIC;
    writel(ctrl, timer->base + TIMER_CONTROL);

    writel(~0, timer->base + TIMER_LOAD);

    ctrl &= ~TIMER_CONTROL_ONESHOT;
    ctrl |= TIMER_CONTROL_TIMESIZE;
    ctrl |= TIMER_CONTROL_MODE_IRQ;
    ctrl |= TIMER_CONTROL_ENABLE;
    writel(ctrl, timer->base + TIMER_CONTROL);
    DBGPrintk(">>jcohwtimer>>open timer>> timer_set_oneshot base:0x%x ctrl:0x%x\r\n",timer->base,ctrl);
    return 0;
}

static int do_next_event(unsigned long delta, struct jco_hwtimer *timer)
{
    u32 ctrl;

    /* Disable timer */
    ctrl = readl_relaxed(timer->base + TIMER_CONTROL);
    ctrl &= ~TIMER_CONTROL_ENABLE;
    writel_relaxed(ctrl , timer->base + TIMER_CONTROL);

    /* write new count */
    writel_relaxed(delta , timer->base + TIMER_LOAD);
    writel_relaxed(delta , timer->base + TIMER_VALUE);
    ctrl |= TIMER_CONTROL_ENABLE;
    writel_relaxed(ctrl , timer->base + TIMER_CONTROL);

    DBGPrintk(">>jcohwtimer>>timer set>> do_next_event base:0x%x ctrl:0x%x delta:0x%x\r\n",timer->base,ctrl,delta);
    return 0;
}

int Start_timer(unsigned int *timer_handle)
{
    struct jco_hwtimer *phwtimer;
    unsigned int ctrl;

    phwtimer = container_of(timer_handle, struct jco_hwtimer, handle);
    writel(0, phwtimer->base + TIMER_INTCLR);         //清除中断；
    ctrl= readl(phwtimer->base + TIMER_CONTROL);
    ctrl &= ~TIMER_CONTROL_ENABLE;                          //停止定时模块工作；
    writel(ctrl, phwtimer->base + TIMER_CONTROL);
    ctrl |= TIMER_CONTROL_MODE_IRQ;                         //开中断；
    ctrl |= TIMER_CONTROL_ENABLE;                           //使能定时模块；
    ctrl |= TIMER_CONTROL_MODE_PERIODIC;                    //自动重加载用户模式；
    writel(ctrl, phwtimer->base + TIMER_CONTROL);
    DBGPrintk(">>jcohwtimer>>start timer>> base:0x%x ctrl:0x%x\r\n",phwtimer->base,ctrl);
    return 0;
}
EXPORT_SYMBOL_GPL(Start_timer);

int Stop_timer(unsigned int *timer_handle)
{
    struct jco_hwtimer *phwtimer;
    unsigned int ctrl;

    phwtimer = container_of(timer_handle, struct jco_hwtimer, handle);
    writel(0, phwtimer->base + TIMER_INTCLR);         //清除中断；
    ctrl= readl(phwtimer->base + TIMER_CONTROL);
    ctrl &= ~TIMER_CONTROL_ENABLE;                          //停止定时模块工作；
    ctrl &= ~TIMER_CONTROL_MODE_IRQ;                        //关中断；
    writel(ctrl, phwtimer->base + TIMER_CONTROL);
    DBGPrintk(">>jcohwtimer>>stop timer>> base:0x%x ctrl:0x%x\r\n",phwtimer->base,ctrl);
    return 0;
}
EXPORT_SYMBOL_GPL(Stop_timer);

int Set_period(unsigned int *timer_handle, unsigned long period)
{
    struct jco_hwtimer *phwtimer;
    unsigned int ctrl;

    phwtimer = container_of(timer_handle, struct jco_hwtimer, handle);
    writel(0, phwtimer->base + TIMER_INTCLR);         //清除中断；
    ctrl= readl(phwtimer->base + TIMER_CONTROL);
    ctrl &= ~TIMER_CONTROL_ENABLE;                          //停止定时模块工作；
    ctrl &= ~TIMER_CONTROL_MODE_IRQ;                        //关中断；
    writel(ctrl, phwtimer->base + TIMER_CONTROL);
    writel(period, phwtimer->base + TIMER_LOAD);            //设重装计数器计数值；
    writel(period, phwtimer->base + TIMER_VALUE);           //更新当前计数器初值；
    DBGPrintk(">>jcohwtimer>>set timer period>> base:0x%x ctrl:0x%x period:0x%x\r\n",phwtimer->base,ctrl,period);
    return 0;
}
EXPORT_SYMBOL_GPL(Set_period);

unsigned int *timer_open(unsigned int index)
{
    spin_lock(&g_jco_hwtimer[index]->info_lock);
    if (g_jco_hwtimer[index]->timer_status == 1) {
        pr_err("timer%d is using\n", index);
        spin_unlock(&g_jco_hwtimer[index]->info_lock);
        return NULL;
    }
    g_jco_hwtimer[index]->timer_status = 1;
    spin_unlock(&g_jco_hwtimer[index]->info_lock);

    timer_set_oneshot(g_jco_hwtimer[index]);
    g_jco_hwtimer[index]->handle = index;
    return &(g_jco_hwtimer[index]->handle);
}
EXPORT_SYMBOL_GPL(timer_open);

int timer_set(unsigned int *timer_handle, unsigned int usec, void (*cb_handle)(void *), void *arg)
{
    struct jco_hwtimer *phwtimer;

    phwtimer = container_of(timer_handle, struct jco_hwtimer, handle);
    phwtimer->cb_handle = cb_handle;
    phwtimer->arg = arg;
    do_next_event(HZ_PER_USEC * usec, phwtimer);

    return 0;
}
EXPORT_SYMBOL_GPL(timer_set);

void timer_close(unsigned int *timer_handle)
{
    struct jco_hwtimer *phwtimer;
    unsigned int ctrl = 0;

    phwtimer = container_of(timer_handle, struct jco_hwtimer, handle);

    spin_lock(&phwtimer->info_lock);
    if (phwtimer->timer_status == 0) {
        spin_unlock(&phwtimer->info_lock);
        return;
    }
    phwtimer->timer_status = 0;
    spin_unlock(&phwtimer->info_lock);

    ctrl = readl(phwtimer->base + TIMER_CONTROL);
    ctrl &= ~TIMER_CONTROL_ENABLE;
    writel(ctrl , phwtimer->base + TIMER_CONTROL);
    DBGPrintk(">>jcohwtimer>>close timer>> base:0x%x ctrl:0x%x\r\n",phwtimer->base,ctrl);
    return;
}
EXPORT_SYMBOL_GPL(timer_close);

static irqreturn_t jco_hwtimer_irq_handler(int irq, void *dev_data)
{
    struct jco_hwtimer * phwtimer = (struct jco_hwtimer *)dev_data;

    //中断清除寄存器 EOI
    writel_relaxed(0, phwtimer->base + TIMER_INTCLR);

    // 处理定时器中断
    //schedule_work(&phwtimer->timer_work);
    if (phwtimer->cb_handle != NULL) {
        phwtimer->cb_handle(phwtimer->arg);
    }
    return IRQ_HANDLED;
}

void handle_channel(struct work_struct *timer_work)
{
    struct jco_hwtimer *timer;
    timer = container_of(timer_work, struct jco_hwtimer, timer_work);

    if (timer->cb_handle != NULL) {
        timer->cb_handle(timer->arg);
    }

    return;
}

static int jco_hwtimer_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    struct device_node *np = pdev->dev.of_node;
    void __iomem *base_addr;
    struct resource *res;
    int ret,index,irq;

    if (of_property_read_u32(np, "timer_index", &index)) {
        pr_err("get timer index failed.\n");
        return -ENODEV;
    }

    if (index < 0 || index >= JCO_HWTIMER_MAXNUM){
        pr_err("get timer index %d failed.\n",index);
        return -ENODEV;
    }

    DBGPrintk("jco_hwtimer Probe index:%d \n",index);
    g_jco_hwtimer[index] = devm_kzalloc(dev, sizeof(*g_jco_hwtimer[0]), GFP_KERNEL);
    if (!g_jco_hwtimer[index]){
        pr_err("kzalloc timer index %d failed.\n",index);
        return -ENOMEM;
    }
    g_jco_hwtimer[index]->dev_id = index;

    // 获取定时器的基地址
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res) {
        dev_err(dev, "Failed to get memory resource\n");
        return -ENODEV;
    }

    base_addr = devm_ioremap_resource(dev, res);
    if (!base_addr) {
        dev_err(dev, "Failed to map memory resource\n");
        return -ENODEV;
    }
    g_jco_hwtimer[index]->base = base_addr;
    DBGPrintk("jco_hwtimer Probe index:%d base_addr:0x%x \n",index,g_jco_hwtimer[index]->base);

    // 获取中断号
    irq = platform_get_irq(pdev, 0);
    if (irq < 0) {
        dev_err(dev, "Failed to get interrupt resource\n");
        return irq;
    }
    g_jco_hwtimer[index]->irq = irq;

    spin_lock_init(&g_jco_hwtimer[index]->info_lock);
    //INIT_WORK(&g_jco_hwtimer[index]->timer_work, handle_channel);

    // 请求中断
    ret = devm_request_irq(dev, irq, jco_hwtimer_irq_handler, 0, "jco_hwtimer", g_jco_hwtimer[index]);
    if (ret) {
        dev_err(dev, "Failed to request IRQ %d: %d\n", irq, ret);
        return ret;
    }

    return 0;
}

static int jco_hwtimer_remove(struct platform_device *pdev)
{
    // 清理资源
    return 0;
}

static const struct of_device_id jco_hwtimer_of_match[] = {
    { .compatible = "jco-vendor, jco-hwtimer" },
    { /* end of list */ }
};

static struct platform_driver jco_hwtimer_driver = {
    .probe = jco_hwtimer_probe,
    .remove = jco_hwtimer_remove,
    .driver = {
        .name = "jco-hwtimer",
        .of_match_table = of_match_ptr(jco_hwtimer_of_match),
    },
};

MODULE_DEVICE_TABLE(of, jco_hwtimer_of_match);

static int __init jco_hwtimer_module_init(void)
{
    return platform_driver_register(&jco_hwtimer_driver);
}

static void __exit jco_hwtimer_module_exit(void)
{
    platform_driver_unregister(&jco_hwtimer_driver);
}

module_init(jco_hwtimer_module_init);
module_exit(jco_hwtimer_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("JABSCO Technology Corp.");
MODULE_DESCRIPTION("jco hwtimer driver");

