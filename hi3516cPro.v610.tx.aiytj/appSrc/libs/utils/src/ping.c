/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    : ping.c
 * Created Time : 2014-03-05
 * Version      : 1.0
 * Author       : tangpengcheng
 * Description  :
 */

#include <stdio.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#include <setjmp.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <stdlib.h>
#include <sys/timerfd.h>
#include <fcntl.h>

#include "ping.h"
#include "utils.h"



#define PACKET_SIZE     1460
#define MAX_WAIT_TIME   5
#define MAX_NO_PACKETS  3
#define DATA_LEN        56

typedef struct ping_obj_s 
{
	int sockfd;
	int pack_num;
	ping_result_cb handle;
	void* data;
	int timeout;
	int nsend;
	int nreceived;
	pid_t pid;	
	struct sockaddr_in dest_addr;
	struct sockaddr_in from;
	struct timeval tvrecv;
} ping_obj_t;

static int pack(HANDLE ping_obj, char* packet);
static int unpack(HANDLE ping_obj, char *buf, int len);
static void tv_sub(struct timeval *out, struct timeval *in);
static unsigned short cal_chksum(unsigned short *addr, int len);

static int pack(HANDLE ping_obj, char* packet)
{
	ping_obj_t* ping_handle = (ping_obj_t*)ping_obj;

	int packsize;
	struct icmp *icmp;
	struct timeval *tval;
	icmp=(struct icmp*)packet;
	icmp->icmp_type = ICMP_ECHO;
	icmp->icmp_code = 0;
	icmp->icmp_cksum = 0;
	icmp->icmp_seq = ping_handle->nsend;
	icmp->icmp_id = ping_handle->pid;
	packsize = 8 + DATA_LEN;
	tval = (struct timeval *)icmp->icmp_data;
	gettimeofday(tval, NULL);
	icmp->icmp_cksum = cal_chksum( (unsigned short *)icmp,packsize);
	return packsize;
}

static int unpack(HANDLE ping_obj, char *buf, int len)
{
	ping_obj_t* ping_handle = (ping_obj_t*)ping_obj;

	int iphdrlen;
	struct ip *ip;
	struct icmp *icmp;
	struct timeval *tvsend;
	double rtt;
	ip = (struct ip *)buf;
	iphdrlen = ip->ip_hl << 2;
	icmp = (struct icmp *)(buf + iphdrlen);
	len -= iphdrlen;	
	char buff[128] = {0};	
	
	if( len < 8) {
		sprintf(buff, "ICMP packets\'s length is less than 8\n");
		
		if(ping_handle->handle)
			ping_handle->handle(buff, ping_handle->data);
	    return -1;
	}

	//printf("%d %d, %d, %d\n", icmp->icmp_type, ICMP_ECHOREPLY, icmp->icmp_id, ping_handle->pid);
	if( (icmp->icmp_type == ICMP_ECHOREPLY) && (icmp->icmp_id == ping_handle->pid) ){
		tvsend = (struct timeval *)icmp->icmp_data;
		tv_sub(&ping_handle->tvrecv, tvsend);
		rtt = ping_handle->tvrecv.tv_sec * 1000 + ping_handle->tvrecv.tv_usec / 1000;

		sprintf(buff, "%d byte from %s: icmp_seq=%u ttl=%d rtt=%.3f ms\n",
		    len,
		    j_inet_ntoa(ping_handle->from.sin_addr),
		    icmp->icmp_seq,
		    ip->ip_ttl,
		    rtt);
		
		if(ping_handle->handle)
			ping_handle->handle(buff, ping_handle->data);
	} else {
		return -1;
	}

	return 0;
}

static void tv_sub(struct timeval *out, struct timeval *in)
{
	if( (out->tv_usec -= in->tv_usec) < 0) {
		--out->tv_sec;
		out->tv_usec += 1000000;
	}
	out->tv_sec -= in->tv_sec;
}

static unsigned short cal_chksum(unsigned short *addr,int len)
{
	int nleft = len;
	int sum = 0;
	unsigned short *w = addr;
	unsigned short answer = 0;
		
	while(nleft > 1){
		sum += *w ++;
		nleft -= 2;
	}

	if( nleft == 1){
		*(unsigned char *)(&answer) =* (unsigned char *)w;
		sum += answer;
	}
	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);
	answer =~ sum;
	return answer;
}


