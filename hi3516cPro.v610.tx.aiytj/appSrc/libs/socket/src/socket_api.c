/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    : socket_api.c
 * Created Time : 2014-03-14
 * Version      : 1.0
 * Author       : tangpengcheng
 * Description  :
 */

#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/select.h>
#include <fcntl.h>
#include <unistd.h>
#include <net/if.h>
#include <netinet/tcp.h>

#include <sys/ioctl.h>     
#include <linux/if_ether.h>
#include <sys/time.h>

#include "socket_api.h"
#include "../include/debug.h"



#ifndef IN_IS_ADDR_MULTICAST
#define IN_IS_ADDR_MULTICAST(a)    ((((in_addr_t)(a)) & 0xf0000000) == 0xe0000000)
#endif

#ifndef IN6_IS_ADDR_MULTICAST
#define IN6_IS_ADDR_MULTICAST(a) ((a)->s6_addr[0] == 0xff)
#endif



int net_socket(net_sockfd_t* sockobj, net_type nettype, sock_type socktype)
{
	int domain, type;
	switch(nettype){
		case IPV4:
			domain = AF_INET;
			break;

		case IPV6:
			domain = AF_INET6;
			break;

		default:
			return -1;
	}

	switch(socktype){
		case TCP:
			type = SOCK_STREAM;
			break;

		case UDP:
			type = SOCK_DGRAM;
			break;

		default:
			return -1;
	}
	
	net_sock_fd(sockobj) = socket(domain, type, 0);
	if(net_sock_fd(sockobj) < 0){
		DBG("socket : %s\n", strerror(errno));
		return -1;
	}
	sockobj->nettype = nettype;
	sockobj->socktype = socktype;
	
	return net_sock_fd(sockobj);		
}

int net_bind(net_sockfd_t* sockobj,  char *ip, unsigned short port)
{
	int iret = -1;
	if(IPV6 == sockobj->nettype) {
		struct sockaddr_in6 addr;
		addr.sin6_family = AF_INET6;
		addr.sin6_port = htons(port);
		if(NULL == ip){
			addr.sin6_addr = in6addr_any;
		} else {		
			inet_pton(AF_INET6, ip, &addr.sin6_addr);
		}

		iret = bind(net_sock_fd(sockobj), (struct sockaddr*)&addr, sizeof(struct sockaddr_in6));
	} else if (IPV4 == sockobj->nettype) {
		struct sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		if(NULL == ip){
			addr.sin_addr.s_addr = INADDR_ANY;
		} else {		
			inet_pton(AF_INET, ip, &addr.sin_addr);
		}

		iret = bind(net_sock_fd(sockobj), (struct sockaddr*)&addr, sizeof(struct sockaddr_in));
	}
	
	if(iret < 0){
		DBG("bind : %s\n", strerror(errno));
	}

	return iret;
}

int net_listen(net_sockfd_t* sockobj, int num)
{
	if(listen(net_sock_fd(sockobj), num) < 0) {
		DBG("listen : %s\n", strerror(errno));
		return -1;
	}
	
	return 0;
}

int net_accept(net_sockfd_t* sockobj, net_sockfd_t* sock_conn)
{
	sock_conn->nettype = sockobj->nettype;
	sock_conn->socktype = sockobj->socktype;
	socklen_t len = sizeof(struct sockaddr_storage);
	if((sock_conn->sockfd = accept(net_sock_fd(sockobj), 
			(struct sockaddr *)&sock_conn->remote_addr, &len)) < 0) {
		DBG("accept : %s\n", strerror(errno));
		return -1;
	}
	
	return sock_conn->sockfd;	
}

