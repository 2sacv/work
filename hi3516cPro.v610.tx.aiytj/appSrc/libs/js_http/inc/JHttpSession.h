/*
 * File Name    :
 * Created Time : 2024-06-18
 * Version      : 1.0
 * Author       : luoshunfa
 * Description  :
 */


#ifndef _JHTTPSESSION_H_
#define _JHTTPSESSION_H_


#include "js_scheduler.h"

#include "http_parser.h"

#include "JHttpMessage.h"
#include "JHttpService.h"

class JHttpSession
{
public:
    JSScheduler     scheduler;
    int             sockfd;

    JSRWHandle      readhandle;
	JHttpService *  service;
	
    JHttpRequest    requeset;
    JHttpResponse   response;

    http_parser     parser;

    int             isheadercomplete;
    int             isbodycomplete;
    int             content_length;

    std::string     header_field;
    std::string     header_value;

public:
    JHttpSession(JSScheduler sch, int fd);
    virtual ~JHttpSession();

    void setService(JHttpService * service)
    {
        this->service = service;
    }

	void start();
    void onReadHanding();
};




#endif

