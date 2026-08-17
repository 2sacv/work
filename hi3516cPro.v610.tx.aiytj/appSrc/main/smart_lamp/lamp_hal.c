/* 
 *       Filename:  lamp_hal.c
 *    Description:  
 *        Version:  1.0
 *        Created:  11/07/2025 11:38:48 AM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  wangr (), 
 *   Organization:  
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/* file */
#include <fcntl.h>
#include <sys/file.h>

/* socket() */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "debug.h"
#include "utils.h"
#include "lamp_hal.h"
#include "ptz_ctrl.h"

int set_lamp_status(int gpio, int value)
{
    if (gpio != GPIO_INFRARED && gpio != GPIO_WHITE && gpio != GPIO_RED_BLUE) {
        ERR("gpio %d is invalid\n", gpio);
        return FAILURE;
    }

    if (0 != value && 1 != value) {
        ERR("value %d is invalid\n", value);
        return FAILURE;
    }

    return gpio_open_set_value(gpio, value);
}

/**
 * @param[in]   is_day 是否切换到白天

 * @retval  0 成功
 * @retval  非0 失败

 * @attention 白天红外滤光片, 晚上全透滤光片
 */
int set_ircut_status(int is_day)
{
    return_val_if_fail(TRUE == is_day || FALSE == is_day, FAILURE);

    int ret = 0;
    static int is_day_prev = -1;

    do {
        if (is_day_prev == is_day) {
            break;
        }

        if (TRUE == is_day) {
            DBG("ircut day mono_usec: %lld\n", mono_usec());
            switch_ircut(20);
        } else {
            DBG("ircut night mono_usec: %lld\n", mono_usec());
            switch_ircut(21);
        }

        ms_sleep(200);
        is_day_prev = is_day;
    } while(0);

    return ret;
}

int lamp_gpio_init(void)
{
    DBG("lamp_gpio_init\n");
    int ret = 0;

    //白光灯
#ifndef STEPLESS_PWM
    ret += gpio_open_export(GPIO_WHITE);
    ret += gpio_open_set_direction(GPIO_WHITE, "out");
    ret += gpio_open_set_value(GPIO_WHITE, GPIO_WHITE_OFF);
#endif

    //红外灯
    ret += gpio_open_export(GPIO_INFRARED);
    ret += gpio_open_set_direction(GPIO_INFRARED, "out");
    ret += gpio_open_set_value(GPIO_INFRARED, GPIO_INFRARED_OFF);

    //红蓝报警灯
#ifdef LIGHT_IO_ALARM
    ret += gpio_open_export(GPIO_RED_BLUE);
    ret += gpio_open_set_direction(GPIO_RED_BLUE, "out");
    ret += gpio_open_set_value(GPIO_RED_BLUE, GPIO_RED_BLUE_OFF);
#endif

    return ret;
}

