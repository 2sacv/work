/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    :
 * Created Time : 2014-04-02
 * Version      : 1.0
 * Author       : luoshunfa
 * Description  :
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/mount.h>
#include <sys/vfs.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <mtd/mtd-user.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

#include "utils.h"
#include "debug.h"
#include "jconfstruct.h"
#include "record_file_manage.h"
#include "record_watch.h"
#include "record_disk.h"
#include "recordapi.h"
#include "confapi.h"
#include "g_stat.h"
#include "g_sys.h"
#include "g_log.h"
#include "g_run.h"
#include "delay_exec.h"
#include "io.h"
#include "gpio.h"
#include "conf_nand.h"
#include "system_ctrl.h"
#include "jconfig.h"
#include "factory_db.h"
#include "sd_recovery.h"
#include "net_check.h"
#include "sim4g_adapter.h"
#include "system_sch.h"
#include "jcpService.h"
#include "system_ctrl.h"
#include "jevent.h"
#include "jcpService.h"

#ifdef PLATFORM_TENCENT
#include "tencent_http_service.h"
#endif

#define PATH_MAX 4096
#define RECORD_DEFAULT_PATH     "/mnt/IPCamera"
// 驱动层检测到状态异常通过该节点将异常计数传递到上层，上层echo up可以过滤异常检测，防止阻塞其他业务
#define SDCARD_BUSY_STATUS_PATH  "/sys/class/mmc_host/mmc0/mmc0*/block/mmcblk0/card_busy_status"
#define REPAIR_FLAG      "Repairable: yes"
#define REPAIR_CMD       "exfat_repair recover /dev/mmcblk0 --apply --backup-dir /tmp/exfat-recover-backup"

typedef enum {
    E_FS_UNKNOWN = -1,
    E_FS_EXFAT,
    E_FS_NOT_EXFAT,
} eFileSystem;

JSScheduler             g_disk_scheduler = NULL;
static JSTCHandle       g_disk_watcher = NULL;
static JSTCHandle       g_readonly_watcher = NULL;
static pthread_mutex_t  g_disk_mutex;

using namespace std;

static int is_need_exfat_repair()
{
    char repair_result[128] = {0};
    
    ReadCmdResult("timeout 30 exfat_repair inspect /dev/mmcblk0 | grep Repairable", 
                    repair_result, sizeof(repair_result));
    if (strstr(repair_result, REPAIR_FLAG) != NULL) {
        return TRUE;
    }

    return FALSE;
}

static void exfat_repair()
{
    DBG("%s start\n", __func__);
    char rep_buf[128] = {0};
    
    jcpcmd_sendrecv("sdcard -act remove -path /mnt", rep_buf, sizeof(rep_buf));

    UtilSystemCmd(REPAIR_CMD);

    return;
}

static vector<string>   g_disk_patharr;

static int g_disk_ticks = 0;
static int g_mmc_absent = 0;

int find_storage_devpath(char *devpath)
{
    int find = 0;
    string strtmp;

    pthread_mutex_lock(&g_disk_mutex);

    for (vector<string>::iterator it = g_disk_patharr.begin(); it != g_disk_patharr.end(); ++it){
        strtmp = *it;
        if(strcmp(strtmp.c_str(), devpath) == 0){
            find = 1;
            break;
        }
    }

    pthread_mutex_unlock(&g_disk_mutex);

    return find;
}

int storage_add_devpath(char *devpath)
{
    int ret = -1;

    do{
        if(devpath == NULL)
            break;

        if(find_storage_devpath(devpath))
            break;

        if(get_g_stat(record, SD_ERR_STOP)) {
            break;
        }

        pthread_mutex_lock(&g_disk_mutex);
        g_disk_patharr.push_back(devpath);
        pthread_mutex_unlock(&g_disk_mutex);

        ret = 0;
    }while(0);

    return ret;
}

int storage_remove_devpath(char *devpath)
{
    string strtmp;

    pthread_mutex_lock(&g_disk_mutex);

    for (vector<string>::iterator it = g_disk_patharr.begin(); it != g_disk_patharr.end(); ++it){
        strtmp = *it;
        if(strcmp(strtmp.c_str(), devpath) == 0){
            g_disk_patharr.erase(it);
            break;
        }
    }

    pthread_mutex_unlock(&g_disk_mutex);

    return 0;
}

