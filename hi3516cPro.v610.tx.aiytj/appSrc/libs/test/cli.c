#include <stdlib.h>
#include <stdio.h>
#include "socket_api.h"


int main()
{
	net_sockfd_t sockobj;
	if(net_tcp_connect(&sockobj, IPV4, "192.168.0.27", 8000, 3) < 0)
		return -1;

	char buf[128];	
	int len = sprintf(buf, "hello world\n");
	
	while(net_read_write_timeout(&sockobj, 'w', 3)< 0);
	net_sendn(&sockobj, buf, len);

	while(net_read_write_timeout(&sockobj, 'r', 3)< 0);
	len = net_recv(&sockobj, buf, sizeof(buf) - 1);
	if(0 >= len){
		net_close(&sockobj);
		return -1;
	}
	buf[len] = 0;
	printf("%s\n", buf);
	
	net_close(&sockobj);	
	return 0;
}

