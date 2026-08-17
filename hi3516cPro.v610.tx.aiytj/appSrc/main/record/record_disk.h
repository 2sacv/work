/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    :
 * Created Time : 2014-04-02
 * Version      : 1.0
 * Author       : luoshunfa
 * Description  :
 */

#ifndef _record_disk_H_
#define _record_disk_H_

#include "js_scheduler.h"

#ifdef __cplusplus
extern "C" {
#endif

#define F_MMC_PRESENT "/sys/class/mmc_host/mmc0/card_present"
#define F_MMC_PWR_CTRL "/sys/class/mmc_host/mmc0/power_control"
#define F_MMC_ABSENT "/opt/conf/mmc_absent"
#define F_ADD_FILE     "/var/run/sdcard_inserted"
#define F_REMOVE_FILE  "/var/run/sdcard_removed"

typedef struct {
    const char *cpuid;
    char        cid[36];
    int         uptime;
    const char *version;
    int         sd_stat;
    int         temprt;
    int         this_boot;
    int         df_used;
    int         cnt_rescan;
    int         is_test;
    int         all_count;
} sSdinfoPkg;

typedef enum{
    E_SD_FORMAT_UNKOWN,
    E_SD_FORMAT_FAT32,
    E_SD_FORMAT_EXFAT,
}eSDFormat;

int get_sdinfo_pkg(sSdinfoPkg *info);
void chk_sdstat();
int get_sdstat();
void sdcard_is_change(void);

int init_record_disk(void);
int uninit_record_disk(void);

int storage_get_mmcpath(char *mmcpath);

int is_storage_devpath_really_full(char *devpath);
int is_storage_devpath_need_remove_oldrecord(char *devpath);
int is_storage_devpath_space_enough(void);

int storage_remove_devpath(char *devpath);
int storage_add_devpath(char *devpath);

int storage_mount_dev(char *mountpath,char *devName);
int storage_umount_dev(char *mountpath);

void storage_print_all_devpath(void);

int storage_card_is_present(void);
int record_storage_dev_manage(void);

#ifdef __cplusplus
}
#endif
#endif

