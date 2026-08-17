/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : net_config.c
 * @Created Time : 2014-03-03
 * @Version      : 1.0
 * @Author       : cheby
 * @Description  :
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/route.h>
#include <net/if_arp.h>
#include <unistd.h>
#include <signal.h>
#include <asm/types.h>
#include <linux/sockios.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/rtnetlink.h>  
#include "net_config.h"
#include "debug.h"
#include "utils.h"
#include "system_ctrl.h"
#include "ethtool.h"

static InterfaceInfoS gInterfaceInfo;
static pthread_mutex_t gInterfaceInfoMutex = PTHREAD_MUTEX_INITIALIZER;

static struct sockaddr_in sa = {
sin_family:
    PF_INET,
    sin_port: 0
};

#define SIOCSEEPROMHWADDR           (SIOCDEVPRIVATE + 1)
#define RESOLV_CONF                 "/etc/resolv.conf"
#define BUFSIZE 8192 

struct route_info{   
    u_int dstAddr;   
    u_int srcAddr;   
    u_int gateWay;   
    char ifName[256];   
};   

int readNlSock(int sockFd, char *bufPtr, int seqNum, int pId)   
{   
    struct nlmsghdr *nlHdr;   
    int readLen = 0, msgLen = 0;   
  
    do{   
        if((readLen = recv(sockFd, bufPtr, BUFSIZE - msgLen, 0)) < 0)   
        {   
            perror("SOCK READ: ");   
            return -1;   
        }   

        nlHdr = (struct nlmsghdr *)bufPtr;   
        if((NLMSG_OK(nlHdr, readLen) == 0) || (nlHdr->nlmsg_type == NLMSG_ERROR))   
        {   
            perror("Error in recieved packet");   
            return -1;   
        }   

        if(nlHdr->nlmsg_type == NLMSG_DONE)    
        {   
            break;   
        }   
        else   
        {   
            bufPtr += readLen;   
            msgLen += readLen;   
        }   

        if((nlHdr->nlmsg_flags & NLM_F_MULTI) == 0)    
        {   

            break;   
        }   
    } while((nlHdr->nlmsg_seq != seqNum) || (nlHdr->nlmsg_pid != pId));   
    
    return msgLen;   
}   
void parseRoutes(struct nlmsghdr *nlHdr, struct route_info *rtInfo,char *gateway)   
{   
    struct rtmsg *rtMsg;   
    struct rtattr *rtAttr;   
    int rtLen;   
    struct in_addr dst;   
    struct in_addr gate;   

    rtMsg = (struct rtmsg *)NLMSG_DATA(nlHdr);   
    // If the route is not for AF_INET or does not belong to main routing table   
    //then return.    
    if((rtMsg->rtm_family != AF_INET) || (rtMsg->rtm_table != RT_TABLE_MAIN)) {
        return;   
    }   
        

    rtAttr = (struct rtattr *)RTM_RTA(rtMsg);   
    rtLen = RTM_PAYLOAD(nlHdr);   
    for(;RTA_OK(rtAttr,rtLen);rtAttr = RTA_NEXT(rtAttr,rtLen)){   
        switch(rtAttr->rta_type) 
        {   
        case RTA_OIF:   
            if_indextoname(*(int *)RTA_DATA(rtAttr), rtInfo->ifName);   
            break;   
        case RTA_GATEWAY:   
            rtInfo->gateWay = *(u_int *)RTA_DATA(rtAttr);   
            break;   
        case RTA_PREFSRC:   
            rtInfo->srcAddr = *(u_int *)RTA_DATA(rtAttr);   
            break;   
        case RTA_DST:   
            rtInfo->dstAddr = *(u_int *)RTA_DATA(rtAttr);   
            break;   
        }   
    }   
    dst.s_addr = rtInfo->dstAddr;   
    if (strstr((char *)inet_ntoa(dst), "0.0.0.0"))   
    {   
        //DBG("oif:%s\n",rtInfo->ifName);   
        gate.s_addr = rtInfo->gateWay;   
        sprintf(gateway, (char *)inet_ntoa(gate));   
        //DBG("%s\n",gateway);   
        gate.s_addr = rtInfo->srcAddr;   
        //DBG("src:%s\n",(char *)inet_ntoa(gate));   
        gate.s_addr = rtInfo->dstAddr;   
        //DBG("dst:%s\n",(char *)inet_ntoa(gate));    
    }   
    return;   
}   

