/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    : search_service.cpp
 * Created Time : 2014-03_31
 * Version      : 1.0
 * Author       : chebiyou
 * Description  :
 */

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/route.h>
#include <net/if.h>
#include <linux/sockios.h>
#include <sys/sysinfo.h>
#include <netdb.h>
#include <time.h>
#include "jconfstruct.h"
#include "jconfig.h"
#include "confapi.h"
#include "debug.h"

#include "search_service.h"
#include "BasicUsageEnvironment.hh"
#include "jcpService.h"
#include "conf_list.h"
#include "system_ctrl.h"
#include "net_config.h"
#include "jevent.h"
#include "js_scheduler.h"
#include "conf_nand.h"
#include "net_check.h"
#include "g_log.h"
#include "system_sch.h"

#define INTV_SEARCH                 (1*1000)
#define SEARCH_BUFLEN               (10*1024)
#define SEARCH_MULTI_ADDR           "230.230.230.230"
#define SEARCH_MULTI_PORT           8002
#define ETH_NIC_NAME                "eth0"
#define WIFI_NIC_NAME               "wlan0"

#define PROCBUFSIZ                  1024  
#define _PATH_PROC_NET_DEV          "/proc/net/dev" 

static int g_sockfd = -1;
static int g_wifi_fd = -1;
static SysInfoS g_info;
static VideoEncS g_ves;
static NetPortS g_netport = {0};
static NetEthS  g_eth;
static JSTCHandle  fd_eth0 = NULL;
static JSTCHandle  fd_wlan = NULL;
static JSTCHandle  hdl_watch = NULL;

/*
SEARCH
Device-Name:NVS-355-V01\r\n
Device-Jcpver:V2.00\r\n
Device-Config:PTZ,\r\n
Device-DevType:DM365\r\n
Device-Update:0|1~100\r\n
Device-Channels:1\r\n
Device-Mode:0/D1,1/CIF,\r\n
Device-RtspPort:554\r\n
Device-WebPort:80\r\n
Device-ID:00000001\r\n
Device-VerKernel:Sat Jan 10 14:16:46 CST 2009\r\n
Device-VerServer:V2.0.0001-20090110-DEVELOP\r\n
Device-VerWeb:V2.0.0-20081210\r\n
Device-Username:admin\r\n
Device-Password:admin\r\n
Device-NetType:0|1\r\n
Device-IP:192.168.1.217\r\n
Device-Mac:00:01:02:03:04:05
*/

typedef struct {
    VencSizeE veSize;
    const char* sizeStr;
} VSizetoStr;

VSizetoStr vsize_str[] = {
    {VencSizeE_QCIF , "QCIF"},
    {VencSizeE_180P , "180P"},
    {VencSizeE_QVGA , "QVGA"},
    {VencSizeE_CIF  , "CIF" },
    {VencSizeE_360P , "360P" },
    {VencSizeE_VGA  , "VGA" },
    {VencSizeE_D1   , "D1"  },
    {VencSizeE_720P , "720P"},
    {VencSizeE_960P , "960P"},
    {VencSizeE_UVGA , "UVGA"},
    {VencSizeE_1080P, "1080P"},
};

struct cmd_info {
    time_t epoch;
    char ip[16];
    char cmd[32];
};

static int check_interface_fromproc(char *interface1, int *result1,char *interface2, int *result2)  
{  
    FILE *fp;  
    char buf[PROCBUFSIZ];   
    char *ptr = NULL;
    static int result1_old = -1;
    static int result2_old = -1;    
    static struct timespec ts = {0};

    if (ms_clock_is_timeup(&ts,10000))
    {
        /* Open /proc/net/dev. */  
        fp = fopen(_PATH_PROC_NET_DEV, "r");  
        if(fp == NULL) {     
            printf("open proc file error\n");  
            return -1; 
        }     

        /* Drop header lines. */  
        fgets(buf, PROCBUFSIZ, fp);  
        fgets(buf, PROCBUFSIZ, fp);  
        bzero(buf, sizeof(buf));
        /* Only allocate interface structure.  Other jobs will be done in if_ioctl.c. */  
        while(fgets(buf, PROCBUFSIZ, fp) != NULL) {     
            ptr = strstr(buf,interface1);
            if (ptr)
            {
                *result1 = 1;   
            }
            ptr = strstr(buf,interface2);
            if (ptr)
            {
                *result2 = 1;   
            }
            bzero(buf, sizeof(buf));
        }     
        fclose(fp);


        result1_old = *result1;
        result2_old = *result2;
    }
    else
    {
        *result1 = result1_old;
        *result2 = result2_old;
    }

    return 0;  
}

