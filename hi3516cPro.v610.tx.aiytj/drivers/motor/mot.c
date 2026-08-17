/******************************************************************************
    Copyright (C), 2008-2028, JABSCO ELECTRONIC Tech. Co., Ltd

    File Name    : motor.c
    Version      : 1.0
    Author       : JABSCO Video Server Software Group
    Created      : 2024-12-24
    Description  :
    History      :
                        created by tianjun. 2015-07-18
******************************************************************************/

#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/clk.h>
#include <linux/pwm.h>
#include <linux/file.h>
#include <linux/list.h>
#include <linux/gpio.h>
#include <linux/time.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/debugfs.h>
#include <linux/kthread.h>
#include <linux/mfd/core.h>
#include <linux/mempolicy.h>
#include <linux/interrupt.h>
#include <linux/semaphore.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <asm/cacheflush.h>
#include <linux/proc_fs.h>
#include "jco_hwtimer.h"
#include "mot.h"
#include "xtm5809.h"

#define DEBUG 0

#define MOTOR_MIN_STEPS 500

#define TCU_BASE_FRQ 3000000 // tcu 基准频率 3MHZ
#define MOTOR_BASE_FRQ 10000  // 马达脉冲基准频率 10KHZ = 0.1ms
#define TCU_PERIOD (TCU_BASE_FRQ / MOTOR_BASE_FRQ) // tcu 定时器中断计数

#define MOTOR_MAX_FRQ 480 // 马达脉冲最高频率，需要低于空载牵出频率
#define MOTOR_MIN_FRQ 10  // 马达脉冲最低频率

#define MOTOR_MAX_SPEED 63 // 速度范围 1~MOTOR_MAX_SPEED
#define TIK_PRE_SEC (MOTOR_BASE_FRQ) // 一秒钟的中断数，等于基准频率
/*
* speed 范围 1~MOTOR_MAX_SPEED
* 该公式是将 MOTOR_MIN_FRQ~MOTOR_MAX_FRQ 形成的等差数列，映射到 tik 数列，使得 tik 对应的频率(MOTOR_BASE_FRQ/tik) 是一个等差数列
* 由于频率粒度的原因，该公式低频下基本均匀，高频下会跳变且多个 tik 对应一个值
*/
#define CALC_TIK(speed) (MOTOR_BASE_FRQ / (MOTOR_MIN_FRQ + (speed - 1) * (MOTOR_MAX_FRQ - MOTOR_MIN_FRQ) / (MOTOR_MAX_SPEED - 1)))

/*每步的加减速值，1~MOTOR_MAX_SPEED，如果不需要加减速，直接把这个宏调成最大速度*/
#define ACC_SPEED 5
#define BRAKE_SPEED (10)  // 刹车速度

/*******************************************************************************/
/* motor io 相关                                                                         									      */
/*******************************************************************************/

static void motor_gpio_step(int motor_no, uint32_t cur_steps)
{
    // 马达步进，按 4 相八拍计算
    step_motor(motor_no, cur_steps);
    return ;
}

static void motor_idle(void)
{
    motor_stop();
}

