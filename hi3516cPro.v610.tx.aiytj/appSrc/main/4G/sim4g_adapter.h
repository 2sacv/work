/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : sim4g_adapter.h
 * @Created Time : 2023-03-2
 * @Version      : 3.0
 * @Author       : hul
 * @Description  :
 */
#ifndef __SIM4G_ADAPTER_H_
#define __SIM4G_ADAPTER_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "net_check.h"

int sim4g_www_reachable(void);

int sim4g_get_security(void);

int sim4g_send_event(JSEventType event_id);

#ifdef __cplusplus
}
#endif
#endif

