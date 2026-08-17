/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    :
 * Created Time : 2014-10-14
 * Version      : 1.0
 * Author       : luoshunfa
 * Description  :
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#ifndef _WIN32
#include <fcntl.h>
#endif

#include "sock.h"

#ifdef _WIN32

int getErrno()
{
    int winErr = GetLastError();


    // Convert to a POSIX errorcode. The *major* assumption is that
    // the meaning of these codes is 1-1 and each Winsock, etc, etc
    // function is equivalent in errors to the POSIX standard. This is
    // a big assumption, but the server only checks for a small subset of
    // the real errors, on only a small number of functions, so this is probably ok.
    switch (winErr) {

        case ERROR_FILE_NOT_FOUND:
            return ENOENT;
        case ERROR_PATH_NOT_FOUND:
            return ENOENT;
        case WSAEINTR:
            return EINTR;
        case WSAENETRESET:
            return EPIPE;
        case WSAENOTCONN:
            return ENOTCONN;
        case WSAEWOULDBLOCK:
            return EAGAIN;
        case WSAECONNRESET:
            return EPIPE;
        case WSAEADDRINUSE:
            return EADDRINUSE;
        case WSAEMFILE:
            return EMFILE;
        case WSAEINPROGRESS:
            return EINPROGRESS;
        case WSAEADDRNOTAVAIL:
            return EADDRNOTAVAIL;
        case WSAECONNABORTED:
            return EPIPE;
        case 0:
            return 0;

        default:
            return ENOTCONN;
    }
}

void socketsStartup()
{
    WORD         wVersionRequested;
    WSADATA      wsaData;

    wVersionRequested = MAKEWORD(1, 1);
    if (WSAStartup(wVersionRequested, &wsaData)) {
        printf("\r\nUnable to initialize WinSock for host info");
        exit(-1);
    }
}

int gettimeofday(struct timeval *tp, void *tzp)
{
    time_t clock;
    struct tm tm;
    SYSTEMTIME wtm;
    GetLocalTime(&wtm);
    tm.tm_year = wtm.wYear - 1900;
    tm.tm_mon = wtm.wMonth - 1;
    tm.tm_mday = wtm.wDay;
    tm.tm_hour = wtm.wHour;
    tm.tm_min = wtm.wMinute;
    tm.tm_sec = wtm.wSecond;
    tm.tm_isdst = -1;
    clock = mktime(&tm);
    tp->tv_sec = clock;
    tp->tv_usec = wtm.wMilliseconds * 1000;
    return 0;
}

#else

int getErrno()
{
    return errno;
}

#endif

int unix_sock_bind(const char *host, int port, eSockType type)
{
    struct sockaddr_in sin;
    int sock, err;

    sock = socket(AF_INET, (TCP == type || LISTEN == type) ? SOCK_STREAM : SOCK_DGRAM, 0);
    if (sock < 0) {
        err = -getErrno();
        return err;
    }

	fcntl(sock, F_SETFD, 1);
	
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = host ? inet_addr(host) : INADDR_ANY;

    if (LISTEN == type) {
        int tag = 1;
		setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void *)&tag, sizeof(tag));

    }

	int iRetry = 50;
    do {
        if (bind(sock, (struct sockaddr*)&sin, sizeof(sin)) < 0 ) {
            if (EADDRINUSE == errno) {
                usleep(200 * 1000);
                continue;
            }
            err = -errno;
        	closesocket(sock);
        	return err;
        } else {
            break;
        }
    } while (0 < --iRetry);

    if (0 >= iRetry) {
        err = -errno;
    	closesocket(sock);
    	return err;
    }

    /* Set the listen file descriptor to no-delay / non-blocking mode. */
    int flags = fcntl(sock, F_GETFL, 0 );
	if ( flags == -1 ) {
        err = -errno;
    	closesocket(sock);
    	return err;
    }

    fcntl( sock, F_SETFL, flags | O_NDELAY );

    return sock;

}

int unix_sock_connect(int sock, const char *host, int port)
{
    struct sockaddr_in sin;

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = host ? inet_addr(host) : INADDR_ANY;

    if (connect(sock, (struct sockaddr*)&sin, sizeof(sin)) < 0)
        return -getErrno();

    return 0;
}

int setNonblocking(int socket)
{
    int err;

#ifdef _WIN32
    unsigned long on = 1;
    err = ioctlsocket(socket, FIONBIO, &on);
#else
    int flags = fcntl(socket, F_GETFL, 0);
    err = fcntl(socket, F_SETFL, flags | O_NONBLOCK);
#endif

    return err;
}


static unsigned getBufferSize(int bufOptName, int socket)
{
    unsigned curSize;
    socklen_t sizeSize = sizeof curSize;
    if (getsockopt(socket, SOL_SOCKET, bufOptName, (char*)&curSize, &sizeSize) < 0) {
        printf("getBufferSize() error: %d\n", getErrno());
        return 0;
    }

    return curSize;
}

unsigned getSendBufferSize(int socket)
{
    return getBufferSize(SO_SNDBUF, socket);
}

unsigned getReceiveBufferSize(int socket)
{
    return getBufferSize(SO_RCVBUF, socket);
}

static unsigned increaseBufferTo(int bufOptName, int socket, unsigned requestedSize)
{
    // First, get the current buffer size.  If it's already at least
    // as big as what we're requesting, do nothing.
    unsigned curSize = getBufferSize(bufOptName, socket);

    // Next, try to increase the buffer to the requested size,
    // or to some smaller size, if that's not possible:
    while (requestedSize > curSize) {
        socklen_t sizeSize = sizeof requestedSize;
        if (setsockopt(socket, SOL_SOCKET, bufOptName, (char*)&requestedSize, sizeSize) >= 0) {
            // success
            return requestedSize;
        }
        requestedSize = (requestedSize+curSize)/2;
    }

    return getBufferSize(bufOptName, socket);
}

unsigned increaseSendBufferTo(int socket, unsigned requestedSize)
{
    return increaseBufferTo(SO_SNDBUF, socket, requestedSize);
}

unsigned increaseReceiveBufferTo(int socket, unsigned requestedSize)
{
    return increaseBufferTo(SO_RCVBUF, socket, requestedSize);
}

