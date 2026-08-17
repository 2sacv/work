/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    :net_check.c
 * @Created Time : 2014-02-25
 * @Version      : 1.0
 * @Author       : cheby
 * @Description  : copy from network
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <stdarg.h>
#include <assert.h>
#include <poll.h>
#include <netdb.h>

#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <arpa/inet.h>

#include  <fcntl.h>
#include  <linux/sockios.h>
#include  <linux/ethtool.h>

#include "net_check.h"
#include "debug.h"
#include "alarmapi.h"
#include "jconfig.h"
#include "conf_list.h"
#include "jconfstruct.h"
#include "jevent.h"
#include "socket_api.h"
#include "logapi.h"
#include "utils.h"
#include "net_config.h"
#include "system_ctrl.h"
#include "js_scheduler.h"
#include "confapi.h"
#include "jdns.h"
#include "sim4g.h"
#include "system_sch.h"
#include "pthread_manage.h"

#ifdef PLATFORM_TENCENT
#include "tencent_server.h"
#endif

#define MAC_BCAST_ADDR            (unsigned char *) "\xff\xff\xff\xff\xff\xff"
#define ETH_INTERFACE             "eth0"
#define INTERNET_MATCH            "/opt/net_match"
#define CONNECTION_BREAK_DELAY    (60 * 3)
#define INTERVAL_ALLOW_IP_CHANGE  (30)
#define CHECK_IP_INTERVAL         (30*1000)
#define IP_STATUS_INTERVAL        (1500)
#define IP_ADA_EXPIRE             (60 * 60 * 24)
#define IP_ADA_BROAD_PORT         1234
#define IP_ADA_BROAD_INTERVAL     2
#define IP_ADA_CHECK_INTERVAL     10
#define IP_ADA_BROAD_TIMEOUT      5
#define IP_ADA_DETECT_TIMEOUT     20
#define IP_ADA_VERSION            1

extern "C" {
int get_rtsp_connecting_nums(void);
}

static int net_arp_init(void *data);
static int net_arp_uninit();

typedef enum {
    IPINFO_TYPE_BEGIN = -1,
    IPINFO_TYPE_NOTE,
    IPINFO_TYPE_ADDR,
    IPINFO_TYPE_END
} IPINFO_TYPE_E;

typedef struct {
    pthread_mutex_t mutex;
    int bExit;
    int bEnable;                    // 是否启用自适应IP
    pthread_t ptThread;
    JSScheduler  fScheduler;
    net_sockfd_t sockObjSvr;
    net_sockfd_t sockObjCli;
    unsigned char macAddr[6];       // 本机MAC地址
    unsigned char reserved[2];
    JSTCHandle m_broadToken;        // 发送广播包定时器
    JSTCHandle soc;                 //

    struct timespec tsIPInfoLast;   // 最后一次收到广播地址信息时间，初始化时为零
    unsigned int bIpUsed[8];        // IP地址占用记录0~255
    int bIpValid;                   // bIpUsed是否有效

    u_int32_t nAddrSvr;             // 服务器IP
    int bAddrSvrChg;                // nAddrSvr是否改变
} IP_ADAPTIVE_S;

typedef struct {
    unsigned char version;
    unsigned char type;
    unsigned char macAddr[6];
    unsigned int bIpUsed[8];        // IP地址占用记录0~255
} IPINFO_PKG_S;

static JSTCHandle  hdl_ip = NULL;
static JSTCHandle  hdl_arp = NULL;
static IP_ADAPTIVE_S g_ipAda = {PTHREAD_MUTEX_INITIALIZER, };
static int g_arp_socked = 0;
static JSTCHandle  hdl_watch_def_route = NULL;

// 参数分别表示 网卡设备类型 接口检索索引 主机IP地址 主机arp地址
static int read_interface(const char *interfaces, int *ifindex, u_int32_t *addr, unsigned char *arp)
{
    int fd = -1;
    struct ifreq ifr;
    int ret = -1;

    if(ifindex == NULL || addr == NULL || arp == NULL)
        return -1;

    memset(&ifr, 0, sizeof(struct ifreq));
    ifr.ifr_addr.sa_family = AF_INET;
    strcpy(ifr.ifr_name, interfaces);
    do {
        fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
        if (fd < 0)
            break;

        if (ioctl(fd, SIOCGIFADDR, &ifr) == 0) {
            struct sockaddr_in *our_ip = (struct sockaddr_in *) &ifr.ifr_addr;
            *addr = our_ip->sin_addr.s_addr;
            //if (_io_timer) DBG("%s (our ip) = %s\n", ifr.ifr_name, inet_ntoa(our_ip->sin_addr));
        } else {
            ERR("SIOCGIFADDR interfaces:%s failed, is the interface up and configured?: %s\n",
                interfaces, strerror(errno));
            break;
        }

        if (ioctl(fd, SIOCGIFINDEX, &ifr) == 0) {
            //if (_io_timer) DBG("interfaces:%s dapter index %d\n", interfaces, ifr.ifr_ifindex);
            *ifindex = ifr.ifr_ifindex;
        } else {
            ERR("SIOCGIFINDEX interfaces:%s failed!: %s\n", interfaces, strerror(errno));
            break;
        }
        if (ioctl(fd, SIOCGIFHWADDR, &ifr) == 0) {
            memcpy(arp, ifr.ifr_hwaddr.sa_data, 6);
            //if (_io_timer) DBG("interfaces:%s adapter hardware address %02x:%02x:%02x:%02x:%02x:%02x\n",
            //   interfaces, arp[0], arp[1], arp[2], arp[3], arp[4], arp[5]);
        } else {
            ERR("SIOCGIFHWADDR interfaces:%s failed!: %s\n", interfaces, strerror(errno));
            break;
        }

        ret = 0;
    } while(0);

    if(fd >= 0)
        close(fd);

    return ret;
}

