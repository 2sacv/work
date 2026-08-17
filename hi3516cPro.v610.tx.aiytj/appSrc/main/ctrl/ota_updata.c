/**
* Copyright (C) by Jabsco Company
*
* @File Name    :
* @Created Time : 2019-10-28
* @Version      : 1.0
* @Author       : Xuls
* @Description  : ota升级
*/
#if 1
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <pthread.h>
#include <time.h>
#include <fcntl.h>
#include <pthread.h>
#include <linux/reboot.h>
#include <sys/file.h>
#include "ota_updata.h"
#include "debug.h"
#include "update.h"
#include "encrypt.h"
#include "jconfig.h"
#include "utils.h"
#include "delay_exec.h"
#include "confapi.h"
#include "system_main.h"
#include "encode_main.h"
#include "js_http_client.h"

OtaUpgradeUrlS pSzUrl = {0};

int OtaUpgradingProcess()
{
    if (access(UPDATE_TMP_FILE, F_OK) != 0) {
        DBG("Warning: %s not exist\n", UPDATE_TMP_FILE);
        conf_set_update_progressbar(UPDATE_ERR_NON_EXIST);
        goto __exit;
    }

    int ret = de_encrypt_file(UPDATE_TMP_FILE);
    if (ret < 0) {
        SYSLOG("Warning: %s de_encrypt_file fail\n", UPDATE_TMP_FILE);
        remove(UPDATE_TMP_FILE);
        goto __exit;
    }

    DBG("---------- going on --------------\n");

    if (SUCCESS == JCOUpdateBegin()) {
        JCOUpdateJoin();
        DBG("---------- join OK --------------\n");
        goto __exit;
    }

__exit:
    SYSLOG("upgrade %s success\n", UPDATE_TMP_FILE);
    sleep(3);
    exit_jco_server();
    return 0;
}

static void download_file_http_on_body_data(void *userdata, const char *body, int bodysize, int isfinal)
{
    //printf("[bodysize:%d, isfinal:%d]:%s\n", bodysize, isfinal, body);
    FILE *w_file = (FILE *)userdata;
    fwrite(body, 1, bodysize, w_file);
}

static int OtaUpdateDownloadFile(char *download_path)
{
    if (download_path == NULL) {
        return -1;
    }

    char ip[128] = {0};
    char path[128] = {0};
    int port = 80;
    const char *scheme = "http";

    // 解析 url 例: http://121.40.104.39:58889/wuhy/a.tgz
    const char *head = download_path;

    // 去掉前面可能存在的空格
    while (*head && isspace((unsigned char)*head)) head++;

    // 搜索协议头，不存在就使用默认的 http
    char *scheme_end = strstr(head, "://");
    if (scheme_end != NULL) {
        // 存在协议头
        if (scheme_end - head == 4 && strncmp(head, "http", 4) == 0) {
            scheme = "http";
            port = 80;
        } else if (scheme_end - head == 5 && strncmp(head, "https", 5) == 0) {
            scheme = "https";
            port = 443;
        } else {
            ERR("unknow scheme\n");
        }
        head += 3;
    }

    // 搜索 host + port
    const char *slash = strchr(head, '/');
    if (!slash) { // 找不到 path 的分隔符
        ERR("url error\n");
        return -1;
    }

    size_t ip_size = slash - head;
    const char *colon = memchr(head, ':', slash - head);
    if (colon) { // 存在端口
        port = atoi(colon + 1);
        ip_size = colon - head;
    }

    if (ip_size >= sizeof(ip)) ip_size = sizeof(ip) - 1;
    strncpy(ip, head, ip_size);
    ip[ip_size] = '\0'; // ip 可以是网段或者域名
    head = slash;

    // path 填入
    size_t path_size = strlen(head);
    if (path_size >= sizeof(path)) path_size = sizeof(path) - 1;
    strncpy(path, head, path_size);
    path[path_size] = '\0';

    FILE *w_file = fopen(UPDATE_TMP_FILE, "wb");
    if (w_file == NULL) {
        ERR("fopen fail\n");
        return -1;
    }

    char host[128 + 8] = {0};
    snprintf(host, sizeof(host), "%s:%d", ip, port);
    jhttp_client_t *client = jhttp_client_create(scheme, (char *)ip, port);
    jhttp_client_set_header(client, "Host", host);
    jhttp_client_get(client, path, NULL, download_file_http_on_body_data, (void *)w_file, 240 * 1000);

    if (w_file) {
        fclose(w_file);
        w_file = NULL;
        DBG("recv file success\n");
    }
    sync();

    return 0;
}

static void*  OtaUpdateThread(void *data)
{
    int ret = 0;

    ret = OtaUpgradingProcess();
    if (ret < 0) {
        return NULL;
    }

    return NULL;
}

int OtaDownFile(const char *upgrade_url)
{
    DBG("upgrade_url=%s\n",upgrade_url);

    system_upmedia_uninit();
    uninit_encode_wait();

    int ret = OtaUpdateDownloadFile((char *)upgrade_url);
    if (ret != 0) {
        SYSLOG("OtaUpdateDownloadFile FAIL\n");
        DELAY_REBOOT_LINUX();
        return 0;
    }
    OtaUpdateThread(NULL);

    return 0;
}

static void*  UpdateThread(void *data)
{
    DBG("szUrl->szUrl=%s\n",pSzUrl.url);

    secs_delay_reboot(4*60, __func__);
    pthread_detach(pthread_self());

    OtaDownFile(pSzUrl.url);

    while (1) {
        sleep(5);
    }

    return NULL;
}

int DownFileThread(OtaUpgradeUrlS *pUrl)
{
    if(pUrl != NULL)
    {
        DBG("pUrl.url=%s\n",pUrl->url);
        memcpy(pSzUrl.url, pUrl->url,sizeof(OtaUpgradeUrlS));
        pthread_t down_thread_pro = 0UL;
        if(pthread_create(&down_thread_pro, NULL, UpdateThread, NULL)) {
            DBG("create thread faile\n");
            return -1;
        }
    }
    else
    {
        DBG("pUrl is NULL\n");
    }

    return 0;
}


#endif


