/******************************************************************************
    Copyright (C), 2008-2018, JABSCO ELECTRONIC Tech. Co., Ltd

    File Name       : jco_upnp.c
    Version     : 1.0
    Author      : JABSCO Video Server Software Group
    Created     : 2009.03.24
    Description : upnp functions
    History     :
                    Create by nomadzhao.2009.03.24
******************************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <pthread.h>

#include "socket_api.h"
#include "confapi.h"
#include "debug.h"

#include "miniwget.h"
#include "miniupnpc.h"
#include "upnpcommands.h"
#include "upnp.h"
#include "upnpdesc.h"
#include "jconfig.h"
#include "conf_list.h"
#include "utils.h"

#if defined(DEV_TYPE_ENHANCED)
/*======================================================================
    local define
======================================================================*/
#define UPNP_PORT_MAX_NUM       32

#define UPNP_PORT_BEGIN         50000
#define UPNP_PORT_END           60000

#define INTV_UPNP               20      // second
#define INTV_UPNP_MS            (INTV_UPNP*1000)

#define SYSTEM_WEB_PATH     "/ipc/web/"

typedef struct {
    unsigned short extPort;
    unsigned short intPort;
    int proto;              // __socket_type, 1 is tcp, 2 is udp
    BOOL enable;            // enable or disable
} UPNP_PORT_ATTR_S;

typedef struct {
    BOOL enable;            // enable or disable
    unsigned short iRandom; // external random port
    char szIntIP[16];       // internal ip address, should get from upnp server
    char szExtIP[16];       // external ip address, should get from upnp server
    int iPortNum;           // number of upnped ports
    UPNP_PORT_ATTR_S ports[UPNP_PORT_MAX_NUM];
} UPNP_PORT_MNG_S;

typedef struct {
    char szIP[32];
    int port;
} UPNP_SERVER_MNG;

UPNP_PORT_MNG_S portMng;
UPNP_SERVER_MNG svrMng;

static JSScheduler sch_upnp = NULL;
static JSScheduler sch_disc = NULL;
static JSRWHandle  soc_ssdp = NULL;
static JSTCHandle  hdl_enable = NULL;
static JSTCHandle  hdl_update = NULL;
static JSTCHandle  hdl_process = NULL;

static net_sockfd_t sockobj = {0};

pthread_mutex_t portMutex;
pthread_mutex_t svrMutex;

static void portLock()
{
    pthread_mutex_lock(&portMutex);
}
static void portUnlock()
{
    pthread_mutex_unlock(&portMutex);
}

static void svrLock()
{
    pthread_mutex_lock(&svrMutex);
}
static void svrUnlock()
{
    pthread_mutex_unlock(&svrMutex);
}

/*======================================================================
    local function of upnp client
======================================================================*/
static void UpnpGetPort(UPNP_PORT_MNG_S *mng, struct UPNPUrls *urls, struct IGDdatas *data)
{
    int r;
    int i = 0;
    int iGetPort = 0;
    char index[6];
    char intClient[16];
    char intPort[6];
    char extPort[6];
    char protocol[4];
    char desc[80];
    char enabled[6];
    char rHost[64];
    char duration[16];

    do {
        snprintf(index, 6, "%d", i);
        rHost[0] = '\0';
        enabled[0] = '\0';
        duration[0] = '\0';
        desc[0] = '\0';
        extPort[0] = '\0';
        intPort[0] = '\0';
        intClient[0] = '\0';
        r = UPNP_GetGenericPortMappingEntry(urls->controlURL, data->servicetype,
                                            index,
                                            extPort, intClient, intPort,
                                            protocol, desc, enabled,
                                            rHost, duration);

        if (UPNPCOMMAND_SUCCESS == r /*&& !strcmp(intClient, mng->szIntIP)*/) {     //edit by ya 20120420 not judge using devid
            mng->ports[iGetPort].proto = strcasecmp(protocol, "UDP") ? 1 : 2;
            mng->ports[iGetPort].extPort = atoi(extPort);
            mng->ports[iGetPort].intPort = atoi(intPort);

            iGetPort++;
            if (UPNP_PORT_MAX_NUM <= iGetPort) {
                DBG("mng full\n");
                break;
            }
        }
        i++;
    } while (UPNPCOMMAND_SUCCESS == r);
    mng->iPortNum = iGetPort;

    // add protect for router's bug
    for (i = SYSTEM_PORT_WEB; i < SYSTEM_PORT_END; i++) {
        portLock();
        if (!portMng.ports[i].enable) {
            portUnlock();
            continue;
        }
        portUnlock();

        int j = 0;
        for (j = 0; j < mng->iPortNum; j++) {
            portLock();
            if (SOCK_STREAM == mng->ports[j].proto
                && mng->ports[j].intPort == portMng.ports[i].intPort) {
                portUnlock();
                break;
            }
            portUnlock();
        }

        if (j >= iGetPort) {
            // not find in list, get it directly
            char szEPort[8];
            char szInPort[8];
            sprintf(szEPort, "%d", mng->ports[j].extPort);

            UPNP_GetSpecificPortMappingEntry(urls->controlURL,
                                             data->servicetype,
                                             szEPort, "TCP",
                                             intClient, szInPort);
            if (!strcmp(intClient, mng->szIntIP)) {
                mng->ports[mng->iPortNum].proto = 1;
                mng->ports[mng->iPortNum].extPort = mng->ports[j].extPort;
                mng->ports[mng->iPortNum].intPort = atoi(szInPort);
                mng->iPortNum++;
            }
        }
    }
}

