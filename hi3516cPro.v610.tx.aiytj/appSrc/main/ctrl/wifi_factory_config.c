/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    : .wifi_factory_config.c
 * Created Time : 2016-05-23
 * Version      : 1.0
 * Author       : cheby
 * Description  :
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/ioctl.h>
#include <sys/sysinfo.h>
#include <sys/reboot.h>
#include <linux/watchdog.h>
#include <linux/reboot.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <math.h>
#include <sys/resource.h>
#include <openssl/md5.h>

#include "airlink.h"
#include "debug.h"
#include "utils.h"
#include "delay_exec.h"
#include "wifi_factory_config.h"
#include "soft_check.h"
#include "conf_nand.h"
#include "net_config.h"
#include "our_md5.h"
#include "base64.h"
#include "factory_db.h"
#include "g_sys.h"
#include "jcpService.h"
#include "update.h"
#include "confapi.h"
#include "logapi.h"
#include "encrypt.h"
#include "encodeapi.h"
#include "conf_list.h"
#include "factory_db.h"
#include "jconfig.h"
#include "encode_audio_queue.h"
#include "system_main.h"

#define FACTORY_SERVER_IP "192.168.1.26"
#define FACTORY_SSID      "TP-LINK_FFJCO"
#define FACTORY_PW        "jabsco123456"
#define FACTORY_GW        "192.168.1.1"
#define ETH_NAME          "wlan0"
#define ETH_7601_USTA     "wlan0"

#define FACTORY_CONFIG    "factory_config"
#define TMP_FACTORY_CONFIG "/tmp/"FACTORY_CONFIG

#define FACTORY_MD5_LEN 16
BOOL check_device_id()
{
    FILE* pfd = NULL;
    char file_buf[1024];
    int len = 0;
    int times = 10;
    char szDevID[16] = {0};
    char *ptr = NULL;

    memset(file_buf, 0, sizeof(file_buf));

    do {
        times--;
        pfd = vpopen("cat /proc/cmdline", "r");
        if (pfd == NULL) {
            DBG("init_factory_config popen fail\n");
            continue;
        }

        break;
    }while(times >= 0);

    if (times < 0) {
        return false;
    }

    len = fread(file_buf, 1, sizeof(file_buf), pfd);
    vpclose(pfd);

    if (len <= 0) {
        return false;
    }

    ptr = strstr(file_buf, "device_id=");
    if (ptr == NULL) {
        DBG("not set device id!\n");
        return false;
    }

    ptr += strlen("device_id=");
    memcpy(szDevID, ptr, 11);

    if (strncmp(szDevID, "00000000000", 11) == 0) {
        return false;
    }

    //DBG("szDevID: %s\n", szDevID);
    return TRUE;
}


static BOOL check_p2p_config()
{
    if (access(F_P2P_TRIPLE, F_OK) == 0) {
        return true;
    }

    return false;
}

int reset_rlimit_stack()
{
    struct rlimit limit = {0};
    if (getrlimit(RLIMIT_AS, &limit) == 0 ) {
        DBG("soft limit %ld, hard limit:%ld\n", (long)limit.rlim_cur, (long)limit.rlim_max);
    }

    memset(&limit, 0, sizeof(struct rlimit));
    if (getrlimit(RLIMIT_STACK, &limit) == 0) {
        DBG("soft limit %ld, hard limit:%ld\n", (long)limit.rlim_cur, (long)limit.rlim_max);
        limit.rlim_max = RLIM_INFINITY;
        limit.rlim_cur = 6*1024*1024;
        if (setrlimit(RLIMIT_STACK, &limit) != 0) {
            ERR("setrlimit fail\n");
        }
    }

    return 0;
}

int init_factory_config(void *data, int *bFactoryMode)
{
    reset_rlimit_stack();

    if (check_device_id() && check_p2p_config()) {
        DBG("device id is ok, danale conf is ok\n");

        return 0;
    } else {
        SYSLOG("Enter factory pattern, only once\n");
        //light_ctrl(AUDIO_FACTORY_MODE);
        if (bFactoryMode) {
            *bFactoryMode = TRUE;
        }
    }

    return 0;
}

