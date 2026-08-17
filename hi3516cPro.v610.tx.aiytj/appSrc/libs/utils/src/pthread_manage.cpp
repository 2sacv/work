/**
 * Copyright (C) by Jabsco Company
 * 
 * @File Name    : servicePthreadManage.cpp
 * @Created Time : 2024-11-22
 * @Version      : 1.0
 * @Author       : zhangj
 * @Description  : 
 */

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/prctl.h>
#include <syslog.h>
#include <signal.h>
#include <string.h>

#include "linux_list.h"
#include "debug.h"
#include "utils.h"
#include "pthread_manage.h"

typedef struct
{
    struct list_head list;
    
    pthread_t    pthID;
    char         srvName[64];
    pthread_func pthFunc;
    void*        pArg;
    int          sPid;
    int          sPpid;
    int          do_free;
}SrvPthStruct;

static pthread_mutex_t  fMutex;
static struct list_head fSrvPthHead = {NULL, NULL};

static void srvList_lock(){pthread_mutex_lock(&fMutex);}
static void srvList_unlock(){pthread_mutex_unlock(&fMutex);}

static int get_pid(void)
{
    return syscall(SYS_gettid);
}

static int init_pthread_manage()
{
    //DBG("next : %p, prev : %p\n", fSrvPthHead.next, fSrvPthHead.prev);
    pthread_mutex_init(&fMutex, NULL);
    
    INIT_LIST_HEAD(&fSrvPthHead);   

    return 0;
}

static void *pthread_info(void *pArg)
{
    void * pvrtn = 0;
    SrvPthStruct *srvpth = (SrvPthStruct *)pArg;
    srvpth->sPid = get_pid();
    srvpth->sPpid = getppid();
    
    struct sigaction sigAction;

    sigAction.sa_handler = SIG_IGN;
    sigAction.sa_flags = 0;
    sigemptyset(&sigAction.sa_mask);
    sigaddset(&sigAction.sa_mask, SIGTERM);
    sigaddset(&sigAction.sa_mask, SIGINT);
    sigaddset(&sigAction.sa_mask, SIGPIPE);
    sigaction(SIGPIPE, &sigAction, NULL);
    SYSLOG("thrd [%s] pid:%d tid:%lu start@ Ppid:%d\n", srvpth->srvName, srvpth->sPid, pthread_self(), getpid());
    prctl(PR_SET_NAME, srvpth->srvName);
    pvrtn = srvpth->pthFunc(srvpth->pArg);
    SYSLOG("thrd [%s] pid:%d tid:%lu exited\n", srvpth->srvName, srvpth->sPid, pthread_self());

    if (srvpth->do_free) {
        free(srvpth);
    }

    return pvrtn;
}

/* 
 * typedef unsigned long int pthread_t;
 *
 * 核心需求：
 * 1. 给线程命名进行线程跟踪;
 * 2. 线程退出时 join 或 DETACHED，防止资源 
 * 3. 简化模式
 *
 * 业务分类:
 * 1. 定时器，及多路复用的 socket 
 * 2. 涉及队列循环读命令，有 exit 标志控制循环的，比如 delay_exec
 * 3. 启动任务过度，等待资源(网络)准备好，比如 p2p,record_watch
 * 
 * 使用使用 create_pthread()
 * 1. 使用 js_scheduler 做任务管理
 * 2. 使用 create_pthread()，
 * 3. 使用 pthread_namecreate()， 默认强制设置 PTHREAD_CREATE_DETACHED
 **/
pthread_t pthread_namecreate(const char *name, pthread_func func, void *arg)
{
    if (NULL == func || NULL == name) {
        SYSLOG("Create [%p] @%p failed of NULL!\n", name, func);
        return 0;
    }

    SrvPthStruct *srvpth = (SrvPthStruct *)system_malloc(sizeof(SrvPthStruct));
    if(NULL == srvpth) {
        SYSLOG("Create [%s] pthread failed of malloc!\n", name);
        return 0;
    }

    srvpth->do_free = true;
    srvpth->pthFunc = func;
    srvpth->pArg = arg;
    strncpy(srvpth->srvName, name, sizeof(srvpth->srvName)-1);

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_t tid = 0;

    if (pthread_create(&tid, &attr, pthread_info, (void *)srvpth) != 0) {
        tid = 0;
        free(srvpth);
        SYSLOG("Create [%s] pthread failed!\n", name);
    }

    return tid;
}