int ping_init(HANDLE* ping_obj_ptr, ping_opt_t* para)
{
	*ping_obj_ptr = malloc(sizeof(ping_obj_t));
	memset(*ping_obj_ptr, 0, sizeof(ping_obj_t));
	ping_obj_t* ping_handle = (ping_obj_t*)(*ping_obj_ptr);
	ping_handle->handle = para->handle;
	ping_handle->data = para->data;
	ping_handle->pack_num = para->pack_num;
	ping_handle->timeout = para->timeout;
	
	struct protoent *protocol = NULL;
	unsigned long inaddr = 0l;
	int size = 50*1024;	
	char buff[128] = {0};
	char protobuf[512];
	struct protoent protosave;

	memset(protobuf, 0, sizeof(protobuf));
	memset(&protosave, 0, sizeof(protosave));
	getprotobyname_r("icmp", &protosave, protobuf, sizeof(protobuf) - 1, &protocol);
	if (NULL == protocol) {
		sprintf(buff, "%s\n", strerror(errno));
		
		if(para->handle)
			para->handle(buff, para->data);
		return -1;
	}

	if( (ping_handle->sockfd = socket(AF_INET, SOCK_RAW, protocol->p_proto) )<0) {		
		sprintf(buff, "%s\n", strerror(errno));
		
		if(para->handle)
			para->handle(buff, para->data);		
		return -1;
	}
	setuid(getuid());
	ping_handle->pid = getpid();
	
	int curFlags = fcntl(ping_handle->sockfd, F_GETFL, 0);
    fcntl(ping_handle->sockfd, F_SETFL, curFlags|O_NONBLOCK);

	setsockopt(ping_handle->sockfd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size) );
	ping_handle->dest_addr.sin_family = AF_INET;

	if( (inaddr = inet_addr(para->dest)) == INADDR_NONE)
	{	
		struct addrinfo *answer, hint, *curr;
		bzero(&hint, sizeof(hint));
		hint.ai_family = AF_INET;
		hint.ai_socktype = SOCK_STREAM;
		
		int ret = getaddrinfo(para->dest, NULL, &hint, &answer);
		if (ret != 0) {
			sprintf(buff, "getaddrinfo: %s\n", gai_strerror(ret));
			
			if(para->handle)
				para->handle(buff, para->data);
			return -1;
		}
		
		for (curr = answer; curr != NULL; curr = curr->ai_next) {
			memcpy((char *)&ping_handle->dest_addr, (char*)curr->ai_addr, sizeof(struct sockaddr_in));
			break;
		}
		freeaddrinfo(answer);
	} else {
		ping_handle->dest_addr.sin_family = AF_INET;
		ping_handle->dest_addr.sin_addr.s_addr = inaddr;	
	}

	sprintf(buff, "PING %s(%s): %d bytes data in ICMP packets.\n", para->dest,
	                j_inet_ntoa(ping_handle->dest_addr.sin_addr), DATA_LEN);
	if(para->handle)
		para->handle(buff, para->data);

	return 0;

}

