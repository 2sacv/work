/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : ddnsstrategy.cpp
 * @Created Time : 2014-04-03
 * @Version      : 1.0
 * @Author       : zengy
 * @Description  :
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "confapi.h"
#include "debug.h"

#include "ddns3322.h"
#include "ddns9299.h"
#include "ddnsdyn.h"
#include "ddnsstrategy.h"
#include "upnp.h"
#include "jconfig.h"
#include "conf_list.h"

#if defined (DEV_TYPE_ENHANCED)

#define  DDNS_INTER      (1000)
#define  DDNS_CFGTI      (2*1000)
#define  DDNS_COUNT      (60)

static JSScheduler sch_ddns = NULL;
static JSTCHandle  hdl_ddns = NULL;
static int ddns_count = 0;

static int   dstatus = 0; //0 : disconnect 1 : connect
static pthread_mutex_t dmutext;

static void statusLock()
{
    pthread_mutex_lock(&dmutext);
}
static void statusUnlock()
{
    pthread_mutex_unlock(&dmutext);
}

static void ddns_9299_ipcheck()
{
    char ip[20] = {0};
    char status[32] = {0};

    int ret = ddns_9299_ip_check(ip, 2, status);
    if(ret < 0) {
        statusLock();
        dstatus = 0;
        statusUnlock();
    }
}

static int ddns_dyn_handle(ddns_opt_t *opt, char status[/*32*/])
{
    return ddns_dyn_update(opt, 2, status);
}

static int ddns_3322_handle(ddns_opt_t *opt, char status[/*32*/])
{
    return ddns_3322_update(opt, 2, status);
}

static int ddns_9299_handle(ddns_opt_t *opt, char status[/*32*/])
{
    int ret = 0;
    char ip[32] = {0};
    int port = 0;

    NetPortS netport = {0};
    if(get_config(handleNetPortCfg, netport) < 0) {
        ERR("Ddns conf_get_netportcfg..\n");
        return -1;
    }

    UPNP_MAP_S umap;
    get_upnp_map_info(&umap);

    if(umap.enable && umap.ports[SYSTEM_PORT_WEB].enable) {
        port = (int)umap.ports[SYSTEM_PORT_WEB].extPort;
    } else {
        port = netport.httpport;
    }

    do {
        ret = ddns_9299_ip_check(ip, 2, status);
        if(ret < 0) {
            ERR("ddns_9299_ip_check failed\n");
            ret = -1;
            break;
        }

        opt->wlan_ip = ip;
        opt->work_mode = 1;
        opt->wlan_port = port;
        opt->mac_addr = (char*)"00-00-00-00-00-00";

        ret = ddns_9299_update(opt, 2, status);
        if(ret < 0) {
            ERR("ddns_9299_update failed!\n");
        }
    } while(0);

    return ret;
}

static int ddns_connct_handle(DdnsS ddns, char status[/*32*/])
{
    int ret = -1;
    ddns_opt_t  opt = {0,};

    opt.user_name = ddns.info[ddns.vendor].user;
    opt.password = ddns.info[ddns.vendor].password;
    opt.sdomain = ddns.info[ddns.vendor].maindomain; //9299不能加前缀9299.org

    switch(ddns.vendor) {
        case 0:
            ret = ddns_dyn_handle(&opt, status);
            break;

        case 1:
            ret = ddns_3322_handle(&opt, status);
            break;

        case 2:
            ret = ddns_9299_handle(&opt, status);
            break;

        default:
            ERR("Unkown ddns vendor\n");
            ret = -1;
            break;
    };

    return ret;
}

static void ddns_timing_task(void *data)
{
    char status[32] = {0};
    int ret = -1;
    DdnsS ddns = {DDNSDYN,};

    ddns_count++;
    if(ddns_count == DDNS_COUNT){
        ddns_count = 0;
        DBG("Start ddns_timing_task....\n");

        if(get_config(handleDdnsCfg, ddns) < 0) {
            ERR("Ddns service conf_get_ddnscfg failed!\n");
            return ;
        }

        if(0 == ddns.info[ddns.vendor].enable) {
            DBG("ddns is disable...\n");
            statusLock();
            dstatus = 0;
            statusUnlock();
            return ;
        }

        statusLock();
        int tstatus = dstatus;
        statusUnlock();

        if(tstatus == 1 && 2 == ddns.vendor) { //9299
            ddns_9299_ipcheck(); //heartBeat
            return ;
        }

        ret = ddns_connct_handle(ddns, status);
        if(ret < 0 || strcmp(status, "success")) {
            ERR("ddns_connct_handle failed!\n");
            statusLock();
            dstatus = 0;
            statusUnlock();
            return ;
        }

        statusLock();
        dstatus = 1;
        statusUnlock();

        DBG("ddns [%d]:[%s] connect success!\n", ddns.vendor, ddns.info[ddns.vendor].maindomain);
    }
}

static void ddns_short_timing_task(void *data)
{
    DBG("Start ddns_short_timing_task...\n");

    statusLock();
    dstatus = 0;   //执行此函数必定参数发生改变,故置0;重新向server注册
    statusUnlock();

    /*if (hdl_ddns) {
        js_delete_timer_r(&hdl_ddns);
    }
    js_create_timer_r(sch_ddns, 0, DDNS_INTER, ddns_timing_task, NULL, &hdl_ddns);*/

    ddns_count = DDNS_COUNT-1;
}
#endif

int init_client_ddns(JSScheduler sch)
{
#if defined (DEV_TYPE_ENHANCED)
    sch_ddns = sch;

    pthread_mutex_init(&dmutext, NULL);

    if (hdl_ddns) {
        js_delete_timer_r(&hdl_ddns);
    }
    js_create_timer_r(sch_ddns, 0, DDNS_INTER, ddns_timing_task, NULL, &hdl_ddns);

#endif

    return SUCCESS;
}

int uninit_client_ddns()
{
#if defined (DEV_TYPE_ENHANCED)
    if(NULL == sch_ddns) {
        return SUCCESS;
    }

    if (hdl_ddns) {
        js_delete_timer_r(&hdl_ddns);
    }

    sch_ddns = NULL;

    pthread_mutex_destroy(&dmutext);
#endif

    return SUCCESS;
}

void ddns_cfg_changed_process()
{
#if defined (DEV_TYPE_ENHANCED)
    if(NULL == sch_ddns) {
        return ;
    }

    DBG("In ddns_cfg_changed_process...\n");

    static JSTCHandle hdl_ddns_pro = NULL;
    js_create_once(hdl_ddns_pro, sch_ddns, DDNS_CFGTI, ddns_short_timing_task, NULL); // once
#endif
}

int ddns_connect_status()
{
    int ret = 0;

#if defined (DEV_TYPE_ENHANCED)
    statusLock();
    ret = dstatus;
    statusUnlock();
#endif

    return ret;
}