static int motor_gpio_init(MotorGpio *motor_gpio, int motor_no)
{
    char lable[32] = {0};
    int ret = 0;

    if (motor_gpio == NULL) {
        return false;
    }

    snprintf(lable, sizeof(lable), "motor%d_gpio", motor_no);
    do {
        // gpio1
        if (motor_gpio->gpio1 == 0) {
            ret = -1;
            break;
        }
        ret = gpio_request(motor_gpio->gpio1, lable);
        if (ret < 0) {
            break;
        }
        gpio_direction_output(motor_gpio->gpio1, 0);
        printk("motor%d_gpio1 init success\n", motor_no);
        // gpio2
        if (motor_gpio->gpio2 == 0) {
            ret = -1;
            break;
        }
        ret = gpio_request(motor_gpio->gpio2, lable);
        if (ret < 0) {
            break;
        }
        gpio_direction_output(motor_gpio->gpio2, 0);
        printk("motor%d_gpio2 init success\n", motor_no);
        // gpio3
        if (motor_gpio->gpio3 == 0) {
            ret = -1;
            break;
        }
        ret = gpio_request(motor_gpio->gpio3, lable);
        if (ret < 0) {
            break;
        }
        gpio_direction_output(motor_gpio->gpio3, 0);
        printk("motor%d_gpio3 init success\n", motor_no);
        // gpio4
        if (motor_gpio->gpio4 == 0) {
            ret = -1;
            break;
        }
        ret = gpio_request(motor_gpio->gpio4, lable);
        if (ret < 0) {
            break;
        }
        gpio_direction_output(motor_gpio->gpio4, 0);
        printk("motor%d_gpio4 init success\n", motor_no);
    } while (0);

    return ret;
}
/*******************************************************************************/
/* 马达运动相关                                                                            									      */
/*******************************************************************************/
/*处理马达旋转函数，返回 true:正在运动  false:停止*/
bool handle_motor_rotation(MotorInfo *motor_info, int motor_no)
{
    MotorRun *motor_run = &motor_info->run;
    int max_steps = motor_run->max_steps;
    int cur_dir = motor_run->cur_dir;
    int cur_step = motor_run->cur_step;
    int cur_speed = motor_run->cur_speed;
    
    int dst_speed = motor_run->dst_speed;
    int dst_run_steps = 0;
    int dst_dir = 0;
    if (motor_run->cur_step == motor_run->dst_steps && motor_run->cur_dir == DIRECTION_STOP) {
        return false;
    }

    if (--motor_run->tiks > 0) {
        return true;
    }

    /*运算的最终目标是计算下一步的运动方向和速度值*/
    dst_run_steps = motor_run->dst_steps - cur_step;
    dst_dir = (dst_run_steps >> 31) | 1; // 提取符号位 正数和 0 是 1，负数是 -1
    if (motor_run->cur_step == motor_run->dst_steps && cur_speed <= BRAKE_SPEED) {
        /*满足刹车条件*/
        motor_run->cur_dir = DIRECTION_STOP;
    } else if (cur_speed != 0 && cur_dir != dst_dir) {
        /*反向运动*/
        if (cur_speed <= BRAKE_SPEED || cur_step <= 0 || cur_step >= max_steps) {
            /*达到边界或者刹车条件，速度归零*/
            cur_speed = 0;
        } else {
            /*其它情况，减速，维持步进*/
            cur_speed -= ACC_SPEED;
            if (cur_speed < 0) {
                cur_speed = 0;
            }
        }
    } else {
        /*单向运动，优先保证减速距离，加速次优先*/
        int dec_steps = cur_speed / ACC_SPEED * cur_dir;
        if (cur_speed > 0 && (cur_step + dec_steps <= 0 || cur_step + dec_steps >= max_steps ||
                                            (dst_run_steps * dst_dir - dec_steps * cur_dir) <= 0)) {
            /*速度不为 0 的情况下，如果减速会碰边，或者接近目标位置，立刻开始减速*/
            cur_speed -= ACC_SPEED;
            if (cur_speed < BRAKE_SPEED) {
                cur_speed = BRAKE_SPEED; // 保证最低以刹车速度运行
            }
        } else {
            /*非制动情况，向目标速度靠近，边界条件由减速判断，这里不用管*/
            if (cur_speed > dst_speed) {
                cur_speed -= ACC_SPEED;
                if (cur_speed < BRAKE_SPEED) {
                    cur_speed = BRAKE_SPEED;
                }
            } else if (cur_speed < dst_speed) {
                cur_speed += ACC_SPEED;
                if (cur_speed > dst_speed) {
                    cur_speed = dst_speed;
                }
            }
        }
        motor_run->cur_dir = dst_dir;
    }

    if (motor_run->cur_dir == DIRECTION_STOP) {
        motor_run->cur_speed = 0;
        motor_run->tiks = 0;
        return false;
    }
    
    motor_run->cur_speed = cur_speed;
    if(cur_speed) {
        motor_run->tiks = CALC_TIK(motor_run->cur_speed);
    } else {
        motor_run->tiks = MOTOR_BASE_FRQ / MOTOR_MIN_FRQ;
    }
    
    motor_run->cur_step += motor_run->cur_dir;
    motor_gpio_step(motor_no, motor_run->cur_step);
#if DEBUG
    printk("motor tk:%d cs:%d ct:%d ds:%d dt:%d\n", motor_run->tiks, motor_run->cur_speed, motor_run->cur_step, motor_run->dst_speed, motor_run->dst_steps);
#endif
    return true;
}