static void UpnpCleanPort(UPNP_PORT_MNG_S *mng, struct UPNPUrls *urls, struct IGDdatas *data)
{
    int i = 0;
    int count = 0;
    int retry = 0;
    int r;
    char index[6];
    char intClient[16];
    char intPort[6];
    char extPort[6];
    char protocol[4];
    char desc[80];
    char enabled[6];
    char rHost[64];
    char duration[16];

    // clear all port from intPort
    do {
        i = 0;
        count = 0;
        char arrDevID[20] = {0};
        SysInfoS info = {{0},};
        get_config(handleSysInfoCfg, info);

        do {
            memset(arrDevID, 0, sizeof(arrDevID));
            snprintf(arrDevID, sizeof(arrDevID), "%s", info.devid);
            snprintf(index, 6, "%d", i);
            rHost[0] = '\0';
            enabled[0] = '\0';
            duration[0] = '\0';
            desc[0] = '\0';
            extPort[0] = '\0';
            intPort[0] = '\0';
            intClient[0] = '\0';
            r = UPNP_GetGenericPortMappingEntry(urls->controlURL, data->servicetype,
                                                index,
                                                extPort, intClient, intPort,
                                                protocol, desc, enabled,
                                                rHost, duration);


            if (UPNPCOMMAND_SUCCESS == r && !strcmp(arrDevID, desc)) { //add by ya 20120419 filter other device using devID
                count++;
                r = UPNP_DeletePortMapping(urls->controlURL,
                                           data->servicetype,
                                           extPort,
                                           protocol,
                                           NULL);
            }
            i++;
        } while (UPNPCOMMAND_SUCCESS == r);
    } while (0 < count && 15 > ++retry);

    // clear mng's extPort
    for (i = 0; i < ARRAY_SIZE(mng->ports); i++) {
        mng->ports[i].extPort = mng->ports[i].intPort;
    }

    mng->iPortNum = 0;
}

static int UpnpRedirectPort(struct UPNPUrls *urls,
                            struct IGDdatas *data,
                            int eport,
                            int proto,
                            SYSTEM_PORT_E offport)
{
    char intClient[16];
    char szInPort[8];
    char szEPort[8];
    int r;
    char arrDevID[20] = {0};

    SysInfoS info = {{0},};
    char intIp[20] = {0};

    get_config(handleSysInfoCfg, info);

    snprintf(arrDevID, sizeof(arrDevID), "%s", info.devid);
    sprintf(szEPort, "%d", eport);

    portLock();
    sprintf(szInPort, "%d", SOCK_STREAM == proto ? portMng.ports[offport].intPort : UPNP_PORT_BEGIN + offport);
    snprintf(intIp, sizeof(intIp), "%s", portMng.szIntIP);
    portUnlock();

    r = UPNP_AddPortMapping(urls->controlURL, data->servicetype,
                            szEPort, szInPort, intIp, arrDevID,
                            SOCK_STREAM == proto ? "TCP" : "UDP", NULL);//add by ya 20120419  fill in desc with devID

    if (UPNPCOMMAND_SUCCESS != r) {
        ERR("AddPortMapping(%s, %s, %s) failed\n", szEPort, szInPort, intIp);
        return FAILURE;
    }

    UPNP_GetSpecificPortMappingEntry(urls->controlURL,
                                     data->servicetype,
                                     szEPort, SOCK_STREAM == proto ? "TCP" : "UDP",
                                     intClient, szInPort);
    if (!strcmp(intClient, intIp)) {
        //DBG("InternalIP:Port = %s:%s\n", intClient, szInPort);
    } else {
        ERR("GetSpecificPortMappingEntry(%s, %s, %s) failed.\n", szEPort, szInPort, intIp);
        return FAILURE;
    }

    portLock();
    if (SOCK_STREAM == proto) {
        portMng.ports[offport].extPort = eport;
        portMng.ports[offport].proto = proto;
    } else {
        portMng.ports[SYSTEM_PORT_END + offport].extPort = eport;
        portMng.ports[SYSTEM_PORT_END + offport].proto = proto;
    }

    portMng.iPortNum++;
    portUnlock();

    return SUCCESS;
}

