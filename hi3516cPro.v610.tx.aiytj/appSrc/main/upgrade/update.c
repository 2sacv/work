/******************************************************************************
    Copyright (C), 2008-2018, JABSCO ELECTRONIC Tech. Co., Ltd
    File Name   : jco_update.c
    History     :
                    Create by wgy.2008.11.20
                    Modify by zhangjian 2012-10-17
                    Modify by zhangjian 2014-04-14
******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <utime.h>
#include <sys/ioctl.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <linux/reboot.h>
#include <netinet/in.h>
#include <openssl/md5.h>

#include "system_ctrl.h"
#include "update.h"
#include "utils.h"

#include "logapi.h"
#include "debug.h"
#include "pthread_manage.h"
//#include "system_app_manage.h"
#include "delay_exec.h"
#include "confapi.h"
#include "jconfig.h"
//#include "ptz_service.h"
#include "conf_list.h"
#include "system_main.h"
#include "encrypt.h"
#include "jevent.h"
#include "system_main.h"
#include "encode_main.h"
#include "airlink.h"
#include "sd_recovery.h"
#include "ble_services.h"

#define UPDATE_ENTRY            "/tmp/updateExt.sh"
#define BYTES_OF_MD5SUM         32
#define BYTES_OF_SCRIPT_SIZE    6
#define MAX_SCRIPT_SIZE         (1024 * 20)
#define MD5SUM_BUF_SIZE         (1024 * 16)
static pthread_t pth_upgrade = 0;

static int do_md5sum(const char *path, char *md5sum)
{
    MD5_CTX ctx;
    unsigned char md[MD5_DIGEST_LENGTH] = {0};
    int ret = -1;
    FILE *fp = NULL;
    unsigned char *buf = NULL;

    buf = (unsigned char *)calloc(1, MD5SUM_BUF_SIZE);
    if (NULL == buf) {
        SYSLOG("calloc flash buf failed!\n");
        ret = FAILURE;
        goto __exit;
    }

    fp = fopen(path, "r");
    if (NULL == fp) {
        ERR("fopen %s failed: %s\n", path, strerror(errno));
        ret = FAILURE;
        goto __exit;
    }

    MD5_Init(&ctx);
    while(1) {
        //int len = read(fd, buf, MD5SUM_BUF_SIZE);
        int len = fread(buf, 1, MD5SUM_BUF_SIZE, fp);
        if (0 >= len) {
            break;
        }
        MD5_Update(&ctx, buf, (unsigned long)len);
    }
    MD5_Final(md, &ctx);

    int i;
    for (i = 0; i < MD5_DIGEST_LENGTH; i++) {
        sprintf(md5sum+i*2, "%02x", md[i]);
    }
    // memcpy(md5sum, md, sizeof(md));
    ret = SUCCESS;

__exit:
    if (NULL != fp) {
        fclose(fp);
    }

    if (NULL != buf) {
        free(buf);
    }

    return ret;
}

static int get_md5sum_peel(const char *path)
{
    int  ret = -1;
    char md5_at_file[64] = {0};
    char md5_at_here[64] = {0};
    int  filesize = -1;
    int  tarsize = -1;

    int  script_size = 0;
    char str_scriptsz[12] = {0};
    char buf_script[MAX_SCRIPT_SIZE] = {0};

    FILE *fp = NULL;

    fp = fopen(path, "r");
    return_val_if_fail(fp != NULL, FAILURE);

    ret = fseek(fp, -BYTES_OF_MD5SUM, SEEK_END);
    return_val_if_fail(-1 != ret, FAILURE);

    filesize = ftell(fp);
    return_val_if_fail(-1 != filesize, FAILURE);

    ret = fread(md5_at_file, 1, BYTES_OF_MD5SUM, fp);
    return_val_if_fail(ret == BYTES_OF_MD5SUM, FAILURE);

    SYSLOG("tar zxf updateExt.sh...\n");

    ret = fseek(fp, -BYTES_OF_MD5SUM-BYTES_OF_SCRIPT_SIZE, SEEK_END);
    return_val_if_fail(-1 != ret, FAILURE);

    ret = fread(str_scriptsz, 1, BYTES_OF_SCRIPT_SIZE, fp);
    return_val_if_fail(ret == BYTES_OF_SCRIPT_SIZE, FAILURE);

    script_size = atoi(str_scriptsz);
    return_val_if_fail(script_size > 0 && script_size < MAX_SCRIPT_SIZE, FAILURE);

    ret = fseek(fp, -BYTES_OF_MD5SUM-BYTES_OF_SCRIPT_SIZE-script_size, SEEK_END);
    return_val_if_fail(-1 != ret, FAILURE);

    tarsize = ftell(fp);
    return_val_if_fail(-1 != tarsize, FAILURE);

    ret = fread(buf_script, script_size, 1, fp);
    return_val_if_fail(ret == 1, FAILURE);

    SYSLOG("tar zxf updateExt.sh over.");
    fclose(fp);
    sync();
    fp = fopen(UPDATE_ENTRY, "w");
    fchmod(fileno(fp), 0755);
    ret = fwrite(buf_script, script_size, 1, fp);
    return_val_if_fail(ret == 1, FAILURE);
    fflush(fp);
    fclose(fp);

    // updateExt.sh end

    ret = truncate(path, filesize);
    return_val_if_fail(-1 != ret, FAILURE);

    SYSLOG("do_md5sum\n");
    do_md5sum(path, md5_at_here);

    SYSLOG("md5_at_file:%s\n", md5_at_file);
    SYSLOG("md5_at_here:%s\n", md5_at_here);

    ret = strncmp(md5_at_file, md5_at_here, BYTES_OF_MD5SUM);
    return_val_if_fail(ret == 0, FAILURE);

    ret = truncate(path, tarsize);
    return_val_if_fail(-1 != ret, FAILURE);

    return 0;
}

/*
 * from 2012-10-16, only (UPDATE_CMD_PACKET == updateCmd) support
 * UPDATE_TMP_FILE is the upgrade file
 */