static int handle_motor_init(MotorLoopInfo *loop_info)
{
    MotorInfo *motor_info = &loop_info->motor[loop_info->init_motor_no];
    int dst_steps = 0, dir = 0;
    int ret = 0;
    /*初始化模式，根据马达编号进行初始化*/
    if (loop_info->init_motor_no < MAX_MOTOR_NUM) {
        /*目前初始化是转半圈，转一圈，再转半圈，每个马达六个阶段*/
        switch (loop_info->init_stage) {
        case 0:
            /*头半圈初始位置为最大步数的一半，再根据目标方向计算目标位置*/
            motor_info->run.max_steps = motor_info->param.max_steps + 2 * motor_info->param.edge_steps; 
            dst_steps = motor_info->run.max_steps / 2;
            dir = motor_info->param.init_dir;
            motor_info->run.cur_step = dst_steps;
            motor_info->run.dst_steps = motor_info->run.cur_step + dst_steps * dir;
            motor_info->run.dst_speed = MOTOR_MAX_SPEED;

            loop_info->init_stage++;
            break;
        case 2:
            /*第二阶段往回转一圈*/
            dir = motor_info->param.init_dir * -1;
            motor_info->run.dst_steps += motor_info->run.max_steps * dir;
            loop_info->init_stage++;
            break;
        case 4:
            /*第三阶段，回中心位置*/
            dir = motor_info->param.init_dir;
            motor_info->run.dst_steps += motor_info->run.max_steps / 2 * dir;
            loop_info->init_stage++;
            break;
        case 6:
            /*最后阶段，初始化结束，填入运行参数，切换下一个马达*/
            motor_info->run.max_steps = motor_info->param.max_steps;
            motor_info->run.cur_step = motor_info->param.max_steps / 2;
            motor_info->run.dst_steps = motor_info->run.cur_step;
            loop_info->init_stage = 0;
            loop_info->init_motor_no++;
            break;
        case 1:
        case 3:
        case 5:
            /*1 3 5 都是向目标运动*/
            ret = handle_motor_rotation(motor_info, loop_info->init_motor_no);
            if (ret == false) {
                loop_info->init_stage++;
            }
            break;
        }
    } else {
        return 0;
    }

    return 1;
}

static void motor_timer_cb(void *dev_id)
{
    static int idle_tik = 0;
    static int ircut_tik = 0;
    /*中断频率很高，尽量不要进行复杂的运算，否则会导致 cpu 占用过多*/
    MotorDevInfo *motor_dev = (MotorDevInfo *)dev_id; 
	MotorLoopInfo *loop_info = &motor_dev->motor_loop;
    int is_run = 0;
    int i = 0;

    if (loop_info->work_mode == WORK_MODE_MOVE) {
        /*正常工作模式，转动马达，最终 is_run 都为 0 表示所有马达都停止了*/
        for (i = 0; i < MAX_MOTOR_NUM; ++i) {
            is_run |= handle_motor_rotation(&loop_info->motor[i], i);
            if (loop_info->is_single_motor_rotation && is_run) {
                break;  // 单电机旋转模式，按电机遍历顺序，只有前面的电机停止了，后面的电机才能动
            }
        }
    } else if (loop_info->work_mode == WORK_MODE_INIT) {
        is_run = handle_motor_init(loop_info);
        if (is_run == DIRECTION_STOP) {
            loop_info->work_mode = WORK_MODE_MOVE;
        }
    } else {
       is_run = false; 
    }

    switch (loop_info->ircut_mode) { // ircut 模式
    case E_IRCUT_START:
        is_run = true;
        ircut_send_data();
        loop_info->ircut_mode = E_IRCUT_WAIT;
        break;
    case E_IRCUT_WAIT:
        is_run = true;
        if (++ircut_tik >= TIK_PRE_SEC / 10) { // 100 ms 切回 ircut
            ircut_tik = 0;
            ir_cut(0);
            ircut_send_data();
            loop_info->ircut_mode = E_IRCUT_STOP;
        }
        break;
    case E_IRCUT_STOP:
        break;
    default:
        break;
    }

    if (!is_run) { 
        if (++idle_tik > 1 * TIK_PRE_SEC) { // 1s 没有运动停止定时器和马达
#if DEBUG
            printk("motor_idle , stop tcu counter\n");
#endif
            for (i = 0; i < MAX_MOTOR_NUM; ++i) {
                motor_idle();
            }
            Stop_timer(motor_dev->timer_hanlde);
        }
    } else {
        idle_tik = 0;
    }
}
/*******************************************************************************/
/* /dev/motor 相关                                                                            									      */
/*******************************************************************************/
static int motor_open(struct inode *inode, struct file *file)	{return 0;}
static int motor_release(struct inode *inode, struct file *file){return 0;}
static long motor_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct miscdevice *dev = filp->private_data;
	MotorDevInfo *motor_dev = container_of(dev, MotorDevInfo, mdev);
    MotorLoopInfo *motor_loop = &motor_dev->motor_loop;
    MotorIoctl motor_ioctl = {0};
    int  i = 0;
    int dst_steps = 0;