static int UpnpReqPorts(struct UPNPUrls *urls, struct IGDdatas *data)
{
    char intClient[16];
    char szInPort[8];
    char szEPort[8];
    int i = 0;
    int retry = 10;

    portLock();
    int iEPort = portMng.iRandom;
    portUnlock();

    // search unused ports
    for (; retry > 0; retry--) {
        // for tcp
        for (i = SYSTEM_PORT_WEB; i < SYSTEM_PORT_END; i++) {
            sprintf(szEPort, "%d", iEPort + i);
            UPNP_GetSpecificPortMappingEntry(urls->controlURL,
                                             data->servicetype,
                                             szEPort, "TCP",
                                             intClient, szInPort);
            portLock();
            if (7 <= strlen(intClient) && strcmp(portMng.szIntIP, intClient)) {
                // "1.1.1.1" strlen == 7
                DBG("Used TCP:Port = %s:%s %d\n", intClient, szInPort, i);
                portUnlock();
                break;
            } else {
                //printf("Unused TCP(%s, %d)\n", szEPort, i);
            }
            portUnlock();
        }

        if (SYSTEM_PORT_END > i) {
            iEPort = rand() & 0xFFFF;
            continue;
        }

        break;
    }

    if (0 >= retry) {
        return FAILURE;
    }

    portLock();
    portMng.iRandom = iEPort;   // update random
    portUnlock();

    for (i = SYSTEM_PORT_WEB; i < SYSTEM_PORT_END; i++) {
        portLock();
        int enable = portMng.ports[i].enable;
        portUnlock();

        if (enable
            && SUCCESS != UpnpRedirectPort(urls, data,  iEPort + i, SOCK_STREAM, (SYSTEM_PORT_E)i)) {
            return FAILURE;
        }
    }

    portLock();
    for (i = SYSTEM_PORT_WEB; i < SYSTEM_PORT_END; i++) {
        if (portMng.ports[i].enable) {
            DBG("%02d - TYPE %d TRAN %d:%d\n",
                i, portMng.ports[i].proto, portMng.ports[i].extPort, portMng.ports[i].intPort);
        }
    }
    DBG("external ipaddr=%s, portNum=%d\n", portMng.szExtIP, portMng.iPortNum);
    portUnlock();

    return SUCCESS;
}