static int burn_ids(struct record *rec)
{
    char cmdline[1024] = {0};
    char respbuf[256]  = {0};

   /* ali.conf */
    sync();
    if(strlen(rec->product_secret)>0){
        snprintf(cmdline,sizeof(cmdline),"aliconfcfg -act set -product_key %s -device_name %s \
            -device_secret %s -product_secret %s",
            rec->product_key, rec->device_name, rec->device_secret,rec->product_secret);
    }
    /*product_key  may be 0,then only burn three params*/
    else if(strlen(rec->product_secret)<=0){
        snprintf(cmdline,sizeof(cmdline),"aliconfcfg -act set -product_key %s \
            -device_name %s -device_secret %s",
                rec->product_key, rec->device_name, rec->device_secret);
        }
    jcpcmd_sendrecv(cmdline, respbuf, sizeof(respbuf));

    if (strstr(respbuf, "Fail") || strstr(respbuf, "Error")) {
        SYSLOG("cmd [%s] exec fail@[%s]\n", cmdline, respbuf);
        return -1;
    }

    DBG("%s\n %s\n", cmdline, respbuf);

    /* devid delay 15s reboot */
    snprintf(cmdline, sizeof(cmdline), "prienv -act set -device_id %s", rec->devid);
    jcpcmd_sendrecv(cmdline, respbuf, sizeof(respbuf));

    if (strstr(respbuf, "Fail") || strstr(respbuf, "Error")) {
        SYSLOG("cmd [%s] exec fail@[%s]\n", cmdline, respbuf);
        return -1;
    }

    DBG("%s\n %s\n", cmdline, respbuf);

    return 0;
}

int is_type_validated()
{
    if (access(FACTORY_TYPE_FILE, F_OK) != 0) {
        DBG("___ %s not exist\n", FACTORY_TYPE_FILE);
        return FALSE;
    }

    int ret;
    char local_type[64] = {0};
    FILE *fp;

    ret = LoadFile(F_LOCAL_TYPE, local_type, sizeof(local_type));
    if (ret <= 0) {
        return FALSE;
    }

    fp = fopen(FACTORY_TYPE_FILE, "rb");
    if (fp == NULL) {
        DBG("open [%s] failed!\n", FACTORY_TYPE_FILE);
        ret = FALSE;
        goto __exit;
    }

    char line2[128] = {0};
    ssize_t read = fread(line2, 1, sizeof(line2)-1, fp);

    if (read <= 0) {
        DBG("2nd line read error return %d\n", read);
        ret = FALSE;
        goto __exit;
    }

    char *p_nl = strchr(line2, '\n');
    if (!p_nl) {
        DBG("can't get newline\n");
        ret = FALSE;
        goto __exit;
    }

    char key[LEN_MD5+4] = {0};
    char type[64] = {0};

    ret = get_key(key);
    if (ret != 0) {
        DBG("get_key error!\n");
        ret = FALSE;
        goto __exit;
    }

    p_nl++;
    //ret = str_decrypt_tfid(type, p_nl, strlen(p_nl), key);
    if (ret != SUCCESS) {
        DBG("decrypt type.txt fail\n");
        ret = FALSE;
        goto __exit;
    }

    SYSLOG("decrypt type [%s] vs. local_type [%s]\n", type, local_type);

    ret = strncmp(local_type, type, strlen(local_type)) ? FALSE : TRUE;

__exit:
    fclose(fp);
    return ret;
}

void init_factory_burning(int *burning_ids)
{
    reset_rlimit_stack();

    if (check_device_id() && check_p2p_config()) {
        DBG("dev id is ok, p2p conf is ok\n");
        return;
    }

    if (!is_type_validated()) {
        return;
    }

    if (access(FACTORY_DB_FILE, F_OK) == 0) {
        if (burning_ids) {
            *burning_ids = TRUE;
        }
    }
    return;
}