int ping(HANDLE ping_obj)
{
	ping_obj_t* ping_handle = (ping_obj_t*)ping_obj;
	if(!ping_handle)
		return -1;

	int max_fd;
	int retval = 0;
	fd_set rfds;
	uint64_t exp;
	struct timeval tv;
	
	int packetsize;	
	char data_packet[PACKET_SIZE];	
	char buff[512] = {0};
	int n;
	socklen_t fromlen;
	fromlen = sizeof(ping_handle->from);

	if(ping_handle->pack_num == 0)
		ping_handle->pack_num = MAX_NO_PACKETS;

	int fd_time;
	struct itimerspec new_value;
	fd_time = timerfd_create(CLOCK_BOOTTIME, TFD_NONBLOCK);
	if (fd_time == -1)
		return -1;

	new_value.it_value.tv_sec =  1;
	new_value.it_value.tv_nsec = 0;
	new_value.it_interval.tv_sec = 0;
	new_value.it_interval.tv_nsec = 500000000;

	if (timerfd_settime(fd_time, 0, &new_value, NULL) == -1) {
        close(fd_time);
		return -1;
	}
	
	while(1) {		
		FD_ZERO(&rfds);
		FD_SET(ping_handle->sockfd, &rfds);		
		if( ping_handle->nsend < ping_handle->pack_num)	{		
			FD_SET(fd_time, &rfds);			
			max_fd = fd_time > ping_handle->sockfd ? fd_time + 1 : ping_handle->sockfd + 1;
		} else {
			new_value.it_value.tv_sec =  0;
			new_value.it_value.tv_nsec = 0;
			timerfd_settime(fd_time, 0, &new_value, NULL);
			max_fd = ping_handle->sockfd + 1;
		}

		tv.tv_sec = ping_handle->timeout <= 0 ? 5 : ping_handle->timeout;
		tv.tv_usec = 0;
		retval = select(max_fd, &rfds, NULL, NULL, &tv);
		if (retval == -1) {
			perror("select()");
			close(fd_time);
			return -1;
        } else if (retval > 0) {			
			if (FD_ISSET(fd_time, &rfds)) {
				packetsize = pack(ping_obj, data_packet);
				if( sendto(ping_handle->sockfd, data_packet, packetsize, 0,
				      (struct sockaddr *)&ping_handle->dest_addr, sizeof(ping_handle->dest_addr) ) < 0 ) {
					sprintf(buff, "%s\n", strerror(errno));
					
					if(ping_handle->handle)
						ping_handle->handle(buff, ping_handle->data);
					continue;
				}				
				ping_handle->nsend ++;
				n = read(fd_time, &exp, sizeof(uint64_t));
			}
			
			if (FD_ISSET(ping_handle->sockfd, &rfds)) {
				if( (n = recvfrom(ping_handle->sockfd, data_packet, sizeof(data_packet), 0,
						(struct sockaddr *)&ping_handle->from, &fromlen)) <0) {
					if(errno == EINTR)
						continue;

					sprintf(buff, "%s\n", strerror(errno));					
					if(ping_handle->handle)
						ping_handle->handle(buff, ping_handle->data);
				}

				gettimeofday(&ping_handle->tvrecv, NULL);
				if(unpack(ping_obj, data_packet, n) == -1) {
					continue;
				}

				ping_handle->nreceived ++;
				if(ping_handle->nreceived == ping_handle->pack_num) {
					break;
				}
			}			
        } else {          
			printf("test %d, %d over time\n", retval, ping_handle->nsend);
			break;
        }		
	}
	
	n = sprintf(buff, "\n--------------------PING statistics-------------------\n");
	if(0 == ping_handle->nsend) {
		sprintf(buff + n, "%d packets transmitted \n", ping_handle->nsend);
	} else {
		sprintf(buff + n, "%d packets transmitted, %d received , %.2f%% lost\n",ping_handle->nsend, 
			ping_handle->nreceived, ((float)ping_handle->nsend - (float)ping_handle->nreceived) * 100/ 
			(float)ping_handle->nsend);
	}
	
	if(ping_handle->handle)
		ping_handle->handle(buff, ping_handle->data);
	close(fd_time);
	return 0;
}

int ping_realse(HANDLE* ping_obj_ptr)
{
	ping_obj_t* ping_handle = (ping_obj_t*)(*ping_obj_ptr);
	if(!ping_handle)
		return -1;
	
	close(ping_handle->sockfd);
	free(*ping_obj_ptr);

	return 0;
}


//-----------------------------------------------------------------------------

typedef struct recvbuf_s
{
	char * recvbuf;
	int recvbufsize;
	int recved;
}recvbuf_t;

static int echo_result(char *buff, void *data)
{
	recvbuf_t* buff_ptr = (recvbuf_t*)data;
	if(!buff_ptr)
		return -1;

	if(buff_ptr->recved >= buff_ptr->recvbufsize)
		return -1;
	
	buff_ptr->recved += sprintf(buff_ptr->recvbuf + buff_ptr->recved, "%s", buff);
	return 0;
}

int ping_ip(char *ip, int packsize, int packnum, char *recvbuf, int recvbufsize, int timeout)
{
	recvbuf_t buff;
	buff.recvbuf = recvbuf;
	buff.recvbufsize = recvbufsize;
	buff.recved = 0;

	ping_opt_t opt;
	opt.data = &buff;
	opt.dest = ip;
	opt.handle = echo_result;
	opt.pack_num = packnum;
	opt.timeout = timeout;

	HANDLE ping_handle;
	if(ping_init(&ping_handle, &opt) != -1)		
		ping(ping_handle);
	ping_realse(&ping_handle);

	return 0;
}