// 参数说明 目标IP地址，本机IP地址，本机mac地址，网卡类型
// 返回值，-1表示失败，0表示冲突，1表示可用
static int arpping_s(u_int32_t yiaddr, u_int32_t ip, unsigned char *mac, char *interfaces)
{
    int ms_timeout = 40*1000;      // 3s as us
    struct timeval tm = {.tv_sec = ms_timeout / 1000};
    int optval = 1;
    int s;                      /* socket */
    int rv = 1;                 /* return value */
    int ret = 0;
    struct sockaddr addr;       /* for interface name */
    struct arpMsg arp;
    fd_set fdset;
    struct timespec tsPrev;

    if ((s = socket (PF_PACKET, SOCK_PACKET, htons(ETH_P_ARP))) == -1) {
        DBG("Could not open raw socket\n");
        return -1;
    }

    if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval)) == -1) {
        DBG("Could not setsocketopt on raw socket\n");
        close(s);
        return -1;
    }

    /* 对arp设置，这里按照arp包的封装格式赋值即可， */
    memset(&arp, 0, sizeof(arp));
    memcpy(arp.ethhdr.h_dest, MAC_BCAST_ADDR, 6);   /* MAC DA */
    memcpy(arp.ethhdr.h_source, mac, 6);        /* MAC SA */
    arp.ethhdr.h_proto = htons(ETH_P_ARP);      /* protocol type (Ethernet) */
    arp.htype = htons(ARPHRD_ETHER);        /* hardware type */
    arp.ptype = htons(ETH_P_IP);            /* protocol type (ARP message) */
    arp.hlen = 6;                   /* hardware address length */
    arp.plen = 4;                   /* protocol address length */
    arp.operation = htons(ARPOP_REQUEST);       /* ARP op code */
    memcpy(arp.sInaddr, &ip, sizeof(arp.sInaddr)); /* source IP address */
    memcpy(arp.sHaddr, mac, 6);         /* source hardware address */
    memcpy(arp.tInaddr, &yiaddr, sizeof(arp.tInaddr)); /* target IP address */

    memset(&addr, 0, sizeof(addr));
    strcpy(addr.sa_data, interfaces);
    if (sendto(s, &arp, sizeof(arp), 0, &addr, sizeof(addr)) < 0) {
        close(s);
        return -1;
    }

    /* wait arp reply, and check it */
    ms_clock_reset(&tsPrev); 
    while (!ms_clock_is_timeup(&tsPrev, ms_timeout)) {
        FD_ZERO(&fdset);
        FD_SET(s, &fdset);
        ret = select(s + 1, &fdset, (fd_set *) NULL, (fd_set *) NULL, &tm);
        if (ret < 0) {
            DBG("Error on ARPING request: %s\n", strerror(errno));
            if (errno != EINTR) rv = 0;
        } else if (ret == 0) {
            //DBG("Timeout of ARPING request\n");
        } else if (FD_ISSET(s, &fdset)) {
            u_int32_t ip4 = 0;
            memcpy(&ip4, arp.sInaddr, sizeof(ip4));
            if (recv(s, &arp, sizeof(arp), 0) < 0 ) {
                rv = 0;
            } else if (/*arp.operation == htons(ARPOP_REPLY) &&
                                        bcmp(arp.tHaddr, mac, 6) == 0 &&  */
                ip == yiaddr) {
                DBG("Valid arp reply receved for this address\n");
                rv = 0;
                break;
            }
        }
    }
    close(s);
    DBG("%salid arp replies for this address, rest time:%d\n", rv ? "No v" : "V", ms_timeout);
    return rv;
}

// 参数说明 目标IP地址，本机IP地址，本机mac地址，网卡类型
static int arpping_m(unsigned int *useflag, u_int32_t yiaddr, u_int32_t ip, unsigned char *mac, char *interfaces)
{
    int ms_timeout = 40*1000;      // 3s as us
    struct timeval tm = {.tv_sec = ms_timeout / 1000};
    int optval = 1;
    int s;                      // socket
    int ret = 0;
    struct sockaddr addr;       // for interface name
    struct arpMsg arp;
    fd_set fdset;
    struct timespec tsPrev;
    int i = 0;

    if ((s = socket (PF_PACKET, SOCK_PACKET, htons(ETH_P_ARP))) == -1) {
        DBG("Could not open raw socket\n");
        return -1;
    }

    if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval)) == -1) {
        DBG("Could not setsocketopt on raw socket\n");
        close(s);
        return -1;
    }

    // 对arp设置，这里按照arp包的封装格式赋值即可
    memset(&arp, 0, sizeof(arp));
    memcpy(arp.ethhdr.h_dest, MAC_BCAST_ADDR, 6);   // MAC DA
    memcpy(arp.ethhdr.h_source, mac, 6);            // MAC SA
    arp.ethhdr.h_proto = htons(ETH_P_ARP);          // protocol type (Ethernet)
    arp.htype = htons(ARPHRD_ETHER);                // hardware type
    arp.ptype = htons(ETH_P_IP);                    // protocol type (ARP message)
    arp.hlen = 6;                                   // hardware address length
    arp.plen = 4;                                   // protocol address length
    arp.operation = htons(ARPOP_REQUEST);           // ARP op code
    memcpy(arp.sInaddr, &ip, sizeof(arp.sInaddr));  // source IP address
    memcpy(arp.sHaddr, mac, 6);                     // source hardware address
    memcpy(arp.tInaddr, &yiaddr, sizeof(arp.tInaddr));  // target IP address

    for (i = 1; i < 255; i++) {
        memset(&addr, 0, sizeof(addr));
        strcpy(addr.sa_data, interfaces);
        arp.tInaddr[3] = i;
        if (sendto(s, &arp, sizeof(arp), 0, &addr, sizeof(addr)) < 0) {
            DBG("sendto failed %s\n", strerror(errno));
        }
    }

    // wait arp reply, and check it
    ms_clock_reset(&tsPrev); 
    while (!ms_clock_is_timeup(&tsPrev, ms_timeout)) {
        FD_ZERO(&fdset);
        FD_SET(s, &fdset);
        ret = select(s + 1, &fdset, (fd_set *) NULL, (fd_set *) NULL, &tm);
        if (ret < 0) {
            DBG("Error on ARPING request: %s\n", strerror(errno));
            if (errno != EINTR) {
            }
        } else if (ret == 0) {
            //DBG("Timeout of ARPING request\n");
        } else if (FD_ISSET(s, &fdset)) {
            if (recv(s, &arp, sizeof(arp), 0) < 0 ) {
            } else if (!memcmp(arp.sInaddr, &yiaddr, 3)) {
                //DBG("Valid arp reply receved for address %d.%d.%d.%d\n",
                //        arp.sInaddr[0], arp.sInaddr[1], arp.sInaddr[2], arp.sInaddr[3]);
                useflag[arp.sInaddr[3] / 32] |= 1 << (arp.sInaddr[3] % 32);
            }
        }
    }
    close(s);
    return 0;
}

static int net_adaptive_search(unsigned int *bIpUsed, const char *ipaddr)
{
    u_int32_t addr = inet_addr(ipaddr);
    u_int32_t server = 0;   // Our IP, in network order
    unsigned char arp[6];   // Our arp address
    int ifindex = 0;        // Index number of the interface to use

    // 读以太网接口函数，获取一些配置信息
    if (read_interface(ETH_INTERFACE, &ifindex, &server, arp) < 0) {
        return FAILURE;
    }

    // IP检测函数
    arpping_m(bIpUsed, addr, server, arp, (char *)ETH_INTERFACE);
    return SUCCESS;
}

static int net_adaptive_setused(unsigned int *bIpUsed, unsigned char subIp)
{
    bIpUsed[subIp / 32] |= 1 << (subIp % 32);
    return SUCCESS;
}