int net_set_ipaddr(const char *eth_name, char *ip)
{
    if (eth_name == NULL || ip == NULL) {
        return -1;
    }

    in_addr_t ipaddr = inet_addr(ip);
    struct ifreq ifr;
    int skfd;
    int times = 2;

    memset(&ifr, 0, sizeof(struct ifreq));
    if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) <= 0) {
        ERR("socket error\n");
        return -1;
    }

    sa.sin_addr.s_addr = ipaddr;
    memcpy(ifr.ifr_name, eth_name, IFNAMSIZ);
    memcpy((char *)&ifr.ifr_addr, (char *)&sa, sizeof(struct sockaddr));

    do {
        if (ioctl(skfd, SIOCSIFADDR, &ifr) < 0) {
            ERR("error : %s\n", strerror(errno));
            times--;
            continue;
        }
        break;
    } while(times >= 0);

    close(skfd);
    if (times < 0) {
        return -1;
    }

    return 0;
}


int net_get_ipaddr(const char *eth_name, char *ip,int len)
{
    if (eth_name == NULL || ip == NULL) {
        return -1;
    }

    struct ifreq ifr;
    int skfd;
    int times = 2;

    memset(&ifr, 0, sizeof(struct ifreq));
    if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) <= 0) {
        ERR("socket error\n");
        return -1;
    }

    memcpy(ifr.ifr_name, eth_name, IFNAMSIZ);
    do {
        if (ioctl(skfd, SIOCGIFADDR, &ifr) < 0) {
            //ERR("error : %s\n", strerror(errno));
            times--;
            continue;
        }
        break;
    } while(times >= 0);

    close(skfd);
    if (times < 0) {
        return -1;
    }
    char *str = j_inet_ntoa(((struct sockaddr_in*)&(ifr.ifr_addr))->sin_addr);
    memset(ip,0,len);
    memcpy(ip, str,strlen(str));
    return 0;
}

int net_get_macaddr(const char *eth_name, char *mac)
{
    struct ifreq ifreq;
    int sock;

    if(mac == NULL)
        return -1;
    
    if ((sock=socket(AF_INET,SOCK_STREAM,0))<0) {
        ERR("socket error\n");
        return -1;
    }
    strcpy(ifreq.ifr_name, eth_name);
    if (ioctl(sock,SIOCGIFHWADDR,&ifreq)<0) {
        ERR("ioctl error\n");
        close(sock);
        return -1;
    }
    
    sprintf(mac,"%02x:%02x:%02x:%02x:%02x:%02x",
                (unsigned char)ifreq.ifr_hwaddr.sa_data[0],
                (unsigned char)ifreq.ifr_hwaddr.sa_data[1],
                (unsigned char)ifreq.ifr_hwaddr.sa_data[2],
                (unsigned char)ifreq.ifr_hwaddr.sa_data[3],
                (unsigned char)ifreq.ifr_hwaddr.sa_data[4],
                (unsigned char)ifreq.ifr_hwaddr.sa_data[5]);
    mac[3*6] = '\0';
    close(sock);
    return 0;
}
//SIOCSEEPROMHWADDR无效

//原先设置是修改了bootarg的参数。
//带后续调整
int net_set_macaddr(const char *eth_name, char *mac)
{
    if (eth_name == NULL || mac == NULL) {
        return -1;
    }

    int i = 0;
    struct ifreq ifr;
    int skfd;
    int times = 2;
    if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) <= 0) {
        ERR("socket error\n");
        return -1;
    }

    int addr[6] = {0};
    memcpy(ifr.ifr_name, eth_name, IFNAMSIZ);
    sscanf(mac, "%02x:%02x:%02x:%02x:%02x:%02x", &addr[0], &addr[1], &addr[2],
           &addr[3], &addr[4], &addr[5]);

    for (i = 0; i < 6; i++) {
        ifr.ifr_hwaddr.sa_data[i] = addr[i];
    }

    do {
        //ioctl(skfd, SIOCSIFHWADDR, &ifr)
        if (ioctl(skfd, SIOCSEEPROMHWADDR, &ifr) < 0) {
            ERR("error : %s\n", strerror(errno));
            times--;
            continue;
        }
        break;
    } while(times >= 0);

    close(skfd);
    if (times < 0) {
        return -1;
    }

    return 0;
}