int net_connect(net_sockfd_t* sockobj, char* remote_addr, unsigned short remote_port)
{
	int iret = -1;
	int length = 0;
	struct sockaddr_storage addr;
	if(IPV6 == sockobj->nettype){		
		struct sockaddr_in6* addr_ptr = (struct sockaddr_in6*)&addr;
		addr_ptr->sin6_family = AF_INET6;
		addr_ptr->sin6_port = htons(remote_port);		
		inet_pton(AF_INET6, remote_addr, &addr_ptr->sin6_addr);
		length = sizeof(struct sockaddr_in6);		
	} else if (IPV4 == sockobj->nettype) {	
		struct sockaddr_in* addr_ptr = (struct sockaddr_in*)&addr;
		addr_ptr->sin_family = AF_INET;
		addr_ptr->sin_port = htons(remote_port);
		inet_pton(AF_INET, remote_addr, &addr_ptr->sin_addr);		
		length = sizeof(struct sockaddr_in);
	} else {
		return -1;
	}

	if ((iret = connect(net_sock_fd(sockobj), (struct sockaddr*)&addr, length)) != 0) {
		 DBG("connect() error (ip: %s, port number: %d):\n", remote_addr, remote_port);
		 return -1;
	}

	return iret;
}

int net_connect_nonb(net_sockfd_t* sockobj, char* remote_addr, unsigned short remote_port, int timeout)
{
	int iret = -1;
	int length = 0;
	struct sockaddr_storage addr;
	if(IPV6 == sockobj->nettype){		
		struct sockaddr_in6* addr_ptr = (struct sockaddr_in6*)&addr;
		addr_ptr->sin6_family = AF_INET6;
		addr_ptr->sin6_port = htons(remote_port);		
		inet_pton(AF_INET6, remote_addr, &addr_ptr->sin6_addr);
		length = sizeof(struct sockaddr_in6);		
	} else if (IPV4 == sockobj->nettype) {	
		struct sockaddr_in* addr_ptr = (struct sockaddr_in*)&addr;
		addr_ptr->sin_family = AF_INET;
		addr_ptr->sin_port = htons(remote_port);
		inet_pton(AF_INET, remote_addr, &addr_ptr->sin_addr);		
		length = sizeof(struct sockaddr_in);
	} else {
		return -1;
	}

	int flags = fcntl(net_sock_fd(sockobj), F_GETFL, 0);
	fcntl(net_sock_fd(sockobj), F_SETFL, flags|O_NONBLOCK);

	if((iret = connect(net_sock_fd(sockobj), (struct sockaddr*)&addr, length)) != 0) {
		if(EINPROGRESS == errno) {
			fd_set wfds;			
			fd_set rfds;
			FD_ZERO(&wfds);
			FD_SET(net_sock_fd(sockobj), &wfds);
			rfds = wfds;

			struct timeval tv;
			tv.tv_sec  = timeout;
        	tv.tv_usec = 0;
			if((iret = select(net_sock_fd(sockobj) + 1, &rfds, &wfds, NULL, &tv)) == 0)
				return -1;

			if(FD_ISSET(net_sock_fd(sockobj), &rfds) || FD_ISSET(net_sock_fd(sockobj), &wfds)) {
				length = sizeof(int);
				if(getsockopt(net_sock_fd(sockobj), SOL_SOCKET, SO_ERROR, &iret, (socklen_t *)&length) < 0){					
					return -1;
				}		
			} else {			
				DBG("select error\n");
				return -1;
			}			
			
		} else {
			return -1;
		}
		
	}

	fcntl(net_sock_fd(sockobj), F_SETFL, flags);
	return iret;
}

int net_tcp_connect(net_sockfd_t* sockobj, net_type nettype, char *ip, int port, int timeout)
{
	if(net_socket(sockobj, nettype, TCP) < 0)
		return -1;

	if(net_connect_nonb(sockobj, ip, port, timeout) < 0){
		net_close(sockobj);
		return -1;
	}

	return 0;
}

int net_sendn(net_sockfd_t* sockobj, const void *buf, int len)
{
	int length = 0, iret;
	while(length < len){
		iret = send(net_sock_fd(sockobj), buf + length, len - length, 0);
		if(iret < 0){
			if(EINTR == errno || errno == EAGAIN) {
				usleep(5000);
				continue;			
			}

			DBG("send : %s\n", strerror(errno));
			return iret;
		} else if (0 == iret) {
			break;
		}

		length += iret;
	}
	return length;	
}

