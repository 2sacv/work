/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : confutils.cpp
 * @Created Time : 2013-11-08
 * @Version      : 1.0
 * @Author       : zengy
 * @Description  :
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <linux/wireless.h>
#include <sys/sysinfo.h>
#include <linux/sockios.h>
#include <linux/ethtool.h>

#include "debug.h"
#include "utils.h"
#include "confutils.h"
#include "conf_nand.h"
#include "system_ctrl.h"
#include "soft_check.h"
#include "id_protect.h"

#ifndef SUCCESS
#define SUCCESS     0
#define FAILURE     (-1)
#endif

int ipMaskGatewayRuleCheck(char *ethip, char *ethmask, char *ethgw)
{
	int ret = SUCCESS;

	unsigned int ip = ntohl(inet_addr(ethip));
    unsigned int mask = ntohl(inet_addr(ethmask));
    unsigned int gw = ntohl(inet_addr(ethgw));

	do {
        if(0xFFFFFFFF == ip || 0xFFFFFFFF == mask || 0xFFFFFFFF == gw) {
            ERR("ip, mask or gw contain invalid char or 255.255.255.255!\n");
            ret = FAILURE;
            break;
        }

        if( (mask & 0xFF000000) == 0 ) {
            ERR("mask should not begin with 0!\n");
            ret = FAILURE;
            break;
        }

        if( (ip & 0xFF000000) == 0 ) {
            ERR("ipaddr should not begin with 0!\n");
            ret = FAILURE;
            break;
        }
   } while(0);
	
	return ret;
}

int ipMaskGatewayCheck(char *ethip, char *ethmask, char *ethgw)
{
    int ret = SUCCESS;

    unsigned int ip = ntohl(inet_addr(ethip));
    unsigned int mask = ntohl(inet_addr(ethmask));
    unsigned int gw = ntohl(inet_addr(ethgw));

    do {
        if(0xFFFFFFFF == ip || 0xFFFFFFFF == mask || 0xFFFFFFFF == gw) {
            ERR("ip, mask or gw contain invalid char or 255.255.255.255!\n");
            ret = FAILURE;
            break;
        }

        if( (mask & 0xFF000000) == 0 ) {
            ERR("mask should not begin with 0!\n");
            ret = FAILURE;
            break;
        }

        if( (ip & 0xFF000000) == 0 ) {
            ERR("ipaddr should not begin with 0!\n");
            ret = FAILURE;
            break;
        }

        if ((ip & mask) != (gw & mask)) {
            ERR("ipaddr and gateway should in same subnet!\n");
            ret = FAILURE;
            break;
        }

        if ( (ip & (~mask)) == 0 ) {
            ERR("ipaddr should not be subnet addr!\n");
            ret = FAILURE;
            break;
        }

        if ( ip == gw ) {
            ERR("ipaddr %s is the same as gateway!\n", ethip);
            ret = FAILURE;
            break;
        }

        if ( (ip & 0xFF000000) == 0x7F000000 ) {
            ERR("ipaddr %s could not be loopback address!\n", ethip);
            ret = FAILURE;
            break;
        }

        if ( (ip | mask) == 0xFFFFFFFF ) {
            ERR("ipaddr should not be broadcast addr!\n");
            ret = FAILURE;
            break;
        }
    } while(0);

    return ret;
}

int processEthMac(ConfAct act, const char *pNic, char *szMac)
{
    int sockFd = -1;
    struct ifreq req;

    if (0 >= (sockFd = socket(AF_INET, SOCK_DGRAM, 0))) {
        ERR("sockfd < 0\n");
        return FAILURE;
    }

    memset(&req, 0, sizeof(req));

    strcpy(req.ifr_name, pNic);

    if(ConfGet == act) {
        if (0 > ioctl(sockFd, SIOCGIFHWADDR, &req)) {
            close(sockFd);
            //ERR("get SIOCGIFHWADDR [%s] mac errno!\n", pNic);
            return FAILURE;
        }

        sprintf(szMac, "%02X:%02X:%02X:%02X:%02X:%02X",
                (unsigned char)req.ifr_hwaddr.sa_data[0],
                (unsigned char)req.ifr_hwaddr.sa_data[1],
                (unsigned char)req.ifr_hwaddr.sa_data[2],
                (unsigned char)req.ifr_hwaddr.sa_data[3],
                (unsigned char)req.ifr_hwaddr.sa_data[4],
                (unsigned char)req.ifr_hwaddr.sa_data[5]);
    } else if(ConfSet == act) {
        int addr[6];

        memset(addr, 0, sizeof(addr));
        sscanf(szMac, "%02X:%02X:%02X:%02X:%02X:%02X", addr + 0, addr + 1, addr + 2,
               addr + 3, addr + 4, addr + 5);

        if (Security_SoftWare == system_get_security_type()) {
			DumpFile(PATH_BKUP_MAC, szMac, strlen(szMac));
            if (uboot_mac_set(szMac) < 0) {
                ERR("set uboot mac fail,mac = %s\n", szMac);
                close(sockFd);
                return FAILURE;
            }
            
            if (SUCCESS != uboot_devinfo_set()) {
                ERR("uboot_secure_set fail of mac=%s\n", szMac);
                close(sockFd);
                return FAILURE;
            }
            DBG("__ uboot_devinfo_set done\n");
        }
    }

    close(sockFd);
    return SUCCESS;
}

