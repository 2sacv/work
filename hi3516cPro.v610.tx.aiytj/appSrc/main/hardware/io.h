/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    : io.h
 * Created Time : 2023-2-27
 * Version      : 1.0
 * Author       : hul
 * Description  :
 */


#ifndef __IO_H__
#define __IO_H__

#ifdef __cplusplus
extern "C" {
#endif


#define IO_EXIST 	  1
#define IO_NOT_EXIST  0

typedef enum {
	E_IO_OUTSIDE_SIM,
	E_IO_INSIDE_SIM,
	E_IO_4G,
	E_IO_WIFI,
	E_IO_ESIM,
} eIOName;

typedef enum {
	E_GPIO_SIM  = 32,
	E_GPIO_4G   = 57,
	E_GPIO_WIFI = 57,
	E_GPIO_ESIM = 32,
	E_GPIO_SD_PWR = 28,
	E_GPIO_SD_CD  = 29,
	E_GPIO_SD_CMD = 33,
	E_GPIO_SD_D0  = 31,
	E_GPIO_SD_D1  = 30,
	E_GPIO_SD_D2  = 35,
	E_GPIO_SD_D3  = 34
}eGPIONum;

typedef enum {
	E_ACT_BAD = -1,
	E_ACT_ON  = 2,
	E_ACT_OFF = 3,
} eAction;

typedef struct {
	eIOName  name;
	int   num;
	int   on;
	int   off;
	int   exist;
	char  dir[4];
}sPINInfo;

int pin_exist(eIOName name);

eAction pin_read(eIOName name);

int pin_write(eIOName name, eAction act);

#ifdef __cplusplus
}
#endif

#endif //__GPIO_INGENIC_IPC_H__
