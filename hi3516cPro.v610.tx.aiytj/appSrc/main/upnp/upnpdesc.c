/******************************************************************************
    Copyright (C), 2008-2018, JABSCO ELECTRONIC Tech. Co., Ltd

    File Name       : jco_upnpdesc.c
    Version     : 1.0
    Author      : JABSCO Video Server Software Group
    Created     : 2009.03.27
    Description : upnp desc xml functions
    History     :
                    Create by nomadzhao.2009.03.27
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "upnpdesc.h"
#include "confapi.h"
#include "debug.h"

#if defined (DEV_TYPE_ENHANCED)
/*======================================================================
    local define
======================================================================*/
static char presentationurl[128] = {"http://192.168.1.217:80/"};
static char friendlayname[128] = {ROOTDEV_FRIENDLYNAME"[192.168.1.217]"};
static char uuidvalue[128] = {"uuid:01010203-0405-0607-0809-101112131415"};

/* root Description of the UPnP Device
 * fixed to match UPnP_IGD_InternetGatewayDevice 1.0.pdf
 * presentationURL is only "recommended" but the router doesn't appears
 * in "Network connections" in Windows XP if it is not present. */
static const struct XMLElt rootDesc[] = {
    /* 0 */
    {ROOT_DEVICE, INITHELPER(1,2)},
    {"specVersion", INITHELPER(3,2)},
    {"device", INITHELPER(5,13)},
    {"/major", "1"},
    {"/minor", "0"},
    /* 5 */
    {"/deviceType", "urn:schemas-upnp-org:device:InternetGatewayDevice:1"},
    {"/friendlyName", friendlayname},   /* required */
    {"/manufacturer", ROOTDEV_MANUFACTURER},        /* required */
    /* 8 */
    {"/manufacturerURL", ROOTDEV_MANUFACTURERURL},  /* optional */
    {"/modelDescription", ROOTDEV_MODELDESCRIPTION}, /* recommended */
    {"/modelName", ROOTDEV_MODELNAME},  /* required */
    {"/modelNumber", MODLENUMBER},
    {"/modelURL", ROOTDEV_MODELURL},
    {"/serialNumber", SERIALNUMBER},
    {"/UDN", uuidvalue},    /* required */
    {"serviceList", INITHELPER(57,1)},
    {"deviceList", INITHELPER(18,1)},
    {"/presentationURL", presentationurl},  /* recommended */
    /* 18 */
    {"device", INITHELPER(19,13)},
    /* 19 */
    {"/deviceType", "urn:schemas-upnp-org:device:WANDevice:1"}, /* required */
    {"/friendlyName", WANDEV_FRIENDLYNAME},
    {"/manufacturer", WANDEV_MANUFACTURER},
    {"/manufacturerURL", WANDEV_MANUFACTURERURL},
    {"/modelDescription" , WANDEV_MODELDESCRIPTION},
    {"/modelName", WANDEV_MODELNAME},
    {"/modelNumber", WANDEV_MODELNUMBER},
    {"/modelURL", WANDEV_MODELURL},
    {"/serialNumber", SERIALNUMBER},
    {"/UDN", uuidvalue},
    {"/UPC", WANDEV_UPC},
    /* 30 */
    {"serviceList", INITHELPER(32,1)},
    {"deviceList", INITHELPER(38,1)},
    /* 32 */
    {"service", INITHELPER(33,5)},
    /* 33 */
    {
        "/serviceType",
        "urn:schemas-upnp-org:service:WANCommonInterfaceConfig:1"
    },
    /*{"/serviceId", "urn:upnp-org:serviceId:WANCommonInterfaceConfig"}, */
    {"/serviceId", "urn:upnp-org:serviceId:WANCommonIFC1"}, /* required */
    {"/controlURL", WANCFG_CONTROLURL},
    {"/eventSubURL", WANCFG_EVENTURL},
    {"/SCPDURL", WANCFG_PATH},
    /* 38 */
    {"device", INITHELPER(39,12)},
    /* 39 */
    {"/deviceType", "urn:schemas-upnp-org:device:WANConnectionDevice:1"},
    {"/friendlyName", WANCDEV_FRIENDLYNAME},
    {"/manufacturer", WANCDEV_MANUFACTURER},
    {"/manufacturerURL", WANCDEV_MANUFACTURERURL},
    {"/modelDescription", WANCDEV_MODELDESCRIPTION},
    {"/modelName", WANCDEV_MODELNAME},
    {"/modelNumber", WANCDEV_MODELNUMBER},
    {"/modelURL", WANCDEV_MODELURL},
    {"/serialNumber", SERIALNUMBER},
    {"/UDN", uuidvalue},
    {"/UPC", WANCDEV_UPC},
    {"serviceList", INITHELPER(51,1)},
    /* 51 */
    {"service", INITHELPER(52,5)},
    /* 52 */
    {"/serviceType", "urn:schemas-upnp-org:service:WANIPConnection:1"},
    /* {"/serviceId", "urn:upnp-org:serviceId:WANIPConnection"}, */
    {"/serviceId", "urn:upnp-org:serviceId:WANIPConn1"},
    {"/controlURL", WANIPC_CONTROLURL},
    {"/eventSubURL", WANIPC_EVENTURL},
    {"/SCPDURL", WANIPC_PATH},
    /* 57 */
    {"service", INITHELPER(58,5)},
    /* 58 */
    {"/serviceType", "urn:schemas-upnp-org:service:Layer3Forwarding:1"},
    {"/serviceId", "urn:upnp-org:serviceId:Layer3Forwarding1"},
    {"/controlURL", L3F_CONTROLURL}, /* The Layer3Forwarding service is only */
    {"/eventSubURL", L3F_EVENTURL}, /* recommended, not mandatory */
    {"/SCPDURL", L3F_PATH},
    {0, 0}
};

