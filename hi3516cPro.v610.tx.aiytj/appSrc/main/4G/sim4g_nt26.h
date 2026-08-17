/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : sim4g_nt26.h
 * @Created Time : 2023-01-31
 * @Version      : 3.0
 * @Author       : hul
 * @Description  :
 */
#ifndef __SIM4G_NT26_H_
#define __SIM4G_NT26_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sim4g.h"

int sim4g_nt26_init(void *data);

int sim4g_nt26_flyreset(void);

int sim4g_nt26_pwr_reset(void);

#ifdef __cplusplus
}
#endif
#endif