static BOOL UpnpClearNeeded(UPNP_PORT_MNG_S *mngTmp)
{
    if (!strlen(portMng.szIntIP)) {
        // 第一次运行UPNP
        strcpy(portMng.szIntIP, mngTmp->szIntIP);

        unsigned int addr = ntohl(inet_addr(portMng.szIntIP));
        srand(addr);
        portMng.iRandom = UPNP_PORT_BEGIN + SYSTEM_PORT_END * (addr & 0xFF);

        return TRUE;
    }

    if (strcmp(portMng.szIntIP, mngTmp->szIntIP)) {
        // local ip change
        strcpy(portMng.szIntIP, mngTmp->szIntIP);

        unsigned int addr = ntohl(inet_addr(portMng.szIntIP));
        srand(addr);
        portMng.iRandom = UPNP_PORT_BEGIN + SYSTEM_PORT_END * (addr & 0xFF);

        return TRUE;
    }

    // port number change
    if (portMng.iPortNum != mngTmp->iPortNum) {
        DBG("portMng.iPortNum != mngTmp->iPortNum\n");
        return TRUE;
    }

    // port config change
    int i = 0;
    int j = 0;

    NetPortS netport = {0};
    int iPorts[SYSTEM_PORT_END] = {0};
    get_config(handleNetPortCfg, netport);

    iPorts[SYSTEM_PORT_WEB] = netport.httpport;
    iPorts[SYSTEM_PORT_FTP] = netport.ftpport;
    iPorts[SYSTEM_PORT_RTSP] = netport.rtspport;
    iPorts[SYSTEM_PORT_VOICE] = netport.audioport;
    iPorts[SYSTEM_PORT_UPDATE] = netport.updateport;

    NetUpnpS upnp = {0};
    BOOL bEn[SYSTEM_PORT_END] = {FALSE, FALSE ,FALSE ,FALSE ,FALSE};

    get_config(handleUpnpCfg, upnp);
    bEn[SYSTEM_PORT_WEB] = upnp.http;
    bEn[SYSTEM_PORT_FTP] = upnp.ftp;
    bEn[SYSTEM_PORT_RTSP] = upnp.rtsp;
    bEn[SYSTEM_PORT_VOICE] = upnp.voice;
    bEn[SYSTEM_PORT_UPDATE] = upnp.update;

    for (i = (int)SYSTEM_PORT_WEB; i < (int)SYSTEM_PORT_END; i++) {
        if (portMng.ports[i].intPort != iPorts[i]
            || portMng.ports[i].enable != bEn[i]) {
            portMng.enable = FALSE;
            for (j = (int)SYSTEM_PORT_WEB; j < (int)SYSTEM_PORT_END; j++) {
                portMng.ports[j].intPort = iPorts[j];
                portMng.ports[j].enable = bEn[j];

                if (portMng.ports[j].enable) {
                    portMng.enable = TRUE;
                }
            }
            return TRUE;
        }
    }

    // upnp router of my device inter port change
    for (i = (int)SYSTEM_PORT_WEB; i < (int)SYSTEM_PORT_END; i++) {
        if (!portMng.ports[i].enable) {
            continue;
        }

        for (j = 0; j < mngTmp->iPortNum; j++) {
            if (SOCK_STREAM == mngTmp->ports[j].proto
                && portMng.ports[i].intPort == mngTmp->ports[j].intPort) {
                break;
            }
        }

        if (j >= mngTmp->iPortNum) {
            DBG("Here!!!\n");
            return TRUE;
        }
    }

    // upnp router of my device ext port change
    for (i = (int)SYSTEM_PORT_WEB; i < (int)SYSTEM_PORT_END; i++) {
        if (!portMng.ports[i].enable) {
            continue;
        }

        for (j = 0; j < mngTmp->iPortNum; j++) {
            if (SOCK_STREAM == mngTmp->ports[j].proto
                && portMng.ports[i].intPort == mngTmp->ports[j].intPort
                && portMng.ports[i].extPort != mngTmp->ports[j].extPort) {
                return TRUE;
            }
        }
    }

    return FALSE;
}