/* strcat_str()
 * concatenate the string and use realloc to increase the
 * memory buffer if needed. */
static char *strcat_str(char *str, int *len, int *tmplen, const char *s2)
{
    int s2len;
    s2len = (int)strlen(s2);
    if(*tmplen <= (*len + s2len)) {
        if(s2len < 256)
            *tmplen += 256;
        else
            *tmplen += s2len + 1;
        str = (char *)realloc(str, *tmplen);
    }
    /*strcpy(str + *len, s2); */
    memcpy(str + *len, s2, s2len + 1);
    *len += s2len;
    return str;
}

/* strcat_char() :
 * concatenate a character and use realloc to increase the
 * size of the memory buffer if needed */
static char *strcat_char(char *str, int *len, int *tmplen, char c)
{
    if(*tmplen <= (*len + 1)) {
        *tmplen += 256;
        str = (char *)realloc(str, *tmplen);
    }
    str[*len] = c;
    (*len)++;
    return str;
}

/* iterative subroutine using a small stack
 * This way, the progam stack usage is kept low */
static char *genXML(char *str, int *len, int *tmplen, const struct XMLElt *p)
{
    unsigned short i, j, k;
    int top;
    const char *eltname, *s;
    char c;
    struct {
        unsigned short i;
        unsigned short j;
        const char *eltname;
    } pile[16]; /* stack */

    top = -1;
    i = 0;  /* current node */
    j = 1;  /* i + number of nodes*/

    for(;;) {
        eltname = p[i].eltname;
        if(!eltname)
            return str;

        if(eltname[0] == '/') {
            if(p[i].data && p[i].data[0]) {
                /*printf("<%s>%s<%s>\n", eltname+1, p[i].data, eltname); */
                str = strcat_char(str, len, tmplen, '<');
                str = strcat_str(str, len, tmplen, eltname+1);
                str = strcat_char(str, len, tmplen, '>');
                str = strcat_str(str, len, tmplen, p[i].data);
                str = strcat_char(str, len, tmplen, '<');
                str = strcat_str(str, len, tmplen, eltname);
                str = strcat_char(str, len, tmplen, '>');
            }

            for(;;) {
                if(top < 0)
                    return str;
                i = ++(pile[top].i);
                j = pile[top].j;
                /*printf("  pile[%d]\t%d %d\n", top, i, j); */
                if(i==j) {
                    /*printf("</%s>\n", pile[top].eltname); */
                    str = strcat_char(str, len, tmplen, '<');
                    str = strcat_char(str, len, tmplen, '/');
                    s = pile[top].eltname;
                    for(c = *s; c > ' '; c = *(++s))
                        str = strcat_char(str, len, tmplen, c);
                    str = strcat_char(str, len, tmplen, '>');
                    top--;
                } else
                    break;
            }
        } else {
            /*printf("<%s>\n", eltname); */
            str = strcat_char(str, len, tmplen, '<');
            str = strcat_str(str, len, tmplen, eltname);
            str = strcat_char(str, len, tmplen, '>');
            k = i;
            /*i = p[k].index; */
            /*j = i + p[k].nchild; */
            i = (unsigned)p[k].data & 0xffff;
            j = i + ((unsigned)p[k].data >> 16);
            top++;
            /*printf(" +pile[%d]\t%d %d\n", top, i, j); */
            pile[top].i = i;
            pile[top].j = j;
            pile[top].eltname = eltname;
        }
    }
}

/*======================================================================
    interface define
======================================================================*/
/* genRootDesc() :
 * - Generate the root description of the UPnP device.
 * - the len argument is used to return the length of
 *   the returned string.
 * - tmp_uuid argument is used to build the uuid string */
char *genRootDesc(int *len, char *szIP, unsigned short port)
{
    char * str;
    int tmplen;

    tmplen = 4096;
    str = (char *)malloc(tmplen);
    if(str == NULL)
        return NULL;

    memset(str, 0, tmplen);

    *len = strlen(XMLVER);
    /*strcpy(str, XMLVER); */
    memcpy(str, XMLVER, *len + 1);

    sprintf(presentationurl, "http://%s:%d/", szIP, port);
    sprintf(friendlayname, "%s[%s]", ROOTDEV_FRIENDLYNAME, szIP);

    str = genXML(str, len, &tmplen, rootDesc);
    str[*len] = '\0';
    return str;
}

void setuuid(void)
{
    char buf[16] = {0,};
    SysInfoS info= {{0},};

    if(conf_get_sysinfocfg(&info) < 0) {
        ERR("get sysinfo failed!\n");
    }

    memset(buf, 0, sizeof(buf));
    sprintf(buf, "%08X", atoi(info.devid)); //devid[32] char buf[16];

    memset(uuidvalue, 0, sizeof(uuidvalue));
    sprintf(uuidvalue, "uuid:01010203-0405-0607-%c%c-%s", buf[0], buf[1], buf + 2);
}

char *getuuid(void)
{
    return uuidvalue;
}
#endif