int net_send_timeout(net_sockfd_t* sockobj, const char* inData, const int inLength, struct timeval* tvTimeOut)
{
	int iRet = 0;
	int sendLen = 0;
	fd_set writeSet;

	while(sendLen < inLength) {
	    FD_ZERO(&writeSet);
		FD_SET(net_sock_fd(sockobj), &writeSet);
		iRet = select(net_sock_fd(sockobj) + 1, NULL, &writeSet, NULL, tvTimeOut);
		if(iRet < 0) {
			if (errno == EINTR && errno == EAGAIN )	{
				usleep(1000);
				continue;
			}
				
			perror("select error");
			return -1;
		} else if (iRet == 0) {	
			DBG("Send() over time\n");
			return sendLen;
		}		
		
		iRet = send(net_sock_fd(sockobj), inData + sendLen, inLength - sendLen, 0);
		if (iRet == -1 ) {
			perror("Send error");
			if (errno != EINTR && errno != EAGAIN )	{
				DBG("Send() socket %d error \n", net_sock_fd(sockobj));
				return -1;
			} else {			
				continue;
			}
		}			
		sendLen += iRet;
	}

	return sendLen;
}

int net_sendto(net_sockfd_t* sockobj, const void *buf, int len, 
	char *ip, unsigned short port)
{
	int iret;
	socklen_t length;
	struct sockaddr_storage addr;
	if(IPV6 == sockobj->nettype){		
		struct sockaddr_in6* addr_ptr = (struct sockaddr_in6*)&addr;
		addr_ptr->sin6_family = AF_INET6;
		addr_ptr->sin6_port = htons(port);		
		inet_pton(AF_INET6, ip, &addr_ptr->sin6_addr);
		length = sizeof(struct sockaddr_in6);		
	} else if (IPV4 == sockobj->nettype) {	
		struct sockaddr_in* addr_ptr = (struct sockaddr_in*)&addr;
		addr_ptr->sin_family = AF_INET;
		addr_ptr->sin_port = htons(port);
		inet_pton(AF_INET, ip, &addr_ptr->sin_addr);		
		length = sizeof(struct sockaddr_in);
	} else {
		return -1;
	}

	iret = net_sendto2(sockobj, buf, len, (struct sockaddr *)&addr, length);

	return iret;
}

int net_sendto2(net_sockfd_t* sockobj, const void *buf, int buff_len, 
	struct sockaddr * ptr_addr, int addr_len)
{
	int iret;
	
DATA_SEND:
	iret = sendto(net_sock_fd(sockobj), buf, buff_len, 0,
		ptr_addr, addr_len);

	if(iret < 0){
		if(EINTR == errno) {
			usleep(5000);
			goto DATA_SEND; 		
		}

		DBG("sendto : %s ip:%s \n", strerror(errno),inet_ntoa(((struct sockaddr_in*)ptr_addr)->sin_addr));
		return iret;
	}

	return iret;
}

int net_recv(net_sockfd_t* sockobj, void *buf, int len)
{
	int recv_len = 0;
	
RECV_DATA:
	recv_len = recv(net_sock_fd(sockobj), buf, len, 0);
	if (recv_len == -1 && EINTR == errno) {
		usleep(5000);
		recv_len = 0;
		goto RECV_DATA;
	}

	return recv_len;
}

int net_recvfrom(net_sockfd_t* sockobj, void *buf, int buff_len)
{
	int recv_len = 0;
	socklen_t len = sizeof(struct sockaddr_storage);	
RECV_DATA:
	recv_len = recvfrom(net_sock_fd(sockobj), buf, buff_len, 0,
		(struct sockaddr *)&sockobj->remote_addr, &len);
	if (recv_len == -1 && EINTR == errno) {
		usleep(5000);
		goto RECV_DATA;
	}

	return recv_len;
}

int net_close(net_sockfd_t* sockobj)
{
	close(net_sock_fd(sockobj));
	net_sock_fd(sockobj) = -1;
	return 0;
}