static void UpnpClientProcess1(void *instance)
{
    //DBG("upnp client proccess...\n");
    static struct UPNPDev * devlist = NULL;
    struct UPNPUrls urls;
    struct IGDdatas data;
    UPNP_PORT_MNG_S mngTmp;
    char lanaddr[16];       // my ip address on the LAN
    char wanaddr[16];       // external ip address of router
    static int failCnt = 0; // upnp reg port fail counter
    int i = 0;

    if (NULL != devlist) {
        freeUPNPDevlist(devlist);
        devlist = NULL;
    }

    NetUpnpS upnp = {0};
    if(get_config(handleUpnpCfg, upnp) < 0) {
        ERR("Upnp conf_get_upnpcfg failed!\n");
        return ;
    }

    portLock();
    if(upnp.ftp || upnp.http || upnp.rtsp
       || upnp.voice || upnp.update) {
        portMng.enable =  TRUE;
    }

    if (FALSE == portMng.enable && 0 >= portMng.iPortNum) //判断portMng.iPortNum为了当所有upnp
        //被关闭清除route上端口的动作
    {
        //DBG("return...\n");
        portUnlock();
        return ;
    }

    if (++failCnt > 3 && strlen(portMng.szExtIP)) {
        // router's upnp become closed or have other error
        // clear upnp value
        memset(portMng.szExtIP, 0, sizeof(portMng.szExtIP));
        for (i = SYSTEM_PORT_WEB; i < SYSTEM_PORT_END; i++) {
            portMng.ports[i].extPort = portMng.ports[i].intPort;
        }
    }
    portUnlock();

    // send discover
    devlist = upnpDiscover(3000, NULL, NULL, 0);
    if (!devlist) {
        return;
    }

    // remove same type device with me
    struct UPNPDev * device = NULL;
    struct UPNPDev **preDevice = &devlist;
    for (device = devlist; device;) {
        if (device->usn
            && device->server
            && !strncasecmp(device->usn, "uuid:01010203-0405-0607", 23)
            && !strncasecmp(device->server, "NVS/4", 5)) {
            struct UPNPDev *next = device->pNext;
            free(device);

            *preDevice =
                device = next;
        } else {
            preDevice = &(*preDevice)->pNext;
            device = device->pNext;
        }
    }

    // get my local ip
    if (1 != UPNP_GetValidIGD(devlist, &urls, &data, lanaddr, sizeof(lanaddr))) {
        return;
    }

    // get router wan ip
    UPNP_GetExternalIPAddress(urls.controlURL, data.servicetype, wanaddr);
    if (7 > strlen(wanaddr)) {  // "1.1.1.1" strlen == 7
        // avoid the router at lan, not have wan ip
        return;
    }
    failCnt = 0;

    // get ports
    memset(&mngTmp, 0, sizeof(mngTmp));
    strcpy(mngTmp.szIntIP, lanaddr);
    strcpy(mngTmp.szExtIP, wanaddr);
    UpnpGetPort(&mngTmp, &urls, &data);

    portLock();
    BOOL bDelPort = UpnpClearNeeded(&mngTmp);
    BOOL pMngEn = portMng.enable;
    portUnlock();

    //DBG("bDelPort : %d, pMngEn : %d\n", bDelPort, pMngEn);
    if (bDelPort || FALSE == pMngEn) {
        if (0 < mngTmp.iPortNum) { //路由已经映射的端口数
            // 清除服务器上存在的映射端口
            DBG("Clear port on server.\n");
            UpnpCleanPort(&mngTmp, &urls, &data);
        }

        portLock();
        for (i = SYSTEM_PORT_WEB; i < SYSTEM_PORT_END; i++) {
            portMng.ports[i].extPort = portMng.ports[i].intPort;
        }
        portMng.iPortNum = 0;

        if (FALSE == portMng.enable) {
            memset(portMng.szExtIP, 0, sizeof(portMng.szExtIP));

            portUnlock();
            return;
        }
        portUnlock();
    }

    portLock();
    if (strcmp(portMng.szExtIP, mngTmp.szExtIP)) {
        strcpy(portMng.szExtIP, mngTmp.szExtIP);
    }
    portUnlock();

    // 申请端口
    if (bDelPort) {
        if (SUCCESS != UpnpReqPorts(&urls, &data)) {
            // if upnp request failed, clear the wan ip
            portLock();
            for (i = SYSTEM_PORT_WEB; i < SYSTEM_PORT_END; i++)
                portMng.ports[i].extPort = portMng.ports[i].intPort;
            memset(portMng.szExtIP, 0, sizeof(portMng.szExtIP));
            portUnlock();
        }
        //else
        //JCONet3322DDNS();
    }
}

