/*
 * File Name    :
 * Created Time : 2024-06-20
 * Version      : 1.0
 * Author       : luoshunfa
 * Description  :
 */


#ifndef _JHTTPSERVICE_H_
#define _JHTTPSERVICE_H_

#include <map>
#include <string>

#include "JHttpMessage.h"

/*
 * @param[in]  req:  http request
 * @param[out] resp: http response
 * @return  0:              handle next
 *          http_status :   handle done
 */
typedef int (*JHttpHandler)(JHttpRequest* req, JHttpResponse* resp);

typedef std::map<std::string, JHttpHandler>  JHttpHandlers;

class JHttpService
{
public:
    JHttpHandlers   handlers;

public:
    JHttpService() {}
    virtual ~JHttpService()
    {
        handlers.clear();
    }

    int AddHanlder(const char* path, JHttpHandler handler);
    int GetHanlder(const char* path, JHttpHandler* phandler);
};


#endif

