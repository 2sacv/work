/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    :
 * Created Time : 2020-06-16
 * Version      : 1.0
 * Author       : luoshunfa
 * Description  :
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "sock.h"
#include "js_scheduler.h"

#include "js_listen.h"


static void __js_listener_on_event(int fd, int events, void *userdata)
{
    JSListener *listener;
	SSL * ssl_sock = NULL;

    int sock;
    socklen_t addrlen;

    listener = (JSListener *)userdata;

    memset(&(listener->peer_addr), 0, sizeof((listener->peer_addr)));
    addrlen = sizeof((listener->peer_addr));
    sock = accept(listener->socket, (struct sockaddr*)&(listener->peer_addr), &addrlen);
    if (sock < 0 && getErrno() != EAGAIN) {
        js_log("[%p]accept err:%d\n", listener, sock);
        return ;
    }
	
	if (NULL != listener->ssl_ctx){
		//建立SSL
		ssl_sock = SSL_new(listener->ssl_ctx);
		if (NULL == ssl_sock){
			js_log("ssl new error\n");
			closesocket(sock);
			return;
		}

		//将SSL与TCP socket连接
		if (0 == SSL_set_fd(ssl_sock, sock)){
			js_log("ssl connect error\n");
			SSL_shutdown (ssl_sock);
			SSL_free(ssl_sock);
			ssl_sock = NULL;
			closesocket(sock);
			return;
		}
		//建立SSL连接
		if (0 >= SSL_accept(ssl_sock)){
			js_log("ssl accept error\n");
			SSL_shutdown (ssl_sock);
			SSL_free(ssl_sock);
			ssl_sock = NULL;
			closesocket(sock);
			return;
		}
	}

    if (listener->cb) {
        listener->cb(listener, sock, ssl_sock);
    } else {
        js_log("[%p]no new cli cb, so close sock:%d\n", listener, sock);
		if (NULL != ssl_sock){
            SSL_shutdown (ssl_sock);
            SSL_free(ssl_sock);
            ssl_sock = NULL;
        }
		closesocket(sock);
    }
}

JSListener* js_listen_create(JSScheduler scheduler,  int port, JSNewClientCB cb,void * data, SSL_CTX * ssl_ctx)
{
    JSListener *listener = NULL;
    int sock = -1;
    int requestedSize = 32*1024;
    int sizeSize = sizeof(requestedSize);

    do {
        if(scheduler == NULL)
            break;

        sock = unix_sock_bind(NULL, port, LISTEN);
        if(sock < 0) {
            js_log("create listen socket error!\n");
            break;;
        }

        setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char*)&requestedSize, sizeSize);
		
        if(listen(sock, 5) < 0) {
            js_log("listen  error!\n");
            closesocket(sock);
            break;
        }

        listener = (JSListener *)malloc(sizeof(JSListener));
        memset(listener, 0, sizeof(JSListener));

        listener->scheduler = scheduler;
        listener->port = port;
        listener->cb = cb;
        listener->userdata = data;

        listener->socket = sock;
		listener->ssl_ctx = ssl_ctx;
        js_create_reader_r(scheduler, sock, JS_READABLE, __js_listener_on_event, listener, &listener->rwhandle);

        js_log("new listen:%p\n", listener);
    } while(0);

    return listener;
}

int js_listen_delete(JSListener *listener)
{
    if(listener == NULL)
        return -1;

    if (listener->rwhandle) {
        js_delete_reader_r(&listener->rwhandle);
    }

    if(listener->socket)
        closesocket(listener->socket);

    listener->socket = -1;

    js_log("free listen:%p\n", listener);
    free(listener);

    return 0;
}