static void *UpnpClientProcess(JSScheduler sch)
{
    NetPortS netport = {0};
    NetUpnpS upnp = {0};

    // init ports
    int iPorts[SYSTEM_PORT_END] = {0};
    get_config(handleNetPortCfg, netport);

    iPorts[SYSTEM_PORT_WEB] = netport.httpport;
    iPorts[SYSTEM_PORT_FTP] = netport.ftpport;
    iPorts[SYSTEM_PORT_RTSP] = netport.rtspport;
    iPorts[SYSTEM_PORT_VOICE] = netport.audioport;
    iPorts[SYSTEM_PORT_UPDATE] = netport.updateport;

    BOOL bEn[SYSTEM_PORT_END] = {FALSE, FALSE ,FALSE ,FALSE ,FALSE};

    get_config(handleUpnpCfg, upnp);
    bEn[SYSTEM_PORT_WEB] = upnp.http;
    bEn[SYSTEM_PORT_FTP] = upnp.ftp;
    bEn[SYSTEM_PORT_RTSP] = upnp.rtsp;
    bEn[SYSTEM_PORT_VOICE] = upnp.voice;
    bEn[SYSTEM_PORT_UPDATE] = upnp.update;

    portLock();
    portMng.enable = FALSE;
    for (int i = SYSTEM_PORT_WEB; i < SYSTEM_PORT_END; i++) {
        portMng.ports[i].intPort =
            portMng.ports[i].extPort = iPorts[i];
        portMng.ports[i].enable = bEn[i];

        if (portMng.ports[i].enable) {
            portMng.enable = TRUE;
        }
    }

    portMng.iRandom = UPNP_PORT_BEGIN;
    portUnlock();

    if (hdl_process == NULL) {
        js_create_timer_r(sch, INTV_UPNP_MS, INTV_UPNP_MS, UpnpClientProcess1, NULL, &hdl_process);
    }

    return NULL;
}

/*======================================================================
    local function of upnp server
======================================================================*/
void UpnpServerSendNotify(int sock)
{
    struct sockaddr_in addr;
    char buf[512];
    int len = 0;
    int ret = 0;

    len = snprintf(buf, sizeof(buf),
                   "NOTIFY * HTTP/1.1\r\n"
                   "HOST:239.255.255.250:1900\r\n"
                   "Cache-Control:max-age=%u\r\n"
                   "Location:http://%s:%d/rootDesc.xml\r\n"
                   "Server: %s UPnP/1.0 MiniUPnPd/1.2 \r\n"
                   "NT:%s\r\n"
                   "USN:%s::%s\r\n"
                   "NTS:ssdp:alive\r\n"
                   "\r\n",
                   INTV_UPNP >> 1,
                   svrMng.szIP, svrMng.port,
                   OS_VERSION,
                   "upnp:rootdevice",
                   getuuid(), "upnp:rootdevice");
#if 1   //add by ya 20120419
    printf("====================buf=%s\n", buf);
#endif
    if (len >= (int)sizeof(buf)) {
        len = sizeof(buf);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1900);
    addr.sin_addr.s_addr = inet_addr("239.255.255.250");
    ret = sendto(sock, buf, len, 0, (struct sockaddr *)&addr, sizeof(addr));
    if (0 > ret) {
        printf("sendto(udp_notify=%d)\n", sock);
    }
}

static void UpnpServerSSDPAnnounce2(int sock, struct sockaddr_in addr,
                                    const char *host, unsigned short port)
{
    char buf[1024];
    int len = 0;
    int ret = 0;

    len = snprintf(buf, sizeof(buf), "HTTP/1.1 200 OK\r\n"
                   "CACHE-CONTROL: max-age=120\r\n"
                   //"DATE: ...\r\n"
                   "ST: %s\r\n"
                   "USN: %s::%s\r\n"
                   "EXT:\r\n"
                   "SERVER: %s UPnP/1.0 MiniUPnPd/1.2 \r\n"
                   "LOCATION: http://%s:%d/rootDesc.xml\r\n"
                   "\r\n",
                   "upnp:rootdevice",
                   getuuid(), "upnp:rootdevice",
                   OS_VERSION,
                   host, port);

    ret = sendto(sock, buf, len, 0, (struct sockaddr *)&addr, sizeof(addr));
    if (0 > ret) {
        //printf("sendto(udp)\n");
    }
}

static void UpnpServerSSDPRequest(int fd, int ev, void *data)
{
    char buf[4096];
    struct sockaddr_in addr;
    int len = 0;
    int ret = 0, sock = -1;
    int count = 0;

    sock = fd;
    len = sizeof(addr);

    do {
        memset(buf, 0, sizeof(buf));
        ret = recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr *)&addr, (socklen_t *)&len);
        if(ret < 0) {
            break;
        }

        count++;
        if(count > 1)
            continue;

        //DBG("recv from:%s:%d: %s\n", j_inet_ntoa(addr.sin_addr), ntohs(addr.sin_port), buf);

        if (!memcmp(buf, "NOTIFY", 6)) { // ignore NOTIFY packets
            ;
        } else if (!memcmp(buf, "M-SEARCH", 8)) {
            int port = -1;
            char szIP[32] = {0};

            if(strstr(buf, "upnp:rootdevice") == NULL && strstr(buf, getuuid()) == NULL) {
                continue;
            }

            svrLock();
            ret = strcmp(j_inet_ntoa(addr.sin_addr), svrMng.szIP);
            snprintf(szIP, 32, "%s", svrMng.szIP);
            port = svrMng.port;
            svrUnlock();

            if (ret) {
                UpnpServerSSDPAnnounce2(sock, addr, szIP, port);
            }
        } else {
            DBG("Unknown udp packet:%s\n", buf);
        }
    } while(1);
}

