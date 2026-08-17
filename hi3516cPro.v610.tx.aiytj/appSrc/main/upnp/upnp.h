/******************************************************************************
    Copyright (C), 2008-2018, JABSCO ELECTRONIC Tech. Co., Ltd

    File Name       : jco_upnp.h
    Version     : 1.0
    Author      : JABSCO Video Server Software Group
    Created     : 2009.03.24
    Description : upnp functions
    History     :
                    Create by nomadzhao.2009.03.24
******************************************************************************/
#ifndef __JCO_UPNP_H__
#define __JCO_UPNP_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "js_scheduler.h"

    typedef enum
    {
        SYSTEM_PORT_BEGIN = -1,
        SYSTEM_PORT_WEB,        // http port
        SYSTEM_PORT_FTP,        // ftp port
        SYSTEM_PORT_RTSP,       // rtsp port
        SYSTEM_PORT_VOICE,      // voice port
        SYSTEM_PORT_UPDATE,     // update port
        SYSTEM_PORT_END
    }
    SYSTEM_PORT_E;

    typedef struct {
        int  enable;
        unsigned short inPort;
        unsigned short extPort;
    } UPNP_MAP_0;

    typedef struct {
        int  enable;
        char szExtIP[32];                               // external ip address
        UPNP_MAP_0 ports[SYSTEM_PORT_END];  // internal port : external port
    } UPNP_MAP_S;

    int init_server_upnp_discovery(void *schedule);

    int init_client_upnp_check(JSScheduler sch);

    int uninit_server_upnp_discovery();

    int uninit_client_upnp_check();

    int get_upnp_map_info(UPNP_MAP_S *umap);

    void delay_upnp_update_descfile();

#ifdef __cplusplus
}
#endif
#endif // __JCO_UPNP_H__