int mcast_join (net_sockfd_t* sockobj, char *ip)
{
	int iret;
	switch(sockobj->nettype){
		case IPV4: {			
				struct ip_mreq mreq; 
				iret = inet_pton(AF_INET, ip, &mreq.imr_multiaddr.s_addr);
				if(1 != iret || !IN_IS_ADDR_MULTICAST(ntohl(mreq.imr_multiaddr.s_addr))){
					DBG("address is not multicast address \n");
					return -1;
				}
				mreq.imr_interface.s_addr = INADDR_ANY;
				return setsockopt(net_sock_fd(sockobj), IPPROTO_IP, IP_ADD_MEMBERSHIP,
	                          &mreq, sizeof(mreq));
			}

		case IPV6: {
				struct ipv6_mreq mreq;
				iret = inet_pton(AF_INET6, ip, &mreq.ipv6mr_multiaddr.s6_addr);
				if(1 != iret || !IN6_IS_ADDR_MULTICAST(mreq.ipv6mr_multiaddr.s6_addr)){
					DBG("address is not multicast address \n");
					return -1;
				}
				mreq.ipv6mr_interface = 0;
		        return(setsockopt(net_sock_fd(sockobj), IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP,
		               &mreq, sizeof(mreq)));
			}

		default:
			return -1;
	}

	return 0;
}

int mcast_leave(net_sockfd_t* sockobj, char *ip)
{
	int iret;
	switch(sockobj->nettype){
		case IPV4: {			
				struct ip_mreq mreq; 
				iret = inet_pton(AF_INET, ip, &mreq.imr_multiaddr.s_addr);
				if(1 != iret || !IN_IS_ADDR_MULTICAST(ntohl(mreq.imr_multiaddr.s_addr))){
					DBG("address is not multicast address \n");
					return -1;
				}
				mreq.imr_interface.s_addr = INADDR_ANY;
				return setsockopt(net_sock_fd(sockobj), IPPROTO_IP, IP_DROP_MEMBERSHIP,
	                          &mreq, sizeof(mreq));
			}

		case IPV6: {
				struct ipv6_mreq mreq;
				iret = inet_pton(AF_INET6, ip, &mreq.ipv6mr_multiaddr.s6_addr);
				if(1 != iret || !IN6_IS_ADDR_MULTICAST(mreq.ipv6mr_multiaddr.s6_addr)){
					DBG("address is not multicast address \n");
					return -1;
				}
				mreq.ipv6mr_interface = 0;
		        return(setsockopt(net_sock_fd(sockobj), IPPROTO_IPV6, IPV6_DROP_MEMBERSHIP,
		               &mreq, sizeof(mreq)));
			}

		default:
			return -1;
	}

	return 0;
}

int net_set_multicast_loop(net_sockfd_t* sockobj)
{
	int opt = 1;
	switch(sockobj->nettype){
		case IPV4: {			
			return setsockopt(net_sock_fd(sockobj), IPPROTO_IP, IP_MULTICAST_LOOP,
								   &opt, sizeof(opt));
			}

		case IPV6: {			
			return setsockopt(net_sock_fd(sockobj), IPPROTO_IPV6, IPV6_MULTICAST_LOOP,
								   &opt, sizeof(opt));
			}

		default:
			break;
	}
	return -1;
}

int bcast_join (net_sockfd_t* sockobj)
{
	switch(sockobj->nettype){
		case IPV4: {
				const int opt = 1;
				return setsockopt(net_sock_fd(sockobj), SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt));
			}

		case IPV6:
		default:
			return -1;
	}

	return 0;
}