static void upnp_update_descfile(void)
{
    DBG("In upnp_update_descfile...\n");
    int fd = -1;
    char *ptr = NULL;
    int len = 0;
    NetEthS eth = {{0},};
    NetPortS netPort = {0};
    if(get_config(handleEthCfg, eth) < 0) {
        ERR("Get conf_get_ethcfg failed!\n");
        return ;
    }

    if(get_config(handleNetPortCfg, netPort) < 0) {
        ERR("Get conf_get_netportcfg failed!\n");
        return ;
    }

    svrLock();
    memset(svrMng.szIP, 0, sizeof(svrMng.szIP));
    strncpy(svrMng.szIP, eth.ip, sizeof(svrMng.szIP) - 1);
    svrMng.port = netPort.httpport;
    svrUnlock();

    ptr = genRootDesc(&len, eth.ip, (unsigned short)netPort.httpport);
    if (!ptr) {
        return;
    }

    static pthread_mutex_t  RootDescXmlLock = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock(&RootDescXmlLock);
    fd = open("/tmp/rootDesc.xml", O_CREAT | O_WRONLY | O_TRUNC);
    if (-1 == fd) {
        free(ptr);
        pthread_mutex_unlock(&RootDescXmlLock);
        return;
    }
    write(fd, ptr, len);
    close(fd);
    pthread_mutex_unlock(&RootDescXmlLock);

    free(ptr);
}

static void UpnpServerProcess(JSScheduler sch)
{
    if(open("/tmp/rootDesc.xml", O_CREAT | O_WRONLY | O_TRUNC) < 0) {
        ERR("Create /tmp/rootDesc.xml file failed!\n");
    }

    // no more because SPI-flash, do it on filesystem
    //if((symlink("/tmp/rootDesc.xml", SYSTEM_WEB_PATH"rootDesc.xml") < 0)
    //   && (errno != EEXIST)) {
    //    ERR("symlink failed! errno : %d\n", errno);
    //    return;
    //}

    setuuid();
    upnp_update_descfile();

    if(net_udp_server(&sockobj, IPV4, NULL, 1900) < 0) {
        ERR("net_udp_server failed!\n");
        return ;
    }

    if(mcast_join(&sockobj, (char*)"239.255.255.250") < 0) {
        ERR("mcast_join failed!\n");
        net_close(&sockobj);

        return ;
    }

    int fd = net_sock_fd(&sockobj);
    js_create_reader_r(sch_disc, JS_READABLE, fd, UpnpServerSSDPRequest, NULL, &soc_ssdp);

    return ;
}

static void delay_upnp_update_descfile1(void *arg)
{
    upnp_update_descfile();
    if (hdl_update) {
        js_delete_timer_r(&hdl_update);
    }
}

