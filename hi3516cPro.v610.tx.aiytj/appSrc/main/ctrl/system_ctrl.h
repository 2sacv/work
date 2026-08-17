/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : system_ctrl.h
 * @Created Time : 2014.04.03
 * @Version      : 1.0
 * @Author       : cheby
 * @Description  :
 */

#ifndef __SYSTEM_CTRL_H__
#define __SYSTEM_CTRL_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "jconfstruct.h"
#include "debug.h"

#define MAX_ID_LEN  11

typedef enum
{
    CPU_NONE = 0,
    CPU_T10,
    CPU_T15,
    CPU_T20,
    CPU_T21,
    CPU_T31L,
    CPU_T31X,
    CPU_T40X,
    CPU_AX620U,
    CPU_AX620Q,
    CPU_HI3516CV610,
    CPU_HI3516CV608,
} ECPUType;


typedef enum
{
    Security_NONE = 0,
    Security_HardWare,
    Security_SoftWare,
} ESecurityType;

    int is_test_ver();
    
    int system_get_uptime();

    int init_client_cpu_usage(void *data);
    void uninit_client_cpu_usage();

    int system_status_info(char *buf);

    int init_client_watchdog_feed(void *data);
    void uninit_client_watchdog_feed();

    int init_client_watch_auto_reboot(void *data);
    void uninit_client_watch_auto_reboot(void);

    void save_record_before_reboot(void);
	
	int system_get_sdexist(int *sdexist);

    int init_networking();
    void uninit_networking();

    int system_init_cmdline(void);
    ECPUType system_get_cpu_type();
    ESensorType system_get_snsr_type();
    
    void system_clr_security(void);

    int system_get_security(void);
    ESecurityType system_get_security_type(void);
    int system_get_dev_id(char *szDevID);

    int system_set_dev_id(char *dev_id);

    int system_set_upgrade_begin(void);

	int system_get_cpu_udid(unsigned long long *id);

	int system_get_maxvencsize();

	void exec_redirect_dbgout();

    int system_get_supportHD(void);

    const char *system_get_product_name(const char *devtype);

    int get_valid_fps(int fps);

    int get_enc_max_fps(void);

    VideoIdxE get_valid_vidx(VideoIdxE vidx, VideoIdxE min, VideoIdxE max);

	int system_eth_rate_init();

 	int system_set_eth_rate(int reticle);
	
	int chanage_auto_reboot(AutoRebootS ars);
	
#ifdef __cplusplus
}

#endif
#endif
