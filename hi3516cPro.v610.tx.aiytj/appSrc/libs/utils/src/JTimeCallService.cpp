/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    : JTimeCallService.cpp
 * Created Time : 2012-09-22
 * Version      : 1.0
 * Author       : luoshunfa
 * Description  :
 */

#include <stdlib.h>
#include <stdio.h>

#include "BasicUsageEnvironment.hh"
#include "JTimeCallService.h"

#include "debug.h"

void JTimeCall::timeCallFuncHandler(void * instance)
{
    JTimeCall* session = (JTimeCall*)instance;
    session->doTimeCallFuncHandler();
}

void JTimeCall::doTimeCallFuncHandler()
{
    //DBG("%p handle time call:%s start\n", this, fDesc);

    if(fFunc)
        fFunc(fData);
	//DBG("%p handle time call:%s end \n", this, fDesc);
    if(fTaskScheduler)
        fTask = fTaskScheduler->scheduleDelayedTask(fTimeInterval*1000000LL,
                (TaskFunc*)JTimeCall::timeCallFuncHandler,
                this);
}

JTimeCallService::JTimeCallService(TaskScheduler * schedule)
    : fTaskScheduler(schedule),
      fTimeCallTable(NULL), fTimeCallReq(NULL)
{     
    if(0 == create_pipe(fpipe_fd))
        set_pipe_nonblock(fpipe_fd);
    fTaskScheduler->turnOnBackgroundReadHandling(fpipe_fd[0], NotifyTaskFunc, this);
    fTimeCallTable = HashTable::create(ONE_WORD_HASH_KEYS);
    fTimeCallReq = create_fifo_queue();
}

JTimeCallService::~JTimeCallService()
{
    clearAllTimeCall();
    delete fTimeCallTable;
    fTimeCallTable = NULL;
    close_pipe(fpipe_fd);
}

void JTimeCallService::NotifyTaskFunc(void* clientData, int mask)
{
    JTimeCallService * pthis = (JTimeCallService *)clientData;
    pthis->DoNotifyTaskFunc();
}

void JTimeCallService::DoNotifyTaskFunc()
{
    char buff[16] = {0};
    int num = read(fpipe_fd[0], buff, sizeof(buff));
    if(num <= 0) {
        DBG("pipe burst\n");
        fTaskScheduler->turnOffBackgroundReadHandling(fpipe_fd[0]);
        close_pipe(fpipe_fd);

        if(0 == create_pipe(fpipe_fd)) {
            set_pipe_nonblock(fpipe_fd);
            fTaskScheduler->turnOnBackgroundReadHandling(fpipe_fd[0], NotifyTaskFunc, this);
        } else {
            fpipe_fd[0] = -1;
            fpipe_fd[1] = -1;
        }
        return;
    }
	
    
    JTimeCall *theTimeCall = (JTimeCall *)fifo_queue_pop_unblock(fTimeCallReq);    
    while (theTimeCall != NULL)
    {
        if(!theTimeCall->fIsRemove)
            addTimeCall(theTimeCall);
        else
            removeTimeCall(theTimeCall);

        theTimeCall = (JTimeCall *)fifo_queue_pop_unblock(fTimeCallReq);
    }
    
}

int JTimeCallService::hasTimeCall(JTimeCall * timecall)
{
    JTimeCall *theTimeCall = NULL;

    if(timecall == NULL)
        return 0;

    theTimeCall = (JTimeCall*)fTimeCallTable->Lookup((char const *)theTimeCall->fFunc);

    if(theTimeCall == NULL)
        return 0;

    return 1;
}

int JTimeCallService::addTimeCall(JTimeCall * timecall)
{
    JTimeCall *oldTimeCall;

    if(timecall == NULL)
        return -1;

    timecall->fTaskScheduler = fTaskScheduler;
    timecall->fTask = fTaskScheduler->scheduleDelayedTask(timecall->fTimeInterval*1000000LL,
                      (TaskFunc*)JTimeCall::timeCallFuncHandler,
                      timecall);

    oldTimeCall = (JTimeCall*)fTimeCallTable->Add((char const *)timecall->fFunc, timecall);

    if(oldTimeCall != NULL)
        delete oldTimeCall;

    return 0;
}

int JTimeCallService::removeTimeCall(JTimeCall * timecall)
{
    JTimeCall *oldTimeCall;

    if(timecall == NULL)
        return -1;

    oldTimeCall = (JTimeCall*)fTimeCallTable->Lookup((char const *)timecall->fFunc);
    if(oldTimeCall != NULL)
    {
        fTimeCallTable->Remove((char const *)timecall->fFunc);
        delete oldTimeCall;
    }

    delete timecall;
    
    return 0;
}

void JTimeCallService::clearAllTimeCall()
{
    JTimeCall *theTimeCall = NULL;

    while (1)
    {
        theTimeCall  = (JTimeCall*)(fTimeCallTable->RemoveNext());
        if (theTimeCall == NULL) break;

        delete theTimeCall;
    }
}

int JTimeCallService::addTimeCallReq(char *desc, int timeinterval, JTimeCallFunc func, void *data)
{
    if(func == NULL || timeinterval <= 0)
        return -1;

    JTimeCall *theTimeCall = new JTimeCall(desc, timeinterval, func, data, 0);
    fifo_queue_push(fTimeCallReq, (void *)theTimeCall);

    write_pipe(fpipe_fd[1], "A", 1);
    return 0;
}

int JTimeCallService::removeTimeCallReq(JTimeCallFunc func)
{
    if(func == NULL)
        return -1;    

    JTimeCall * theTimeCall = new JTimeCall(NULL, 0, func, 0, 1);
    fifo_queue_push(fTimeCallReq, (void *)theTimeCall);
    
    write_pipe(fpipe_fd[1], "A", 1);

    return 0;
}