#if DEBUG
    printk("ioctl cmd:%d\n", cmd);
#endif
    switch (cmd) {
    case MOTOR_IOCTL_INIT:
        motor_loop->work_mode = WORK_MODE_INIT;
        motor_loop->init_motor_no = 0;
        motor_loop->init_stage = 0;
        Start_timer(motor_dev->timer_hanlde);
        break;
    case MOTOR_IOCTL_SET_PARAM:
        if (copy_from_user(&motor_ioctl, (void __user *)arg, sizeof(MotorIoctl))) {
            dev_err(motor_dev->dev, "[%s][%d] copy from user error\n",__func__, __LINE__);
            return -EFAULT;
        }

        for (i = 0; i < MAX_MOTOR_NUM; ++i) {
            motor_loop->motor[i].param.edge_steps = motor_ioctl.init_param.edge_steps[i];
            motor_loop->motor[i].param.max_steps = motor_ioctl.init_param.max_steps[i];
            motor_loop->motor[i].param.init_dir = motor_ioctl.init_param.init_dir[i];
            if (motor_loop->motor[i].param.max_steps < MOTOR_MIN_STEPS) {
                motor_loop->motor[i].param.max_steps = MOTOR_MIN_STEPS;
            }

            if (motor_loop->motor[i].run.max_steps > motor_ioctl.init_param.max_steps[i]) {
                // 如果最大步数小改大，设置当前步数为最大步数的一半，解决最大步数改小之后当前步数越界问题
                motor_loop->motor[i].run.cur_step = motor_ioctl.init_param.max_steps[i] / 2;
                motor_loop->motor[i].run.dst_steps = motor_loop->motor[i].run.cur_step;
            }
            motor_loop->motor[i].run.max_steps = motor_ioctl.init_param.max_steps[i]; // 刷新最大步数，解决最大步数小改大，会导致云台运行缓慢问题
        }

        motor_loop->is_single_motor_rotation = motor_ioctl.init_param.single_motor_rotation;
        break;			
    case MOTOR_IOCTL_GET_PARAM:
        for (i = 0; i < MAX_MOTOR_NUM; ++i) {
            motor_ioctl.init_param.edge_steps[i] = motor_loop->motor[i].param.edge_steps;
            motor_ioctl.init_param.max_steps[i] = motor_loop->motor[i].param.max_steps;
            motor_ioctl.init_param.init_dir[i] = motor_loop->motor[i].param.init_dir;
        }
        motor_ioctl.init_param.single_motor_rotation = motor_loop->is_single_motor_rotation;
        if (copy_to_user((void __user *)arg, &motor_ioctl,  sizeof(MotorIoctl))) {
            dev_err(motor_dev->dev, "[%s][%d] copy to user error\n", __func__, __LINE__);
            return -EFAULT;
		}
        break;
    case MOTOR_IOCTL_MOVE:
        /*马达的 dst_speed 和 dst_steps 只在 ioctl 里面修改，不需要加锁和停定时器*/
        if (copy_from_user(&motor_ioctl, (void __user *)arg, sizeof(MotorIoctl))) {
            dev_err(motor_dev->dev, "[%s][%d] copy from user error\n",__func__, __LINE__);
            return -EFAULT;
        }
        
        if (motor_loop->work_mode != WORK_MODE_MOVE) {
            return -EFAULT;
        }

        for (i = 0; i < MAX_MOTOR_NUM; ++i) {
            if (motor_ioctl.move.speed[i] < 1)  
                motor_ioctl.move.speed[i] = 1;
            if (motor_ioctl.move.speed[i] > MOTOR_MAX_SPEED)
                motor_ioctl.move.speed[i] = MOTOR_MAX_SPEED;

            motor_loop->motor[i].run.dst_speed = motor_ioctl.move.speed[i];

            if (motor_ioctl.move.dir[i] == DIRECTION_DOWN) {
                /*down 方向移动是调用坐标为 0 的预置位*/
                motor_loop->motor[i].run.dst_steps = 0;
            } else if (motor_ioctl.move.dir[i] == DIRECTION_STOP) {
                /*停止计算目标减速后的位置，作为目标位置*/
                dst_steps = motor_loop->motor[i].run.cur_step + 
                                motor_loop->motor[i].run.cur_speed / ACC_SPEED * motor_loop->motor[i].run.cur_dir;
                if (dst_steps < 0) {
                    dst_steps = 0;
                } else if (dst_steps > motor_loop->motor[i].param.max_steps) {
                    dst_steps = motor_loop->motor[i].param.max_steps;
                }
                motor_loop->motor[i].run.dst_steps = dst_steps;
                motor_loop->motor[i].run.dst_speed = MOTOR_MAX_SPEED;
            } else if (motor_ioctl.move.dir[i] == DIRECTION_UP) {
                /*up 方向移动是调用坐标为 max 的预置位*/
                motor_loop->motor[i].run.dst_steps = motor_loop->motor[i].param.max_steps;
                
            }
        }
        Start_timer(motor_dev->timer_hanlde);
        break;
    case MOTOR_IOCTL_CALL_PRESET:
        if (copy_from_user(&motor_ioctl, (void __user *)arg, sizeof(MotorIoctl))) {
            dev_err(motor_dev->dev, "[%s][%d] copy from user error\n",__func__, __LINE__);
            return -EFAULT;
        }
        
        if (motor_loop->work_mode != WORK_MODE_MOVE) {
            return -EFAULT;
        }

        for (i = 0; i < MAX_MOTOR_NUM; ++i) {
            if (motor_ioctl.preset.speed[i] < 1)  
                motor_ioctl.preset.speed[i] = 1;
            if (motor_ioctl.preset.speed[i] > MOTOR_MAX_SPEED)
                motor_ioctl.preset.speed[i] = MOTOR_MAX_SPEED;
            motor_loop->motor[i].run.dst_speed = motor_ioctl.preset.speed[i];

            if (motor_ioctl.preset.steps[i] == MOTOR_CUR_STEP) {
                motor_ioctl.preset.steps[i] = motor_loop->motor[i].run.cur_step;
            } else if (motor_ioctl.preset.steps[i] < 0) {
                motor_ioctl.preset.steps[i] = 0;
            } else if (motor_ioctl.preset.steps[i] > motor_loop->motor[i].param.max_steps) {
                motor_ioctl.preset.steps[i] = motor_loop->motor[i].param.max_steps;
            }
            motor_loop->motor[i].run.dst_steps = motor_ioctl.preset.steps[i];
        }
        Start_timer(motor_dev->timer_hanlde);
        break;
    case MOTOR_IOCTL_GET_STATUS:
        for (i = 0; i < MAX_MOTOR_NUM; ++i) {
            motor_ioctl.status.dir[i] = motor_loop->motor[i].run.cur_dir;
            motor_ioctl.status.steps[i] = motor_loop->motor[i].run.cur_step;
            motor_ioctl.status.dst_steps[i] = motor_loop->motor[i].run.dst_steps;
        }
        if (copy_to_user((void __user *)arg, &motor_ioctl,  sizeof(MotorIoctl))) {
            dev_err(motor_dev->dev, "[%s][%d] copy to user error\n", __func__, __LINE__);
            return -EFAULT;
		}
        break;
    case MOTOR_IOCTL_SET_ZERO:
        /*测试马达最大步数时使用，非线程安全，请确保马达停下再调用*/
        for (i = 0; i < MAX_MOTOR_NUM; ++i) {
            motor_loop->motor[i].run.cur_step = 0;
            motor_loop->motor[i].run.max_steps = motor_loop->motor[i].param.max_steps;
        }
        break;
    case IRCUT_IOCTL_N:
    case IRCUT_IOCTL_P:
        ir_cut(cmd);
        motor_loop->ircut_mode = E_IRCUT_START;
        Start_timer(motor_dev->timer_hanlde);
        break;
    default:break;
	}
	return 0;
}