static int net_adaptive_select(unsigned int *bIpUsed, unsigned char *pSubIp, const unsigned char *mac)
{
    int i = 0;
    unsigned char subIp = 0;
    unsigned char bgIp = rand()*mac[5];

    for (i = bgIp; i < bgIp + 255; i++) {
        subIp = i;
        if (0 == subIp || 1 == subIp || 254 == subIp || 255 == subIp) {
            continue;
        }
        if (!((bIpUsed[subIp / 32] >> (subIp % 32)) & 0x01)) {
            break;
        }
    }

    if (i < bgIp + 255 && 0 != subIp && 1 != subIp && 254 != subIp && 255 != subIp) {
        DBG("get free Ip %d\n", subIp);
        *pSubIp = subIp;
        bIpUsed[subIp / 32] |= 1 << (subIp % 32);
        return SUCCESS;
    }

    return FAILURE;
}

static void net_adaptive_recv_cb(int fd, int ev, void *instances)
{
    int ret = 0;
    int sock = -1;
    struct sockaddr_in addr;
    int len = 0;
    IPINFO_PKG_S ipInfo;

    sock = net_sock_fd((net_sockfd_t*)instances);

    do {
        memset(&ipInfo, 0, sizeof(ipInfo));
        ret = recvfrom(sock, &ipInfo, sizeof(ipInfo), 0, (struct sockaddr *)&addr, (socklen_t *)&len);
        if (ret < 0) {
            break;
        }

        if (IP_ADA_VERSION != ipInfo.version || IPINFO_TYPE_BEGIN >= ipInfo.type || IPINFO_TYPE_END <= ipInfo.type) {
            DBG("recieve incorrect ver=%d type=%d\n", ipInfo.version, ipInfo.type);
            continue;
        }

        ret = memcmp(g_ipAda.macAddr, ipInfo.macAddr, sizeof(g_ipAda.macAddr));
        if (0 == ret) {
            continue;
        } else if (0 < ret) {
            //DBG("recieve form little mac\n");
            ms_clock_reset(&g_ipAda.tsIPInfoLast);
        }

        if (IPINFO_TYPE_ADDR == ipInfo.type) {
            //DBG("recieve addr update\n");
            pthread_mutex_lock(&g_ipAda.mutex);
            memcpy(g_ipAda.bIpUsed, ipInfo.bIpUsed, sizeof(g_ipAda.bIpUsed));
            g_ipAda.bIpValid = TRUE;
            pthread_mutex_unlock(&g_ipAda.mutex);
        }
    } while(1);
}

int net_adaptive_broad_send(const IPINFO_TYPE_E type, const unsigned int *bIpUsed)
{
    IPINFO_PKG_S ipInfo;

    // build packet
    memset(&ipInfo, 0, sizeof(ipInfo));
    ipInfo.version = IP_ADA_VERSION;
    ipInfo.type = type;
    memcpy(ipInfo.macAddr, g_ipAda.macAddr, sizeof(ipInfo.macAddr));
    if (IPINFO_TYPE_ADDR == type) {
        memcpy(ipInfo.bIpUsed, bIpUsed, sizeof(ipInfo.bIpUsed));
    }

    // send packet
    if (0 > net_sendto(&g_ipAda.sockObjCli, &ipInfo, sizeof(ipInfo), (char *)"255.255.255.255", IP_ADA_BROAD_PORT)) {
        DBG("udp sendto failed\n");
    }

    return SUCCESS;
}

static void net_adaptive_broad_task(void *instance)
{
    // 重新调度任务
    if (!(g_ipAda.bEnable && !g_ipAda.bIpValid)) {
        static int tick = 0;
        if ((tick++)%2 != 0 ) { // nTimeout = IP_ADA_BROAD_INTERVAL*1000;
            return;
        }
    }

    // 动态创建、销毁监听端口
    if (g_ipAda.bEnable) {
        if (0 >= net_sock_fd(&g_ipAda.sockObjSvr)) {
            if (net_udp_server(&g_ipAda.sockObjSvr, IPV4, NULL, IP_ADA_BROAD_PORT) < 0) {
                LOG("net_udp_server failed!\n");
            } else if (bcast_join(&g_ipAda.sockObjSvr) < 0) {
                LOG("bcast_join failed!\n");
                net_close(&g_ipAda.sockObjSvr);
            } else {
                ms_clock_reset(&g_ipAda.tsIPInfoLast);
                js_create_reader_r(g_ipAda.fScheduler, net_sock_fd(&g_ipAda.sockObjSvr),
                        JS_READABLE, net_adaptive_recv_cb, (void*)&g_ipAda.sockObjSvr, &g_ipAda.soc);
            }
        }
    } else {
        if (0 < net_sock_fd(&g_ipAda.sockObjSvr)) {
            js_delete_reader_r(&g_ipAda.soc);
            net_close(&g_ipAda.sockObjSvr);
            memset(&g_ipAda.tsIPInfoLast, 0, sizeof(g_ipAda.tsIPInfoLast));
        }
    }

    //
    if (g_ipAda.bEnable) {
        pthread_mutex_lock(&g_ipAda.mutex);
        if (sec_since_previous(&g_ipAda.tsIPInfoLast) > IP_ADA_BROAD_TIMEOUT) {
            if (g_ipAda.bIpValid) { // 群主会超时，发送地址广播包
                net_adaptive_broad_send(IPINFO_TYPE_ADDR, g_ipAda.bIpUsed);
            } else {    // 所有成员发送提示包
                net_adaptive_broad_send(IPINFO_TYPE_NOTE, NULL);
            }
        }
        pthread_mutex_unlock(&g_ipAda.mutex);
    }
}

