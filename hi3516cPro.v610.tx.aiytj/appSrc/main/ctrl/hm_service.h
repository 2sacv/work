/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    : hm_service.h
 * Created Time : 2012-10-15
 * Version      : 1.0
 * Author       : luoshunfa
 * Description  :
 */

#ifndef _hm_service_H_
#define _hm_service_H_

#include "js_scheduler.h"

class JHMService
{
public:
    JHMService(JSScheduler schedule);
    ~JHMService();

    static void  incomingHMhandle(int fd, int events, void *userdata);
    void doIncomingHMhandle();

private:
    int initHMService();
    void uninitHMService();

private:
    JSScheduler             fTaskScheduler;
	JSRWHandle				fReadHandle;
    int                     fSockFd;
};

#endif

