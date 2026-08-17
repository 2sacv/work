#if 0
/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    : hm_api.cpp
 * Created Time : 2012-10-15
 * Version      : 1.0
 * Author       : luoshunfa
 * Description  :
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include "debug.h"
#include "system_ctrl.h"
#include "BasicUsageEnvironment.hh"

#include "hm_service.h"
#include "hm_api.h"

JHMService *sHmService = NULL;

int init_hm_service(void *data)
{
    if(sHmService == NULL) {
        TaskScheduler* theScheduler = (TaskScheduler *)data;
        sHmService = new JHMService(theScheduler);
        return 0;
    }

    return -1;
}

void uninit_hm_service(void)
{
    if(sHmService != NULL) {
        delete sHmService;
        sHmService = NULL;
    }
}

void handle_hm_msg_storage(char *action, char *path)
{
    DBG("storage %s %s\n", action, path);

    if(strcmp(action, "add") == 0) {

    } else if(strcmp(action, "remove") == 0) {

    }
}

void handle_hm_msg_usb(char *action, char *devpath)
{
    DBG("usb %s %s\n", action, devpath);

    if(strcmp(action, "add") == 0) {

    } else if(strcmp(action, "remove") == 0) {

    }
}

void handle_hm_msg_net(char *action, char *interface)
{
    DBG("action:%s interface:%s\n", action, interface);
}

void handle_hm_msg_dhcp(char *action, char *interface)
{
    DBG("%s dhcp %s!\n", interface, action);

    if(strcmp(action, "fail") == 0) {
        // dhcp 获取IP 失败

    } else if(strcmp(action, "ok") == 0) {
        //todo
    }
}
#endif