static void *net_adaptive_check(void *data)
{
    int rtsp_exists_connection = 0;
    int interval_allow_ip_change = INTERVAL_ALLOW_IP_CHANGE;
    uint32_t my_ip0 = 0, my_ip1 = 0, my_ip2 = 0, my_ip3 = 0;
    uint32_t out_ip0 = 0, out_ip1 = 0, out_ip2 = 0, out_ip3 = 0;
    time_t interval = 0;
    long nTimeIpCheck = 0;  // IP冲突检测时间

    static time_t time_ip_change = 0;
    static int rtsp_exists_connection_prev = 0;

    struct timespec tsNow = {0};
    NetEthS eth = {0};


	DBG("net_adaptive_check, pid=%d, pthread_self(%lu)\n", getpid(), pthread_self());
    while (!g_ipAda.bExit) {
        sleep(2);

        // 检查IP自适应开关
        get_config(handleEthCfg, eth);
        if (!eth.ipadaen) {
            g_ipAda.bEnable = FALSE;
            continue;
        } else {
            g_ipAda.bEnable = TRUE;
        }

        ms_clock_reset(&tsNow);

        rtsp_exists_connection = get_rtsp_connecting_nums();

        if (1 == rtsp_exists_connection_prev && 0 == rtsp_exists_connection) {
            time_ip_change = mono_stamp();
            interval_allow_ip_change = CONNECTION_BREAK_DELAY;
        }

        if (CONNECTION_BREAK_DELAY == interval_allow_ip_change && 
            abs(mono_stamp() - time_ip_change) > CONNECTION_BREAK_DELAY) {
            interval_allow_ip_change = INTERVAL_ALLOW_IP_CHANGE;
        }

        // 搜索服务器IP改变
        interval = abs(mono_stamp() - time_ip_change);
        //DBG("interval:%ld, interval_allow_ip_change:%d, rtsp_exits_connection:%d, rtsp_exits_connection_prev:%d\n", 
        //    interval, interval_allow_ip_change, rtsp_exists_connection, rtsp_exists_connection_prev);
        if (g_ipAda.bAddrSvrChg && interval >= interval_allow_ip_change && 0 == rtsp_exists_connection) {
            LOG("check svraddr=%d.%d.%d.%d local=%s\n",
                    (g_ipAda.nAddrSvr >> 24) & 0xFF,
                    (g_ipAda.nAddrSvr >> 16) & 0xFF,
                    (g_ipAda.nAddrSvr >> 8) & 0xFF,
                    g_ipAda.nAddrSvr & 0xFF,
                    eth.ip);

            pthread_mutex_lock(&g_ipAda.mutex);
            memset(g_ipAda.bIpUsed, 0, sizeof(g_ipAda.bIpUsed));
            g_ipAda.bIpValid = FALSE;
            g_ipAda.bAddrSvrChg = FALSE;

            sscanf(eth.ip, "%u.%u.%u.%u", &my_ip0, &my_ip1, &my_ip2, &my_ip3);
            DBG("analyse result: %u.%u.%u.%u\n", my_ip0, my_ip1, my_ip2, my_ip3);
            out_ip0 = (g_ipAda.nAddrSvr >> 24) & 0xFF;
            out_ip1 = (g_ipAda.nAddrSvr >> 16) & 0xFF;
            out_ip2 = (g_ipAda.nAddrSvr >> 8) & 0xFF;
            out_ip3 = g_ipAda.macAddr[5];
            if (my_ip0 == out_ip0 && my_ip1 == out_ip1 && my_ip2 == out_ip2) { //同网段不再改变 ip
                rtsp_exists_connection_prev = rtsp_exists_connection;
                DBG("the same network segment, ignore ip change\n");
                continue;
            }

            if (out_ip3 > 1 && out_ip3 < 254) {
                sprintf(eth.ip, "%d.%d.%d.%d", out_ip0, out_ip1, out_ip2, out_ip3);    // 防止IP冲突检测机制失效，导致出厂217地址冲突
            } else {
                u_int32_t addrMy = 0;
                addrMy = ntohl(inet_addr(eth.ip));
                sprintf(eth.ip, "%d.%d.%d.%d", out_ip0, out_ip1, out_ip2, addrMy & 0xFF);
            }
            sprintf(eth.gw, "%d.%d.%d.%d", out_ip0, out_ip1, out_ip2, 1);
            pthread_mutex_unlock(&g_ipAda.mutex);

            nTimeIpCheck = tsNow.tv_sec - IP_ADA_CHECK_INTERVAL + 5;    // 5s 是因为设置IP地址会有延时
            set_config(handleEthCfg, eth);
            DBG("follow to addr=%s gw=%s\n", eth.ip, eth.gw);
            time_ip_change = mono_stamp();
        }

        if (!g_ipAda.bIpValid
            && 0 != g_ipAda.tsIPInfoLast.tv_sec
            && g_ipAda.tsIPInfoLast.tv_sec + IP_ADA_DETECT_TIMEOUT < tsNow.tv_sec) {// 群主会超时，发送探测包
            unsigned int bIpUsed[8];
            memset(bIpUsed, 0, sizeof(bIpUsed));
            if (SUCCESS == net_adaptive_search(bIpUsed, eth.ip)) {
                // update record
                pthread_mutex_lock(&g_ipAda.mutex);
                memcpy(g_ipAda.bIpUsed, bIpUsed, sizeof(g_ipAda.bIpUsed));
                g_ipAda.bIpValid = TRUE;
                pthread_mutex_unlock(&g_ipAda.mutex);
            }
        }

        if (nTimeIpCheck + IP_ADA_CHECK_INTERVAL < tsNow.tv_sec) {    // 检查IP冲突
            nTimeIpCheck = tsNow.tv_sec;
            if (get_arp_conflict_flag() == FALSE) {
                rtsp_exists_connection_prev = rtsp_exists_connection;
                continue;
            }
            LOG("ip conflict of ip=%s\n", eth.ip);

            if (!g_ipAda.bIpValid) {
                unsigned int bIpUsed[8];
                memset(bIpUsed, 0, sizeof(bIpUsed));
                if (SUCCESS == net_adaptive_search(bIpUsed, eth.ip)) {
                    // update record
                    pthread_mutex_lock(&g_ipAda.mutex);
                    memcpy(g_ipAda.bIpUsed, bIpUsed, sizeof(g_ipAda.bIpUsed));
                    g_ipAda.bIpValid = TRUE;
                    pthread_mutex_unlock(&g_ipAda.mutex);
                }
            }

            if (g_ipAda.bIpValid) {
                struct in_addr addr_tmp;
                unsigned char subIp = 0;

                subIp = ntohl(inet_addr(eth.ip)) & 0xFF;
                net_adaptive_setused(g_ipAda.bIpUsed, subIp);   // 计入冲突IP
                
                if (SUCCESS == net_adaptive_select(g_ipAda.bIpUsed, &subIp, g_ipAda.macAddr)) {
                    net_adaptive_broad_send(IPINFO_TYPE_ADDR, g_ipAda.bIpUsed);

                    addr_tmp.s_addr = inet_addr(eth.ip);
                    ((unsigned char *)&addr_tmp.s_addr)[3] = subIp;
                    memset(eth.ip, 0, sizeof(eth.ip));
                    strncpy(eth.ip, j_inet_ntoa(addr_tmp), sizeof(eth.ip) - 1);
                    set_config(handleEthCfg, eth);
                    DBG("select to addr=%s\n", eth.ip);
                } else {
                    DBG("no addr find\n");
                }
            }
        }

        rtsp_exists_connection_prev = rtsp_exists_connection;
    }

    return NULL;
}

static int net_adaptive_verify(const unsigned int ipaddr)
{
    unsigned char uIp[4];

    if (!ipaddr) {
        return FAILURE;
    }
    memcpy(uIp, &ipaddr, sizeof(uIp));

    // 1~223.0~254.0~254.1~254
    if (0xFF == uIp[0] || 0xFF == uIp[1] || 0xFF == uIp[2] || 0xFF == uIp[3]) {
        return FAILURE;
    }

    if (0 == uIp[3] || 223 < uIp[3] || 127 == uIp[3] || 0 == uIp[0]) {
        return FAILURE;
    }

    return SUCCESS;
}

