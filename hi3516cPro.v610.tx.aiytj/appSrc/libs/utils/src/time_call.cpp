/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    : time_call.cpp
 * Created Time : 2014-02-26
 * Version      : 1.0
 * Author       : luoshunfa
 * Description  :
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <signal.h>

#include "utils.h"
#include "time_call.h"
#include "JTimeCallService.h"
#include "BasicUsageEnvironment.hh"


class JTimeEngineData
{
public:
    JTimeEngineData() {
        cWatch = 0;
        tThread = 0;
        pService = 0;

        DBG("new time engine:%p\n", this);
    }
    ~JTimeEngineData() {
        DBG("del time engine:%p\n", this);
    }

public:
    char                cWatch;
    pthread_t           tThread;
    JTimeCallService *  pService;
};

void * time_call_thread_func(void *data)
{
    JTimeEngineData *theEngine = (JTimeEngineData *)data;
    if(theEngine == NULL)
        return 0;

    struct sigaction sigAction;

    sigAction.sa_handler = SIG_IGN;
    sigAction.sa_flags = 0;
    sigemptyset(&sigAction.sa_mask);
    sigaddset(&sigAction.sa_mask, SIGTERM);
    sigaddset(&sigAction.sa_mask, SIGINT);
    sigaddset(&sigAction.sa_mask, SIGPIPE);
    sigaction(SIGPIPE, &sigAction, NULL);

    TaskScheduler* theScheduler = BasicTaskScheduler::createNew();
    theEngine->pService = new JTimeCallService(theScheduler);

    SYSLOG("[getpid():%d pid:] time call main loop...\n", getpid());
    theScheduler->doEventLoop(&theEngine->cWatch);

    delete theEngine->pService;
    theEngine->pService = NULL;

    delete theScheduler;

    return 0;
}

int create_time_engine(JTimeEngine *engine)
{
    int ret = 0;

    if(engine == NULL)
        return -1;

    JTimeEngineData *theEngine = new JTimeEngineData;
    *engine= theEngine;

    ret = pthread_create(&theEngine->tThread, NULL, time_call_thread_func, theEngine);
    if (0 != ret) {
        theEngine->tThread = 0;
    }

    while (!theEngine->pService) {
        usleep(1000 * 100);
    }

    return ret;
}

void release_time_engine(JTimeEngine *engine)
{
    if(engine == NULL)
        return ;

    JTimeEngineData *theEngine = (JTimeEngineData *)(*engine);

    theEngine->cWatch = 1;
    if (theEngine->tThread) {
        pthread_join(theEngine->tThread, NULL);
        theEngine->tThread = 0;
    }

    delete theEngine;
    *engine = 0;
}

int register_time_func(JTimeEngine engine,char * desc,int timeinterval,JTimeCallFunc func,void * data)
{
    if(engine == NULL)
        return -1;

    JTimeEngineData *theEngine = (JTimeEngineData *)engine;
    if(theEngine->pService == NULL)
        return -1;

    return theEngine->pService->addTimeCallReq(desc, timeinterval, func, data);
}

void unregister_timel_func(JTimeEngine engine,JTimeCallFunc func)
{
    if(engine == NULL)
        return ;

    JTimeEngineData *theEngine = (JTimeEngineData *)engine;
    if(theEngine->pService == NULL)
        return ;

    theEngine->pService->removeTimeCallReq(func);
}

