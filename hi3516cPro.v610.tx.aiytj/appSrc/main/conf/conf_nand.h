/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : conf_nand.h
 * @Created Time : 2014-07-07
 * @Version      : 1.0
 * @Author       : cheby
 * @Description  :
 */

#ifndef  __CONF_NAND_H__
#define  __CONF_NAND_H__

#include "jcpCmd.h"
#include "jconfstruct.h"

#ifdef __cplusplus
extern "C" {
#endif
#define MAX_CPUID_LEN 24

    const char *uboot_devid_get(char *devid, size_t len);
    const char *uboot_mac_get(char *mac, int len);
    const char *uboot_devinfo_get(char *devinfo, int len);
    int uboot_aliconf_get(char *aliconf,int len);

    int uboot_mac_set(char *szMAC);

    int uboot_devid_set(char *szDevID);

    int uboot_devinfo_set();

    int uboot_p2pconf_set(char *name, char *szP2pConf);

    int config_uboot_env(char* action, ArgOptS opts[], char *msgbuf);

    int config_bootargs(const char *action, ArgOptS opts[], BOOTARGS_CFG_S *outer);

    int is_spiflash_board();

    int get_feature();

    int get_maxheight_2MTo3M();

    const char *get_uid();

    const char *get_cpuid();

    int uboot_txconf_set(char *name, char *szTxConf);

    int uboot_ssid_set(char *szbuf);

    int uboot_ssid_get(char *ssid, int len);

#ifdef __cplusplus
}
#endif

#endif