void exec_factory_burning(int burning_ids)
{
    if (!burning_ids) {
        return;
    }

    SYSLOG("burn from JCPCMD...\n");

    int ret = 0;
    int total, used;
    char key[LEN_MD5+4] = {0};
    struct record rec = {{0},};
    struct record rec2 = {{0},};

    ret = stat_sdcard(&total, &used);
    if (ret != 0) {
        DBG("stat_sdcard error!\n");
        goto __exit;
    }

    if (total == used) {
        SYSLOG("total:%d have run out!\n", total);
        encode_audio_queue_push_amr(AUDIO_RUNINGOUT_ID, TRUE);
        ret = -1;
        goto __exit;
    }

    ret = fetch_record(&rec);
    if (ret != 0) {
        DBG("fentch_record error!\n");
        ret = -1;
        goto __exit;
    }

    ret = get_key(key);
    if (ret != 0) {
        DBG("get_key error!\n");
        ret = -1;
        goto __exit;
    }

    /* wait to be filled by Zhao Sir */
    //ret = decrypt_record(key,&rec,&rec2);

    if (ret != 0) {
        DBG("decrypt_record error!\n");
        ret = -1;
        goto __exit;
    }
    ret = burn_ids(&rec2);
    if (ret != 0) {
        DBG("burn_ids error!\n");
        ret = -1;
        goto __exit;
    }

    char hwaddr[20] = {0};
    net_get_macaddr("wlan0", hwaddr);

    ret = use_record(hwaddr);
    if (ret != 0) {
        DBG("use_record error!\n");
        ret = -1;
        goto __exit;
    }

    DBG("burn ids success!\n");

__exit:
    quit_sdcard();
    SYSLOG("burn_ids ret[%s]\n", (ret==0)?"succ":"fail");
    sync_syslog();
    log_sync();
    sync();

    if (ret ==  0) {
        DBG("--------------------------------------------\n");
        DBG("              Success and reboot !!!        \n");
        DBG("--------------------------------------------\n");
        encode_audio_queue_push_amr(AUDIO_BURNING_SUCC, FALSE);
        //clean_light_ctrl(LED_RED_ON);
        sleep(5);
        reboot(LINUX_REBOOT_CMD_RESTART);
    } else {
        DBG("--------------------------------------------\n");
        DBG("              Failure, pause!!!             \n");
        DBG("--------------------------------------------\n");
        encode_audio_queue_push_amr(AUDIO_BURNING_FAIL, FALSE);
        //clean_light_ctrl(LED_BLUE_BLINK);

        while (1) {
            sleep(10);
        }
    }

    return;
}

/*
 * 直接使用网络烧录，不使用 sd 卡烧录了，相关的语音播报及 amr 文件都可以删除。
 */
void init_factory_testing(void)
{
    if (is_pattern_exist("/mnt", "super_*.txt", NULL) || is_okey(FACTORY_FACTEST)) {
        set_g_sys(factest);
    } else if (is_okey(AGING_TEST_FILE)) {
        set_g_sys(agingtest);
    }
    // FACTORY_IP 基本上不使用，而是 12 路通过通断器每次只连通一路
    if (get_g_sys(factest) && is_okey(FACTORY_IP)) {
        char ip[32] = {0};

        LoadFile(FACTORY_IP, ip, sizeof(ip));
        drop_tail_space(ip);

        NetEthS data = {{0}};
        conf_get_ethcfg(&data);

        data.dhcpen = 0;
        data.ipadaen = 0;
        strcpy(data.ip, ip);
        strcpy(data.gw, ip);

        char * p = strrchr(data.gw, '.');
        if (NULL != p) {
            p[1] = '1';
            p[2] = '\0';
            conf_set_ethcfg(data);
        }
    }
    return;
}

void init_factory_upgrading(void)
{
    if (get_g_sys(factest)) {
        encode_audio_queue_push_amr(AUDIO_FACTORY_TEST, FALSE);
    }

    if (!is_okey(FACTORY_FFW_FILE)) {
        DBG("%s not exist\n", FACTORY_FFW_FILE);
        return;
    }

    UtilSystemCmd("/ipc/bin/lzbox ffw untar");
    if (!is_okey(UPDATE_TMP_FILE)) {
        DBG("Warning: %s not exist\n", UPDATE_TMP_FILE);
        return;
    }
    int ret = de_encrypt_file(UPDATE_TMP_FILE);
    if (ret < 0) {
        DBG("Warning: %s de_encrypt_file fail\n", UPDATE_TMP_FILE);
        remove(UPDATE_TMP_FILE);
        return;
    }

    DBG("---------- going on --------------\n");

    encode_audio_queue_push_amr(AUDIO_UPGRADING_NO_OFF, FALSE);   // pleas wait
    usleep(1000*1200);

    if (SUCCESS == JCOUpdateBegin()) {
        //clean_light_ctrl(LED_BLUE_ON);
        JCOUpdateJoin();
        DBG("---------- join OK --------------\n");
        goto __exit;
    }

__exit:
    SYSLOG("upgrade %s success\n", UPDATE_TMP_FILE);
    sleep(3);
    exit_jco_server();
    return;
}
