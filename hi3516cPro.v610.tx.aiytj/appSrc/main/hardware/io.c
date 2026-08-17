/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    : io.c
 * Created Time : 2023-2-27
 * Version      : 1.0
 * Author       : hul
 * Description  :
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "debug.h"
#include "io.h"
#include "gpio.h"

/*
 * io.c:IO抽象层
 * 目的:上层应用使用gpio口时无需关心硬件层IO定义
 * 使用方法:不同产品做IO区分,无需关注内部代码实现
 * 比如:
 	#if defined(Y231_4G)
	{E_IO_OUTSIDE_SIM, 	E_GPIO_SIM, 	1,	0, 	NOT_EXIST,	"out"},
	#else
	{E_IO_OUTSIDE_SIM, 	E_GPIO_SIM,		1,	0, 	EXIST,		"out"},
	#endif
 **/

static sPINInfo pin_list[] = {
	{E_IO_OUTSIDE_SIM, 	E_GPIO_SIM,		0,	1, 	IO_EXIST,		"out"},
	{E_IO_INSIDE_SIM, 	E_GPIO_SIM,		1,	0,	IO_EXIST,		"out"},
	{E_IO_4G, 		 	E_GPIO_4G,		1,	0,	IO_EXIST,		"out"},
	{E_IO_WIFI, 		E_GPIO_WIFI,	1,	0, 	IO_EXIST,		"in"},
	{E_IO_ESIM,		 	E_GPIO_ESIM,	1,	0,	IO_EXIST,		"in"},
};

// return: TRUE when exist or FALSE
int pin_exist(eIOName name)
{
	int i = 0;

    for (i = 0; i < sizeof(pin_list)/sizeof(sPINInfo); i++) {
        if (pin_list[i].name == name) {
			return pin_list[i].exist;
		}
    }

    return FALSE;
}

// return: ON or OFF
eAction pin_read(eIOName name)
{
	int i = 0;
	int val = 0;

    for (i = 0; i < sizeof(pin_list)/sizeof(sPINInfo); i++) {
        if (name == pin_list[i].name) {
			gpio_get_value(pin_list[i].num, &val);
        	return (val == pin_list[i].on) ? E_ACT_ON : E_ACT_OFF;
		}
    }

    return E_ACT_BAD;
}

// return: SUCCESS FAILURE
int pin_write(eIOName name, eAction act)
{
	int i = 0;
	
	if (name == E_IO_4G) { 
		DBG("---------------------------\n");
		DBG("act;%d\n", act);
		DBG("---------------------------\n");
	}
	for (i = 0; i < sizeof(pin_list)/sizeof(sPINInfo); i++) {
        if (name == pin_list[i].name) {
			if (act == E_ACT_ON){
				gpio_set_value(pin_list[i].num, pin_list[i].on);
			} else if (act == E_ACT_OFF) {
				gpio_set_value(pin_list[i].num, pin_list[i].off);
			}else {
				SYSLOG("[act=%d] is illegal\n", act);
				return FAILURE;
			}
		}
    }

	return SUCCESS;
}