static void motor_loop_init(MotorDevInfo *motor_dev)
{
    MotorInfo *motor_info;
    int i = 0;

#if MAX_MOTOR_NUM != 2  // 修改了马达数量就要修改初始化 IO
    #error "MAX_MOTOR_NUM is change, please modify gpio init"
#endif

    init_xtm5809();
    
    /*马达参数初始化，和填入默认值*/
    for (i = 0; i < MAX_MOTOR_NUM; ++i) {
        motor_info = &motor_dev->motor_loop.motor[i];

        motor_info->param.edge_steps = 20;
        motor_info->param.max_steps = MOTOR_MIN_STEPS;
        motor_info->param.init_dir = DIRECTION_UP;
    }
}

static struct file_operations g_motor_fops = 
{
	.open = motor_open,
	.release = motor_release,
	.unlocked_ioctl = motor_ioctl,
};

static int motor_probe(struct platform_device *pdev)
{
    int ret = 0;
    MotorDevInfo *motor_dev;
    /*申请*/
    motor_dev = devm_kzalloc(&pdev->dev, sizeof(MotorDevInfo), GFP_KERNEL);
	if (!motor_dev) {
		ret = -ENOENT;
		dev_err(&pdev->dev, "kzalloc motor_info memery error\n");
		goto error_devm_kzalloc;
	}

	motor_dev->dev  		  = &pdev->dev;
    platform_set_drvdata(pdev, motor_dev);

	if ((motor_dev->timer_hanlde = timer_open(0)) == NULL) 
	{
		ret = -ENOENT;
		dev_err(&pdev->dev, "timer open failed\n");
		goto error_kthread_run;
	} else {
		timer_set(motor_dev->timer_hanlde, TCU_PERIOD, &motor_timer_cb, motor_dev);
        Set_period(motor_dev->timer_hanlde, TCU_PERIOD);
		Stop_timer(motor_dev->timer_hanlde);
    }

    motor_dev->mdev.minor 	= MISC_DYNAMIC_MINOR;
	motor_dev->mdev.name 	= "motor";
	motor_dev->mdev.fops 	= &g_motor_fops;
	ret = misc_register(&motor_dev->mdev);
	if (ret < 0) {
		ret = -ENOENT;
		dev_err(&pdev->dev, "misc_register failed\n");
		goto error_misc_register;
	}

    motor_loop_init(motor_dev);

    printk("Motor Probe is OK , Acc_speed:%d\n", ACC_SPEED);
	return 0;

error_misc_register:
error_kthread_run:
error_devm_kzalloc:
	return ret;
}

