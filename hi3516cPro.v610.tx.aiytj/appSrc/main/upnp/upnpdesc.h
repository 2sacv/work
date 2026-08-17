/******************************************************************************
    Copyright (C), 2008-2018, JABSCO ELECTRONIC Tech. Co., Ltd

    File Name       : jco_upnpdesc.h
    Version     : 1.0
    Author      : JABSCO Video Server Software Group
    Created     : 2009.03.27
    Description : upnp desc xml declare
    History     :
                    Create by nomadzhao.2009.03.27
******************************************************************************/
#ifndef __JCO_UPNPDESC_H__
#define __JCO_UPNPDESC_H__
#ifdef __cplusplus
extern "C" {
#endif

    /*======================================================================
        const define
    ======================================================================*/
#define ROOTDESC_PATH       "/rootDesc.xml"

#define WANCFG_PATH         "/WANCfg.xml"
#define WANCFG_CONTROLURL   "/ctl/CmnIfCfg"
#define WANCFG_EVENTURL     "/evt/CmnIfCfg"

#define WANIPC_PATH         "/WANIPCn.xml"
#define WANIPC_CONTROLURL   "/ctl/IPConn"
#define WANIPC_EVENTURL     "/evt/IPConn"

#define L3F_PATH            "/L3F.xml"
#define L3F_CONTROLURL      "/ctl/L3F"
#define L3F_EVENTURL        "/evt/L3F"

#define UPNP_VERSION        "20090327"
#define USE_NETFILTER       1
#define OS_NAME             "NVS"
#define OS_VERSION          "NVS/4"
#define OS_URL              "http://www.kernel.com/"

    /* strings used in the root device xml description */
#define XMLVER                      "<?xml version=\"1.0\"?>\r\n"
#define SERIALNUMBER                "12345678"
#define MODLENUMBER                 "1"
#define ROOT_SERVICE                "scpd xmlns=\"urn:schemas-upnp-org:service-1-0\""
#define ROOT_DEVICE                 "root xmlns=\"urn:schemas-upnp-org:device-1-0\""

#define ROOTDEV_FRIENDLYNAME        "NVS"
#define ROOTDEV_MANUFACTURER        OS_NAME
#define ROOTDEV_MANUFACTURERURL     OS_URL
#define ROOTDEV_MODELNAME           OS_NAME " NVS"
#define ROOTDEV_MODELDESCRIPTION    OS_NAME " NVS"
#define ROOTDEV_MODELURL            OS_URL

#define WANDEV_FRIENDLYNAME         "WANDevice"
#define WANDEV_MANUFACTURER         "MiniUPnP"
#define WANDEV_MANUFACTURERURL      "http://miniupnp.free.fr/"
#define WANDEV_MODELNAME            "WAN Device"
#define WANDEV_MODELDESCRIPTION     "WAN Device"
#define WANDEV_MODELNUMBER          UPNP_VERSION
#define WANDEV_MODELURL             "http://miniupnp.free.fr/"
#define WANDEV_UPC                  "MINIUPNPD"

#define WANCDEV_FRIENDLYNAME        "WANConnectionDevice"
#define WANCDEV_MANUFACTURER        WANDEV_MANUFACTURER
#define WANCDEV_MANUFACTURERURL     WANDEV_MANUFACTURERURL
#define WANCDEV_MODELNAME           "MiniUPnPd"
#define WANCDEV_MODELDESCRIPTION    "MiniUPnP daemon"
#define WANCDEV_MODELNUMBER         UPNP_VERSION
#define WANCDEV_MODELURL            "http://miniupnp.free.fr/"
#define WANCDEV_UPC                 "MINIUPNPD"

    /* for the root description
     * The child list reference is stored in "data" member using the
     * INITHELPER macro with index/nchild always in the
     * same order, whatever the endianness */
    struct XMLElt {
        const char *eltname;    /* begin with '/' if no child */
        const char *data;       /* Value */
    };

    /* little endian
     * The code has now be tested on big endian architecture */
#define INITHELPER(i, n) ((char *)((n<<16)|i))

    /*======================================================================
        interface declare
    ======================================================================*/
    /* char * genRootDesc(int *);
     * returns: NULL on error, string allocated on the heap */
    char *genRootDesc(int *len, char *szIP, unsigned short port);

    void setuuid(void);
    char *getuuid(void);

#ifdef __cplusplus
}
#endif
#endif  // __JCO_UPNPDESC_H__

