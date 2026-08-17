/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : net_config.h
 * @Created Time : 2014-03-03
 * @Version      : 1.0
 * @Author       : cheby
 * @Description  :
 */

#ifndef __NET_SETTING_H__
#define __NET_SETTING_H__

#ifdef __cplusplus
extern "C" {
#endif
    typedef struct {
        char nic[32];
        char ip[32];
        char mask[32];
        char gw[32];
        char mac[32];
        char dns[32];
    } InterfaceInfoS;

    /**
     * @brief   set ip of an interface
     * @param   "char *ip" : ip address
     * @param   "char *eth_name" : such as "eth0"
     * @retval  0 : success ; -1 : fail
     */
    int net_set_ipaddr(const char *eth_name, char *ip);


    int net_get_macaddr(const char *eth_name, char *mac);
    /**
     * @brief   set ip of an interface
     * @param   "char *mac" : mac address  such as "00:60:6E:84:09:87"
     * @param   "eth_name" : such as "eth0"
     * @retval  0 : success ; -1 : fail
     */
    int net_set_macaddr(const char *eth_name, char *mac);

    /**
     * @brief   set ip of an interface
     * @param   "char *gateway" : maskaddr address  such as "192.168.2.1"
     * @retval  0 : success ; -1 : fail
     */
    int net_set_gateway(char *gateway);

	int get_dev_gateway(const char *netdev, char *gateway);

    /**
     * @brief   set ip of an interface
     * @param   "char *dnsname" : maskaddr address  such as "202.96.128.68"
     * @retval  0 : success ; -1 : fail
     */
    int net_set_dnsaddr(char *dnsname);


    /**
     * @brief   set ip of an interface
     * @param   "char *dst_addr" : source ipaddr  such as "202.96.128.68"
     * @param   "char *mask"    : mask ipaddr  such as "255.255.255.0"
     * @param   "char *dev"      : dev info  such as "eth0"
     * @retval  0 : success ; -1 : fail
     */
    int net_set_route(char *dst_addr, char *mask, char *src_gateway, char *dev);


    int net_get_ipaddr(const char *eth_name, char *ip, int len);

    int net_get_dnsaddr(char *dnsaddr);

	int net_get_interface_running_info(InterfaceInfoS *pInfo, int *interface_num);
	void net_set_interface_info(InterfaceInfoS *hInterfaceInfo);
	void net_get_interface_info(InterfaceInfoS *hInterfaceInfo);
	/**
     * @brief   set mtu of an interface
     * @param   "char *eth_name" : network card name such as "eth0" "waln0"
     * @param   "int *mtu" : return mtu value
     * @retval  0 : success ; -1 : fail
     */
	int net_get_mtu(const char *eth_name, int *mtu);

	/**
     * @brief   get mtu of an interface
     * @param   "char *eth_name" : network card name such as "eth0" "waln0"
     * @param   "int mtu" : the mtu value will be set
     * @retval  0 : success ; -1 : fail
     */
	int net_set_mtu(const char *eth_name, int mtu);
	
	int net_set_speed(const char *devname, int speed, int duplex, int autoneg);
	
#ifdef __cplusplus
}
#endif
#endif

