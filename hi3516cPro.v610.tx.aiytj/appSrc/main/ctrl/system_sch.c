/**
 * Copyright (C) by Jabsco Company
 * 
 * @File Name    : system_sch.c
 * @Created Time : 2020-8-20
 * @Version      : 1.0
 * @Author       : zhangxj zhangj
 * @Description  : Check the thread is interlocked
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "debug.h"
#include "system_sch.h"

#define MAX_SCHEDULER_NUM   100

static JSScheduler g_sch_test = NULL;
static JSTCHandle  g_hdl_user  = NULL;
static JSTCHandle  g_hdl_block = NULL;

/*
 * to enable __TEST__ thread, do:
 * ccli jsdbg go
 */
static void cb_user(void *data)
{
    DBG("JS_DBG = test  OK\n");
}

static void cb_block(void *data)
{
    sleep(50);
}

int start_test_sch(void)
{
    g_sch_test = js_create_scheduler("jsdbg_test");

    js_create_timer_r(g_sch_test, 2*1000, 2*1000, cb_user, NULL, &g_hdl_user); //todo close ?
    js_create_timer_r(g_sch_test, 5*1000, 2*1000, cb_block, NULL, &g_hdl_block); //todo close ?

    return 0;
}