static int motor_remove(struct platform_device *pdev)
{
    int i = 0;
    MotorDevInfo *motor_dev = platform_get_drvdata(pdev);

    misc_deregister(&motor_dev->mdev);

    return 0;
}

static const struct of_device_id motor_of_id_table[] = {
    { .compatible = "motor" },
    {}
};

static struct platform_driver g_motor_driver = {
	.probe = motor_probe,
	.remove = motor_remove,
	.driver = {
		.name	= "motor",
        .of_match_table = motor_of_id_table,
	}
};
/*******************************************************************************/
/* /sys/motor/step 相关                                                                            									      */
/*******************************************************************************/
struct sys_attribute 
{
	struct attribute attr;
	ssize_t (*show)(struct kobject *kobj, char *buf);
	ssize_t (*store)(struct kobject *kobj, const char *buffer, size_t size);
};

ssize_t sys_default_attr_show(struct kobject *kobj, struct attribute *attr, char *buf)
{
	struct sys_attribute *sys_attr = container_of(attr, struct sys_attribute, attr);
	int result = 0;
	
	if (sys_attr->show)	result = sys_attr->show(kobj, buf);
	return result;
}

ssize_t sys_default_attr_store(struct kobject *kobj, struct attribute *attr, const char *buffer, size_t size)
{
	struct sys_attribute *sys_attr = container_of(attr, struct sys_attribute, attr);
	int result = 0;
	
	if (sys_attr->store) result = sys_attr->store(kobj, buffer, size);
	return result;
}

