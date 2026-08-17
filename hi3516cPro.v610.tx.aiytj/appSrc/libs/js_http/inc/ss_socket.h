/*
 * File Name    :
 * Created Time : 2024-01-24
 * Version      : 1.0
 * Author       : luoshunfa
 * Description  :
 */


#ifndef  _SS_SOCKET_H_
#define _SS_SOCKET_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <string.h>

#if defined(__linux__) || defined(__APPLE__)

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>

#define closesocket(s) close(s)

/* these are not needed for Linux */
#define socketsShutdown()   do{}while(0);
#define socketsStartup()    do{}while(0);

#elif defined(_WIN32)

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

void socketsStartup();
#define socketsShutdown() WSACleanup()

int gettimeofday(struct timeval *tp, void *tzp);

struct iovec {
    size_t  	iov_len; 
    void *      iov_base;
};

#endif

typedef union {
    struct sockaddr     sa;
    struct sockaddr_in  sin;
    struct sockaddr_in6 sin6;
} sockaddr_u;

int ss_get_errno(void);

int ss_is_ipv4(const char* host);
int ss_is_ipv6(const char* host);
int ss_is_ipaddr(const char* host);

int ss_resolve_addr(const char* host, sockaddr_u* addr);

const char* ss_sockaddr_ip(sockaddr_u* addr, char *ip, int len);
int ss_sockaddr_port(sockaddr_u* addr);
int ss_sockaddr_len(sockaddr_u* addr);

int ss_sockaddr_set_ip(sockaddr_u* addr, const char* host);
int ss_sockaddr_set_port(sockaddr_u* addr, int port);
int ss_sockaddr_set_ipport(sockaddr_u* addr, const char* host, int port);

void ss_sockaddr_print(sockaddr_u* addr);

/*family  : AF_INET or AF_INET6
 *type    : SOCK_STREAM or SOCK_DGRAM
 *protocol: always be 0 
 */
int ss_sock_create(int family, int type, int protocol);
int ss_sock_bind(int sock, const char *host, int port);

/*
 *type    : SOCK_STREAM or SOCK_DGRAM
 */
int ss_sock_create_bind(int type, const char *host, int port);
int ss_sock_listen(int sock, int len);

int ss_sock_connect(int sock, const char* host, int port);
int ss_sock_connect_nonblock(int sock, const char* host, int port);
int ss_sock_connect_timeout(int sock,const char* host, int port, int ms);

int ss_sock_pair(int family, int type, int fd[2]);

int ss_sock_send(int sock, char *data, int len);
int ss_sock_writev(int sock, struct iovec *vec, int vec_len);
int ss_sock_sendto(int sock, char *buf, int len, char *ip, int port);

int ss_sock_recv(int sock, char *data, int len);
int ss_sock_readv(int sock, struct iovec *vec, int vec_len);

int ss_sock_nodelay(int sock, int on);
int ss_sock_keepalive(int sock, int on, int delay, int cnt, int intvl);

int ss_sock_broadcast(int sock, int on);
int ss_sock_set_ipmulticastif(int sock, char *ip);
int ss_sock_set_addmembership(int sock, char *multiip, char *locolip);
int ss_sock_set_dropmembership(int sock, char *multiip, char *locolip);
int ss_sock_set_ipmulticastloop(int sock, int opt);

int ss_sock_sndtimeo(int sockfd, int timeout);
int ss_sock_rcvtimeo(int sockfd, int timeout);
int ss_sock_sndbuf(int sockfd, int len);
int ss_sock_rcvbuf(int sockfd, int len);
int ss_sock_reuseaddr(int sockfd, int on);
int ss_sock_reuseport(int sockfd, int on);
int ss_sock_linger(int sockfd, int timeout);

int ss_sock_set_nonblocking(int fd);
int ss_sock_set_blocking(int fd);

int ss_sock_select_write(int sock, int timeoutms);
int ss_sock_select_read(int sock, int timeoutms);

int ss_sock_send_by_time(int sock, char *data, int datasize, int timeoutms);
int ss_sock_recv_by_time(int sock, char *buf, int buflen, int timeoutms);
int ss_sock_send_full_by_time(int sock, char *data, int datasize, int timeoutms);
int ss_sock_recv_full_by_time(int sock, char *buf, int buflen, int timeoutms);
int ss_sock_sendto_by_time(int sock, char *data, int datasize,
                           char *ip, int port, int timeoutms);
int ss_sock_recvfrom_by_time(int sock, char *buf, int buflen,
                             struct sockaddr* from, int* fromlen, int timeoutms);

#ifdef __cplusplus
}
#endif


#endif