static int init_storage_mount_table(void)
{
    int ret = 0;
    char curPath[256] = "/mnt";

    if (is_mountpoint("/mnt")) {
        js_log("add mounted devpath:%s\n", curPath);

        if ((is_okey(F_SD_AUTH) || is_okey("/mnt/IPCamera")) || is_okey(FACTORY_SSID_FILE)) {
            storage_add_devpath(curPath);
            firmware_answer(false);
        } else {
            set_g_stat(record, SD_ERR_UNAUTH);
        }
    } else {
        storage_remove_devpath(curPath);
    }

    return ret;
}

int storage_umount_dev(char *mountpath)
{
    int err;

    err = umount2(mountpath, 2);
    DBG("umount %s return %d [22 not mountpoint]\n", mountpath, err);

    if (err == EINVAL) {
        return 0;
    }

    if(err != 0) {
        usleep(500*1000);
        err = umount2(mountpath, 2);
        DBG("umount %s return %d [22 not mountpoint]\n", mountpath, err);
        if(err != 0) {
            UtilSystemCmd((char *)"lsof | grep mmc");
            return err;
        }
    }

    storage_remove_devpath(mountpath);

    return err;
}

int  storage_mount_dev(char *mountpath,char *devName)
{
    char pathDev[128];

    generate_record_dir(mountpath);

    sprintf(pathDev, "/dev/%s", devName);
    if (-1 == mount(pathDev, mountpath, "vfat", MS_MGC_VAL, NULL)
        && -1 == mount(pathDev, mountpath, "ext3", MS_MGC_VAL, NULL)
        && -1 == mount(pathDev, mountpath, "ntfs", MS_MGC_VAL, NULL)) {
        ERR("mount %s return error!\n", mountpath);
        remove(mountpath);
    } else {
        DBG("mount %s return success!\n", mountpath);
        storage_add_devpath(mountpath);
    }

    return 0;
}

void storage_print_all_devpath(void)
{
    int no = 0;
    string strtmp;

    pthread_mutex_lock(&g_disk_mutex);

    for (vector<string>::iterator it = g_disk_patharr.begin(); it != g_disk_patharr.end(); ++it){
        strtmp = *it;
        js_log("no:%d path:%s\n", no++, strtmp.c_str());
    }

    pthread_mutex_unlock(&g_disk_mutex);
}

int get_free_xholdM_of_total(long total)
{
    int xholdM = 1024;

    if (total > 128*1024) {       // 256G   6~8G
        xholdM = 6*1024;
    } else if (total > 64*1024) { // 128G   5~7G
        xholdM = 5*1024;
    } else if (total > 32*1024) { // 64G    4~6G
        xholdM = 4*1024;
    } else if (total > 16*1024) { // 32G    2~3G
        xholdM = 2*1024;
    } else {                      // <=16G  1~2G
        xholdM = 1*1024;
    }
    if (is_okey("/opt/etc/xholdM")) {
        int pct = 0;
        LoadFile2("/opt/etc/xholdM", "%d", &pct);
        if (pct >= 10 && pct < 100) {
            xholdM = total * pct / 100;
        } else {
            js_log("invalid value of /opt/etc/xholdM");
        }
    }

    return xholdM;
}

int get_richM_of_total(long total)
{
    int richM = 0;

    if (get_free_xholdM_of_total(total) > 2*1024) {
        richM = get_free_xholdM_of_total(total) + 2*1024;
    } else {
        richM = get_free_xholdM_of_total(total) + 1*1024;
    }

    if (richM > total) {    // 防止手动设置 100%
        richM = total;
    }

    return richM;
}

