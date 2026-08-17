/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    :
 * Created Time : 2020-06-16
 * Version      : 1.0
 * Author       : luoshunfa
 * Description  :
 */
#ifndef _JS_LISTEN_H_
#define _JS_LISTEN_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "js_scheduler.h"
#include "ossl_typ.h"
#include "ssl.h"

typedef void (*JSNewClientCB)(void *userdata, int acceptsock, SSL * ssl_sock);
typedef struct {
    JSScheduler     scheduler;

    unsigned int    host;
    int             port;

    int             socket;
	SSL_CTX *       ssl_ctx;
    JSRWHandle      rwhandle;

    JSNewClientCB   cb;
    void *          userdata;
    struct sockaddr_in  peer_addr;
} JSListener;

JSListener* js_listen_create(JSScheduler scheduler, int port, JSNewClientCB cb, void *data, SSL_CTX * ssl_ctx);
int js_listen_delete(JSListener *listener);

#ifdef __cplusplus
}
#endif

#endif

