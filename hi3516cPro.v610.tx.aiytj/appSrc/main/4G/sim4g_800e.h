/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : sim4g_800e.h
 * @Created Time : 2023-01-31
 * @Version      : 3.0
 * @Author       : hul
 * @Description  :
 */
#ifndef __SIM4G_800E_H_
#define __SIM4G_800E_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sim4g.h"

int sim4g_800e_init(void *data);

int sim4g_800e_iccid_imei_imsi(void);

int sim4g_800e_flyreset(void);

int sim4g_800e_pwr_reset(void);

int sim4g_800e_location(Sim4g *info);

#ifdef __cplusplus
}
#endif
#endif