int is_storage_devpath_really_full(char *devpath)
{
    long total;
    long free;

    struct statfs stfs;
    int ret;

    if(devpath == NULL)
        return 1;

    ret = statfs(devpath, &stfs);
    if(ret < 0) {
        js_log("statfs [%s] error:%s\n", devpath, strerror(errno));
        return 1;
    }

    free = ((stfs.f_bfree >> 10) * stfs.f_bsize) >> 10; // m
    total = ((stfs.f_blocks >> 10) * stfs.f_bsize) >> 10; // m

    if (free <= get_free_xholdM_of_total(total)) {
        // less than 1G or 5%, stop record files
        js_log("got a full, dev:%s free(%ld)\n", devpath, free);
        return TRUE;
    }

    return FALSE;
}

int is_storage_devpath_need_remove_oldrecord(char * devpath)
{
    return is_storage_devpath_really_full(devpath);
}


int is_storage_devpath_space_enough(void)
{
    long total = 0;
    long free = 0;

    struct statfs stfs;
    int ret = 0;

    char devpath[128] = {0};

    ret = storage_get_mmcpath(devpath);
    if(ret < 0){
        return 1;
    }

    ret = statfs(devpath, &stfs);
    if(ret < 0) {
        js_log("statfs [%s] error:%s\n", devpath, strerror(errno));
        return 1;
    }

    free = ((stfs.f_bfree >> 10) * stfs.f_bsize) >> 10; // m
    total = ((stfs.f_blocks >> 10) * stfs.f_bsize) >> 10; // m

    if(free >= get_richM_of_total(total)) {
        js_log("reach ENOUGH dev:%s free(%ld)\n", devpath, free);
        return TRUE;
    }

    return FALSE;
}

int storage_get_mmcpath(char *mmcpath)
{
    int find = -1;
    string strtmp;

    //alway reture first path
    pthread_mutex_lock(&g_disk_mutex);

    for (vector<string>::iterator it = g_disk_patharr.begin(); it != g_disk_patharr.end(); ++it){
        strtmp = *it;
        if(strlen(strtmp.c_str()) > 0){
            strcpy(mmcpath, strtmp.c_str());
            find = 0;
            break;
        }
    }

    pthread_mutex_unlock(&g_disk_mutex);

    return find;
}

void storage_devpath_full_watch(void *data)
{
    int isFull = 0;
    int isNeedDelete = 0;
    char curDevPath[128] = {0,};
    int ret;

    do{
        ret = storage_get_mmcpath(curDevPath);
        if(ret < 0)
            break;

        isFull = is_storage_devpath_need_remove_oldrecord(curDevPath);
        isNeedDelete = is_need_delete_old_files();

        if(isFull && isNeedDelete){
            js_log("go to remove oldest dir!\n");
            if(!get_g_stat(record, SD_ERR_WRITE_PROTECT) && !get_g_stat(record, SD_ERR_WRITE) && !get_g_stat(record, SD_ERR_READ)) {
                remove_oldest_dir(NULL);
            } else {
                js_log("sd card read/write error, do not delete the video!\n");
            }
        }

    }while(0);
}

static int check_storage_readonly(char *mntpoint)
{
    if (mntpoint == NULL) {
        SYSLOG("========find NULL path:========\n");
        return FAILURE;
    }

    //js_log("check readonly device:%s\n", mntpoint);
    struct statvfs stvfs;

    if(statvfs(mntpoint, &stvfs) < 0) {
        ERR("statvfs error path:%s  errmsg:%s\n", mntpoint, strerror(errno));
        SYSLOG("statvfs error path:%s  errmsg:%s\n", mntpoint, strerror(errno));
        return FAILURE;
    }

    if (stvfs.f_flag & MS_RDONLY) {
        SYSLOG("========find readonly path:%s========\n", mntpoint);
        return FAILURE;
    }

    // 模拟
    if (get_g_run(record, RUN_RECORD_ERR_WR)) {
        DBG("sderror testing\n");
        return FAILURE;
    }

    return SUCCESS;
}

static int storage_get_read_stat(char *path)
{
    int ret = 0;
    DIR *pDir = NULL;
    struct dirent *pDirent = NULL;
    struct dirent dlocal = {0};

    pDir = opendir(path);
    if (pDir == NULL) {
        js_log("opendir :%s error\n", path);
        return -1;
    }

    if (0 != readdir_r(pDir, &dlocal, &pDirent)) {
        js_log("readdir_r :%s error\n", path);
        ret = -1;
    }

    if (NULL != pDir) {
        closedir(pDir);
        pDir = NULL;
    }

    return ret;
}