static void upnp_client_enable(void *arg)
{
    DBG("In upnp_client_enable...\n");
    NetPortS netport = {0};
    NetUpnpS upnp = {0};

    BOOL bEnable = FALSE;

    // get configs
    int iPorts[SYSTEM_PORT_END] = {0};
    if(get_config(handleNetPortCfg, netport) < 0) {
        ERR("conf_get_netportcfg failed!\n");
        return ;
    }

    iPorts[SYSTEM_PORT_WEB] = netport.httpport;
    iPorts[SYSTEM_PORT_FTP] = netport.ftpport;
    iPorts[SYSTEM_PORT_RTSP] = netport.rtspport;
    iPorts[SYSTEM_PORT_VOICE] = netport.audioport;
    iPorts[SYSTEM_PORT_UPDATE] = netport.updateport;

    BOOL bEn[SYSTEM_PORT_END] = {FALSE, FALSE ,FALSE ,FALSE ,FALSE};

    if(get_config(handleUpnpCfg, upnp) < 0) {
        ERR("conf_get_netportcfg failed!\n");
        return ;
    }

    DBG("Upnp port infomation update success!\n");

    bEn[SYSTEM_PORT_WEB] = upnp.http;
    bEn[SYSTEM_PORT_FTP] = upnp.ftp;
    bEn[SYSTEM_PORT_RTSP] = upnp.rtsp;
    bEn[SYSTEM_PORT_VOICE] = upnp.voice;
    bEn[SYSTEM_PORT_UPDATE] = upnp.update;

    // update ports
    portLock();
    for(int i = SYSTEM_PORT_WEB; i < SYSTEM_PORT_END; i++) {
        if (portMng.ports[i].intPort != iPorts[i]) {
            portMng.ports[i].extPort =
                portMng.ports[i].intPort = iPorts[i];
        }
        portMng.ports[i].enable = bEn[i];

        if (bEn[i]) {
            bEnable = TRUE;
        }
    }
    portMng.enable = bEnable;

    if(FALSE == bEnable) {
        for (int i = SYSTEM_PORT_WEB; i < SYSTEM_PORT_END; i++) {
            portMng.ports[i].extPort = portMng.ports[i].intPort;
        }
    }
    portUnlock();

    if (hdl_enable) {
        js_delete_timer_r(&hdl_enable);
    }
}

#define INTV_ENABLE (5*1000)

static void delay_upnp_client_enable()
{
    if (hdl_enable == NULL) {
        js_create_timer_r(sch_upnp, INTV_ENABLE, INTV_ENABLE, upnp_client_enable, NULL, &hdl_enable);
    }
}
#endif

/*======================================================================
    interface functions
======================================================================*/

int init_server_upnp_discovery(void *data)
{
#if defined(DEV_TYPE_ENHANCED)
    sch_disc = data;

    memset(&svrMng, 0, sizeof(svrMng));

    pthread_mutex_init(&svrMutex, NULL);

    UpnpServerProcess(sch_disc);
#endif

    return SUCCESS;
}

int init_client_upnp_check(JSScheduler sch)
{
#if defined(DEV_TYPE_ENHANCED)
    sch_upnp = sch;

    memset(&portMng, 0, sizeof(portMng));

    pthread_mutex_init(&portMutex, NULL);

    UpnpClientProcess(sch_upnp);
#endif

    return SUCCESS;
}

int uninit_server_upnp_discovery()
{
#if defined(DEV_TYPE_ENHANCED)
    DBG("Uninit upnp Server...\n");
    if(NULL == sch_disc) {
        return SUCCESS;
    }

    js_delete_reader_r(&soc_ssdp);

    mcast_leave(&sockobj, (char*)"239.255.255.250");

    net_close(&sockobj);

    sch_disc = NULL;

    pthread_mutex_destroy(&svrMutex);
#endif

    return SUCCESS;
}

int uninit_client_upnp_check()
{
#if defined(DEV_TYPE_ENHANCED)
    DBG("Uninit upnp Client...\n");
    if(NULL == sch_upnp) {
        return SUCCESS;
    }

    if (hdl_process) {
        js_delete_timer_r(&hdl_process);
    }

    sch_upnp = NULL;

    pthread_mutex_destroy(&portMutex);
#endif

    return SUCCESS;
}

int get_upnp_map_info(UPNP_MAP_S *umap)
{
#if defined(DEV_TYPE_ENHANCED)
    portLock();
    umap->enable = portMng.enable;
    for (int i = 0; i < SYSTEM_PORT_END; i++) {
        umap->ports[i].enable = portMng.ports[i].enable;
        umap->ports[i].inPort = portMng.ports[i].intPort;
        umap->ports[i].extPort = portMng.ports[i].extPort;
    }

    memset(umap->szExtIP, 0, sizeof(umap->szExtIP));
    snprintf(umap->szExtIP, sizeof(umap->szExtIP), "%s", portMng.szExtIP);
    portUnlock();
#endif

    return SUCCESS;
}

#define INTV_UPDATE (5*1000)

void delay_upnp_update_descfile()
{
#if defined(DEV_TYPE_ENHANCED)
    if (hdl_update == NULL) {
        js_create_timer_r(sch_upnp, INTV_UPDATE, INTV_UPDATE, delay_upnp_update_descfile1, NULL, &hdl_update);
    }
#endif
}

