/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    : delay_handle.c
 * Created Time : 2012-10-08
 * Version      : 1.0
 * Author       : luoshunfa
 * Description  :
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/syscall.h>

#include "debug.h"
#include "utils.h"
#include "fifo_queue.h"
#include "record_delay.h"

#include "pthread_manage.h"

typedef struct {
    JDelayFuncPtr   func;
    void *          data;
    char            str[128];
} delay_handle_t;

static int          sDelayHandleWatch = 0;
static queue_t *    sDelayHandleQueue = NULL;
static pthread_t    sDelayHandleThread = 0;
static int countNum = 0;

static void *thrd_delay_exec(void* data)
{
    delay_handle_t *handle = NULL;

    struct sigaction sigAction;

    sigAction.sa_handler = SIG_IGN;
    sigAction.sa_flags = 0;
    sigemptyset(&sigAction.sa_mask);
    sigaddset(&sigAction.sa_mask, SIGTERM);
    sigaddset(&sigAction.sa_mask, SIGINT);
    sigaddset(&sigAction.sa_mask, SIGPIPE);
    sigaction(SIGPIPE, &sigAction, NULL);

    while( !sDelayHandleWatch ) {
        if(sDelayHandleQueue == NULL) {
            usleep(100*1000);
        }

        usleep(10*1000); // pretect cpu too high when much delay task

        handle = (delay_handle_t *)fifo_queue_pop_unblock(sDelayHandleQueue);
        if(handle == NULL) {
            continue;
        }

        countNum--;
        if(handle->func)
            handle->func(handle->data);

        free(handle);
    }

    return NULL;
}

int init_delay_handle(void)
{
    DBG("init_delay_handle\n");

    sDelayHandleQueue = create_fifo_queue();
    if ((sDelayHandleThread = create_pthread("thrd_delay_exec", thrd_delay_exec, NULL, NULL)) == 0) {
        sDelayHandleThread = 0;
        return FAILURE;
    }
    return SUCCESS;
}

void uninit_delay_handle(void)
{
    DBG("uninit_delay_handle\n");

    sDelayHandleWatch = 1;
    if (0 != sDelayHandleThread) {
        join_pthread(sDelayHandleThread);
        sDelayHandleThread = 0;
    }
}

int delay_handle_add(JDelayFuncPtr func, void *data, const char *str)
{
    int ret = 0;
    delay_handle_t * handle = NULL;

    if(countNum >= 50) {
        DBG("countNum(%d) >= 50\n", countNum);
        return -1;
    }

    if(sDelayHandleQueue == NULL)
        return -1;

    handle = (delay_handle_t *)malloc(sizeof(delay_handle_t));
    if(handle == NULL) {
        DBG("malloc pRemoveFlag error:%s\n", strerror(errno));
        return -1;
    }

    handle->func = func;
    handle->data = data;
    if(str)
        sprintf(handle->str, "%s", str);

    ret = fifo_queue_push(sDelayHandleQueue, (void *) handle);
    if(ret < 0) {
        DBG("jco_queue_push error\n");
        if(handle)
            free(handle);
    }

    countNum++;

    return ret;
}