static int update_firware_routine(int type)
{
    int ret;

    SYSLOG("md5sum type:%d...\n", type);
    conf_set_update_progressbar(UPDATE_DO_MD5SUM);
    ret = get_md5sum_peel(UPDATE_TMP_FILE);
    if (ret != SUCCESS) {
        SYSLOG("get_md5sum_peel fail\n");
        conf_set_update_progressbar(UPDATE_ERR_MD5SUM);
        goto cleanup;
    }

    ret = access(UPDATE_ENTRY, F_OK);
    if (ret != 0) {
        SYSLOG("access %s fail\n", UPDATE_ENTRY);
        conf_set_update_progressbar(UPDATE_ERR_ENTRY);
        goto cleanup;
    }
    
    if (3 != type) {
        system_upgrade_uninit();
    }
    conf_set_update_progressbar(UPDATE_DO_BASH);

    /* handover will not sync all files */
    LOG("______ handover to updateExt.sh _______\n");
    uninit_server_config();
    log_sync();
    sync_syslog();      // roll syslog only when reboot
    sync();
    
    DropCache(__func__); // sd recovery, md5peel may product _cache_
    UtilSystemCmd("free; /ipc/bin/ztop j once");
    SYSLOG("Prepare handing over to updateExt.sh\n");
    UtilSystemCmd(UPDATE_ENTRY " " UPDATE_TMP_FILE);

    if (UPDATE_SUCCESS == conf_get_update_progressbar()) {
        SYSLOG("------ update success ------ !!!\n");
        LOG("------ update success ------ !!!\n");
        ret = SUCCESS;
    } else {
        SYSLOG("****** update failure ****** !!!\n");
        LOG("****** update failure %d ****** !!!\n", conf_get_update_progressbar());
        ret = FAILURE;
    }

    if (3 != type) {// reboot
        DELAY_REBOOT_LINUX();
    }
    return ret;

cleanup:
    /* sleep 4s for webpage read status */
    sleep(4);
    unlink(UPDATE_TMP_FILE);
    DELAY_REBOOT_LINUX();

    return FAILURE;
}

static void *start_upgrade()
{
    SYSLOG("-----------start_upgrade-----------------\n");
    wifi_stop();
    UpdateS inner = {0};
    get_config(handleUpdateCfg, inner);
    update_firware_routine(inner.type);

    return NULL;
}

/*
 * 脚本执行体的唯一入口，
 * 函数执行前，已经做好了解密
 * set_g_sys(upgrading) 状态及事件，都在这里发出。
 */
int JCOUpdateBegin()
{
    set_g_sys(upgrading);
    send_conf_nake(JEvent_UpdateBegin);
    usleep(5*1000*1000);    // 腾出 cpu 给事件响应
    DropCache(__func__);
    CompactMemo(__func__);
    DBG("is upgrading\n");

    if ((pth_upgrade = create_pthread("Update", start_upgrade, NULL, NULL)) == 0) {
        SYSLOG("JCO_PthreadMNG_Create is error\n");
        return FAILURE;
    }

    return SUCCESS;
}

int JCOUpdateJoin()
{
    join_pthread(pth_upgrade);
    return 0;
}