/*
static void net_check(void *data)
{
    NetEthS eth = {{0,},};

    get_config(handleEthCfg, eth);
    if (eth.ipadaen) {
        return;
    }

    if (SUCCESS == net_check_ip(eth.ip)) {
        DBG("ip can use\n");
    } else {
        alarm_report(JALARM_TYPE_IP_CONFLICT, 0, NO_NEED_TIME_CHECK, "ip conflict");
        DBG("ip conflict\n");
    }
}
*/

/*华硕路由器初始连接时会有二次连接导致内核网络mac层交互异常，默认路由丢失需要修复*/
static void check_and_repair_getway(void *data)
{
	NetEthS stNetEthcfg = {{0},};
	conf_get_ethcfg(&stNetEthcfg);
	if(!stNetEthcfg.enable && get_g_sys(usb_4g)) {
		return;			//4g设备且关闭网口
	}

	//dhcp模式下会自动获取IP并配置dhcp，无需看守，否则会导致网络异常重新载入导致烧录ID失败
	if (stNetEthcfg.dhcpen == 1){
		return;
	}
	
    char buf[8] = {0};
    ReadCmdResult("route -n |grep UG |wc -l", buf, sizeof(buf));
    if(buf[0] != '1') {
		DBG("no default route ,need restart networking\n");
        UtilSystemCmd((char *)"networking init &");
    }
}

void net_status(void *data)
{
    static int g_last_net_status = 0xBADF00D;
	NetEthS eth = {{0},};

    if (is_okey("/sys/class/net/eth0/statistics/tx_bytes")) {
	    get_config(handleEthCfg, eth);
    } else {
        // 只在产测及有 eth0 时，才进行 link 检查
        return;
    }

    if (g_last_net_status == 0xBADF00D) {
        g_last_net_status = net_link_status(eth.nic);
        SYSLOG("init eth0 %s\n", g_last_net_status == 1 ? "up" : "down");
        return;
    }

    int ret = net_link_status(eth.nic);
    if (g_last_net_status == ret) {
        return;
    }
	
    if (1 == ret) {
        SYSLOG("interface link up @%s %s\n", get_timestr(), eth.nic);
        send_event(JEvent_AlarmCableNormal);
    } else if (0 == ret) {
		SYSLOG("interface link down @%s\n", get_timestr());
        alarm_report(JALARM_TYPE_CABLE_DISC, 0, NO_NEED_TIME_CHECK, "cable broke");
    } else {
        SYSLOG("error:%d, details can check errno\n", ret);
    }

    g_last_net_status = ret;
}

//网卡与网线连接状态
// if_name like "ath0", "eth0". Notice: call this function
// need root privilege.
// return value:
// -1 -- error , details can check errno
// 1 -- interface link up
// 0 -- interface link down.
//#define ETHTOOL_GLINK       0x0000000a
int net_link_status(const char *eth_name)
{
    if (NULL == eth_name) {
        return -1;
    }

	if(!get_g_sys(eth)) {
		return 0;
	}

    int skfd;
    struct ifreq ifr;
    struct ethtool_value edata;
    edata.cmd = ETHTOOL_GLINK;
    edata.data = 0;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, eth_name, sizeof(ifr.ifr_name) - 1);
    ifr.ifr_data = (char *)&edata;
    if ((skfd = socket( AF_INET, SOCK_DGRAM, 0 )) <= 0) {
        DBG("socket error\n");
        return -1;
    }
 
    if (ioctl( skfd, SIOCETHTOOL, &ifr ) == -1) {
        DBG("ioctl SIOCETHTOOL errno\n");
        close(skfd);
        return -1;
    }
    close(skfd);
    return edata.data;
}

//该函数，暂时不用，可能探测不到设备
int net_check_ip(const char *ipaddr)
{
    u_int32_t addr = inet_addr(ipaddr);
    u_int32_t server = 0;   // Our IP, in network order
    unsigned char arp[6];   // Our arp address
    int ifindex = 0;        // Index number of the interface to use
    int ret = 0;

    // 读以太网接口函数，获取一些配置信息
    if (read_interface(ETH_INTERFACE, &ifindex, &server, arp) < 0) {
        return FAILURE;
    }

    // IP检测函数
    ret = arpping_s(addr, server, arp, (char *)ETH_INTERFACE);
    if (0 == ret) {
        struct in_addr temp;
        temp.s_addr = addr;
        DBG("%s belongs to someone\n", j_inet_ntoa(temp));
        return FAILURE;
    } else if (1 == ret) {
        return SUCCESS;
    }

    return FAILURE;
}

int net_adaptive_server(const char *ipaddr)
{
    NetEthS eth = {{0,},};
    u_int32_t addrSvr = 0;
    u_int32_t addrMy = 0;

    get_config(handleEthCfg, eth);
    if (!eth.ipadaen) {
        return SUCCESS;
    }

    // 适配IP地址、网关
    addrSvr = ntohl(inet_addr(ipaddr));
    if (SUCCESS == net_adaptive_verify(addrSvr)) {
        addrMy = ntohl(inet_addr(eth.ip));
        if ((addrSvr & 0xFFFFFF00) != (addrMy & 0xFFFFFF00)) {
            pthread_mutex_lock(&g_ipAda.mutex);
            g_ipAda.nAddrSvr = addrSvr;
            g_ipAda.bAddrSvrChg = TRUE;
            pthread_mutex_unlock(&g_ipAda.mutex);
            DBG("svraddr=%s local=%s\n", ipaddr, eth.ip);
        }
    } else {
        LOG("get a incorrect ip=%s\n", ipaddr);
    }
    return SUCCESS;
}

int init_client_iplink_check(void *data)
{
    if (get_g_sys(eth)) {
        int frist_time = 15*1000;

        DBG("init_client_iplink_check2\n");
        js_create_timer_r(sch_slow, frist_time, IP_STATUS_INTERVAL, net_status, NULL, &hdl_ip);
        js_create_timer_r(sch_slow, 15*1000, CHECK_IP_INTERVAL, check_and_repair_getway, NULL, &hdl_watch_def_route);

        // IP 冲突检测, todo: 冲突时 udhcpc reset 发送 discovery x3
        net_arp_init(data);
    }

    return SUCCESS;
}

int uninit_client_iplink_check()
{
    net_arp_uninit();

    if (NULL != hdl_ip) {
        js_delete_timer_r(&hdl_ip);
    }

    if (NULL != hdl_watch_def_route) {
        js_delete_timer_r(&hdl_watch_def_route);
    }

    return SUCCESS;
}