int net_set_gateway(char *gateway)
{
    if (gateway == NULL) {
        return -1;
    }

    struct rtentry rt;
    int         skfd;
    in_addr_t   addr = inet_addr(gateway);
    int times = 2;

    memset((char *)&rt, 0, sizeof(struct rtentry));
    rt.rt_flags = (RTF_UP | RTF_GATEWAY);
    rt.rt_dst.sa_family = PF_INET;
    rt.rt_genmask.sa_family = PF_INET;

    sa.sin_addr.s_addr = addr;
    memcpy((char *)&rt.rt_gateway, (char *)&sa, sizeof(struct sockaddr));

    if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        DBG("socket error\n");
        return -1;
    }

    do {
        if (ioctl(skfd, SIOCADDRT, &rt) < 0) {
            //ERR("error : %s\n", strerror(errno));
            times--;
            continue;
        }
        break;
    } while(times >= 0);

    close(skfd);
    if (times < 0) {
        return -1;
    }

    return 0;
}
int net_get_gateway(char *gateway)   
{   
    struct nlmsghdr *nlMsg;     
    struct route_info *rtInfo;   
    char msgBuf[BUFSIZE];   
    int sock, len, msgSeq = 0;   

    if((sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE)) < 0)   
    {   
        perror("Socket Creation: ");   
        return -1;   
    }   

    memset(msgBuf, 0, BUFSIZE);   

    nlMsg = (struct nlmsghdr *)msgBuf;     

    nlMsg->nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg)); // Length of message.   
    nlMsg->nlmsg_type = RTM_GETROUTE; // Get the routes from kernel routing table .   

    nlMsg->nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST; // The message is a request for dump.   
    nlMsg->nlmsg_seq = msgSeq++; // Sequence of the message packet.   
    nlMsg->nlmsg_pid = getpid(); // PID of process sending the request.   


    if(send(sock, nlMsg, nlMsg->nlmsg_len, 0) < 0){   
        ERR("Write To Socket Failed…\n"); 
        close(sock);
        return -1;   
    }   


    if((len = readNlSock(sock, msgBuf, msgSeq, getpid())) < 0) {   
        ERR("Read From Socket Failed…\n");  
        close(sock);
        return -1;   
    }   

    rtInfo = (struct route_info *)malloc(sizeof(struct route_info));   
    for(;NLMSG_OK(nlMsg,len);nlMsg = NLMSG_NEXT(nlMsg,len)){   
        memset(rtInfo, 0, sizeof(struct route_info));   
        parseRoutes(nlMsg, rtInfo,gateway);   
    }   
    free(rtInfo);   
    close(sock);   
    return 0;   
}  

int get_dev_gateway(const char *netdev, char *gateway)
{
    FILE *fp = NULL;
    char buf[1024] = {0};
    char iface[16] = {0};
    unsigned char tmp[100] = {'\0'};
    unsigned int dest_addr = 0, gate_addr = 0;
    if (NULL == gateway) {
        DBG("gateway is NULL \n");
        return -1;
    }

    fp = fopen("/proc/net/route", "r");
    if (fp == NULL) {
        DBG("fopen error \n");
        return -1;
    }

    fgets(buf, sizeof(buf), fp);
    while (fgets(buf, sizeof(buf), fp)) {
        if ((sscanf(buf, "%s\t%X\t%X", iface, &dest_addr, &gate_addr) == 3)
            && (memcmp(netdev, iface, strlen(netdev)) == 0) && gate_addr != 0) {
            memcpy(tmp, (unsigned char *)&gate_addr, 4);
            sprintf(gateway, "%d.%d.%d.%d", (unsigned char)*tmp, (unsigned char)*(tmp+1), (unsigned char)*(tmp+2), (unsigned char)*(tmp+3));
            fclose(fp);
            return 0;
        }
    }

    fclose(fp);
    return -1;
}


int net_set_route(char *dst_addr, char *mask, char *src_gateway, char *dev)
{
    struct rtentry rt;
    int skfd;
    int times = 2;

    memset(&rt, 0, sizeof(struct rtentry));

    rt.rt_flags = (unsigned short)(RTF_UP | RTF_GATEWAY | RTF_DEFAULT);

    rt.rt_dst.sa_family = PF_INET;

    inet_aton(dst_addr, &sa.sin_addr);
    memcpy(&rt.rt_dst, &sa, sizeof(struct sockaddr));

    inet_aton(src_gateway, &sa.sin_addr);
    memcpy(&rt.rt_gateway, &sa, sizeof(struct sockaddr));

    inet_aton(mask, &sa.sin_addr);
    memcpy(&rt.rt_genmask, &sa, sizeof(struct sockaddr));

    rt.rt_dev = dev;

    if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        DBG("socket error\n");
        return -1;
    }
    do {
        if (ioctl(skfd, SIOCADDRT, &rt) < 0) {
            ERR("error : %s\n", strerror(errno));
            times--;
            continue;
        }
        break;
    } while(times >= 0);

    close(skfd);
    if (times < 0) {
        return -1;
    }
    return 0;

}


