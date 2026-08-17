/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : upnputils.h
 * @Created Time : 2014-04-02
 * @Version      : 1.0
 * @Author       : zengy
 * @Description  :
 */

#ifndef __UPNPUTILS_H_
#define __UPNPUTILS_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <netinet/in.h>
#include <arpa/inet.h>

    /*====================================================================
     discrib: sendto broadcast packet
     param:
        int NetSendToBroadCast( -OUT SUCCESS/FAILURE
            int sock,                   -IN socket
            char *buf,              -IN send buffer
            int bufLen,             -IN buffer length
            struct sockaddr_in *addr)   -IN send address
    =====================================================================*/
    int NetSendToBroadCast(int sock, char *buf, int bufLen, struct sockaddr_in *addr);


#ifdef __cplusplus
}
#endif
#endif