int init_ip_adaptive(void *schedule)
{
    u_int32_t server = 0;
    int ifindex = 0;

    if (!schedule) {
        return FAILURE;
    }
    if (!get_g_sys(eth)) {
        return FAILURE;
    }

    if (read_interface(ETH_INTERFACE, &ifindex, &server, g_ipAda.macAddr) < 0) {
        return FAILURE;
    }
    if (net_socket(&g_ipAda.sockObjCli, IPV4, UDP) < 0) {
        return FAILURE;
    }
    if (bcast_join(&g_ipAda.sockObjCli) < 0) {
        return FAILURE;
    }

/*
    // 不发送到回环接口
    int optval = 0;
    if(-1 == setsockopt(net_sock_fd(&g_ipAda.sockObjCli), IPPROTO_IP, IP_MULTICAST_LOOP, &optval, sizeof(optval))) {
        ERR("setsockopt IP_MULTICAST_LOOP fail!\n");
    }*/

    net_sock_fd(&g_ipAda.sockObjSvr) = -1;
    g_ipAda.fScheduler = schedule;
    js_create_timer_r(g_ipAda.fScheduler, 1000, 1000, net_adaptive_broad_task, &g_ipAda, &g_ipAda.m_broadToken);

    if ((g_ipAda.ptThread = create_pthread("net_adaptive_check", net_adaptive_check, NULL, &g_ipAda)) == 0) {
        g_ipAda.ptThread = 0;
        return FAILURE;
    }

    return SUCCESS;
}

int uninit_ip_adaptive()
{
    g_ipAda.bExit = TRUE;
    if (g_ipAda.ptThread) {
        join_pthread(g_ipAda.ptThread);
        g_ipAda.ptThread = 0;
    }

    if (g_ipAda.fScheduler) {
        js_delete_timer_r(&g_ipAda.m_broadToken);

        if (0 < net_sock_fd(&g_ipAda.sockObjSvr)) {
            js_delete_reader_r(&g_ipAda.soc);
            net_close(&g_ipAda.sockObjSvr);
        }
        g_ipAda.fScheduler = NULL;
    }

    if (0 >= net_sock_fd(&g_ipAda.sockObjCli)) {
        net_close(&g_ipAda.sockObjCli);
    }
    return SUCCESS;
}

#if 1
static JSTCHandle  soc_arp = NULL;
static pthread_mutex_t g_arp_mutex = PTHREAD_MUTEX_INITIALIZER;
static NetEthS g_arp_eth = {{0,},};
static BOOL g_ip_conflict = FALSE;

void set_arp_network(NetEthS *eth)
{
	pthread_mutex_lock(&g_arp_mutex);
	memcpy(&g_arp_eth, eth, sizeof(NetEthS));
	pthread_mutex_unlock(&g_arp_mutex);
}

static void get_arp_network(NetEthS *eth)
{
	pthread_mutex_lock(&g_arp_mutex);
	memcpy(eth, &g_arp_eth, sizeof(NetEthS));
	pthread_mutex_unlock(&g_arp_mutex);

}

void set_arp_conflict_flag(BOOL flag)
{
	pthread_mutex_lock(&g_arp_mutex);
	g_ip_conflict = flag;
	pthread_mutex_unlock(&g_arp_mutex);
}

BOOL get_arp_conflict_flag()
{
	BOOL flag = FALSE;
	pthread_mutex_lock(&g_arp_mutex);
	flag = g_ip_conflict;
	pthread_mutex_unlock(&g_arp_mutex);
	return flag;
}

static int net_create_arp_socket()
{
	int fd = 0;
	int optval = 1;
	
    if ((fd = socket (PF_PACKET, SOCK_PACKET, htons(ETH_P_ARP))) == -1) {
        DBG("Could not open raw socket\n");
        return -1;
    }

    if (setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval)) == -1) {
        DBG("Could not setsocketopt on raw socket\n");
        close(fd);
        return -1;
    }

	return fd;
}

static void net_send_arppkg(void *data)
{
	if (g_arp_socked <= 0) {
		return;
	}	
	
    NetEthS eth = {{0,},};

    get_config(handleEthCfg, eth);
	set_arp_network(&eth);

    u_int32_t server = 0;   // Our IP, in network order
    unsigned char mac[6];   // Our arp address
    int ifindex = 0;        // Index number of the interface to use
    
    struct sockaddr addr;       /* for interface name */
    struct arpMsg arp;

	// 读以太网接口函数，获取一些配置信息
	if (read_interface(ETH_INTERFACE, &ifindex, &server, mac) < 0) {
		ERR("read_interface fail\n");
		return;
	}

	/* 对arp设置，这里按照arp包的封装格式赋值即可， */
	memset(&arp, 0, sizeof(arp));
	memcpy(arp.ethhdr.h_dest, MAC_BCAST_ADDR, 6);	/* MAC DA */
	memcpy(arp.ethhdr.h_source, mac, 6);		/* MAC SA */
	arp.ethhdr.h_proto = htons(ETH_P_ARP);		/* protocol type (Ethernet) */
	arp.htype = htons(ARPHRD_ETHER);		/* hardware type */
	arp.ptype = htons(ETH_P_IP);			/* protocol type (ARP message) */
	arp.hlen = 6;					/* hardware address length */
	arp.plen = 4;					/* protocol address length */
	arp.operation = htons(ARPOP_REQUEST);		/* ARP op code */
	memcpy(arp.sInaddr, &server, sizeof(arp.sInaddr)); /* source IP address */
	memcpy(arp.sHaddr, mac, 6); 		/* source hardware address */
	memcpy(arp.tInaddr, &server, sizeof(arp.tInaddr)); /* target IP address */

	memset(&addr, 0, sizeof(addr));
	sprintf(addr.sa_data, (char *)ETH_INTERFACE);

	
	if (sendto(g_arp_socked, &arp, sizeof(arp), 0, &addr, sizeof(addr)) < 0) {
		ERR("send to fail\n");
		return;
	}

	return;
}

static void net_arp_recv_cb(int fd, int ev, void *userdata)
{
	if (g_arp_socked <= 0)
		return;

	struct arpMsg arp;
	NetEthS eth = {{0,},};
    int i = 0;
    int readlen = 0;
	struct timespec clock;

    ms_clock_reset(&clock);

	do {
		memset(&arp, 0, sizeof(struct arpMsg));
        if (i++<20) {
            readlen = recv(g_arp_socked, &arp, sizeof(arp), 0);
            if (readlen < 0) {
                break;
            }
        } else {
            // 20 arp most
            struct arpMsg arps[10];
            while (recv(g_arp_socked, &arps, sizeof(arps), 0) == sizeof(arps));
            break;
        }
		
		get_arp_network(&eth);
		memset(eth.ip,0,sizeof(eth.ip));
		net_get_ipaddr("eth0", eth.ip, sizeof(eth.ip));
		u_int32_t yiaddr = inet_addr(eth.ip);

		char mactemp[20] = {0};
		sprintf(mactemp,"%02x:%02x:%02x:%02x:%02x:%02x",
			arp.sHaddr[0], arp.sHaddr[1], arp.sHaddr[2],arp.sHaddr[3], arp.sHaddr[4], arp.sHaddr[5]);
        u_int32_t ip4 = 0;
        memcpy(&ip4, arp.sInaddr, sizeof(ip4));
		
		//DBG("mac:%s, mactemp:%s, eth.ip:%s, sInaddr:%d, tInaddr:%d\n", eth.mac, mactemp, eth.ip, arp.sInaddr, arp.tInaddr);
		if (ip4 == yiaddr && strcasecmp(eth.mac,mactemp)) {
			if (ms_clock_is_timeup(&clock, 60*1000)) {
				alarm_report(JALARM_TYPE_IP_CONFLICT, 0, NO_NEED_TIME_CHECK, "ip conflict");
				DBG("ip conflict ============%lld\n", time(NULL)); 
			}
						
			set_arp_conflict_flag(TRUE);
		}
	}while(1);
	
	return ;
}

