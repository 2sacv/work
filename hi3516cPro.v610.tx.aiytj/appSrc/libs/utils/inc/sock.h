/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    :
 * Created Time : 2014-10-14
 * Version      : 1.0
 * Author       : luoshunfa
 * Description  :
 */

#ifndef _SOCK_H_
#define _SOCK_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <time.h>

#if defined(__GNUC__)

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netdb.h>

typedef int SOCKET;

#define closesocket(s) close(s)

/* these are not needed for Linux */
#define socketsShutdown()   do{}while(0);
#define socketsStartup()    do{}while(0);

#elif defined(__MINGW32__) || defined(_MSC_VER)

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

void socketsStartup();
#define socketsShutdown() WSACleanup()

//const char *inet_ntop(int af, const void *src, char *dst, unsigned cnt);
//int inet_pton(int af, const char *src, void *dst);

int gettimeofday(struct timeval *tp, void *tzp);

#else

#error "Unrecognized compiler"

#endif

typedef enum {
    TCP,
    UDP,
    LISTEN
}
eSockType;

int unix_sock_bind(const char *host, int port, eSockType type);
int unix_sock_connect(int sock, const char *host, int port);

int setNonblocking(int socket);
unsigned getSendBufferSize(int socket);
unsigned getReceiveBufferSize(int socket);
unsigned increaseSendBufferTo(int socket, unsigned requestedSize);
unsigned increaseReceiveBufferTo(int socket, unsigned requestedSize);

int getErrno();

#ifdef __cplusplus
}
#endif

#endif

