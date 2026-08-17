#include <stdlib.h>
#include <stdio.h>
#include "socket_api.h"


int main()
{
	net_sockfd_t sockobj;
	if(net_tcp_server(&sockobj, IPV4, NULL, 8000) < 0)
		return -1;
	
	while(1){
		net_sockfd_t sockconn;
		if(net_read_write_timeout(&sockobj, 'r', 3)< 0)
			continue;
		
		if(net_accept(&sockobj, &sockconn) < 0){
			continue;
		}

		char ip[128];
		unsigned short port;
		if(!net_get_ipaddr_port(&net_remote_addr(&sockconn), ip, sizeof(ip), &port)){
			net_close(&sockconn);
			continue;
		}
		
		printf("accept client: %s : %d\n", ip, port);
		char buf[128];
		int len = sprintf(buf, "welcome client : %s\n", ip);
		
		while(net_read_write_timeout(&sockconn, 'w', 3)< 0);
		net_sendn(&sockconn, buf, len);

		while(net_read_write_timeout(&sockconn, 'r', 3)< 0);
		len = net_recv(&sockconn, buf, sizeof(buf) - 1);
		if(0 >= len){
			net_close(&sockconn);
			continue;
		}
		buf[len] = 0;
		printf("%s\n", buf);

		net_close(&sockconn);		
	}
	
	return 0;
}
