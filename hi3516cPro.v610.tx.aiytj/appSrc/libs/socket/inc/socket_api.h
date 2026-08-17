/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    : socket_api.h
 * Created Time : 2014-03-14
 * Version      : 1.0
 * Author       : tangpengcheng
 * Description  :
 */

#ifndef __SOCKET_API__H__
#define __SOCKET_API__H__
#include <sys/socket.h>
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SOCK_NONE = -1,
    TCP,
    UDP,
} sock_type;

typedef enum {
    IP_NONE = -1,
    IPV4,
    IPV6,
} net_type;

typedef struct net_sockfd_s {
    int sockfd;
    net_type nettype;
    sock_type socktype;

    struct sockaddr_storage remote_addr;
}net_sockfd_t;


#define net_sock_fd(s_ptr) ((s_ptr)->sockfd)
#define net_remote_addr(s_ptr) ((s_ptr)->remote_addr)


int net_tcp_server(net_sockfd_t* sockobj, net_type nettype, char *ip, unsigned short port);
int net_udp_server(net_sockfd_t* sockobj, net_type nettype, char *ip, unsigned short port);


int net_socket(net_sockfd_t* sockobj, net_type nettype, sock_type socktype);

int net_bind(net_sockfd_t* sockobj, char *ip, unsigned short port);

int net_listen(net_sockfd_t* sockobj, int num);

int net_accept(net_sockfd_t* sockobj, net_sockfd_t* sock_conn);

int net_connect(net_sockfd_t* sockobj, char* remote_addr,
    unsigned short remote_port);

int net_connect_nonb(net_sockfd_t* sockobj, char* remote_addr,
    unsigned short remote_port, int timeout);

int net_tcp_connect(net_sockfd_t* sockobj, net_type nettype, char *ip, int iPort, int timeout);

int net_sendn(net_sockfd_t* sockobj, const void *buf, int len);

int net_send_timeout(net_sockfd_t* sockobj, const char* inData, const int inLength, struct timeval* tvTimeOut);
int net_sendto(net_sockfd_t* sockobj, const void *buf, int len,
    char *ip, unsigned short port);

int net_sendto2(net_sockfd_t* sockobj, const void *buf, int buff_len,
    struct sockaddr * ptr_addr, int addr_len);

int net_recv(net_sockfd_t* sockobj, void *buf, int len);

int net_recvfrom(net_sockfd_t* sockobj, void *buf, int buff_len);

int net_close(net_sockfd_t* sockobj);

int mcast_join (net_sockfd_t* sockobj, char *ip);
int mcast_leave(net_sockfd_t* sockobj, char *ip);
int net_set_multicast_loop(net_sockfd_t* sockobj);

int bcast_join (net_sockfd_t* sockobj);

const char * net_domain_to_ipaddr(char *domain, net_type nettype, sock_type socktype,
    char *ip, int len);

const char * net_get_ipaddr_port(struct sockaddr_storage* ptr_addr, char *ip,
    int len, unsigned short* port);

int net_set_socket_noblock(net_sockfd_t* sockobj);
int net_set_reuse_addr(net_sockfd_t* sockobj, int flag);
int net_set_sendbuf_size(net_sockfd_t* sockobj, int isize);
int net_get_sendbuf_size(net_sockfd_t* sockobj);
int net_set_recvbuf_size(net_sockfd_t* sockobj, int isize);
int net_get_recvbuf_size(net_sockfd_t* sockobj);
int net_read_write_timeout(net_sockfd_t* sockobj, char mode, int timeout);


int get_net_phyaddr(char *name, char *addr , char *bMac);
unsigned net_set_keep_alive_time(net_sockfd_t* sockobj, unsigned requestedtime);


#ifdef __cplusplus
}
#endif
#endif
