/*
 * File Name    :
 * Created Time : 2024-06-12
 * Version      : 1.0
 * Author       : luoshunfa
 * Description  :
 */


#ifndef _JHTTPSERVER_H_
#define _JHTTPSERVER_H_

#include <vector>
#include <string>

#include "js_scheduler.h"
#include "JHttpService.h"

class JHttpServer
{
public:
    std::string                 host;
    int                         port;
    int                         usehttps;

    int                         threadnums;
    std::vector<JSScheduler>    schedulers;

    int                         listenfd;
    JSRWHandle                  listenHandle;

    JHttpService *              service;

public:
    JHttpServer();
    virtual ~JHttpServer();

    void setHostPort(const char *host, int port, int usehttps);
    void setThreadNum(int threadnums)
    {
        if(threadnums > 0 && threadnums < 128)
            this->threadnums = threadnums;
    }

    void setService(JHttpService * service)
    {
        this->service = service;
    }

    void onAcceptHanding();

    int start();
    int stop();

};



#endif
