/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    : hm_service.cpp
 * Created Time : 2012-10-15
 * Version      : 1.0
 * Author       : luoshunfa
 * Description  :
 */
 
#if 0
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>

#include "hm_api.h"
#include "hm_service.h"

#define HT_MSG_PORT     45566

JHMService::JHMService(JSScheduler schedule)
    : fTaskScheduler(schedule),
      fReadHandle(NULL),
      fSockFd(-1)
{
    initHMService();
}

JHMService::~JHMService()
{
    uninitHMService();
}

void JHMService::incomingHMhandle(int fd, int events, void *userdata)
{
    JHMService* session = (JHMService*)userdata;
    session->doIncomingHMhandle();
}

void JHMService::doIncomingHMhandle()
{
    char msgbuf[1024] = {0,};
    int  msglen;

    char subsystem[64] = {0,};
    char action[64] = {0,};
    char interface[64] = {0,};
    char devpath[128] = {0,};
    char *p = NULL;

    msglen = recv(fSockFd, msgbuf, sizeof(msgbuf) - 1, 0);
    fprintf(stderr, "read msg:%s\n", msgbuf);

    p = strstr(msgbuf, "subsystem:");
    if(p)
        sscanf(p, "subsystem:%[^;];", subsystem);

    p = strstr(msgbuf, "action:");
    if(p)
        sscanf(p, "action:%[^;];", action);

    p = strstr(msgbuf, "interface:");
    if(p)
        sscanf(p, "interface:%[^;];", interface);

    p = strstr(msgbuf, "devpath:");
    if(p)
        sscanf(p, "devpath:%[^;];", devpath);

    fprintf(stderr, "subsystem:%s action:%s interface:%s devpath:%s\n",
            subsystem, action, interface, devpath);

    if(strcmp(subsystem, "storage") == 0)
        handle_hm_msg_storage(action, devpath);
    else if(strcmp(subsystem, "net") == 0)
        handle_hm_msg_net(action, interface);
    else if(strcmp(subsystem, "dhcp") == 0)
        handle_hm_msg_dhcp(action, interface);
    else if(strcmp(subsystem, "usb") == 0)
        handle_hm_msg_usb(action, devpath);
}

int JHMService::initHMService()
{
    int                 opt = 1;

    struct sockaddr_in  sock_addr;
    int                 sock_addrlen;
    int                 err;
    int                 curFlags;

    fSockFd = socket(AF_INET, SOCK_DGRAM, 0);
    if(fSockFd < 0) {
        fprintf(stderr, "socket error :%s\n", strerror(errno));
        return -1;
    }

    sock_addr.sin_family = AF_INET;
    sock_addr.sin_addr.s_addr = htonl((127<<24)|1);
    sock_addr.sin_port = htons(HT_MSG_PORT);

    sock_addrlen = sizeof(struct sockaddr_in);

    err = setsockopt(fSockFd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(int));
    if(err < 0) {
        fprintf(stderr,"setsockopt error :%s\n", strerror(errno));
    }

    err = bind(fSockFd, (struct sockaddr *)&sock_addr, sock_addrlen);
    if(err < 0) {
        fprintf(stderr,"bind (port:%d) error :%s\n", HT_MSG_PORT, strerror(errno));
        close(fSockFd);
        fSockFd = -1;
        return -1;
    }

    curFlags = fcntl(fSockFd, F_GETFL, 0);
    err = fcntl(fSockFd, F_SETFL, curFlags|O_NONBLOCK);

	js_create_reader_r(fTaskScheduler, fSockFd, JS_READABLE, incomingHMhandle, (void *)this, &fReadHandle);

    return 0;
}

void JHMService::uninitHMService()
{
    if(fSockFd > 0) {
		js_delete_reader_r(&fReadHandle);
        close(fSockFd);
    }

    fSockFd = -1;
}

#endif