int get_sysinfo(SysInfoS *info)
{
    static int sysflag = 0XBADF00D;

    if (sysflag != 0XBADF00D) {
        memcpy(info, &g_info, sizeof(SysInfoS));
        return 0;
    }

    memset(&g_info, 0, sizeof(SysInfoS));
    sysflag = get_config(handleSysInfoCfg, g_info);
    memcpy(info, &g_info, sizeof(SysInfoS));
    return sysflag;
}


int set_sysinfo(SysInfoS *info)
{
    memcpy(&g_info, info, sizeof(SysInfoS));
    return 0;
}

int get_videoinfo(VideoEncS *ves)
{
    static int videoflag = 0XBADF00D;
    if (videoflag != 0XBADF00D) {
        memcpy(ves, &g_ves, sizeof(VideoEncS));
        return 0;
    }

    memset(&g_ves, 0, sizeof(VideoEncS));
    videoflag = get_config(handleRealVideoCfg, g_ves);
    memcpy(ves, &g_ves, sizeof(VideoEncS));
    return videoflag;
}

int set_videoinfo(VideoEncS *ves)
{
    memcpy(&g_ves, ves, sizeof(VideoEncS));
    return 0;
}

int get_portinfo(NetPortS *netport)
{
    static int portflag = 0XBADF00D;

    if (portflag != 0XBADF00D) {
        memcpy(netport, &g_netport, sizeof(NetPortS));
        return 0;
    }

    memset(&g_netport, 0, sizeof(NetPortS));
    portflag = get_config(handleNetPortCfg, g_netport);
    memcpy(netport, &g_netport, sizeof(NetPortS));
    return portflag;
}

int set_portinfo(NetPortS *netport)
{
    memcpy(&g_netport, netport, sizeof(NetPortS));
    return 0;
}

int get_ethinfo(NetEthS *eth)
{
    static int ethflag = 0XBADF00D;

    if (ethflag != 0XBADF00D) {
        memcpy(eth, &g_eth, sizeof(NetEthS));
        return 0;
    }

    memset(&g_eth, 0, sizeof(NetEthS));
    ethflag = get_config(handleEthCfg, g_eth);
    memcpy(eth, &g_eth, sizeof(NetEthS));
    return ethflag;
}

int set_ethinfo(NetEthS *eth)
{
    memcpy(&g_eth, eth, sizeof(NetEthS));
    return 0;
}

int get_video_string(char *buf)
{
    VideoEncS encode = {0};
    char stream[2][8] = {"stream1", "stream2"};
    get_videoinfo(&encode);
    int i = 0;
    int len = 0;
    for (i = 0; i < ENCODE_MAX_CHN-1; i++) {
        if (1 == encode.enc[i].enable) {
            len += sprintf(buf + len, "%s,", stream[i]);
        }
    }
    return SUCCESS;
}