/* 
 * typedef unsigned long int pthread_t;
 *
 * 不要使用 attr 设置线程分离，严格使用 join_pthread(), join_pthread_name() 进行回收资源
 *
 * pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
 **/
pthread_t create_pthread(const char *name, pthread_func func, pthread_attr_t *attr, void *arg)
{
    if(NULL == func || NULL == name)
    {
        ERR("Parameter illegal!\n");
        return 0;
    }
    
    if((NULL == fSrvPthHead.next) && (NULL == fSrvPthHead.prev))
    {
        DBG("Init pthreadManage...\n");
        init_pthread_manage();
    }

    SrvPthStruct *srvpth = (SrvPthStruct *)malloc(sizeof(SrvPthStruct));
    if(NULL == srvpth)
    {
        ERR("allocate SrvPthStruct memory failed!\n");
        return 0;
    }

    bzero(srvpth, sizeof(SrvPthStruct));

    srvpth->pthFunc = func;
    srvpth->pArg = arg;
    snprintf(srvpth->srvName, sizeof(srvpth->srvName), "%s", name);
    
    if (pthread_create(&srvpth->pthID, attr, pthread_info, (void *)srvpth) != 0)
    {
        ERR("Create [%s] pthread failed!\n", name);
        srvpth->pthID = 0;
        free(srvpth);
        return 0;
    }
    
    srvList_lock();
    list_add_tail(&srvpth->list, &fSrvPthHead);
    srvList_unlock();

    return srvpth->pthID;
}

void join_pthread(pthread_t pid)
{
    if((NULL == fSrvPthHead.next) && (NULL == fSrvPthHead.prev))
    {
        DBG("pthreadManage didn't init and num of pthread is 0!\n");
        return ;
    }

    struct list_head *pPos = NULL;
    struct list_head *pNn = NULL;
    SrvPthStruct *srvpth = NULL;

    srvList_lock();
    list_for_each_safe(pPos, pNn, &fSrvPthHead)
    {
        srvpth = list_entry(pPos, SrvPthStruct, list);
        
        if(srvpth->pthID != pid) {
            continue;
        }

        list_del(pPos);
        break;
    }
    srvList_unlock();

    if(NULL !=  srvpth){
        if ((pthread_t)0 != srvpth->pthID)
        {
            pthread_join(srvpth->pthID, NULL);
            DBG("[%s] service manual quit, ID : %u\n", srvpth->srvName, (unsigned int)srvpth->sPid);
        }
        free(srvpth);
        srvpth = NULL;
    }

    return ;
}

void quit_all_pthread()
{
    if((NULL == fSrvPthHead.next) && (NULL == fSrvPthHead.prev))
    {
        DBG("pthreadManage didn't init and num of pthread is 0!\n");
        return ;
    }

    struct list_head *pPos = NULL;
    struct list_head *pNn = NULL;
    
    SrvPthStruct *srvpth = NULL;

    srvList_lock();
    list_for_each_safe(pPos, pNn, &fSrvPthHead)
    {
        srvpth = list_entry(pPos, SrvPthStruct, list);

        if((pthread_t)0 != srvpth->pthID)
        {   
            pthread_join(srvpth->pthID, NULL);
            srvpth->pthID = 0;
        }

        DBG("[%s] service pthread quit, ID : %u\n", srvpth->srvName, (unsigned int)srvpth->sPid);
        list_del(pPos);
        free(srvpth);
        srvpth = NULL;
    }
    srvList_unlock();

    return ;
}

void print_all_pthread_info()
{
    if((NULL == fSrvPthHead.next) && (NULL == fSrvPthHead.prev))
    {
        DBG("pthreadManage didn't init and num of pthread is 0!\n");
        return ;
    }   

    struct list_head *pPos = NULL;
    
    SrvPthStruct *srvpth = NULL;

    srvList_lock();
    list_for_each(pPos, &fSrvPthHead)
    {
        srvpth = list_entry(pPos, SrvPthStruct, list);
        DBG("pthread desc : %s, id : %u, ID : %d\n", srvpth->srvName, (unsigned)srvpth->pthID, srvpth->sPid);
    }
    srvList_unlock();
}