static int net_arp_init(void *data)
{
	int times = 10;
	do {
		g_arp_socked = net_create_arp_socket();
		if (g_arp_socked > 0) {
            break;
		}
	} while (--times > 0);
	
	if (g_arp_socked <= 0) {
        SYSLOG("%s fail", __func__);
		return -1;
    }

	set_arp_conflict_flag(FALSE);

	js_create_timer_r(sch_slow, 100, CHECK_IP_INTERVAL, net_send_arppkg, NULL, &hdl_arp);

    js_create_reader_r(sch_slow, g_arp_socked, JS_READABLE, net_arp_recv_cb, NULL, &soc_arp);

	return 0;
}

static int net_arp_uninit()
{
	if (soc_arp) {
        js_delete_reader_r(&soc_arp);
	}

    if (hdl_arp) {
        js_delete_timer_r(&hdl_arp);
    }

	if (g_arp_socked > 0) {
		close(g_arp_socked);
		g_arp_socked = 0;
	}
	return 0;
}

#endif

/* Attention:
 *      ping ok spend 3s
 *      ping not ok spend 12s
 *
 * Note:
 *      ping -w10 HOST, also spend 30 seconds when weak-network, so we USE
 *      timeout 10 ping -c3 HOST
 **/
static int is_alive_ip3(const char *ip, int count, int sec, const char *func)
{
    char szCmd[128] = {0};
    char buf[640] = {0};

    if (usbdev_busy) {
        return FALSE;
    }

    snprintf(szCmd, sizeof(szCmd) - 1, "timeout %d ping -c%d %s", sec, count, ip);
    ReadCmdResult(szCmd, buf, sizeof(buf));

    if (NULL == strstr(buf, "packets received")) {  // no output or 'bad address'
        static const char *p = NULL;
        static int i = 0;
        if (p != ip || is_inc_mod0(i, 20)) {
            p = ip;
            ERR("%s %s: %s\n", get_timestr(), __func__, buf);
        }
        goto __fail;
    }

    //只有100%丢包的情况下才判定IP不通，避免wifi信号不佳时频繁切换IP地址导致业务中断
    if (NULL != strstr(buf, ", 100% packet loss")) {
        ERR("%s: %s\n", __func__, buf);
        goto __fail;
    }

    return TRUE;

__fail:
        static int tick = 0;
        DBG("[fail[%d] ping %s from:%s] %s\n", ++tick, ip, func, buf);
        return FALSE;

}

int is_alive_ip(const char *ip, const char *func)
{
    return is_alive_ip3(ip, 3, 15, func);
}

int is_alive_ip_quick(const char *ip, const char *func)
{
    return is_alive_ip3(ip, 2, 10, func);
}

#define socket_invalid	-1
static inline int socket_select(int n, fd_set* rfds, fd_set* wfds, fd_set* efds, struct timeval* timeout)
{
    int r = select(n, rfds, wfds, efds, timeout);
    while(-1 == r && (EINTR == errno || EAGAIN == errno)) {
        r = select(n, rfds, wfds, efds, timeout);
    }
    return r;
}

static inline int socket_connect(socket_t sock, const struct sockaddr* addr, socklen_t addrlen)
{
    return connect(sock, addr, addrlen);
}

/*static int socket_select_writefds(int n, fd_set* fds, struct timeval* timeout)
{
    return socket_select(n, NULL, fds, NULL, timeout);
}*/

static int socket_select_write(socket_t sock, int timeout)
{
    int r;
    struct pollfd fds;

    fds.fd = sock;
    fds.events = POLLOUT;
    fds.revents = 0;

    r = poll(&fds, 1, timeout);
    while(-1 == r && (EINTR == errno || EAGAIN == errno))
        r = poll(&fds, 1, timeout);

    return r;
}

static int socket_select_connect(socket_t sock, int timeout)
{
    int r;
    int errcode;
    int errlen = sizeof(errcode);

    // https://linux.die.net/man/2/connect
    // The socket is nonblocking and the connection cannot be
    // completed immediately.  It is possible to select(2) or poll(2)
    // for completion by selecting the socket for writing.  After
    // select(2) indicates writability, use getsockopt(2) to read the
    // SO_ERROR option at level SOL_SOCKET to determine whether
    // connect() completed successfully (SO_ERROR is zero) or
    // unsuccessfully (SO_ERROR is one of the usual error codes
    // listed here, explaining the reason for the failure).
    r = socket_select_write(sock, timeout);
    if (1 == r) {
        r = getsockopt(sock, SOL_SOCKET, SO_ERROR, (void*)&errcode, (socklen_t*)&errlen);
        DBG("getsockopt [r = %d, errno = %d]\n", r, errno);
        return 0 == r ? errcode : errno;
    }
    return 0 == r ? ETIMEDOUT : errno;

}

static int socket_setnonblock(socket_t sock, int noblock)
{
    // 0-block, 1-no-block
    // http://stackoverflow.com/questions/1150635/unix-nonblocking-i-o-o-nonblock-vs-fionbio
    // Prior to standardization there was ioctl(...FIONBIO...) and fcntl(...O_NDELAY...) ...
    // POSIX addressed this with the introduction of O_NONBLOCK.
    int flags = fcntl(sock, F_GETFL, 0);
    return fcntl(sock, F_SETFL, noblock ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK));
}

/// @Notice: need restore block status
/// @param[in] timeout: <0-forever
/// @return 0-ok, other-error(by socket_geterror())
static int socket_connect_by_time(socket_t sock, const struct sockaddr* addr, socklen_t addrlen, int timeout)
{
    int r;
    socket_setnonblock(sock, 1);
    r = socket_connect(sock, addr, addrlen);
    assert(r <= 0);

    if (0 != r && EINPROGRESS == errno) {
        r = socket_select_connect(sock, timeout);
    }
    // r = socket_setnonblock(sock, 0);
    return r;
}

static int socket_close(socket_t sock)
{
    return close(sock);
}