static int storage_get_write_stat(char *path)
{
    int ret = 0;
    char test_path[128] = {0};

    snprintf(test_path, sizeof(test_path)-1, "%s/test_dir", path);
    ret = generate_record_dir(test_path);
    if (ret != 0) {
        SYSLOG("========mkdir path error %s:========\n", test_path);
        return FAILURE;
    }

    ret = remove(test_path);
    if (ret != 0) {
        SYSLOG("========remove path error %s:========\n", test_path);
        return FAILURE;
    }

    return ret;
}

static int sdcard_repair(void)
{
    int ret = FAILURE;
    char result[8] = {0};
    char mnt_buf[1024] = {0};
    eSDFormat eCurSDFormat = E_SD_FORMAT_UNKOWN;

    DBG("Repair sdcard\n");
    ret = LoadFile(F_MMC_PRESENT, result, sizeof(result));
    if (ret < 0 || (0 != strncmp(result, "1", 1))) {
        DBG("No SD card found, please insert!\n");
        return ret;
    }

    memset(mnt_buf,0,sizeof(mnt_buf));
    ReadCmdResult("fdisk -l", mnt_buf, sizeof(mnt_buf)-1);
    // umount success & file exits
    if (NULL != strstr(mnt_buf, "mmcblk0p1")){
        do {

            memset(mnt_buf,0,sizeof(mnt_buf));
            ReadCmdResult("head -c16 /dev/mmcblk0p1", mnt_buf, sizeof(mnt_buf)-1);

            if (NULL != strstr(mnt_buf, "XFAT")){
                eCurSDFormat = E_SD_FORMAT_EXFAT;
            }else if (NULL != strstr(mnt_buf, "fat")){
                eCurSDFormat = E_SD_FORMAT_FAT32;
            }else{
                DBG("unkown format sd card, do not fsck\n");
                break;
            }

            DBG("Do fsck for sd card format: %d (0 unkown,1 fat,2 exfat)!\n",eCurSDFormat);
            switch (eCurSDFormat){
                case E_SD_FORMAT_FAT32:
                    //UtilSystemCmd("fsck -t fat -a /dev/mmcblk0p1");  //使用vfat格式修复T卡会占用较多内存可能导致内存不足问题，暂时屏蔽
                    break;
                case E_SD_FORMAT_EXFAT:
                    UtilSystemCmd((char *)"fsck -t exfat -a /dev/mmcblk0p1");
                    break;
                default:
                    break;

            }
        }while(0);

        mkdir("/mnt",0755);
        UtilSystemCmd((char *)"mount -o noatime,nodiratime /dev/mmcblk0p1 /mnt");

        record_storage_dev_add((char *)"/mnt");
    }

    //sd 卡 off->up 修复之后更新一次状态
    chk_sdstat();

    return ret;
}

int storage_card_is_present(void)
{
    int ret = 0;
    char result[8] = {0};
    const char *file_present = F_MMC_PRESENT;

    ret = LoadFile(file_present, result, sizeof(result));
    if (ret < 0 || (0 != strncmp(result, "1", 1))) {
        return 0;
    }
    return 1;
}

int get_sdinfo_pkg(sSdinfoPkg *info)
{
    if (info == NULL) {
        return FAILURE;
    }

    info->cpuid = get_cpuid();
    info->uptime = mono_sec();
    info->version = get_fw_ver();
    info->sd_stat = get_sdstat();
    info->temprt = get_cpu_temperature();
    info->is_test = get_g_sys(testing);

    if (is_okey(F_SD_REPORT)) {
        int date = 0;
        char cid[36] = {0};
        LoadFile2(F_SD_REPORT, "cid:%s date:%d count:%d", cid, &date, &info->all_count);
    }

    if (is_okey("/sys/block/mmcblk0/device/cid")) {
        LoadFile2("/sys/block/mmcblk0/device/cid", "%s", info->cid);
    } else {
        strcpy(info->cid, "badbeef");
    }

    if (is_okey("/tmp/this_boot")) {
        LoadFile2("/tmp/this_boot", "%d", &info->this_boot);
    } else {
        ERR("failed to read /tmp/this_boot\n");
    }

    info->df_used = df_used_p("/mnt");
    info->cnt_rescan = 1; // 非AOV设备始终为1

    return 0;
}