int net_set_dnsaddr(char *dnsname)
{
    char *buf = NULL;
    char szcmd[128] = {0};
    int fd = -1;
    int iread = 0;
    int iwrite = 0;

    fd = open(RESOLV_CONF, O_RDONLY);
    if (fd < 0) {
        return -1;
    }

    iread = lseek(fd, 0, SEEK_END);
    if (iread == -1) {
        close(fd);
        return -1;
    }

    lseek(fd, 0, SEEK_SET);

    buf = malloc(iread+1);
    if (buf == NULL) {
        close(fd);
        return -1;
    }
    memset(buf, 0, iread+1);

    iwrite = read(fd, buf, iread);
    if(iread != iwrite) {
        close(fd);
        free(buf);
        return -1;
    }

    sprintf(szcmd, "nameserver %s\n", dnsname);
    WriteFile(RESOLV_CONF, szcmd);
    AppendFile(RESOLV_CONF, buf);

    free(buf);
    close(fd);
    return 0;
}

int net_get_dnsaddr(char *dnsaddr)
{
    if (NULL == dnsaddr) {
        return -1;
    }

    FILE *fp = NULL;
    fp = fopen(RESOLV_CONF, "r");
    if (NULL == fp) {
        return -1;
    }

    char dns_addr[64] = {0};
    while (NULL != fgets(dns_addr, sizeof(dns_addr), fp)) {
        if (!strncasecmp(dns_addr, "nameserver", strlen("nameserver"))) {
            memcpy(dnsaddr, dns_addr + 11,strlen(dns_addr + 11));
            dnsaddr[strlen(dnsaddr)-1] = '\0';
            break;
        }
        memset(dns_addr, 0, sizeof(dns_addr));
    }

    fclose(fp);
    fp = NULL;
    return 0;
}

