/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    : JTimeCallService.h
 * Created Time : 2012-09-22
 * Version      : 1.0
 * Author       : luoshunfa
 * Description  :
 */

#ifndef  _JTimeCallService_H_
#define _JTimeCallService_H_

#include "HashTable.hh"
#include "UsageEnvironment.hh"

#include "debug.h"
#include "time_call.h"
#include "pipe_utils.h"
#include "fifo_queue.h"


class JTimeCall
{
    public:
        JTimeCall(char *desc, int timeinterval, JTimeCallFunc func, void *data, int isremove)
            : fTimeInterval(timeinterval),
              fFunc(func),
              fData(data),

              fIsRemove(isremove),
              fTask(NULL),
              fTaskScheduler(NULL)
        {
            memset(fDesc, 0, sizeof(fDesc));
            if(desc)
                snprintf(fDesc, sizeof(fDesc)-1, "%s", desc);
			else
				snprintf(fDesc, sizeof(fDesc)-1, "unkown");

			DBG("new time call:%s\n", fDesc);
        }

        ~JTimeCall()
		{
			if(fTaskScheduler)
				fTaskScheduler->unscheduleDelayedTask(fTask);
		
			if(strlen(fDesc) > 0)
				DBG("delete time call:%s\n", fDesc);
		}

        static void  timeCallFuncHandler(void *instance);
        void doTimeCallFuncHandler();

    public:
        char                fDesc[64];
        int                 fTimeInterval;
        JTimeCallFunc       fFunc;
        void *              fData;

        int                 fIsRemove;
        void *              fTask;
        TaskScheduler*      fTaskScheduler;
};

class JTimeCallService
{
    public:
        JTimeCallService(TaskScheduler *schedule);
        ~JTimeCallService();

    public:
        int addTimeCallReq(char *desc, int timeinterval, JTimeCallFunc func, void *data);
        int removeTimeCallReq(JTimeCallFunc func);

    private:		
		static void NotifyTaskFunc(void* clientData, int mask);
		void DoNotifyTaskFunc();
        int hasTimeCall(JTimeCall *timecall);
        int addTimeCall(JTimeCall *timecall);
        int removeTimeCall(JTimeCall * timecall);
        void clearAllTimeCall();

    private:
        TaskScheduler*          fTaskScheduler;
        HashTable*              fTimeCallTable;
		queue_t *               fTimeCallReq;
        int fpipe_fd[2];
};

#endif