int get_sd_filesystem(void)
{
    int fs_type = E_FS_UNKNOWN;
    char sector_buf[16] = {0};

    int nsize = LoadFile("/dev/mmcblk0p1", sector_buf, sizeof(sector_buf));
    if (nsize <= 0) {
        ERR("read sector fail\n");
        return fs_type;
    }

    if (!strncmp((char*)&sector_buf[0x03], "EXFAT", strlen("EXFAT"))) { //EXFAT位于偏移 0x03
        fs_type = E_FS_EXFAT;
    } else {
        fs_type = E_FS_NOT_EXFAT;
    }
    DBG("sd filesystem is: %d\n", fs_type);

    return fs_type;
}

/* 通知 APP 错误类型
 * 00 正常
 * 01 读错
 * 10 写错
 * 11 读写错
 * 99 写保护
 **/
int get_sdstat()
{
    if (get_g_stat(record, SD_ERR_WRITE_PROTECT) || get_g_stat(record, SD_ERR_STOP)) {
        return 99;
    } else if (get_g_stat(record, SD_ERR_MMCNODE)) {
        return 98;
    } else if (get_g_stat(record, SD_ERR_ACCESS3)) {
        return 3;
    }
    return 0;
}

void sdcard_is_change(void)
{
    char dev_cid[64] = {0};
    char busypath[128] = {0};
    static char rec_cid[64] = {0};
    LoadFile(SD_CARD_CID, dev_cid, sizeof(dev_cid));

    if(strlen(rec_cid) <= 0) {
        LoadFile(SD_CARD_CID, rec_cid, sizeof(rec_cid));
    }

    ReadCmdResult("ls " SDCARD_BUSY_STATUS_PATH, busypath, sizeof(busypath)-1);
    if (strlen(dev_cid) > 0 && strlen(rec_cid) > 0
        && strncmp(dev_cid, rec_cid, sizeof(dev_cid)) != 0) {
        strncpy(rec_cid, dev_cid, sizeof(dev_cid));
        if(get_g_stat(record, SD_ERR_STOP)) {
            UtilSystemCmd2((char *)"echo off > %s", busypath);
            clr_g_stat(record, SD_ERR_STOP);
        }
        DBG("dev_cid:[%s], rec_cid:[%s]\n", dev_cid, rec_cid);
    } else {
        if(get_g_stat(record, SD_ERR_STOP)) {
            UtilSystemCmd2((char *)"echo up > %s", busypath);
        }
    }
}

/*
| NO. | note_en       | note_cn        | stat
| :-- | :------       | :------        | :---
| 1   | cd_in ok      | 无sd卡         | SD_CD_IN
| 2   | node mmc ok   | 有卡，无节点   | SD_ERR_MMCNODE
| 3   | ERR_wrprotect | 有卡，写保护   | SD_ERR_WRITE_PROTECT
| 4   | mount ok      | 有卡，挂载异常 | SD_ERR_MOUNT
| 5   | Err_read      | 有卡，读失败   | SD_ERR_READ
| 6   | ERR_write     | 有卡，写失败   | SD_ERR_WRITE

 */