int net_get_interface_running_info(InterfaceInfoS *pInfo, int *interface_num)
{
    int fd;
    int interfaceNum = 0;
    struct ifreq buf[16];
    struct ifconf ifc;
    struct ifreq ifrcopy;
    char mac[32];
    char ip[32];
    char gw[32];
    char broadAddr[32];
    char subnetMask[32];
    char dns[64];
    int count = 0;

    memset(mac, 0, sizeof(mac));
    memset(ip, 0, sizeof(ip));
    memset(gw, 0, sizeof(gw));
    memset(broadAddr, 0, sizeof(broadAddr));
    memset(subnetMask, 0, sizeof(subnetMask));
    memset(dns, 0, sizeof(dns));

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket");
        close(fd);
        *interface_num = 0;
        return -1;
    }

    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = (caddr_t)buf;
    if (!ioctl(fd, SIOCGIFCONF, (char *)&ifc))
    {
        interfaceNum = ifc.ifc_len / sizeof(struct ifreq);
        while (interfaceNum-- > 0)
        {
            if (interfaceNum < 0) {
                break;
            }
            //DBG("interfaceNum:%d, interface: %s\n", interfaceNum, buf[interfaceNum].ifr_name);

            if(!strncmp(buf[interfaceNum].ifr_name, "lo", 2))
                continue;

            //ignore the interface that not up or not runing  
            ifrcopy = buf[interfaceNum];
            if (ioctl(fd, SIOCGIFFLAGS, &ifrcopy))
            {
                DBG("ioctl: %s\n", strerror(errno));
                continue;
            }

            //get the mac of this interface  
            if (!ioctl(fd, SIOCGIFHWADDR, (char *)(&buf[interfaceNum])))
            {
                memset(mac, 0, sizeof(mac));
                snprintf(mac, sizeof(mac), "%02x:%02x:%02x:%02x:%02x:%02x",

                    (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[0],
                    (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[1],
                    (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[2],
                    (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[3],
                    (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[4],
                    (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[5]);
                //DBG("%s mac: %s\n",buf[interfaceNum].ifr_name, mac);
            }
            else
            {
                DBG("ioctl: %s\n", strerror(errno));
                continue;
            }

            //get the IP of this interface  
            if (!ioctl(fd, SIOCGIFADDR, (char *)&buf[interfaceNum]))
            {
                memset(ip, 0, sizeof(ip));
                snprintf(ip, sizeof(ip), "%s",
                    (char *)inet_ntoa(((struct sockaddr_in *)&(buf[interfaceNum].ifr_addr))->sin_addr));
                //DBG("%s ip: %s\n",buf[interfaceNum].ifr_name, ip);
            }
            else
            {
                DBG("ioctl: %s\n", strerror(errno));
                continue;
            }

            //get the broad address of this interface  
            if (!ioctl(fd, SIOCGIFBRDADDR, &buf[interfaceNum]))
            {
                snprintf(broadAddr, sizeof(broadAddr), "%s",
                    (char *)inet_ntoa(((struct sockaddr_in *)&(buf[interfaceNum].ifr_broadaddr))->sin_addr));
                //DBG("%s broadAddr: %s\n",buf[interfaceNum].ifr_name, broadAddr);
            }
            else
            {
                DBG("ioctl: %s\n", strerror(errno));
                continue;
            }

            //get the subnet mask of this interface  
            if (!ioctl(fd, SIOCGIFNETMASK, &buf[interfaceNum]))
            {
                memset(subnetMask, 0, sizeof(subnetMask));
                snprintf(subnetMask, sizeof(subnetMask), "%s",
                    (char *)inet_ntoa(((struct sockaddr_in *)&(buf[interfaceNum].ifr_netmask))->sin_addr));
                //DBG("%s subnetMask: %s\n",buf[interfaceNum].ifr_name, subnetMask);
            }
            else
            {
                DBG("ioctl: %s\n", strerror(errno));
                continue;
            }

            net_get_dnsaddr(dns);
            net_get_gateway(gw);
            memcpy(pInfo[count].nic, buf[interfaceNum].ifr_name,strlen(buf[interfaceNum].ifr_name));
            memcpy(pInfo[count].ip, ip,strlen(ip));
            memcpy(pInfo[count].gw, gw,strlen(gw));
            memcpy(pInfo[count].mac, mac,strlen(mac));
            memcpy(pInfo[count].mask, subnetMask,strlen(subnetMask));
            memcpy(pInfo[count].dns, dns,strlen(dns));  
            count++;    
            if (count >= 4) {
                break;
            }
        }
    }
    else
    {
        DBG("ioctl: %s\n", strerror(errno));
        close(fd);
        *interface_num = 0;
        return -1;
    }
    close(fd);
    *interface_num = count;
    return 0;
}

void net_set_interface_info(InterfaceInfoS *hInterfaceInfo)
{
    pthread_mutex_lock(&gInterfaceInfoMutex);
    memcpy(&gInterfaceInfo, hInterfaceInfo, sizeof(gInterfaceInfo));
    pthread_mutex_unlock(&gInterfaceInfoMutex); 
}

void net_get_interface_info(InterfaceInfoS *hInterfaceInfo)
{
    pthread_mutex_lock(&gInterfaceInfoMutex);
    memcpy(hInterfaceInfo, &gInterfaceInfo, sizeof(gInterfaceInfo));
    pthread_mutex_unlock(&gInterfaceInfoMutex);
}

int net_get_mtu(const char *eth_name, int *mtu)
{
    if(eth_name == NULL || mtu == NULL) {
        ERR("eth_name eq NULL,or mtu eq NULL\n");
        return FAILURE;
    }

    int fd;
    struct ifreq ifr;

    fd = socket(AF_INET, SOCK_DGRAM, 0);

    strncpy(ifr.ifr_name, eth_name, IFNAMSIZ - 1);

    if(ioctl(fd, SIOCGIFMTU, &ifr) != 0){
        ERR("ioctl :%s\n",strerror(errno));
        close(fd);
        return FAILURE;
    }
    *mtu = ifr.ifr_mtu;
    close(fd);

    return SUCCESS;
}

/*
 * usb0 setting will block @ioctl(SIOCSIFMTU)
 **/
int net_set_mtu(const char *eth_name, int mtu)
{
    if(eth_name == NULL) {
        ERR("eth_name eq NULL,or mtu eq NULL\n");
        return FAILURE;
    }

    if(mtu < 1200 || mtu > 1500) {
        ERR("%s mtu[%d] less than 1200 or greater than 1500\n", eth_name, mtu);
        return FAILURE;
    }

    int fd;
    struct ifreq ifr;

    fd = socket(AF_INET, SOCK_DGRAM, 0);

    strncpy(ifr.ifr_name, eth_name, IFNAMSIZ - 1);

    ifr.ifr_mtu = mtu;

    if(ioctl(fd, SIOCSIFMTU, &ifr) != 0){
        ERR("ioctl :%s\n",strerror(errno));
        close(fd);
        return FAILURE;
    }
    close(fd);

    DBG("Maximun Transmisson Unit:%d\n", ifr.ifr_mtu);

    return SUCCESS;
}

int net_set_speed(const char *devname, int speed, int duplex, int autoneg)
{    
    int speed_wanted = -1;    
    int duplex_wanted = -1;    
    int autoneg_wanted = AUTONEG_ENABLE;   
    int advertising_wanted = -1;    
    struct ethtool_cmd ecmd;    
    struct ifreq ifr;    
    int fd = 0;    
    int err = 0;  

    do {
        if (devname == NULL) {        
            ERR("devname is emtpy\n");        
            err = -2;   
            break;
        }   
    
        speed_wanted = speed;    
        duplex_wanted = duplex;   
        autoneg_wanted = autoneg;  
    
        strcpy(ifr.ifr_name, devname);    
        fd = socket(AF_INET, SOCK_DGRAM, 0); 
        if (fd < 0) {        
            ERR("Cannot get control socket");  
            err = -1;
            break;
        }   
    
        ecmd.cmd = ETHTOOL_GSET;   
        ifr.ifr_data = (caddr_t)&ecmd;  
        err = ioctl(fd, SIOCETHTOOL, &ifr);   
        if (err < 0) {       
            ERR("Cannot get current device settings");    
            err = -1;
            break;   
        }   
    
        if (speed_wanted != -1) {  
            ecmd.speed = speed_wanted;   
        }  
    
        if (duplex_wanted != -1) {   
            ecmd.duplex = duplex_wanted;    
        }  
    
        if (autoneg_wanted != -1) {    
            ecmd.autoneg = autoneg_wanted; 
        }   
    
        if ((autoneg_wanted == AUTONEG_ENABLE) && (advertising_wanted < 0)) {  
            if (speed_wanted == SPEED_10 && duplex_wanted == DUPLEX_HALF) {   
                advertising_wanted = ADVERTISED_10baseT_Half;       
            } else if (speed_wanted == SPEED_10 && duplex_wanted == DUPLEX_FULL) {     
                advertising_wanted = ADVERTISED_10baseT_Full;       
            } else if (speed_wanted == SPEED_100 && duplex_wanted == DUPLEX_HALF) { 
                advertising_wanted = ADVERTISED_100baseT_Half;       
            } else if (speed_wanted == SPEED_100 && duplex_wanted == DUPLEX_FULL) {  
                advertising_wanted = ADVERTISED_100baseT_Full;       
            } else if (speed_wanted == SPEED_1000 && duplex_wanted == DUPLEX_HALF) {    
                advertising_wanted = ADVERTISED_1000baseT_Half;        
            } else if (speed_wanted == SPEED_1000 && duplex_wanted == DUPLEX_FULL) {  
                advertising_wanted = ADVERTISED_1000baseT_Full;      
            } else if (speed_wanted == SPEED_2500 && duplex_wanted == DUPLEX_FULL) {
                advertising_wanted = ADVERTISED_2500baseX_Full;        
            } else if (speed_wanted == SPEED_10000 && duplex_wanted == DUPLEX_FULL) {   
                advertising_wanted = ADVERTISED_10000baseT_Full;       
            } else {            
                advertising_wanted = 0;       
            }    
        } 
    
        if (advertising_wanted != -1) {        
            if (advertising_wanted == 0) {          
                ecmd.advertising = ecmd.supported &  
                                    (ADVERTISED_10baseT_Half |   
                                    ADVERTISED_10baseT_Full |                              
                                    ADVERTISED_100baseT_Half |                                
                                    ADVERTISED_100baseT_Full |                               
                                    ADVERTISED_1000baseT_Half |                               
                                    ADVERTISED_1000baseT_Full |                                
                                    ADVERTISED_2500baseX_Full |                                
                                    ADVERTISED_10000baseT_Full);       
            } else {           
                ecmd.advertising = advertising_wanted;       
            }    
        }    

        ecmd.cmd = ETHTOOL_SSET;    
        ifr.ifr_data = (caddr_t)&ecmd;    
        err = ioctl(fd, SIOCETHTOOL, &ifr);    
        if (err < 0) {        
            ERR("Cannot set new settings");     
            err = -1;
            break;   
        }   
    } while (0);

    if(fd > 0) {
        close(fd); 
        fd = -1;
    }
    
    return err;
}