const char * net_domain_to_ipaddr(char *domain, net_type nettype, 
	sock_type socktype, char *ip, int len)
{
	struct addrinfo *answer, hint, *curr;
	bzero(&hint, sizeof(hint));

	switch(nettype) {
		case IPV4:
			hint.ai_family = AF_INET;
			break;

		case IPV6:
			hint.ai_family = AF_INET6;
			break;

		default:
			return NULL;
	}

	switch(socktype) {
		case TCP:
			hint.ai_socktype = SOCK_STREAM;
			break;

		case UDP:
			hint.ai_socktype = SOCK_DGRAM;
			break;

		default:
			return NULL;
	}

	const char* str = NULL;
	int ret = getaddrinfo(domain, NULL, &hint, &answer);
	if (ret != 0) {
		DBG("getaddrinfo: %s\n", gai_strerror(ret));
		return NULL;
	}
	
	for (curr = answer; curr != NULL; curr = curr->ai_next) {
		if(AF_INET == curr->ai_family){
			struct sockaddr_in * p_addr = (struct sockaddr_in *)curr->ai_addr;
			str = inet_ntop(AF_INET, &p_addr->sin_addr, ip, len);		
		} else if (AF_INET6 == curr->ai_family) {
			struct sockaddr_in6 * p_addr = (struct sockaddr_in6 *)curr->ai_addr;
			str = inet_ntop(AF_INET6, &p_addr->sin6_addr, ip, len);
		}
		
		//DBG("%s ---> %s\n", domain, ip);
	}
	freeaddrinfo(answer);

	return str;
}

const char * net_get_ipaddr_port(struct sockaddr_storage* ptr_addr, char *ip, 
	int len, unsigned short* port)
{
	if(!ptr_addr) return NULL;
	const char* str = NULL;
	if(AF_INET == ptr_addr->ss_family){
		struct sockaddr_in * p_addr = (struct sockaddr_in *)ptr_addr;
		str = inet_ntop(AF_INET, &p_addr->sin_addr, ip, len);
		if(port)
			*port = ntohs(p_addr->sin_port);
	} else if (AF_INET6 == ptr_addr->ss_family) {
		struct sockaddr_in6 * p_addr = (struct sockaddr_in6 *)ptr_addr;
		str = inet_ntop(AF_INET6, &p_addr->sin6_addr, ip, len);
		if(port)
			*port = ntohs(p_addr->sin6_port);
	}
	if(!str)
		DBG("net_get_ip_addr error: %s\n", strerror(errno));

	return str;
}

int net_set_socket_noblock(net_sockfd_t* sockobj)
{
	int flags = fcntl(net_sock_fd(sockobj), F_GETFL, 0);
	return (fcntl(net_sock_fd(sockobj), F_SETFL, flags|O_NONBLOCK) >= 0);
}

int net_set_reuse_addr(net_sockfd_t* sockobj, int flag)
{
	if (setsockopt(net_sock_fd(sockobj), SOL_SOCKET, SO_REUSEADDR,
			&flag, sizeof(flag)) < 0) {
    	DBG("setsockopt(SO_REUSEADDR) error \n");
    	return -1;
	}
	return 0;
}

int net_set_sendbuf_size(net_sockfd_t* sockobj, int isize)
{
	socklen_t size_len = sizeof(isize);
	if (setsockopt(net_sock_fd(sockobj), SOL_SOCKET, SO_SNDBUF, 
		(char*)&isize, size_len) < 0) {
		DBG("SetSocketSendBufSize error \n");
    	return -1;
	}
	return 0;
}

int net_get_sendbuf_size(net_sockfd_t* sockobj)
{
	int cur_size;
	socklen_t size_len = sizeof(cur_size);
	if (getsockopt(net_sock_fd(sockobj), SOL_SOCKET, SO_SNDBUF,
		(char*)&cur_size, &size_len) < 0)
	{
		DBG("GetSendBufferSize() error ");
		return -1;
	}
	return cur_size;
}

int net_set_recvbuf_size(net_sockfd_t* sockobj, int isize)
{
	socklen_t size_len = sizeof(isize);
	if (setsockopt(net_sock_fd(sockobj), SOL_SOCKET, SO_RCVBUF, 
		(char*)&isize, size_len) < 0)
	{
		DBG("SetSocketRcvBufSize error \n");
    	return -1;
	}
	return 0;
}