void chk_sdstat()
{
    const char *dir_path = "/sys/class/mmc_host/mmc0";
    char temp_path[512] = {0};
    char full_path[1024] = {0};
    char curDevPath[128] = {0,};
    char result[8] = {0};
    char busypath[128] = {0};
    char jcpresult[128] = {0};
    int  ret = 0;
    static pthread_mutex_t stat_mutex;

    pthread_mutex_lock(&stat_mutex);
    // 1
    LoadFile(F_MMC_PRESENT, result, sizeof(result));
    if (result[0] == '1') {
        set_g_stat(record, SD_CD_IN);
    } else {
        //清除SD卡异常判断(需要保留tmp_repair bit位,用于下次插入卡时修复临时文件)
        clr_g_stat(record, __FF__ & (~SD_REC_TMP_REPAIR));
        goto __exit;
    }

    if (is_okey2(SDCARD_BUSY_STATUS_PATH)) {
        memset(result, 0, sizeof(result));
        ret = ReadCmdResult("cat " SDCARD_BUSY_STATUS_PATH, result, sizeof(result)-1);
        if (0 != strncmp(result, "0", 1) && ret != FAILURE) {
            if(!get_g_stat(record, SD_ERR_STOP)) {
                set_g_stat(record, SD_ERR_STOP);
                ReadCmdResult("ls " SDCARD_BUSY_STATUS_PATH, busypath, sizeof(busypath)-1);
                UtilSystemCmd2((char *)"echo up > %s", busypath);
                jcpcmd_sendrecv((char *)"sdcard -act stop -path /mnt", jcpresult, sizeof(jcpresult));
                SYSLOG("sdcard busy status, stop usd sdcard\n");
            }
            goto __exit;
        } else {
            if(get_g_stat(record, SD_ERR_STOP)) {
                ReadCmdResult("ls " SDCARD_BUSY_STATUS_PATH, busypath, sizeof(busypath)-1);
                UtilSystemCmd2((char *)"echo off > %s", busypath);
                clr_g_stat(record, SD_ERR_STOP);
            }
        }
    }

    // 2
    if (!is_okey("/dev/mmcblk0")) {
        set_g_stat(record, SD_ERR_MMCNODE);
        goto __exit;
    } else {
        clr_g_stat(record, SD_ERR_MMCNODE);
    }

    // 3 写保护，只能提示换卡，mmc0:xxxx 会随 cid 不同而不同，使用 * 匹配
    result[0] = '\0';
    if (is_pattern_exist(dir_path, "mmc0*", temp_path)) {
        snprintf(full_path, sizeof(full_path) - 1, "%s/block/mmcblk0/ro", temp_path);
        LoadFile2(full_path, "%s", result);
    }

    if (result[0] == '1') {
        SYSLOG("SD_ERR_WRITE_PROTECT\n");
        set_g_stat(record, SD_ERR_WRITE_PROTECT);
        goto __exit;
    } else {
        clr_g_stat(record, SD_ERR_WRITE_PROTECT);
    }

    // 未初始化跳过不处理
    if (get_g_stat(record, SD_ERR_UNAUTH)) {
        goto __exit;
    }

    // 4
    if (storage_get_mmcpath(curDevPath) < 0) {
        set_g_stat(record, SD_ERR_MOUNT);
        goto __exit;
    } else {
        clr_g_stat(record, SD_ERR_MOUNT);
    }

    // 5
    ret = check_storage_readonly(curDevPath);
    ret += storage_get_write_stat(curDevPath);
    if (ret < 0) {
        SYSLOG("SD_ERR_WRITE\n");
        set_g_stat(record, SD_ERR_WRITE);
    } else {
        clr_g_stat(record, SD_ERR_WRITE);
    }

    // 6
    ret = storage_get_read_stat(curDevPath);
    if (ret < 0) {
        SYSLOG("SD_ERR_READ\n");
        set_g_stat(record, SD_ERR_READ);
    } else {
        clr_g_stat(record, SD_ERR_READ);
    }

__exit:
    pthread_mutex_unlock(&stat_mutex);
}

/**
 * 是否去上报 SD 状态
 * 引入本地存放文件 /opt/log/sd_report, 存放 cid, date(记录天数), count
 * 内容: cid:xxxxxxxx date:xxx count:x
 * 上报条件: 满足上电时间大于5分钟, 且满足以下任一条件就上报
 * 1. cid 改变
 * 2. date 改变
 * 每次上电 count 会加 1, 每次上报成功后将 count 清零
 */
