#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include "ddns3322.h"
#include "debug.h"
#include "base64.h"
#include "socket_api.h"

#if defined (DEV_TYPE_ENHANCED)

#define DDNS_3322_SERVER_DOMAIN "members.3322.net"

int ddns_3322_update(ddns_opt_t *opt, int timeout, char status[/*32*/])
{
    assert(status);
    char ipaddr[16];
    if(!net_domain_to_ipaddr(DDNS_3322_SERVER_DOMAIN, IPV4, TCP, ipaddr, sizeof(ipaddr)))
        return -1;

    net_sockfd_t sockobj;
    if(net_tcp_connect(&sockobj, IPV4, ipaddr, 80, timeout) < 0) {
        strcpy(status, "connect server error");
        net_close(&sockobj);
        return -1;
    }

    char ddnsbuf[512];
    char auth[128] = {0}, base64_auth[175] = {0};
    int len = sprintf(auth, "%s:%s", opt->user_name, opt->password);
    base64encode(base64_auth, &len, auth, len);

    len = sprintf(ddnsbuf, "GET /dyndns/update?hostname=%s HTTP/1.1\r\n"
                  "Host: %s\r\n"
                  "Authorization: Basic %s\r\n"
                  "User-Agent: jabsco_nxp/1.0\r\n\r\n", opt->sdomain, DDNS_3322_SERVER_DOMAIN, base64_auth);

    if(net_read_write_timeout(&sockobj, 'w', timeout) < 0) {
        strcpy(status, "overtime");
        net_close(&sockobj);
        return -1;
    }

    net_sendn(&sockobj, ddnsbuf, len);
    DBG("%s\n", ddnsbuf);

    if(net_read_write_timeout(&sockobj, 'r', timeout) < 0) {
        strcpy(status, "overtime");
        net_close(&sockobj);
        return -1;
    }

    ddnsbuf[0] = 0;
    int nread = 0, r;
    len = sizeof(ddnsbuf) - 1;
    while ( nread < len ) {
        r = read(net_sock_fd(&sockobj), ddnsbuf + nread, len - nread);
        if (0 > r) {
            if (errno == EINTR) {
                usleep(5000);
                continue;
            } else {
                strcpy(status, "failed");
                net_close(&sockobj);
                return -1;
            }
        } else if (0 == r) {
            break;
        }
        nread += r;
        ddnsbuf[nread] = 0;
        if(strstr(ddnsbuf, "\r\n0") || strstr(ddnsbuf, "\r\n\r\n"))
            break;
    }
    DBG("%s\n", ddnsbuf);

    if(strstr(ddnsbuf, "good") || strstr(ddnsbuf, "nochg"))
        strcpy(status, "success");
    else
        strcpy(status, "failed");

    net_close(&sockobj);
    return 0;
}

#endif

