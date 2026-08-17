/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : sim4g_adapter.c
 * @Created Time : 2023-3-2
 * @Version      : 3.0
 * @Author       : hul
 * @Description  :
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "debug.h"
#include "utils.h"
#include "jevent.h"
#include "confapi.h"
#include "jconfig.h"
#include "net_check.h"
#include "system_ctrl.h"
#include "url.h"

#include "sim4g_adapter.h"
/*
 * sim4g_adapt.c:差异点接口文件
 * 目的:使4G移植简单,封装性更完善
 * 使用方法:本文件适用于4G代码在各分支差异点
 * 比如:send_alarm()和alarm_send(),system_is_security()和system_get_security(),不同平台
 **/

/*
 * test_tcp_connect(xxx, 80, 5) 在弱网时，长达 180S，暂停使用
 * ping 失败后，下次使用时，会换一次服务器
 **/

/* 兼容不同平台ping操作 */
int sim4g_www_reachable(void)
{
    return www_reachable();
}

/* 兼容加密方式 */
int sim4g_get_security(void)
{
    int ret = SUCCESS;

    ret = system_get_security();

    return ret;
}

/* 兼容各类发送事件接口 */
int sim4g_send_event(JSEventType event_id)
{
    if (get_g_run(sim4g, RUN_RESTART_TX)) {
        send_event(event_id);
        clr_g_run(sim4g, RUN_RESTART_TX);
        return SUCCESS;
    }

    return FAILURE;
}

