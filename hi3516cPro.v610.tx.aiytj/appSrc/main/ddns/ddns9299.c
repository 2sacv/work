#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

#include "ddns9299.h"
#include "socket_api.h"
#include "debug.h"

#if defined (DEV_TYPE_ENHANCED)

#define DDNS_9299_SERVER_DOMAIN "www.9299.org"
#define DDNS_9299_IP_PROBE "ip.9299.org"

int ddns_9299_ip_check(char *ip, int timeout, char status[/*32*/])
{
    assert(ip);
    char ipaddr[16];
    if(!net_domain_to_ipaddr(DDNS_9299_IP_PROBE, IPV4, TCP, ipaddr, sizeof(ipaddr)))
        return -1;

    net_sockfd_t sockobj;
    if(net_tcp_connect(&sockobj, IPV4, ipaddr, 80, timeout) < 0) {
        strcpy(status, "connect server error");
        net_close(&sockobj);
        return -1;
    }

    char ddnsbuf[512];
    int len = sprintf(ddnsbuf, "GET / HTTP/1.1\r\n"
                      "Host: ip.9299.org\r\n\r\n");

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

    char szNeedle[] = "Current IP Address:</font>";
    int nread = 0, r;
    len = sizeof(ddnsbuf) - 1;
    ddnsbuf[0] = 0;
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
        if(strstr(ddnsbuf, szNeedle) || strstr(ddnsbuf, "\r\n\r\n"))
            break;
    }

    DBG("%s\n", ddnsbuf);

    char* ptr;
    if (NULL != (ptr = strstr(ddnsbuf, szNeedle))) {
        ptr += strlen(szNeedle);
        strcpy(ip, ptr);
    } else {
        net_close(&sockobj);
        return -1;
    }

    net_close(&sockobj);
    return 0;
}


int ddns_9299_update(ddns_opt_t* opt, int timeout, char status[/*32*/])
{
    assert(status);
    char ipaddr[16];
    if(!net_domain_to_ipaddr(DDNS_9299_SERVER_DOMAIN, IPV4, TCP, ipaddr, sizeof(ipaddr)))
        return -1;

    net_sockfd_t sockobj;
    if(net_tcp_connect(&sockobj, IPV4, ipaddr, 80, timeout) < 0) {
        strcpy(status, "connect server error");
        net_close(&sockobj);
        return -1;
    }

    char ddnsbuf[512];
    int len = sprintf(ddnsbuf, "GET /upgengxin.asp?username=%s&userpwd=%s&"
                      "userdomain=%s&userport=%d&userip=%s&usermac=%s&mod=%d HTTP/1.1\r\n"
                      "Host: www.9299.org\r\n\r\n",
                      opt->user_name,
                      opt->password,
                      opt->sdomain,
                      opt->wlan_port,
                      opt->wlan_ip,
                      opt->mac_addr,
                      opt->work_mode);

    if(net_read_write_timeout(&sockobj, 'w', timeout) < 0) {
        goto OVER_TIME_EXIT;
    }

    net_sendn(&sockobj, ddnsbuf, len);
    DBG("%s\n", ddnsbuf);

    if(net_read_write_timeout(&sockobj, 'r', timeout) < 0) {
        goto OVER_TIME_EXIT;
    }

    ddnsbuf[0] = 0;
    len = sizeof(ddnsbuf) - 1;
    int nread = 0, r;
    while ( nread < len ) {
        r = read(net_sock_fd(&sockobj), ddnsbuf + nread, len - nread);
        if (0 > r) {
            if (errno == EINTR) {
                usleep(5000);
                continue;
            } else {
                goto FAIL_EXIT;
            }
        } else if (0 == r) {
            break;
        }
        nread += r;

        ddnsbuf[nread] = 0;
        char* str = strstr(ddnsbuf, "\r\n\r\n");
        if(NULL == str)
            continue;

        int headlen = str - ddnsbuf + 4;
        str = strstr(ddnsbuf, "Content-Length:");
        if(str) {
            str += strlen("Content-Length:");
            int len = atoi(str);
            if(headlen + len > strlen(ddnsbuf))
                continue;
            else
                break;
        }
    }
    DBG("%s\n", ddnsbuf);

    if(strstr(ddnsbuf, "UP") || strstr(ddnsbuf, "OK"))
        strcpy(status, "success");
    else
        strcpy(status, "failed");

    net_close(&sockobj);
    return 0;

OVER_TIME_EXIT:
    net_close(&sockobj);
    strcpy(status, "failed");
    return -1;

FAIL_EXIT:
    net_close(&sockobj);
    strcpy(status, "failed");
    return -1;
}

#endif