static int search_get_devinfo(char *szbuf, int maxlen, char *srchost, char *nic_name)
{
    int ret = 0;
    int buflen = 0;
    if (NULL == szbuf) {
        return FAILURE;
    }

    sprintf(szbuf, "SEARCH ");
    buflen = strlen(szbuf);

    SysInfoS info = {{0,},};
    NetPortS netport = {0};
    NetEthS eth = {{0,},};
    char videinfo[64] = {0};
    int net_type = 0;
    NetWifiS netwifi = {0};
    int ip[4] = {0};
    
    get_sysinfo(&info);
    get_video_string(videinfo);

    get_portinfo(&netport);
    conf_get_wificfg(&netwifi);
    
    int eth_exist = 0;
    int wifi_exist = 0;

    // 拔掉网线，有线也被识别成存在
    ret = check_interface_fromproc((char*)ETH_NIC_NAME,&eth_exist,(char*)WIFI_NIC_NAME,&wifi_exist);
    if (0 != ret){
        return FAILURE;
    }

    if (net_link_status("eth0") == 1) {
        eth_exist = 1;
    } else {
        eth_exist = 0;
    }
    
    if(eth_exist) {
        get_ethinfo(&eth);
        net_get_ipaddr(eth.nic, eth.ip,sizeof(eth.ip));
        sscanf(eth.ip, "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]);
        snprintf(eth.gw, sizeof(eth.gw), "%d.%d.%d.1", ip[0], ip[1], ip[2]);
    }
    
    char wifiip[16] = {0};
    if (wifi_exist) {
        // 不使用内置静态 get_ethinfo()，以免第一次设置时查不到
        net_get_ipaddr(WIFI_NIC_NAME, wifiip,sizeof(wifiip));
        sscanf(wifiip, "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]);
        snprintf(netwifi.gw, sizeof(netwifi.gw), "%d.%d.%d.1", ip[0], ip[1], ip[2]);
    }

    if(eth_exist && wifi_exist)
        net_type = 2;
    else if(wifi_exist)
        net_type = 1;
    else if(eth_exist)
        net_type = 0;
    else
        return FAILURE;

    int searchfrom = 0;
    int mask = ntohl(inet_addr("255.255.255.0"));
    int wip = ntohl(inet_addr(wifiip));
    int sip = ntohl(inet_addr(srchost));

    if((wip & mask) == (sip & mask)) {
        searchfrom = 1;
    } else {
        searchfrom = 0;
    }

    time_t now = time(NULL);
    char idbuf[16] = {0};
    system_get_dev_id(idbuf);

    SysUserS suser = {0,}; 
    conf_get_usercfg(&suser); 

    buflen += snprintf(szbuf + buflen, maxlen - buflen, "Device-Solution:%s\r\n", info.solution);
    buflen += snprintf(szbuf + buflen, maxlen - buflen, "Device-Name:%s\r\n", info.devname);
    buflen += snprintf(szbuf + buflen, maxlen - buflen, "Device-Config:%s\r\n", "PTZ");
    buflen += snprintf(szbuf + buflen, maxlen - buflen, "Device-DevType:%s\r\n", info.devtype);
    buflen += snprintf(szbuf + buflen, maxlen - buflen, "Device-Update:%d\r\n", conf_get_update_progressbar());
    buflen += snprintf(szbuf + buflen, maxlen - buflen, "Device-Jcpver:V2.00\r\n");
    buflen += snprintf(szbuf + buflen, maxlen - buflen, "Device-Channels:1\r\n");
    buflen += snprintf(szbuf + buflen, maxlen - buflen, "Device-Mode:%s\r\n", videinfo);
    buflen += snprintf(szbuf + buflen, maxlen - buflen, "Device-RtspPort:%d\r\n", netport.rtspport);
    buflen += snprintf(szbuf + buflen, maxlen - buflen, "Device-WebPort:%d\r\n", netport.httpport);
    buflen += snprintf(szbuf + buflen, maxlen - buflen, "Device-ID:%s\r\n", idbuf);
    buflen += snprintf(szbuf + buflen, maxlen - buflen, "Device-Cpuid:%s\r\n", get_cpuid());
    buflen += snprintf(szbuf + buflen, maxlen - buflen, "Device-CustomAppid:%s\r\n", info.custom_appid);
    buflen += snprintf(szbuf + buflen, maxlen - buflen, "Device-P2p:%s\r\n", "tx");
    buflen += snprintf(szbuf + buflen, maxlen - buflen, "Device-VerKernel:%s\r\n", info.kernelver);
    buflen += snprintf(szbuf + buflen, maxlen - buflen, "Device-VerServer:%s\r\n", info.serverver);
    buflen += snprintf(szbuf + buflen, maxlen - buflen, "Device-VerWeb:%s\r\n", info.webver);
    buflen += snprintf(szbuf + buflen, maxlen - buflen, "Device-Username:%s\r\n", suser.user[0].username);
    buflen += snprintf(szbuf + buflen, maxlen - buflen, "Device-Password:%s\r\n", suser.user[0].onvifpasswd);

    if(2 == net_type)
        buflen += snprintf(szbuf + buflen, maxlen - buflen, "Device-Search-From:%d\r\n", searchfrom);

    buflen += snprintf(szbuf + buflen, maxlen - buflen, "Device-NetType:%d\r\n", net_type);

    if (eth_exist) {
        buflen += snprintf(szbuf + buflen, maxlen - buflen, "Device-IP:%s\r\n", eth.ip);
        buflen += snprintf(szbuf + buflen, maxlen - buflen, "Device-SubMask:%s\r\n", eth.mask);
        buflen += snprintf(szbuf + buflen, maxlen - buflen, "Device-GateWay:%s\r\n", eth.gw);
        buflen += snprintf(szbuf + buflen, maxlen - buflen, "Device-Mac:%s\r\n", eth.mac);
    } 

    if (wifi_exist) {
        if(!eth_exist) {
            buflen += snprintf(szbuf + buflen, maxlen - buflen, "Device-IP:%s\r\n", wifiip);
            buflen += snprintf(szbuf + buflen, maxlen - buflen, "Device-SubMask:%s\r\n", netwifi.mask);
            buflen += snprintf(szbuf + buflen, maxlen - buflen, "Device-GateWay:%s\r\n", netwifi.gw);
            buflen += snprintf(szbuf + buflen, maxlen - buflen, "Device-Mac:%s\r\n", netwifi.mac);
        } else {
            buflen += snprintf(szbuf + buflen, maxlen - buflen, "Device-WIFI-IP:%s\r\n", wifiip);
            buflen += snprintf(szbuf + buflen, maxlen - buflen, "Device-WIFI-SubMask:%s\r\n", netwifi.mask);
            buflen += snprintf(szbuf + buflen, maxlen - buflen, "Device-WIFI-GateWay:%s\r\n", netwifi.gw);
            buflen += snprintf(szbuf + buflen, maxlen - buflen, "Device-WIFI-Mac:%s\r\n", netwifi.mac); 
        }
    }

    if (get_g_sys(usb_4g) && is_okey("/tmp/4g/ATI")) {
        static char ATI[32] = "badbeef";
        if (NULL != strstr(ATI, "badbeef")) {
            LoadFile("/tmp/4g/ATI",ATI,sizeof(ATI));
        }
        buflen += snprintf(szbuf + buflen, maxlen - buflen, "Device-Ver4G:%s\r\n", ATI); 
    }
        
    buflen += snprintf(szbuf + buflen, maxlen - buflen, "Device-Time:%lld\r\n", now);

    return SUCCESS;

}


static int search_response(char *buf_recv, int sock_fd, char *nic_name, char *srchost)
{
    char dest_ip[64] = {0};
    int dest_port = 0;
    char dest_remain[64] = {0};
    char buf_send[SEARCH_BUFLEN] = {0};

    struct sockaddr_in addr;
    struct ifreq req;

    unsigned int ipaddr = 0;
    unsigned int ipmask = 0;
    unsigned int ipbroad = 0;

    if (!strstr(buf_recv, "LOCALIP") && !strstr(buf_recv, "LOCALPORT")) {
        return FAILURE;
    }

    sscanf(buf_recv, "%*[^=]=%32[^#]#%63s", dest_ip, dest_remain);
    sscanf(dest_remain, "%*[^=]=%d#", &dest_port);

    if (dest_port <= 0 || strlen(dest_ip) <= 0) {
        return FAILURE;
    }

    memset(&req, 0, sizeof(req));
    sprintf(req.ifr_name, "%s", nic_name);

    int ret = ioctl(sock_fd, SIOCGIFADDR, &req);
    if (-1 == ret) {
        ERR("SIOCGIFADDR %s fail, errno:%d\n", nic_name, errno);
        return FAILURE;
    }
    ipaddr = ((struct sockaddr_in *)&req.ifr_addr)->sin_addr.s_addr;

    ret = ioctl(sock_fd, SIOCGIFNETMASK, &req);
    if (-1 == ret) {
        ERR("SIOCGIFNETMASK %s fail, errno:%d\n", nic_name, errno);
        return FAILURE;
    }

    ipmask = ((struct sockaddr_in *)&req.ifr_addr)->sin_addr.s_addr;

    ret = ioctl(sock_fd, SIOCGIFBRDADDR, &req);
    if (-1 == ret) {
        ERR("SIOCGIFBRDADDR %s fail, errno:%d\n", nic_name, errno);
        return FAILURE;
    }

    ipbroad = ((struct sockaddr_in *)&req.ifr_addr)->sin_addr.s_addr;

    unsigned int dest_ipaddr = inet_addr(dest_ip);

    if ((ipaddr & ipmask) != (dest_ipaddr & ipmask)) {
        // devie subnet + pc ip net addr
        dest_ipaddr = (ipaddr & ipmask) | (dest_ipaddr & ~ipmask);
    }

    search_get_devinfo(buf_send, sizeof(buf_send) - 1, srchost, nic_name);

    if (strlen(buf_send) <= 0) {
        return FAILURE;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = dest_ipaddr;
    addr.sin_port = htons(dest_port);

    ret = sendto(sock_fd, buf_send, strlen(buf_send) + 1, 0, (struct sockaddr*)&addr,  sizeof(struct sockaddr));

    addr.sin_addr.s_addr = ipbroad;
    ret = sendto(sock_fd, buf_send, strlen(buf_send) + 1, 0, (struct sockaddr*)&addr,  sizeof(struct sockaddr));

    // send broadcast
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("255.255.255.255");
    addr.sin_port = htons(dest_port);
    ret = sendto(sock_fd, buf_send, strlen(buf_send) + 1, 0, (struct sockaddr*)&addr,  sizeof(struct sockaddr));

    return SUCCESS;
}

static int search_ipset(char *buf_recv)
{
    /*
     * IPSET * HDS/1.0
     * ID=83918977#IP=192.168.2.45#SubMask=255.255.255.0#GateWay=192.168.2.1#
     */

    char recv_bak[SEARCH_BUFLEN] = {0};

    char *pbuf = buf_recv + strlen("IPSET * HDS/1.0\r\n") - 1;

    *pbuf = '\0';
    pbuf++;

    memcpy(recv_bak, pbuf, sizeof(recv_bak) - 1);

    if (!strstr(pbuf, "ID=")) {
        ERR("not find ID\n");
        return FAILURE;
    }

    char *pstr = NULL;
    char *pstr_save = NULL;
    char szdevid[64] = {0};
    char szip[64] = {0};
    char szmask[64] = {0};
    char szgateway[64] = {0};
    char idbuf[16] = {0};

    pstr = strtok_r(pbuf, "#", &pstr_save);
    if (pstr == NULL) {
        return FAILURE;
    }

    do {
        if (!strncmp(pstr, "ID=", 3)) {
            memcpy(szdevid, pstr + 3, sizeof(szdevid) - 1);
        } else if (!strncmp(pstr, "IP=", 3)) {
            memcpy(szip, pstr + 3, sizeof(szip) - 1);
        } else if (!strncmp(pstr, "SubMask=", 8)) {
            memcpy(szmask, pstr + 8, sizeof(szmask) -1);
        } else if (!strncmp(pstr, "GateWay=", 8)) {
            memcpy(szgateway, pstr + 8, sizeof(szgateway) - 1);
        }

        pstr = strtok_r(NULL, "#", &pstr_save);
    } while (NULL != pstr);

    system_get_dev_id(idbuf);
    if (strcmp(szdevid, idbuf) != 0) {
        return FAILURE;
    }

    if (szip[0] || szmask[0] || szgateway[0]) {
        char jcpcmd[256] = "ethcfg -act set ";
        char jcpresult[128] = {0};
        char temp[128] = {0};

        if (szip[0]) {
            sprintf(temp, "-ethip %s ", szip);
            strcat(jcpcmd, temp);
        }

        if (szmask[0]) {
            sprintf(temp, "-ethmask %s ", szmask);
            strcat(jcpcmd, temp);
        }

        if (szgateway[0]) {
            sprintf(temp, "-ethgw %s ", szgateway);
            strcat(jcpcmd, temp);
        }
        DBG("jcpcmd = %s\n", jcpcmd);
        jcpcmd_sendrecv(jcpcmd, jcpresult, sizeof(jcpresult));
    }

    return SUCCESS;
}

static int search_devaffifun_set(char *buf_recv)
{
    char cid[64] = {0,};
    char config[128] = {0,};

    if(2 != sscanf(buf_recv,"DEVAFFIFUNSET ID=%10[0-9]#%127s", cid, config)) {
        return FAILURE;
    }

    DBG("the jcp cmd is not use\n");

    return SUCCESS;
}

static int search_jcpcmd_set(char *buf_recv, char *szhost, int port, int sock_fd, char *eth_name)
{
    /*
     * JCPMETHOD * HDS/1.0
     * ID=83918977#JcpCmd=devvecfg -act list#
     */
    char *pstr_save = NULL;
    char szjcp[JCP_MAX_LEN] = {0};
    char szjcp_result[JCP_MAX_LEN] = {0};
    char devid[64] = {0};
    char idbuf[16] = {0};
    int maxheight;
    maxheight = get_maxheight_2MTo3M();
    char *pjcp_buf = buf_recv + strlen("JCPMETHOD * HDS/1.0\r\n") - 1;
    *pjcp_buf = '\0';
    pjcp_buf++;

    if (!strstr(pjcp_buf, "ID=")) {
        return FAILURE;
    }

    char *pstr = strtok_r(pjcp_buf, "#", &pstr_save);
    if (pstr == NULL) {
        return FAILURE;
    }

    do {
        if (!strncmp(pstr, "ID=", 3)) {
            strncpy(devid, pstr + 3, sizeof(devid) - 1);
        } else if (!strncmp(pstr, "JcpCmd=", 7)) {
            strncpy(szjcp, pstr + 7, sizeof(szjcp)-1);
        }

        pstr = strtok_r(NULL, "#", &pstr_save);
    } while(NULL != pstr);


    system_get_dev_id(idbuf);
    if (strcmp(devid, idbuf) != 0) {
        if(1296 == maxheight){
            if (strncmp(devid, "NVR_", 4) != 0) {
                return FAILURE;
            }
            //20191210修改，增强判断条件：当wlan0、eth0和接收IP都不相同的时候返回。
            char szeth0IP[32];
            char szwlan0IP[32];
            
            net_get_ipaddr("eth0", szeth0IP,sizeof(szeth0IP));
            net_get_ipaddr("wlan0", szwlan0IP,sizeof(szwlan0IP));
            if ((strcmp(devid + 4, szeth0IP) != 0)&&(strcmp(devid + 4, szwlan0IP) != 0)){
                return FAILURE;
            }
        }else{
            return FAILURE;
        }
    }


    if (strlen(szjcp) == 0) {
        ERR("jcp len  = 0\n");
        return FAILURE;
    }

    if (strcmp(eth_name, "wlan0") == 0 && (strstr(szjcp, "ethcfg") != NULL)) {
        sprintf(szjcp_result, "%s", "[Success]\r\n");
    } else {
        jcpcmd_sendrecv(szjcp, szjcp_result, sizeof(szjcp_result));
    }

    dbg_search("\n__Send: %s\n__Resp: %s\n", szjcp, szjcp_result);

    char szreply[JCP_MAX_LEN + 128/*header*/] = {0};
    snprintf(szreply, sizeof(szreply)-1, "JCPMETHOD %sDevice-ID:%s\r\n", szjcp_result, idbuf);

    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(szhost);
    addr.sin_port = htons(port);

    int ret = sendto(sock_fd, szreply, strlen(szreply), 0, (struct sockaddr*)&addr, sizeof(addr));
    if (ret <= 0) {
        dbg_search("sendto %s:%d fail\n", szhost, port);
        return FAILURE;
    }

    //广播回复
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("255.255.255.255");
    addr.sin_port = htons(port);
    ret = sendto(sock_fd, szreply, strlen(szreply), 0, (struct sockaddr*)&addr, sizeof(addr));
    if (ret <= 0) {
        dbg_search("sendto %s:%d fail\n", "255.255.255.255", port);
        return FAILURE;
    }

    return SUCCESS;
}

static int search_recv_agtest_file(char *buf_recv)
{
    if (get_g_sys(agingtest)) {
        dbg_search("===================runing aging test file======================\n");
        return 1;
    }

    DBG("======================search_recv_agtest_file==========================\n");
    const char * dstFile = "/opt/aging_test.sh";
    if(0 == access(dstFile, F_OK)){
        DBG("============/opt/agtest is exit==========\n");
        remove(dstFile);
    }
    WriteFile(dstFile, buf_recv);

    return 0;
}

static void search_net_handle(int fd, int ev, void *userdata)
{
    char buf_recv[SEARCH_BUFLEN] = {0};
    int  readbyte = 0;
    char szhost[32] = {0};
    struct sockaddr_in addr;
    int addrlen = sizeof(addr);
    char *host = NULL;
    int port = 0;
    int nRetry = 0;

    do {
        memset(buf_recv, 0, sizeof(buf_recv));
        readbyte = recvfrom(fd, buf_recv, sizeof(buf_recv)-1, 0,
                            (struct sockaddr *)&addr, (socklen_t *)&addrlen);
        if (readbyte < 0){
            break;
        }

        host = j_inet_ntoa(addr.sin_addr);
        port = ntohs(addr.sin_port);
        memcpy(szhost, host,strlen(host));

        if (get_g_log(search)) {
            printf("event:%d  readbyte:%d loop:%d\n", ev, readbyte, nRetry); 
            printf("-----------------------------------------------------\n");
            printf("%s IPCtool.msg from %s:%d\n%s\n", (char *)userdata, szhost, port, buf_recv);
            printf("-----------------------------------------------------\n\n");
        }

        if (strstr(buf_recv, "SEARCH")) {
            search_response(buf_recv, fd, (char *)userdata, szhost);
        } else if (strstr(buf_recv, "IDENTIFY")) {
            ;
        } else if (strstr(buf_recv, "IPSET * HDS/1.0\r\n")) {
            search_ipset(buf_recv);
        } else if (strstr(buf_recv, "DEVAFFIFUNSET")) { //设置附加值
            search_devaffifun_set(buf_recv);
        } else if (strstr(buf_recv, "JCPMETHOD * HDS/1.0\r\n")) {
            search_jcpcmd_set(buf_recv, szhost, port, fd, (char *)userdata);
        } else if(strstr(buf_recv, "[AGING]")){
            search_recv_agtest_file(buf_recv);
        }
        if (++nRetry >= 10) {
            DBG("snaker search frome badguy: %s\n", szhost);
            usleep(500*1000);  // make sure cpu not to high
            break;
        }
    } while(1);
}

static int search_multicast_socket(const char *nic_name)
{
    char ip[32] = {0};
    net_get_ipaddr(nic_name, ip,sizeof(ip));
    if (strlen(ip) == 0) {
        //printf("getip %s fail\n", nic_name);
        return -1;
    }

    int sock_fd = -1;
    int ret = 0;
    struct sockaddr_in addr;
    int isize = 100*1024;
    socklen_t size_len;
    int cur_flag = 0;
    struct ifreq ifr;

    // 创建组播套接字
    if (0 > (sock_fd = socket(AF_INET, SOCK_DGRAM, 0))) {
        ERR("create socket fail\n");
        goto cleanup;
    }
    ret = 1;
    if (-1 == setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &ret, sizeof(ret))) {
        ERR("_________ setsockopt SO_REUSEADDR %s fail \n", nic_name);
        goto cleanup;
    }
        
    // 设置广播属性
    ret = 1;
    if (setsockopt(sock_fd, SOL_SOCKET, SO_BROADCAST, &ret, sizeof(ret))) {
        DBG("setsockopt SO_BROADCAST fail, errno=%d %s\n", errno, strerror(errno));
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SEARCH_MULTI_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(-1 == bind(sock_fd, (struct sockaddr *)&addr, sizeof(addr))) {
        ERR("bind fail!\n");
        goto cleanup;
    }
    cur_flag = fcntl(sock_fd, F_GETFL, 0);
    cur_flag = fcntl(sock_fd, F_SETFL, cur_flag | O_NONBLOCK);

    // 不发送到回环接口
    ret = 0;
    if(-1 == setsockopt(sock_fd, IPPROTO_IP, IP_MULTICAST_LOOP, &ret, sizeof(ret))) {
        ERR("setsockopt IP_MULTICAST_LOOP fail!\n");
    }

    size_len = sizeof(isize);
    if (setsockopt(sock_fd, SOL_SOCKET, SO_RCVBUF, (char*)&isize, size_len) < 0) {
        DBG("setsockopt SO_RCVBUF error!\n");
    }

    // 绑定网卡
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, nic_name, IFNAMSIZ);
    if (-1 == setsockopt(sock_fd, SOL_SOCKET, SO_BINDTODEVICE, (char *)&ifr, sizeof(ifr))) {
        DBG("setsockopt SO_BINDTODEVICE error, %s may not exist!\n", nic_name);
        goto cleanup;
    }

    return sock_fd;

cleanup:
    if (sock_fd >= 0) {
        close(sock_fd);
    }
    return FAILURE;
}

static int search_multicast_add(int *fd, const char *nic)
{
    char ip[32] = {0};

    net_get_ipaddr(nic, ip,sizeof(ip));
    if (strlen(ip) == 0) {
        //printf("getip %s fail\n", nic);
        return 0;
    }

    struct ip_mreq mreq;
    memset(&mreq, 0, sizeof(struct ip_mreq));
    mreq.imr_multiaddr.s_addr = inet_addr(SEARCH_MULTI_ADDR);
    mreq.imr_interface.s_addr = inet_addr(ip);

    dbg_search("__to__ IP_ADD_MEMBERSHIP %s [%s] fd:%d\n", nic, ip, *fd);

    if (-1 == setsockopt(*fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq))) {
        dbg_search("fail to IP_ADD_MEMBERSHIP %s, fd=%d, error=[%d] %s\n", nic, *fd, errno, strerror(errno));
        if (*fd >= 0) {
            close(*fd);
            *fd = -1;
        }
        return FAILURE;
    }
    
    struct in_addr inAddr;
    memset(&inAddr, 0, sizeof(struct in_addr));
    inAddr.s_addr = inet_addr(ip);
    if (-1 == setsockopt(*fd, IPPROTO_IP, IP_MULTICAST_IF, (char *)&inAddr, sizeof(inAddr))) {
        dbg_search("IP_MULTICAST_IF faild of %s\n", ip);
        if (*fd >= 0) {
            close(*fd);
            *fd = -1;
        }
    
        return FAILURE;
    }

    return SUCCESS;
}

static int search_multicast_drop(int *fd, const char *nic)
{
    char ip[32] = {0};
    net_get_ipaddr(nic, ip,sizeof(ip));
    if (strlen(ip) == 0) {
        //printf("getip %s fail\n", nic);
        return 0;
    }
    struct ip_mreq mreq;

    memset(&mreq, 0, sizeof(struct ip_mreq));
    mreq.imr_multiaddr.s_addr = inet_addr(SEARCH_MULTI_ADDR);
    mreq.imr_interface.s_addr = inet_addr(ip); 

    if (-1 == setsockopt(*fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq))) {
        dbg_search("fail of IP_DROP_MEMBERSHIP fd=%d [%d]%s\n", *fd, errno, strerror(errno));
        return FAILURE;
    }

    return SUCCESS;
}

