#ifdef PLATFORM_TENCENT
#include "utils.h"
#include "debug.h"
#include "js_scheduler.h"
#include "confapi.h"
#include "jconfstruct.h"
#include "cJSON.h"
#include "conf_nand.h"
#include "id_protect.h"
#include "factory_db.h"
#include "net_check.h"

#include "qcloud_iot_export.h"
#include "tencent_param_conf.h"
#include "tencent_server.h"
#include "net_qrcode.h"

int tencent_load_triple_info(const char* file_name, TripleInfoS* info)
{
    int read_bytes = 0;
    char conf_buf[1024] = {0};
    char* pconf = conf_buf;
    int fd = -1;

    fd = open(file_name, O_RDONLY);
    if (fd < 0) {
        ERR("open %s fail\n", file_name);
        return FAILURE;
    }

    read_bytes = Readfully(fd, conf_buf, sizeof(conf_buf));
    if (read_bytes <= 0) {
        close(fd);
        return FAILURE;
    }

    close(fd);
    fd = -1;

    int i = 0;
    while (char *str = strtok(pconf, ";")) {
        switch (i++) {
            case 0:
                pconf = NULL;
                snprintf(info->product_key, sizeof(info->product_key), str);
                break;
            case 1:
                snprintf(info->device_name, sizeof(info->device_name), str);
                break;
            case 2:
                snprintf(info->device_secret, sizeof(info->device_secret), str);
                break;
            default:
                break;
        }
    }

    if (strlen(info->product_key) == 0 || strlen(info->device_name) == 0 || strlen(info->device_secret) == 0) {
        ERR("get triple info error, please check conf file\n");
        return FAILURE;
    }

    return SUCCESS;
}

int tencent_uboot_triple_repair(const char *original)
{
    if (NULL == original) {
        return FAILURE;
    }

    int ret = FAILURE;
    char txconf[256]  = {0};
    char cmdline[1024] = {0};
    char back_buf[1024]   = {0};

    do {
        ret = LoadFile("/proc/cmdline", cmdline, sizeof(cmdline) - 1);
        if (ret <= 0) {
            break;
        }
        get_val(cmdline, "txconf=", txconf);
        if (is_okey(original)) {            // tx_conf文件存在
            LoadFile(original, back_buf, sizeof(back_buf) - 1);
            if (!strstr(txconf, ";")) {      // env不存在
                SYSLOG("cp tx.conf bootargs\n");
                ret = uboot_p2pconf_set((char *)"txconf", back_buf);
            }
        } else {                            // tx_conf文件不存在
            if (strstr(txconf,";")) {       // env存在
                SYSLOG("cp bootargs tx.conf\n");
                WriteFile(original, txconf);
            }
        }
    } while(0);

    sync();

    return ret;
}

int tencent_get_conf_info(char *info_buf, int buf_size)
{
    TripleInfoS info = {0};

    tencent_load_triple_info(F_P2P_TRIPLE, &info);

    build_qrcode();

    snprintf(info_buf, buf_size, 
                "product_key=%s;device_name=%s;device_secret=%s;"
                "connect_status=%d;bmp_path=%s;"
                , info.product_key
                , info.device_name
                , info.device_secret
                , platform_on_line()
                , WEB_QRCODE_BMP_PATH);

    DBG("tencent get conf info:%s\n", info_buf);
    return 0;
}

int tencent_get_key_secret(char *pt_key, char* dev_name, char* dev_secret, char* pt_secret)
{
    TripleInfoS triple = {0};
    int ret = tencent_load_triple_info(F_P2P_TRIPLE ,&triple);
    DBG("ret = %d\n", ret);
    strcpy(pt_key    , triple.product_key);
    strcpy(dev_name  , triple.device_name);
    strcpy(dev_secret, triple.device_secret);
    DBG("pt_key = %s, dev_name = %s\n", pt_key, dev_name);
    return 0;
}

#endif
