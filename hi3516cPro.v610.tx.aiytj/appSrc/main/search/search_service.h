/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    : search_service.h
 * Created Time : 2014-03_31
 * Version      : 1.0
 * Author       : chebiyou
 * Description  :
 */


#ifndef __SEARCH_SERVICE_H__
#define __SEARCH_SERVICE_H__

#include "utils.h"
#include "jconfstruct.h"

#ifdef __cplusplus
extern "C" {
#endif

    void init_server_search();

    void uninit_server_search();

    int set_sysinfo(SysInfoS *info);

    int set_videoinfo(VideoEncS *ves);

    int set_portinfo(NetPortS *netport);

    int set_ethinfo(NetEthS *eth);

	int get_ethinfo(NetEthS *eth);

	int get_portinfo(NetPortS *netport);

	int get_sysinfo(SysInfoS *info);
#ifdef __cplusplus
}

#endif

#endif