int net_get_recvbuf_size(net_sockfd_t* sockobj)
{
	int cur_size;
	socklen_t size_len = sizeof(cur_size);
	if (getsockopt(net_sock_fd(sockobj), SOL_SOCKET, SO_RCVBUF,
		(char*)&cur_size, &size_len) < 0)
	{
		DBG("GetRecvBufferSize() error ");
		return -1;
	}

	return cur_size;
}

int net_read_write_timeout(net_sockfd_t* sockobj, char mode, int timeout)
{
    struct timeval tv;
    fd_set rdfd;
    fd_set wrfd;

    FD_ZERO(&rdfd);
    FD_ZERO(&wrfd);

    tv.tv_sec = timeout;
    tv.tv_usec = 0;

    if(mode == 'r')
        FD_SET(net_sock_fd(sockobj), &rdfd);

    if(mode == 'w')
        FD_SET(net_sock_fd(sockobj), &wrfd);

    int ret = select(net_sock_fd(sockobj) + 1, &rdfd, &wrfd, NULL, &tv);
    if (0 >= ret) {
		DBG("select %c timeout.\n", mode);
        return -1;
    } else {
        return 0;
    }
}

int net_tcp_server(net_sockfd_t* sockobj, net_type nettype, char *ip, unsigned short port)
{
	if(net_socket(sockobj, nettype, TCP) < 0)
		return -1;

	net_set_reuse_addr(sockobj, 1);
	net_set_socket_noblock(sockobj);

	if(net_bind(sockobj, ip, port) < 0) {
		net_close(sockobj);
		return -1;
	}

	if(net_listen(sockobj, 5) < 0) {
		net_close(sockobj);
		return -1;
	}
	
	return 0;
}

int net_udp_server(net_sockfd_t* sockobj, net_type nettype, char *ip, unsigned short port)
{
	if(net_socket(sockobj, nettype, UDP) < 0)
		return -1;

	net_set_reuse_addr(sockobj, 1);
	net_set_socket_noblock(sockobj);

	if(net_bind(sockobj, ip, port) < 0) {
		net_close(sockobj);
		return -1;
	}

	return 0;
}

int get_net_phyaddr(char *name, char *addr , char *bMac)
{											// addr : 12:13:14:15:16:17
	int 		sockfd;		
	struct 	ifreq ifr;
	char 	buff[24];
	
	/* Create a channel to the NET kernel. */
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){		
		return -1;
	}

	/* get net physical address */
	strcpy(ifr.ifr_name, name);
	if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) < 0) {	
		close(sockfd);
		return -1;
	}
	strcpy(ifr.ifr_name, name);
	if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) < 0) {
		close(sockfd);
		return -1;	
	} else {
		memcpy(buff, ifr.ifr_hwaddr.sa_data, 6);
		
		if(bMac)
			memcpy(bMac , ifr.ifr_hwaddr.sa_data , 6);

		if(addr) {
			sprintf(addr, "%02X:%02X:%02X:%02X:%02X:%02X",
			 	(buff[0] & 0377), (buff[1] & 0377), (buff[2] & 0377),
			 	(buff[3] & 0377), (buff[4] & 0377), (buff[5] & 0377));
		
			//printf("HW address: %s\n", addr);
		}
		
		close(sockfd);
		return  0;
	}
}

unsigned net_set_keep_alive_time(net_sockfd_t* sockobj, unsigned requestedtime) 
{
	int keepalive = 1;
	int keepinterval = 10;
	int keepcount = 3;

	int iret = setsockopt(net_sock_fd(sockobj), SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive , sizeof(keepalive ));
	iret += setsockopt(net_sock_fd(sockobj), IPPROTO_TCP, TCP_KEEPIDLE, (void*)&requestedtime , sizeof(requestedtime ));
	iret += setsockopt(net_sock_fd(sockobj), IPPROTO_TCP, TCP_KEEPINTVL, (void *)&keepinterval , sizeof(keepinterval ));
	iret += setsockopt(net_sock_fd(sockobj), IPPROTO_TCP, TCP_KEEPCNT, (void *)&keepcount , sizeof(keepcount ));

	return iret;
}