static struct kobject g_kobject;
struct sysfs_ops g_sysfs_ops = {
	.show	= sys_default_attr_show,
	.store	= sys_default_attr_store,
};

struct kobj_type g_sys_ktype = {
	.sysfs_ops	= &g_sysfs_ops,
};

ssize_t step_attr_show(struct kobject *kobject, char *buf)
{
    printk("Usage: no+dir\n"
           "dir :\n"
           "0 STOP"
           "1 Up\n"
           "2 Down\n"
           "ex : \n"
           "11 # motor 1 up"
           "22 # motor 2 down"
           "10 # motor 1 stop"
           );
    return 0;
}

static long step_ioctl(char cmd)
{ 
#if 0 // 调试功能暂不实现
    int motor_no = cmd[0] - '0';
    int motor_dir = cmd[1] - '0';

    if (motor_no < 0 || motor_no >= MAX_MOTOR_NUM) {
        return step_attr_show(NULL, NULL);
    }

    if (motor_dir == 0) {
        g_motor_loop->motor[i].run.dst_steps = 
    }
#endif
    static uint32_t motor0_step = 5000;
    static uint32_t motor1_step = 5000;

    switch (cmd) {
    case '1':
        step_motor(0, motor0_step++);
        printk("step_motor1\n");
        break;
    case '2':
        step_motor(0, motor0_step--);
        printk("step_motor2\n");
        break;
    case '3':

        step_motor(1, motor1_step++);
        printk("step_motor3\n");
        break;
    case '4':

        step_motor(1, motor1_step--);
        printk("step_motor4\n");
        break;
    case '5':

        ir_cut(1);
        printk("step_motor5\n");
        break;
    case '6':

        ir_cut(2);
        printk("step_motor6\n");
        break;
    }
    
	return 0;
}

ssize_t step_attr_store(struct kobject *g_kobject, const char *buffer, size_t size)
{
    if (size != 2) {
        return step_attr_show(NULL, NULL);
    }

    printk("hello cmd: %c\n", buffer[0]);

    step_ioctl(buffer[0]);
    return size;
}

struct sys_attribute g_step_attr = {
	.attr = {
		.name = "step",
		.mode = S_IRUGO | S_IWUSR,
	},
    .show = step_attr_show,
    .store = step_attr_store,
	
};
/*******************************************************************************/
/* 驱动模块相关                                                                            									      */
/*******************************************************************************/
static int __init motor_init(void)
{
    int ret = 0;
	ret = platform_driver_register(&g_motor_driver);
    if (ret < 0)
    {
        printk("Cannot register device platform driver (%d).\n",ret);
        return -1;
    }
    
    g_kobject.ktype = &g_sys_ktype;
    ret = kobject_init_and_add(&g_kobject, &g_sys_ktype, NULL, "motor");
	if (ret < 0) {
		printk("Cannot register device kobject g_kobject.\n");
		return -1;
	}
    
    ret = sysfs_create_file(&g_kobject, &g_step_attr.attr);
	if (ret < 0) {
		printk("Cannot create device step_attr.attr.\n");
		return -1;
    }
    
    return ret;
}

static void __exit motor_exit(void)
{
	platform_driver_unregister(&g_motor_driver);
    sysfs_remove_file(&g_kobject, &(g_step_attr.attr));
    kobject_put(&g_kobject);

}

module_init(motor_init);
module_exit(motor_exit);

MODULE_LICENSE("GPL");
