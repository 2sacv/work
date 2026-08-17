/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    :net_check.h
 * @Created Time : 2014-02-25
 * @Version      : 1.0
 * @Author       : cheby
 * @Description  : copy from network
 */

#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <arpa/inet.h>
#include "jconfstruct.h"
#include "debug.h"

#ifndef __CHECKIP_H__
#define __CHECKIP_H__

#ifdef __cplusplus
extern "C" {
#endif

#define NO_CONFIGURED_NET  0
#define HAD_CONFIGURED_NET 1

typedef int socket_t;
struct arpMsg {
    struct ethhdr ethhdr;   /* Ethernet header */
    u_short htype; /* hardware type (must be ARPHRD_ETHER) */
    u_short ptype; /* protocol type (must be ETH_P_IP) */
    u_char  hlen; /* hardware address length (must be 6) */
    u_char  plen; /* protocol address length (must be 4) */
    u_short operation; /* ARP opcode */
    u_char  sHaddr[6]; /* sender's hardware address */
    u_char  sInaddr[4]; /* sender's IP address */
    u_char  tHaddr[6]; /* target's hardware address */
    u_char  tInaddr[4]; /* target's IP address */
    u_char  pad[18]; /* pad for min. Ethernet payload (60 bytes) */
};

/**
 * @brief   set ip of an interface
 * @param   "char *eth_name" : maskaddr address  such as "eth0"
 * @retval   -1 : error , details can check errno  , 0  interface link down.,   1 interface link up
 */
int net_link_status(const char *eth_name);

/**
 * @brief   set ip of an interface
 * @param   "char *ipaddr" : ip addr address  such as "192.1658.2.34"
 * @retval  0 : success ; 1 : IP conflict   -1 get devinfo fail
 */
int net_check_ip(const char *ipaddr);

int net_adaptive_server(const char *ipaddr);

int init_client_iplink_check(void *data);
int uninit_client_iplink_check();
int is_alive_ip(const char *ip, const char *func);
int is_alive_ip_quick(const char *ip, const char *func);

int init_ip_adaptive(void *schedule);
int uninit_ip_adaptive();

void set_arp_network(NetEthS *eth);
void set_arp_conflict_flag(BOOL flag);
BOOL get_arp_conflict_flag();
int is_alive_tcp(const char *domain_name, int port,int overtime);
int is_alive_name(const char *domain_name);
int www_reachable(void);
int is_same_seg(char *dst_ip, char *src_ip);
void net_status(void *data);
int get_net_match_status(void);
void set_net_match_status(int flag);
int platform_on_line(void);

#ifdef  __cplusplus
}
#endif
#endif /* __CHECKIP_H__ */