int whether_to_report_sd_stat(void)
{
    if (system_get_uptime() < 5*60) {
        return FAILURE;
    }

    if (get_g_stat(record, SD_REPORTED) || !is_mountpoint("/mnt") || !www_reachable()) {
        return FAILURE;
    }

    char cid[36] = {0};
    int date = 0, count = 0;
    sSdinfoPkg sdinfo = {0};
    get_sdinfo_pkg(&sdinfo);
    if (is_okey(F_SD_REPORT)) {
        LoadFile2(F_SD_REPORT, "cid:%s date:%d count:%d", cid, &date, &count);
    }

    struct tm localTime;
    time_t currTime = time(NULL);
    localtime_r(&currTime, &localTime);
    int day_of_year = localTime.tm_yday + 1;  // +1 转换为 1~366, date +%j

    if (!strcmp(cid, sdinfo.cid) && date == day_of_year) {
        return FAILURE;
    }

    if (report_sd_stat() != SUCCESS) {
        return FAILURE;
    }

    set_g_stat(record, SD_REPORTED);
    char buf[256] = {0};
    snprintf(buf, sizeof(buf), "cid:%s date:%d count:%d", sdinfo.cid, day_of_year, 0);
    DBG("buf: %s\n", buf);
    DumpFile(F_SD_REPORT, buf, strlen(buf));

    return SUCCESS;
}

void storage_error_watch(void *data)
{
    dbg_record("tick %d\n", g_disk_ticks);

    if (is_okey("/tmp/no_autommc") || get_g_stat(record, SD_REC_FORMAT)) {
        DBG("sd debuging[%s] now, RETURN\n", get_g_stat(record, SD_REC_FORMAT) ? "fmt" : "run");
        return;
    }

    int old = get_g_stat(record, __FF__);
    chk_sdstat();

    if (!get_g_stat(record, SD_CD_IN)) {
        return;
    }

    if(get_g_stat(record, SD_ERR_STOP)) {
        return;
    }

    whether_to_report_sd_stat();

    // 有 mmcblk0 无 mmcblk0p1 分区表损坏
    if (get_g_stat(record, SD_ERR_MOUNT)) {
        if (is_need_exfat_repair()) {
            exfat_repair();
            goto REINSERT;
        }
    }

    // 写保护判断开始
    if (get_g_stat(record, SD_ERR_WRITE_PROTECT)) {
        if (!(old & SD_ERR_WRITE_PROTECT)){
            uninit_record_watch();
            storage_remove_devpath((char *)"/mnt");
            init_record_watch();
        }
        return;
    }

    // 访问异常
    if (get_g_stat(record, SD_ERR_ACCESS3 | SD_ERR_MMCNODE)) {
        ++g_mmc_absent;
        ERR("SD card present, unable to get mmc path @%d!\n", g_mmc_absent);
        if (g_mmc_absent > 7) {
            // 不能打断升级
            if (!is_okey(F_MMC_ABSENT) && !get_g_sys(upgrading)) {
                TouchFile(F_MMC_ABSENT);
                DELAY_REBOOT_LINUX();
                return;
            }
        }
    } else {
        if (is_okey(F_MMC_ABSENT)) {
            remove(F_MMC_ABSENT);
        }
        g_disk_ticks = g_mmc_absent = 0;
        return;
    }

REINSERT:
    // RESCAN
    g_disk_ticks++;
    if (g_disk_ticks == 2 || g_disk_ticks == 4 || g_disk_ticks == 6 || g_disk_ticks == 100 || g_disk_ticks == 1000) {
        char mnt_buf[128] = {0};
        char szCmd[128] = {0};
        snprintf(szCmd, sizeof(szCmd), "rm %s", F_ADD_FILE);
        UtilSystemCmd(szCmd);
        memset(szCmd, 0, sizeof(szCmd));
        snprintf(szCmd, sizeof(szCmd), "rm %s", F_REMOVE_FILE);
        UtilSystemCmd(szCmd);
        uninit_record_watch();
        toggle_redirect(0);
        sync();

        if (SUCCESS == storage_get_mmcpath(mnt_buf)) {
            storage_remove_devpath(mnt_buf);
        }

        if (is_mountpoint("/mnt")) {
            umount2("/mnt", MNT_DETACH);
        }

        if (is_mountpoint("/mnt")) {
            usleep(500*1000);
            DBG("umount twice\n");
            umount2("/mnt", MNT_DETACH);
        }

        // rescan
        SYSLOG("SD card may have errors, go to rescan times: %d!\n", g_disk_ticks);
        DumpFile(F_MMC_PWR_CTRL, "off", strlen("off"));
        usleep(500*1000);
        DumpFile(F_MMC_PWR_CTRL, "up", strlen("up"));
        usleep(100*1000);
        sdcard_repair();
        clr_g_stat(record, SD_REPORTED);
        // start
        init_record_watch();
    }
    return;
}

