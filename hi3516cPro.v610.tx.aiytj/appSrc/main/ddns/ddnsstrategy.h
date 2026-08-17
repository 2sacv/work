/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : ddnsstrategy.h
 * @Created Time : 2014-04-03
 * @Version      : 1.0
 * @Author       : zengy
 * @Description  :
 */

#ifndef __DDNS_STRATEGY_H_
#define __DDNS_STRATEGY_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "js_scheduler.h"

    int init_client_ddns(JSScheduler sch);

    int uninit_client_ddns();

    void ddns_cfg_changed_process();

    int ddns_connect_status();

#ifdef __cplusplus
}
#endif
#endif