static int server_search(int *fd, const char*nic, JSRWHandle *lfd)
{
    *fd = search_multicast_socket(nic);

    if (*fd >= 0) {
        js_create_reader_r(sch_slow, *fd, JS_READABLE, search_net_handle, (void *)nic, lfd);
        if (*fd) {
            SYSLOG("create %s search success @fd:%d\n", nic, *fd);
        } else {
            dbg_search("__to__ create bgread @%s fd:%d\n", nic, *fd);
        }
    }
    
    return SUCCESS;
}
#if 0
static int net_exist_eth()
{
    static int exist = -1;

    if (exist != -1) {
        return exist;
    }

    int    i = 0, sock = 0;
    struct ifreq ifreq;

    if( (sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("socket fail\n");
        return 0;
    }

    exist = 0; // init-non-exist

    for(i = 1; i < 10; i++){
        ifreq.ifr_ifindex = i;
        if(ioctl(sock, SIOCGIFNAME, &ifreq) < 0 ) {
            printf("check eth over @%d\n", --i);
            break;
        }

        printf("index %d is %s\n", i, ifreq.ifr_name);

        if (strstr(ifreq.ifr_name, "eth")) {
            exist = 1;
            break;
        }
    }

    close(sock);
    SYSLOG("eth is %s\n", exist?"exist":"not exist");
    return exist;
}
static int is_same_seg(char *dst_ip, char *src_ip)
{
    if (dst_ip == NULL || src_ip == NULL) {
        return -1;
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
#endif
static void server_search_cb(void *data)
{
    static int ticks = 0;
    dbg_search("__intv__ call cb is %d seconds, ticks:%d\n", INTV_SEARCH, ticks);

    if(is_inc_mod0(ticks,60)) {
        if (g_sockfd == -1) {
            server_search(&g_sockfd, ETH_NIC_NAME, &fd_eth0);
            search_multicast_add(&g_sockfd, ETH_NIC_NAME);
        } else {
             search_multicast_drop(&g_sockfd, ETH_NIC_NAME);
             search_multicast_add(&g_sockfd, ETH_NIC_NAME);
        }

        if(g_wifi_fd != -1) {
            search_multicast_drop(&g_wifi_fd, WIFI_NIC_NAME);
            search_multicast_add(&g_wifi_fd, WIFI_NIC_NAME);
        }
        
        ticks = 1;
    }
    
    if (get_g_sys(usb_4g)) {
        return;
    }
    
    char wifiip[16] = {0};
    char eth0ip[16] = {0};
    
    net_get_ipaddr("wlan0", wifiip, sizeof(wifiip));
    net_get_ipaddr("eth0", eth0ip, sizeof(eth0ip));
    
    if (1 == net_link_status("eth0")) {
        if (fd_wlan) {
            dbg_search("_____________ eth0 is working, no Resp on WiFi ___________\n");
            js_delete_reader_r(&fd_wlan);
        }
        
        if (g_wifi_fd > 0) {
            search_multicast_drop(&g_wifi_fd, WIFI_NIC_NAME);
            close(g_wifi_fd);
            g_wifi_fd = -1;
        }
    } else {
        if (g_wifi_fd == -1) {
            dbg_search("_____________ wifi is working, Resp on WiFi start _____\n");
            server_search(&g_wifi_fd, WIFI_NIC_NAME, &fd_wlan);
            search_multicast_add(&g_wifi_fd, WIFI_NIC_NAME);
        }
    }
}

void uninit_server_search()
{
    js_delete_reader_r(&fd_eth0);

    js_delete_reader_r(&fd_wlan);
 
    js_delete_timer_r(&hdl_watch);

    if (g_sockfd > 0) {
        search_multicast_drop(&g_sockfd, ETH_NIC_NAME);;
        close(g_sockfd);
        g_sockfd = -1;
    }
    if (g_wifi_fd > 0) {
        search_multicast_drop(&g_wifi_fd, WIFI_NIC_NAME);
        close(g_wifi_fd);
        g_wifi_fd = -1;
    }
}

void init_server_search()
{
    js_create_timer_r(sch_slow, 15*1000, INTV_SEARCH, server_search_cb, NULL, &hdl_watch);
}

