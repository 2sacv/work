/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : sim4g_yuga.h
 * @Created Time : 2023-01-31
 * @Version      : 3.0
 * @Author       : hul
 * @Description  :
 */
#ifndef __SIM4G_YUGA_H_
#define __SIM4G_YUGA_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sim4g.h"

int sim4g_ygx09_init(void *data);

int sim4g_ygx09_flyreset(void);

int sim4g_ygx09_pwr_reset(void);

int sim4g_ygx09_location(Sim4g *info);

#ifdef __cplusplus
}
#endif
#endif