int init_record_disk(void)
{
    int ret = -1;

    do {
        if(g_disk_scheduler != NULL) {
            break;
        }

        pthread_mutex_init(&g_disk_mutex, NULL);
        g_disk_scheduler = sch_disk;
        js_create_timer_r(g_disk_scheduler, 30*1000, 15*1000, storage_devpath_full_watch, NULL, &g_disk_watcher);
        js_create_timer_r(g_disk_scheduler, 5*1000, 15*1000 , storage_error_watch, NULL, &g_readonly_watcher);

        ret = init_storage_mount_table();

        storage_print_all_devpath();

        js_log("init_record_disk!\n");

    } while(0);

    return ret;
}

int uninit_record_disk(void)
{
    js_log("uninit_record_disk\n");

    if(g_disk_scheduler == NULL)
        return -1;

    js_delete_timer_r(&g_disk_watcher);

    js_delete_timer_r(&g_readonly_watcher);

    g_disk_scheduler = NULL; // js_delete_scheduler() 在 sd_recovery() 中反初始化

    return 0;
}

int record_storage_dev_add(char *path)
{
    return storage_add_devpath(path);
}

int record_storage_dev_remove(char *path)
{
    return storage_remove_devpath(path);
}

int record_get_storage_info(RecordCtrlS *ctrls, char *buf, int bufsize)
{
    int ret = 0;
    char curDevPath[128] = {0};
    char *p = buf;
    int no = 0;
    int line = 0;
    struct statfs stfs;

    do{
        p += sprintf(p, "recordstatus%d=%d;", 0, record_get_currec_status());

        if (get_g_stat(record, SD_ERR_MMCNODE|SD_ERR_MOUNT)) {
            line = __LINE__; break;
        }

        if (get_g_stat(record, SD_ERR_UNAUTH)) {
            strncpy(curDevPath, "/mnt", sizeof(curDevPath));
        } else {
            ret = storage_get_mmcpath(curDevPath);
            if(ret < 0) {
                line = __LINE__; break;
            }
        }

        /* sd状态查询加速 */
        ret = statfs(curDevPath, &stfs);
        if (ret < 0) {
            line = __LINE__; break;
        }

        long free = ((stfs.f_bfree >> 10) * stfs.f_bsize) >> 10; // m
        long total = ((stfs.f_blocks >> 10) * stfs.f_bsize) >> 10; // m

        if (2 == ctrls->sd_times || 4 == ctrls->sd_times || 8 == ctrls->sd_times) {
            free = free*ctrls->sd_times;
            total = total*ctrls->sd_times;
        }

        p += sprintf(p, "diskname%d=%s;disktotal%d=%ld;diskfree%d=%ld;xholdM=%d;removing=%d;",
                 no, curDevPath,
                 no, total,
                 no, free,
                 get_free_xholdM_of_total(total),
                 is_removing_files()
                 );

        return 0;
    } while(0);

//__errout:
    DBG("errout from line: %d\n", line);
    if (!get_g_stat(record, SD_CD_IN)) {
        p += sprintf(p, "disktotal0=0;diskfree0=0;");
    } else {
        p += sprintf(p, "diskname0=/mnt;disktotal0=0;diskfree0=0;");
    }    return 0;
}

void record_storage_dev_manage_cb(void* userdata)
{
    // 异常状态只有在真实拔插卡后才清理
    if(!storage_card_is_present()) {
        clr_g_stat(record, SD_ERR_WRITE | SD_ERR_READ | SD_ERR_MOUNT | SD_ERR_WRITE_PROTECT | SD_ERR_MMCNODE);
        g_disk_ticks = 0;
        g_mmc_absent = 0;
    }
    clr_g_stat(record, SD_ERR_UNAUTH);
}

int record_storage_dev_manage(void)
{
    js_run_function(g_disk_scheduler, record_storage_dev_manage_cb, NULL, 0);
    return 0;
}