static int socket_addr_setport(struct sockaddr* sa, socklen_t salen, u_short port)
{
    if (AF_INET == sa->sa_family) {
        struct sockaddr_in* in = (struct sockaddr_in*)sa;
        assert(sizeof(struct sockaddr_in) == salen);
        in->sin_port = htons(port);
    } else if (AF_INET6 == sa->sa_family) {
        struct sockaddr_in6* in6 = (struct sockaddr_in6*)sa;
        assert(sizeof(struct sockaddr_in6) == salen);
        in6->sin6_port = htons(port);
        } else {
        assert(0);
        return -1;
    }

    (void)salen;
    return 0;
}

/// @param[in] timeout ms, -1==infinite
socket_t socket_connect_host(const char* ipv4_or_ipv6_or_dns, u_short port, int timeout)
{
    int r;
    socket_t sock;
    char portstr[16];
    struct addrinfo hints, *addr, *ptr;

    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    snprintf(portstr, sizeof(portstr), "%hu", port);
    r = getaddrinfo(ipv4_or_ipv6_or_dns, portstr, &hints, &addr);
    if (0 != r) {
        DBG("getaddrinfo [%s], port = %u\n", ipv4_or_ipv6_or_dns, port);
        return socket_invalid;
    }

    r = -1; // not found
    sock = socket_invalid;
    for (ptr = addr; 0 != r && ptr != NULL; ptr = ptr->ai_next) {
        sock = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (socket_invalid == sock) {
            continue;
        }
        char addr_buf[32] = {0,};
        struct sockaddr_in *sockaddr = (sockaddr_in*)addr->ai_addr;
        inet_ntop(AF_INET, &sockaddr->sin_addr, addr_buf, sizeof(addr_buf));
        DBG("inet_ntop [%s:%s]\n", ipv4_or_ipv6_or_dns, addr_buf);

        // fixed ios getaddrinfo don't set port if nodename is ipv4 address
        socket_addr_setport(ptr->ai_addr, (socklen_t)ptr->ai_addrlen, port);
        if (timeout < 0) {
            r = socket_connect(sock, ptr->ai_addr, (socklen_t)ptr->ai_addrlen);
        } else {
            r = socket_connect_by_time(sock, ptr->ai_addr, (socklen_t)ptr->ai_addrlen, timeout);
        }
        if (0 != r) {
            socket_close(sock);
        }
    }

    freeaddrinfo(addr);
    return 0 == r ? sock : socket_invalid;
}

/*
 * test_tcp_connect
 * RETURN
 * 可写  : TRUE
 * 不可写: FALSE
 **/
int is_alive_tcp(const char *domain_name, int port, int overtime)
{
    if (NULL == domain_name || port < 0) {
        printf("please check your domain_name and port\n");
        return -1;
    }

    int     nfds;
    struct timeval timeout;
    int     result = -1, ret = FALSE;
    char    ip[16] = { 0 };

    //将域名转换为对应的ip地址
    if (NULL == get_ip_by_domain(domain_name, ip, sizeof(ip))) {
        DBG("get_ip_by_domain fail\n");
        return ret;
    }

    int tcp_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (tcp_fd < 0) {
        printf("socket : %s\n", strerror(errno));
        return ret;
    }
    //设置为非阻塞的方式
    int flags = fcntl(tcp_fd, F_GETFL, 0);
    fcntl(tcp_fd, F_SETFL, flags | O_NONBLOCK);

    // 设置目标主机地址和端口号
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr);

    if (connect(tcp_fd, (const struct sockaddr *)&addr, sizeof(addr)) != 0
        && errno != EINPROGRESS) {
        printf("dna_connect failed %d\r\n", tcp_fd);
        goto __err;
    }

    if (overtime <= 0 || overtime >= 60) {
        overtime = 2;
    }

    nfds = tcp_fd + 1;
    timeout.tv_sec = overtime;  //Waiting 2s to connect to tcp server
    timeout.tv_usec = 0;
    fd_set  writefds;

    FD_ZERO(&writefds);
    FD_SET(tcp_fd, &writefds);

    result = select(nfds, NULL, &writefds, NULL, &timeout);
    if (result <= 0) {
        DBG("fail ip: %s result: %d timeout %lld\n", ip, result, timeout.tv_sec);
        goto __err;
    } else {
        if (FD_ISSET(tcp_fd, &writefds)) {

            int     error = 0;
            socklen_t len = sizeof(error);

            result = getsockopt(tcp_fd, SOL_SOCKET, SO_ERROR, &error, &len);

            if (result < 0 || error !=0) {
                printf("getsockopt fail,connected  fail \r\n");
                printf("ret %d error = %d , connected fail \r\n", result, error);
                goto __err;
            }

            ret = TRUE;
        }
    }

__err:
    close(tcp_fd);
    return ret;
}

int is_alive_name(const char *domain_name)
{
    char dns_ip_list[JDNS_RESULT_COUNT][16] = {{0}};
    char *ip[JDNS_RESULT_COUNT] = {0};
    uint8_t dns_count = 0;
    int ret = FALSE;

    jdns_getaddrinfo(domain_name, dns_ip_list, ip);
    for (dns_count = 0; dns_count < JDNS_RESULT_COUNT; dns_count++) {
        if (ip[dns_count] == NULL || strlen(ip[dns_count]) == 0) {
            continue;
        } else {
            ret = TRUE;
            DBG("[%s] ip -> %s\n", domain_name, ip[dns_count]);
            break;
        }
    }

    return ret;
}

int www_reachable(void)
{
    size_t i = 0;

    static const char *list[] = {
        "aliyun.com",
        "public.iot-as-mqtt.cn-shanghai.aliyuncs.com",          // not stabe like before 0 and 1
    };

    for (i = 0; i < ARRAY_SIZE(list); i++) {
        if (is_alive_name(list[i])) {
            return TRUE;
        }
    }

    return FALSE;
}

/**
 *  检查 dst_ip 与 src_ip 是否在同一网段
 *  @返回值  TRUE  - 处于同一网段
 *           FALSE - 不在同一网段或任一 ip 不存在
 **/
int is_same_seg(char *dst_ip, char *src_ip)
{
    if (dst_ip == NULL || src_ip == NULL) {
        return FALSE;
    }

    char dst_buf[32] = {0};
    char src_buf[32] = {0};

    strncpy(dst_buf, dst_ip, sizeof(dst_buf)-1);
    strncpy(src_buf, src_ip, sizeof(src_buf)-1);

    char *p = strrchr(dst_buf, '.'); 
    if (p) {
        *p = '\0';
    }

    char *q = strrchr(src_buf, '.');
    if (q) {
        *q = '\0';
    }
    return (0 == strcmp(dst_buf, src_buf));
}

/* 获取平台在线状态 */
int platform_on_line(void)
{
    int ret = 0;
#if defined(PLATFORM_TENCENT)
    ret = is_tencent_on_line();
#endif
    return ret;
}

