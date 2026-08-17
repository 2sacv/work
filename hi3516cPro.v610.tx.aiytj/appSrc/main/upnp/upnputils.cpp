/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : upnputils.cpp
 * @Created Time : 2014-04-02
 * @Version      : 1.0
 * @Author       : zengy
 * @Description  :
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>

#include "debug.h"

#include "upnputils.h"

#define NET_NIC_NUM     2   // support nic number

#if defined (DEV_TYPE_ENHANCED)

/*====================================================================
 discrib: sendto broadcast packet
 param:
    int NetSendToBroadCast( -OUT SUCCESS/FAILURE
        int sock,                   -IN socket
        char *buf,              -IN send buffer
        int bufLen,             -IN buffer length
        struct sockaddr_in *addr)   -IN send address
=====================================================================*/
int NetSendToBroadCast(int sock, char *buf, int bufLen, struct sockaddr_in *addr)
{
    struct ifreq req;
    unsigned int broadAddr[NET_NIC_NUM];
    int ret = 0;
    int i = 0;

    // get subnet broadcast addr
    memset(broadAddr, 0, sizeof(broadAddr));
    for (i = 0; i < NET_NIC_NUM; i++) {
        memset(&req, 0, sizeof(req));
        sprintf(req.ifr_name, "eth%d", i);
        if (-1 == ioctl(sock, SIOCGIFBRDADDR, &req)) {
            //DBG("ioctl fail\n");
            break;
        }

        memcpy(&broadAddr[i], req.ifr_broadaddr.sa_data + 2, sizeof(broadAddr[0]));
    }

    if (0 >= i) {
        return FAILURE;
    }

    // set SO_BROADCAST
    ret = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &ret, sizeof(ret))) {
        DBG("setsockopt fail, errno=%d %s\n", errno, strerror(errno));
        return FAILURE;
    }

    // send broadcast
    for (i = 0; i < NET_NIC_NUM && broadAddr[i]; i++) {
        addr->sin_addr.s_addr = broadAddr[i];
        ret = sendto(sock, buf, bufLen, 0, (struct sockaddr *)addr, sizeof(*addr));
        if (0 > ret) {
            DBG("sendto, errno=%d %s\n", errno, strerror(errno));
            return FAILURE;
        }
    }

    return SUCCESS;
}
#endif

