/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : airlink.c
 * @Created Time : 2023-04-11
 * @Version      : 2.0
 * @Author       : hul
 * @Description  :
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <pthread.h>

#include "g_sys.h"
#include "g_stat.h"
#include "g_run.h"
#include "g_log.h"
#include "delay_exec.h"

#include "utils.h"
#include "confapi.h"
#include "jevent.h"
#include "jconfig.h"

#include "factory_db.h"
#include "net_check.h"
#include "net_qrcode.h"
#include "net_config.h"
#include "airlink.h"
#include "sim4g.h"
#include "jconfig.h"
#include "debug.h"
#include "encode_audio_queue.h"
#include "ble_services.h"
#include "cmdstat.h"
#include "conf_list.h"
#include "system_sch.h"
#include "system_ctrl.h"
#include "conf_nand.h"
#include "base64.h"

#include "url.h"

static sWIFIDev g_wifi_info = { // wifi内部全局变量
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .quality_flag = 1,
    .wifiscan_flag = 3,
};

static char *g_wifi_event[E_WMAX] = {
    "E_AP_STA",
    "E_STA",
    "E_WPA_RESET",
    "E_WPA_RESET_OOM",
    "E_DRV_RESET",
    "E_DRV_RESET_OOM",
    "E_LNX_RESET",
    "E_REFRESH",
    "E_WAITING",
};

enum {
    CMD_WIFI_CFG     = 1 << 0,
    CMD_ETH_CFG      = 1 << 1,
    CMD_UPDATING     = 1 << 2,
    CMD_CABLE_CHANGE = 1 << 3,
    CMD_CABLE_IN     = 1 << 4,
    CMD_CABLE_OUT    = 1 << 5,
};

static struct wifi_cfg raw = {0};
static struct wifi_cfg cfg = {0};
static struct wifi_cfg *g_raw_wifi = &raw;
static struct wifi_cfg *g_cfg_wifi = &cfg;

static void cb_wifi_cfg(int id, void *p_src, int size, void *ctx)
{
    CPY2CMDCFG(CMD_WIFI_CFG, &g_raw_wifi->wificfg, p_src, size);
}

static void cb_eth_cfg(int id, void *p_src, int size, void *ctx)
{
    CPY2CMDCFG(CMD_ETH_CFG, &g_raw_wifi->ethcfg, p_src, size);
}

static void cb_update(int id, void *p_src, int size, void *ctx)
{
    CPY2CMD(CMD_UPDATING);
}

static void cb_cable_in(int id, void *p_src, int size, void *ctx)
{
    // 只处理最新信号
    CPY2CMD(CMD_CABLE_CHANGE);
}

static void cb_cable_out(int id, void *p_src, int size, void *ctx)
{
    // 只处理最新信号
    CPY2CMD(CMD_CABLE_CHANGE);
}

static void diff_cfg2cmd(void * ctx)
{
    struct cmdstat *p_cmd = (struct cmdstat *)ctx;
    if (p_cmd->cmd_stage) {
        if (p_cmd->cmd_stage & CMD_WIFI_CFG) {
            memcpy(&g_cfg_wifi->wificfg, &g_raw_wifi->wificfg, sizeof(NetWifiS));
        }

        if (p_cmd->cmd_stage & CMD_ETH_CFG) {
            memcpy(&g_cfg_wifi->ethcfg, &g_raw_wifi->ethcfg, sizeof(NetEthS));
        }

        if (p_cmd->cmd_stage & CMD_CABLE_CHANGE) {
            cmd_set_command(p_cmd, net_link_status("eth0") == 1 ? CMD_CABLE_IN : CMD_CABLE_OUT);
        }
    }
}

// 防止接串口时，电气干扰导致设备重启
// 即便是接上网线，使用 wifi route，不响应网线的 in out 事件 dhcp，不打断策略执行
static int is_wifi_forced(void)
{
    return is_okey("/mnt/uwifi.txt");
}

static char *get_wifi_runinfo(void)
{
    static char buf[1024] = {0};

    snprintf(buf, sizeof(buf)-1, "\n"
        "www_alive   :%d  eth_up   :%d  curr_ev:%s\n"
        "www_fail_cnt:%d  chip_ever:%d  acc    :%d\n"
        "wpa_reset   :%d  ripe     :%d\n"
        "pwr_reset   :%d  quality  :%d\n"
        "lnx_reset   :%d  p2p_plat4m:%d  @%s\n",
        g_wifi_info.www_alive   , g_wifi_info.eth_up   , g_wifi_event[g_wifi_info.curr_ev],
        g_wifi_info.www_fail_cnt, g_wifi_info.chip_ever, g_wifi_info.acc_shift,
        g_wifi_info.wpa_reset   , g_wifi_info.ripe,
        g_wifi_info.drv_reset   , g_wifi_info.quality,
        g_wifi_info.lnx_reset   , platform_on_line(), get_timestr());
    return buf;
}

void syslogwifi(const char *file, int lineno)
{
    char *p = get_wifi_runinfo();
    AppendFile("/tmp/messages", p);
    printf("%s:%d %s\n", file, lineno, p);
}

int get_wifi_quality(void)
{
    return g_wifi_info.quality;
}

int get_wifi_hotspot_list(char *data)
{
    pthread_mutex_lock(&g_wifi_info.mutex);
    memcpy(data, g_wifi_info.wifiscan, sizeof(g_wifi_info.wifiscan));
    g_wifi_info.quality_flag = 1;
    g_wifi_info.wifiscan_flag = 1;
    pthread_mutex_unlock(&g_wifi_info.mutex);
    return 0;
}

int get_wifiexist(void)
{
    int length = 0;
    char shellcmd[128] = {0};
    char result[256] = {0};

    sprintf(shellcmd, "%s", "ifconfig -a | grep wlan0");
    length = ReadCmdResult(shellcmd, result, sizeof(result));
    if (length <= 0) {
        ERR("Length: %d <= 0\n", length);
        return FALSE;
    }

    return ((strstr(result, "wlan0") == NULL)? FALSE:TRUE);
}

char *get_ssid(const char *file, char ssid[])
{
    char buf[512] = {0};
    int nr = LoadFile(file, buf, sizeof(buf)-1);
    return_val_if_fail(nr > 0, NULL);
    char *p = strstr(buf, "ssid=");
    return_val_if_fail(p != NULL, NULL);
    p += strlen("ssid=."); // skip "
    nr = sscanf(p, "%[^\"]", ssid);
    return_val_if_fail(nr > 0, NULL);
    return ssid;
}

int wifi_reset_factory(void)
{
    SYSLOG("open anti-disturb manual, though a repeat of DELAY_CMD_DEFAULT\n");
    remove(SUPPLICANT_OK_CONF);
    return SUCCESS;
}

static int wifi_drv_ok(void)
{
    char modules[2048]  = {0};

    LoadFile("/proc/modules", modules, sizeof(modules)-1);

    if (strstr(modules, "plat_soc") && strstr(modules, "wifi_soc")) {
        return TRUE;
    } else if (strstr(modules, "ssv6x5x") || strstr(modules, "8188")) {
        return TRUE;
    }

    SYSLOG("wifi modules ko not exist\n");
    return FALSE;
}

static int wifi_chip_ok(void)
{
    char idVendor[8]  = {0};
    char idProduct[8] = {0};

    // wifi 模块探测不到
    if (LoadFile(USBDEV_IDVENDOR, idVendor, sizeof(idVendor)-1) <= 0 || 
       LoadFile(USBDEV_IDPRODUCT, idProduct, sizeof(idProduct)-1) <= 0) {
        SYSLOG("USBDEV_IDVENDOR or USBDEV_IDVENDOR not exist\n");
        g_wifi_info.chip_ever = FALSE;
    } else {
        g_wifi_info.chip_ever = TRUE;
    }

    return g_wifi_info.chip_ever;
}

static int wifi_start_ap(void)
{
    DBG("start ap\n");
    return UtilSystemCmd("wifi ap reset");
}

static int wifi_stop_ap(void)
{
    DBG("stop ap\n");
    return UtilSystemCmd("wifi ap off");
}

static int wifi_reset_supplicant(void)
{
    DBG("reset wpa_supplicant\n");
    return UtilSystemCmd("wifi sta reset");
}

static int wifi_start_udhcpc(void)
{
    DBG("wifi start udhcpc\n");
    return UtilSystemCmd("wifi udhcpc");
}

void reset_udhcpc(char nic)
{
    int ipid;
    char upid[16] = {0};
    if (LoadFile((nic == 'w') ? PID_OF_DHCP_WLAN : PID_OF_DHCP_ETH0, upid, sizeof(upid))) {
        if (1 == sscanf(upid, "%d", &ipid) && 0 == kill(ipid, 0)) {
            kill(ipid, SIGKILL);
        }
    }

    if (nic == 'w') {
        UtilSystemCmd2("udhcpc -i wlan0 -p %s -b -R -T2 -A3", PID_OF_DHCP_WLAN);
    } else {
        UtilSystemCmd2("udhcpc -i eth0  -p %s -b -R -T2 -A3", PID_OF_DHCP_ETH0);
    }
}

static void exec_ev_command(void *ctx)
{
    int cmd = cmd_get_command((struct cmdstat *)ctx);

    if (cmd & CMD_WIFI_CFG) {
        // 对码直接走 sta 模式
        if (g_cfg_wifi->wificfg.mode == WifiModeE_AP_STATION) {
            g_wifi_info.ripe = FALSE;          // 对码重新初始化
            g_wifi_info.curr_ev = E_STA;
            encode_audio_queue_push_amr(AUDIO_DI_DI, FALSE);
            encode_audio_queue_push_amr(AUDIO_RECV_PWD, FALSE);
            set_g_stat(wifi, WIFI_MATCHGOT);
            SYSLOG("WIFI_MATCHGOT\n");
        }
    }

    if (cmd & CMD_UPDATING) {
        g_wifi_info.updating = TRUE;
    }

    if (get_g_sys(factest)  || !(system_get_security() || is_okey(F_P2P_TRIPLE))) {
        SYSLOG("it is factory test or devid is null and ALI_CONF_PATH is not exist\n");    // 产测模式下不响应网线拔插
        return;
    }

    if ((cmd & CMD_CABLE_OUT) && is_okey("/sys/class/net/eth0/statistics/tx_bytes")) {
        if (g_wifi_info.eth_up == FALSE) {
            SYSLOG("ignore inital event %s\n", "CABLE_OUT");
            return;
        }

        SYSLOG("____ eth0 breaking, wifi %s\n", g_wifi_info.ripe?"ripe":"raw");
        g_wifi_info.eth_up = FALSE;
        // 无串口时的有线调试模式
        if (is_wifi_forced()) {
            SYSLOG("uwifi.txt, keep ROUTE TABLE though %s\n", "CABLE_IN");
            return;
        }

        if (!get_g_stat(wifi, WIFI_STA)) {
            DBG("not compare eth0 and wlan0 network segments\n");
            return;
        }

        reset_udhcpc('w');
        UtilSystemCmd("route -n");
    }

    if ((cmd & CMD_CABLE_IN) && is_okey("/sys/class/net/eth0/statistics/tx_bytes")) {
        if (g_wifi_info.eth_up == TRUE) {
            DBG("ignore inital event %s\n", "CABLE_IN");
            return;
        }

        SYSLOG("____ eth0 connecting\n");
        g_wifi_info.eth_up = TRUE;

        // 无串口时的有线调试模式
        if (is_wifi_forced()) {
            SYSLOG("uwifi.txt, keep ROUTE TABLE though %s\n", "CABLE_IN");
            return;
        }

        UtilSystemCmd("ifconfig wlan0 0.0.0.0");
        UtilSystemCmd("/ipc/bin/networking init; route -n");
    }
}

const char *supplicant_conf(void)
{
    if (is_okey(FACTORY_SSID_FILE) || get_g_sys(factest)) {
        return TMP_SUPPLICANT_CONF;
    } else if (is_okey(SUPPLICANT_OK_CONF)) {
        return SUPPLICANT_OK_CONF;
    }

    return TMP_SUPPLICANT_CONF;
}

/*
 * ssid.txt 配网优先,存在配网文件则跳过
 * config.xml 中 wifi 配置更新，从 xml 中获取账号、密码
 */
static int wifi_modify_supplicant_conf(NetWifiS *info)
{
    int  len = 0;
    char sbuf[256] = {0};
    char supp_conf[256] = {0};

    if (is_okey(FACTORY_SSID_FILE) && !is_okey(supplicant_conf())) {
        CopyFile(TMP_SUPPLICANT_CONF, FACTORY_SSID_FILE);
        LoadFile(TMP_SUPPLICANT_CONF, sbuf, sizeof(sbuf)-1);
        SYSLOG("use FACT ssid %s\n", sbuf);
        return 0;
    }

    if (get_g_stat(wifi, WIFI_MATCHGOT)) {
        if (0 == strlen(info->weppasswd)) {
            len = snprintf(supp_conf, sizeof(supp_conf) - 1,
                "network={\n"
                "    ssid=\"%s\"\n"
                "    scan_ssid=1\n"
                "    key_mgmt=NONE\n}\n", info->ssid);
        } else {
            len = snprintf(supp_conf, sizeof(supp_conf) - 1,
                "network={\n"
                "    ssid=\"%s\"\n"
                "    key_mgmt=WPA-PSK WPA-PSK-SHA256\n"
                "    scan_ssid=1\n"
                "    ieee80211w=1\n"
                "    psk=\"%s\"\n}\n", info->ssid, info->weppasswd);
        }

        WriteFile(TMP_SUPPLICANT_CONF, supp_conf);
        SYSLOG("%d bytes writed to %s, ssid:%s passwd:%s\n", len, TMP_SUPPLICANT_CONF,
            info->ssid, (strlen(info->weppasswd)==0) ? "NONE" : info->weppasswd);
    }

    return 0;
}

//老式五位加密方式 WEP 的配置文件
static void wifi_write_wep_conf(const char *ssid, const char *passwd)
{
    char supp_conf[256] = {0};
    int nr_passwd = strlen(passwd);
    int len = 0;

    if (nr_passwd == 5 || nr_passwd == 13 || nr_passwd == 16) {
        len = snprintf(supp_conf, sizeof(supp_conf) - 1,
            "network={\n"
            "    ssid=\"%s\"\n"
            "    scan_ssid=1\n"
            "    key_mgmt=NONE\n"
            "    wep_key0=\"%s\"\n"
            "    wep_tx_keyidx=0\n"
            "}\n", ssid, passwd);
    } else {
        len = snprintf(supp_conf, sizeof(supp_conf) - 1,
            "network={\n"
            "    ssid=\"%s\"\n"
            "    scan_ssid=1\n"
            "    key_mgmt=NONE\n"
            "    wep_key0=%s\n"
            "    wep_tx_keyidx=0\n"
            "}\n", ssid, passwd);
    }

    WriteFile(TMP_SUPPLICANT_CONF, supp_conf);
    SYSLOG("WEP: %d bytes writed to %s, %s@%s\n", len, TMP_SUPPLICANT_CONF, ssid, passwd);
    return ;
}

static int wifi_uboot_conf_repair(void)
{
    return 0;
#if 0
    int ret = 0;
    int len = 0;
    int file_len = 0;
    char sbuf[256] = {0};
    char base64_env[512] = {0};
    char base64_file[512] = {0};
    const char *pconf = SUPPLICANT_OK_CONF;
    AliTripleBackupType backup_type = STAY_TRIPLE;

    // 配网文件信息 base64 校验
    int file_ok = is_okey(pconf);
    if (file_ok) {
        LoadFile(pconf, sbuf, sizeof(sbuf));
        base64encode(base64_file, &file_len, sbuf, strlen(sbuf));
        DBG("base64 file:%s file_len:%d\n", base64_file, file_len);
    }

    int env_ok = uboot_ssid_get(base64_env, sizeof(base64_env));
    if (env_ok) {
        memset(sbuf, 0, sizeof(sbuf));
        base64decode(sbuf, &len, base64_env, sizeof(base64_env));
        DBG("sbuf:\n%s len:%d\n", sbuf, len);
    }

    do {
        if (file_ok && !env_ok) {
            backup_type = BACKUP_TRIPLE;
        } else if (!file_ok && env_ok) {
            backup_type = ORIGINAL_TRIPLE;
        } else if (file_ok && env_ok) {
            // base64 编码及长度不一致，从 SUPPLICANT_OK_CONF 同步
            if (strncmp(base64_file, base64_env, file_len) != 0) {
                DBG("file:%s\n", base64_file);
                DBG(" env:%s\n", base64_env);
                backup_type = BACKUP_TRIPLE;
            }
        } else {
            break;
        }

        switch(backup_type) {
        case ORIGINAL_TRIPLE:
            SYSLOG("cp bootargs supplicantOK.conf\n");
            DumpFile(pconf, sbuf, sizeof(sbuf));
            break;

        case BACKUP_TRIPLE:
            SYSLOG("cp supplicantOK.conf bootargs\n");
            //ret = uboot_ssid_set(base64_file);
            break;

        default:
            DBG("Both supplicantOK.conf and bootargs are good\n");
            break;
        }
    } while(0);

    return ret;
#endif
}

/*
 * WEP 检测，需要重写 TMP_SUPPLICANT_CONF
 * SD卡配网不检查 WEP 加密
 **/
static void wifi_wpa_scan(const char *ssid, const char *passwd)
{
    int retry         = 0;
    int scan_ok       = FALSE;
    char cmdbuf[256]  = {0};
    char result[256]  = {0};
    char cmdline[128] = {0}; 

    if (5 == strlen(passwd)) {
        goto __WEP;
    }

    clr_g_stat(wifi, WIFI_WEP);
    wifi_stop_ap();
    wifi_reset_supplicant();
    ms_sleep(1000);
    if (get_g_sys(factest) || is_okey(FACTORY_SSID_FILE)) {
        return;
    }

    sprintf(cmdbuf, "wpa_cli -p %s -iwlan0 scan_result | grep -w %s | sed -e 's/].*//g' -e 's/^.*\\[//g' ",
            WLAN0_SUPPLICAN, ssid);

    do {
        UtilSystemCmd2("wpa_cli -p %s -iwlan0 scan", WLAN0_SUPPLICAN);
        ms_sleep(1000);
        ReadCmdResult(cmdbuf, result, sizeof(result)-1);
        if (strlen(result) > 0) {
            scan_ok = TRUE;
            break;
        }
    } while(++retry < 5);

    if (!scan_ok) {
        SYSLOG("scan ssid: %s passwd: %s\n", ssid, passwd);
        goto __exit;
    }

    // 老式五位加密方式 WEP，如今国内大多数路由器已经弃用，国外可能还在使用
    if (strstr(result, "WEP")) {
__WEP:
        SYSLOG("WEP mode\n");
        wifi_write_wep_conf(ssid, passwd);
        set_g_stat(wifi, WIFI_WEP);
        wifi_reset_supplicant();
    }

__exit:
    if (is_okey(TMP_SUPPLICANT_CONF)) {
        snprintf(cmdline, sizeof(cmdline)-1, "cat %s" , TMP_SUPPLICANT_CONF);
    } else if (is_okey(SUPPLICANT_OK_CONF)) {
        snprintf(cmdline, sizeof(cmdline)-1, "cat %s" , SUPPLICANT_OK_CONF);
    }
    UtilSystemCmd(cmdline);
    return ;
}

int get_wifi_status(void)
{
    int result = 0;
    char buffer[512]   = {0};
    char shellcmd[128] = {0};
    char statusbuf[64] = {0};

    do {
        sprintf(shellcmd, "wpa_cli -p %s -iwlan0 status", WLAN0_SUPPLICAN);
        ReadCmdResult(shellcmd, buffer, sizeof(buffer));

        if (get_val(buffer, "wpa_state=", statusbuf) <= 0) {
            result = FAILURE;
            break;
        }

        if (strncmp(statusbuf, "INTERFACE_DISABLED", strlen("INTERFACE_DISABLED")) == 0) {
            result = INTERFACE_DISABLED;
        } else if (strncmp(statusbuf, "COMPLETED", strlen("COMPLETED")) == 0) {
            result = COMPLETED;
        } else if (strncmp(statusbuf, "4WAY_HANDSHAKE", strlen("4WAY_HANDSHAKE")) == 0) {
            result = _4WAY_HANDSHAKE;
        } else if (strncmp(statusbuf, "SCANNING", strlen("SCANNING")) == 0) {
            result = SCANNING;
        } else if (strncmp(statusbuf, "DISCONNECTED", strlen("DISCONNECTED")) == 0) {
            result = DISCONNECTED;
        } else if (strncmp(statusbuf, "ASSOCIATING", strlen("ASSOCIATING")) == 0) {
            result = ASSOCIATING;
        }
    } while(0);

    return result;
}

//_VLD_WEAKSIGNAL状态有缺陷
static int validate_passwd_and_signal(void)
{
    int try = 0;
    int stat = 0;
    int handshake = 0;
    eVLDStat result = _VLD_WEAKSIGNAL;

    do {
        stat = get_wifi_status();
        if (stat < 0) {
            DBG("wpa_supplicant start fail\n");
            result = _VLD_WEAKSIGNAL;
            break;
        }

        if (COMPLETED == stat) {
            result = _VLD_COMPLTED;
            break;
        }

        if (stat == _4WAY_HANDSHAKE) {
            handshake++;
        }

        if (try >= 8) {
            if (handshake > 1 && stat != _4WAY_HANDSHAKE) {
                SYSLOG("_VLD_ERRPASSWD\n");
                result = _VLD_ERRPASSWD;
                break;
            } else {
                DBG("(ASSO=1,DISC,SCAN,_4WAY)[%d] handshake_cnt=%d and try@%d\n", stat, handshake, try);
            }
        }
        ms_sleep(1000);
    } while(++try<=35);  // 35 senconds

    return result;
}

/*
 * 1. 用户中途修改已连接的路由器密码，此时需要复位后重新配网。
 * 2. 绑定成功后，切换 wifi，但是密码错误，重新以 SUPPLICANT_OK_CONF 自动配网
 * 3. 一切出现 wifi 密码错误的情况均会走 AP_STA 模式，重新配网。
 */
static int wifi_start_sta(void)
{
    int stat = _VLD_COMPLTED;

    wifi_modify_supplicant_conf(&g_cfg_wifi->wificfg);
    wifi_wpa_scan(g_cfg_wifi->wificfg.ssid, g_cfg_wifi->wificfg.weppasswd);
    switch (stat = validate_passwd_and_signal()) {  // stat出现_VLD_WEAKSIGNAL处理方式
    case _VLD_COMPLTED:
        // 联网成功后保存配置，产测下 TF 卡配网不保存配置
        if (!get_g_sys(factest) && is_okey(TMP_SUPPLICANT_CONF)) {
            CopyFile(SUPPLICANT_OK_CONF, TMP_SUPPLICANT_CONF);
            wifi_uboot_conf_repair();
        }
        wifi_start_udhcpc();
        stop_qrcode_server();
        clr_g_stat(wifi, WIFI_PASSWDERR);
        SYSLOG("wifi sta succ\n");
        break;
    case _VLD_ERRPASSWD:        // 密码错误
    case _VLD_WEAKSIGNAL:       // 网络信号差
        if (get_g_stat(wifi, WIFI_MATCHGOT)) {
            /*
             * 对码出错的三种情况:
             * 1. 首次配网，TMP_SUPPLICANT_CONF 配置出错，无需复位设备可重新手动配网
             * 2. 已配网设备，上电后 SUPPLICANT_OK_CONF 配置出错，需复位设备后重新配网
             * 3. 切换 wifi，TMP_SUPPLICANT_CONF 配置出错，需复位设备后重新配网
             */ 
            if (g_wifi_info.eth_up != TRUE) {
                if (stat == _VLD_ERRPASSWD) {
                    encode_audio_queue_push_amr(AUDIO_PASSWORD_ERR, FALSE);
                    encode_audio_queue_push_amr(AUDIO_PASSWORD_ERR, FALSE);
                } else {
                    encode_audio_queue_push_amr(AUDIO_CON_FAIL, FALSE);
                    encode_audio_queue_push_amr(AUDIO_CON_FAIL, FALSE);
                }
            }

            if (!is_okey(SUPPLICANT_OK_CONF)) {
                clr_g_stat(wifi, WIFI_STA);
                remove(TMP_SUPPLICANT_CONF);
                wifi_start_ap();
                start_qrcode_server();
                g_cfg_wifi->wificfg.mode = WifiModeE_AP;
                conf_set_wificfg(g_cfg_wifi->wificfg);
            }
        }

        if (stat ==_VLD_ERRPASSWD) {
            set_g_stat(wifi, WIFI_PASSWDERR);
            SYSLOG("wifi passwd error\n");
        } else {
            SYSLOG("wifi connection fail\n");
        }
        break;
    default:
        break;
    }

    return stat;
}

static int dbm2pct(int dbm) /* always negtive int */
{
    int level = abs(dbm);

    if (level == 0) {
        level = 0;
    } else if (level <= 50) {
        level = 100;
    } else {
        level = 100 - (level - 50)*2;
    }
    return abs(level);
}

static int wifi_get_wireless()
{
    char shellcmd[256] = {0};
    char level[64] = {0};
    memset(shellcmd,0,sizeof(shellcmd));
    sprintf(shellcmd, "cat /proc/net/wireless | grep -E wlan0 | awk '{print $4}'");
    if (ReadCmdResult(shellcmd, level, sizeof(level)) < 0) {
        return 0;
    }

    int value = dbm2pct(atoi(level));
    DBG("level:%s, value:%d\n", level, value);
    return value;
}

static void get_wifiquality_process(void)
{
    char ssid[64] = {0};
    char result[256] = {0};
    char shellcmd[256] = {0};

    if (!is_okey(supplicant_conf()) || g_wifi_info.quality_flag == 0) {
        return;
    }

    g_wifi_info.quality_flag = 0;
    if (NULL == get_ssid(supplicant_conf(), ssid)) {
        SYSLOG("get ssid from %s fail\n", supplicant_conf());
        return;
    }

    //level
    sprintf(shellcmd, "iwlist wlan0 scanning | grep -B 3 \"%s\" | awk -F'=' '/Signal level/{print $3}'", ssid);
    ReadCmdResult(shellcmd, result, sizeof(result));

    dbg_wifi("Signal level:%s\n", result);

    g_wifi_info.quality = dbm2pct(atoi(result));
    if (g_wifi_info.quality == 0) {   // 防止 iwlist 失效导致 wifi 信号质量获取失败
        g_wifi_info.quality = wifi_get_wireless();
    }

    DBG("ssid:%s scan@%lld s, quality:%d\n", ssid, time(NULL)%60, g_wifi_info.quality);
    return;
}

static int wifi_check_hostapd_dhcp(void)
{
    char cmdline[64] = {0,};

    ReadCmdResult("ps | awk '/[h]ostapd/{print $5}'", cmdline, sizeof(cmdline)-1);
    if (NULL == strstr(cmdline, "hostapd")) {
        DBG("cmdline = %s\n", cmdline);
        return FALSE;
    }

    ReadCmdResult("ps | awk '/[u]dhcpd/{print $5}'", cmdline, sizeof(cmdline)-1);
    if (NULL == strstr(cmdline, "udhcpd")) {
        DBG("cmdline = %s\n", cmdline);
        return FALSE;
    }

    return TRUE;
}

/*
 * 采用shell命令去解析wlan0的IP，而不是采用net_get_ipaddr()去解析获得wlan0的IP，
 * 是考虑到wlan0 实际存在，但又是处于down掉状态的情况。此时通讯是有问题的，应该要走相应的硬复位或者飞行模式逻辑。
 */
static int wifi_get_wlan0_ip(char ip[])
{
    char *ptr = NULL;
    char cmdbuf[64] = {0};
    char result[64] = {0};

    sprintf(cmdbuf, "%s", "ifconfig | grep -r wlan0 -A 1 | grep -r inet | awk '{print $2}'");
    ReadCmdResult(cmdbuf,result, sizeof(result));

    if (strlen(result) == 0) {
        ERR("result is NULL\n");
    } else {
        ptr = strstr(result, "addr:");
        if (ptr == NULL) {
           ERR("ptr is NULL\n");
        } else {
            ptr = ptr + 5;
            sscanf(ptr, "%s", ip);
            return SUCCESS;
        }
    }
    return FAILURE;
}

static void wifi_refresh()
{
    char router[16] = {0};
    static int www_alive_prev = FALSE;
    const char *tag = "UNKNOW";

    if (!g_wifi_info.initialized) {
        // 首次启动，tid 到 syslog，快速检查 chip driver
        SYSLOG("wifi: thread %d initialized\n", (int)gettid());
        g_wifi_info.initialized = TRUE;
        if (wifi_chip_ok() != TRUE) {
            tag = "chip"; goto __w_lnx_reset;
        }

        if (wifi_drv_ok() != TRUE) {
            tag = "driver"; goto __w_drv_reset;
        }
    }

    // WWW 检查, ping TENCENT
    g_wifi_info.www_alive = platform_on_line() || www_reachable();

    if (g_wifi_info.www_alive) {
        g_wifi_info.www_fail_cnt = 0;
        if (!platform_on_line()) {
            DBG("platform offline but www_alive @cnt: %d\n", g_wifi_info.www_fail_cnt);
        }

        if (g_wifi_info.www_alive != www_alive_prev) {
            SYSLOG("wifi clean flag, www_alive=%d, www_alive_prev=%d\n",g_wifi_info.www_alive, www_alive_prev);
            g_wifi_info.wpa_reset = g_wifi_info.drv_reset = g_wifi_info.lnx_reset = 0;
            remove(WIFI_LNX_REBOOT);
        }
    } else {
        // wlan0 检查，获取 IP
        char self[16] = {0};
        if (SUCCESS != wifi_get_wlan0_ip(self)) {
            tag = "ipaddr_wlan0"; goto __w_wpa_reset;
        } else {
            tag = "wlan0_route"; goto __w_dhcp_reset;
        }
    }

    www_alive_prev = TRUE;
    g_wifi_info.ms_step_www = 10*1000;
    return;

__w_dhcp_reset:
    // ping gw 不通 -> 进行 wpa 修复
    // ping gw 成功 -> 可以快速 dhcp
    LoadFile("/tmp/wlan_gw", router, sizeof(router));
    if (!is_alive_ip(router, __func__)) {
        tag = "wlan_gw"; goto __w_wpa_reset;
    }

    // 3rd 失败 -> 可以快速 dhcp
    if (is_inc_modc(g_wifi_info.www_fail_cnt, 3)) {
        reset_udhcpc('w');
        g_wifi_info.ms_step_www = 10*1000;
        DBG("err_tag: %s %s\n", tag, get_wifi_runinfo());
        return;
    }

    if (g_wifi_info.www_fail_cnt <= 3) {
        dbg_wifi("www_fail_cnt: %d\n", g_wifi_info.www_fail_cnt);
        return;
    }

__w_wpa_reset:
    // wpa 修复失败一次，再播报网络连接失败语音
    if (g_wifi_info.wpa_reset >= 1) {
        play_conditionally(AUDIO_CON_FAIL);
        play_conditionally(AUDIO_CON_FAIL);
    }

    www_alive_prev = FALSE;
    g_wifi_info.prev_ev = g_wifi_info.curr_ev;
    g_wifi_info.curr_ev = E_WPA_RESET;
    UtilSystemCmd("route -n; ifconfig wlan0");
    if (wifi_chip_ok() != TRUE) {
        tag = "chip"; goto __w_lnx_reset;
    }

    if (wifi_drv_ok() != TRUE) {
        tag = "driver"; goto __w_drv_reset;
    }

    SYSLOG("__w_wpa_rest from line: %s\n", tag);
    syslogwifi(__FILE__, __LINE__);
    return;
__w_drv_reset:
    // wifi 驱动不存在
    www_alive_prev = FALSE;
    g_wifi_info.prev_ev = g_wifi_info.curr_ev;
    g_wifi_info.curr_ev = E_DRV_RESET;
    syslogwifi(__FILE__, __LINE__);
    return;
__w_lnx_reset:
    www_alive_prev = FALSE;
    g_wifi_info.prev_ev = g_wifi_info.curr_ev;
    g_wifi_info.curr_ev = EVENT_LNX_RESET;
    syslogwifi(__FILE__, __LINE__);
    return;
}

/*
 * 1. 软重启路径: wpa_reset -> drv_reset -> lnx_reset，www 连接失败时执行，每个重启达 4 次，时向下个演进
 * 2. 硬重启路径: drv_reset -> lnx_reset, 适用于 WIFI 模块和驱动检测不到时
 * 3. 全系统重启: lnx_reset, 最多只重启 4 次，保存在 /opt/conf/wifi/lnx_reset, 次数超过时会转回到 drv_reset
 * 4. 时间间隔  : 所有 xxx_reset 都是 2 分钟，但系统第一次异常检测后的 xxx_reset 会立即执行。
 * 5. 系统修复  : www 连接成功，所有标志位，都会清 0
 */
static void wifi_loop(void *ctx)
{
    int stat = _VLD_COMPLTED;
    static int tick = 0;
    static int time_tick = 0;
    static int sec_left = 0;

    // 事件响应，wifi列表，信号质量，对码实时性高，使用 1s 定时
    exec_ev_command(ctx);

    // iwlist 工具会发送空口包，所以只在外部调用后更新一次，上电未获取成功一直获取
    if (g_wifi_info.quality == 0) {
		g_wifi_info.quality_flag = 1;
    	get_wifiquality_process();
	}

    if (g_wifi_info.wifiscan_flag) {
        g_wifi_info.wifiscan_flag--;
        UtilSystemCmd("wifi scan_list");
        if (is_okey("/tmp/wlist.yammy")) {
            LoadFile("/tmp/wlist.yammy", g_wifi_info.wifiscan, sizeof(g_wifi_info.wifiscan)-1);
        }
        dbg_wifi("wifiscanbuf:%s\n", g_wifi_info.wifiscan);
    }

    if (!get_g_sys(testing) && is_okey("/tmp/wlist.yammy")) {
        remove("/tmp/wlist.out");
        remove("/tmp/wlist.ripe");
        remove("/tmp/wlist.tmp");
        remove("/tmp/wlist.yammy");
    }

    if (!is_inc_mod0(time_tick, 5) && g_wifi_info.ripe) {
        return;
    }

    // 摇头机产测且配有 ssid.txt 的情况下，由 auto_run.sh 进行配网，加快上线速度
    //if (get_g_sys(factest) && is_okey(FACTORY_SSID_FILE)) {
    //    DBG("ytj factest return\n");
    //    return ;
    //}

    if (g_wifi_info.updating) {
        DBG("reseting or updating, exit wifi process\n");
        return;
    }

    if (get_g_run(wifi, RUN_PAUSE_WIFI)) {
        dbg_wifi("pause wifi loop\n");
        return;
    }

    // 信息打印
    if (is_inc_mod0(tick, 3)) {
        dbg_wifi("%s", get_wifi_runinfo());
    } else if (get_g_run(wifi, RUN_PRINT_WIFI)) {
        if (is_okey(WIFI_ACC_SHIFT)) {
            // 动态加载一次
            char nr[8] = {0};
            LoadFile(WIFI_ACC_SHIFT, nr, sizeof(nr)-1);
            g_wifi_info.acc_shift = MAX(0, MIN(2,atoi(nr)));    // 0,1,2  1X 2X 4X 时间
        }
        clr_g_run(wifi, RUN_PRINT_WIFI);
        syslogwifi(__FILE__, __LINE__);
    }

    // 非测试模式下，接有线时启动 WIFI，但不跑策略，可快速响应网线插拔事件
    if (!get_g_sys(factest) && g_wifi_info.ripe && (g_wifi_info.eth_up == TRUE)) {
        dbg_wifi("stopped @%s\n", "eth_up");
        return ;
    }

    int acc = g_wifi_info.acc_shift;

    /**
     *  STA  AP
     *   1   1  AP+STA 模式
     *   1   0  STA 模式
     */
    switch(g_wifi_info.curr_ev) {
    case E_AP_STA:  //初始状态
        if (is_okey(FACTORY_SSID_FILE) || is_okey(SUPPLICANT_OK_CONF)) {
            g_wifi_info.curr_ev = E_STA;
        } else {
            if (g_wifi_info.eth_up != TRUE) {
                if ((TRUE != wifi_chip_ok() || TRUE != wifi_drv_ok())) {
                }
                clr_g_stat(wifi, WIFI_STA);
            }
        }
        break;
    case E_STA:
        DBG("FSM @%s\n", g_wifi_event[g_wifi_info.curr_ev]);
        set_g_stat(wifi, WIFI_STA);
        g_wifi_info.prev_ev = g_wifi_info.curr_ev;
        g_wifi_info.www_fail_cnt = 0;
        stat = wifi_start_sta();
        if (_VLD_ERRPASSWD == stat && !is_okey(SUPPLICANT_OK_CONF)) {
            g_wifi_info.curr_ev = E_AP_STA;
        } else {
            g_wifi_info.curr_ev = E_REFRESH;
        }

        g_wifi_info.ripe = TRUE;
        clr_g_stat(wifi, WIFI_MATCHGOT);
        break;
    case E_REFRESH:
        if (ms_clock_is_timeup2(&g_wifi_info.ms_clock_wpa, g_wifi_info.ms_step_www, &sec_left)) {
            dbg_wifi("FSM @%s\n", g_wifi_event[g_wifi_info.curr_ev]);
            // 事件发生器
            wifi_refresh();
        } else {
            dbg_wifi("FSM @%s left %d secs\n", g_wifi_event[g_wifi_info.curr_ev], sec_left);
        }
        break;
    case E_WPA_RESET:
        if (ms_clock_is_timeup2(&g_wifi_info.ms_clock_wpa, g_wifi_info.ms_step_wpa>>acc, &sec_left)) {
            if (is_inc_modc(g_wifi_info.wpa_reset, 4)) {
                DBG("____\n");
                g_wifi_info.curr_ev = E_WPA_RESET_OOM;
                g_wifi_info.ms_step_wpa = 120*1000;
                break;
            }
            SYSLOG("wifi: @%s from %s\n", g_wifi_event[g_wifi_info.curr_ev], g_wifi_event[g_wifi_info.prev_ev]);
            UtilSystemCmd("wifi sta reset");
            g_wifi_info.prev_ev = g_wifi_info.curr_ev;
            g_wifi_info.curr_ev = E_WAITING;
            g_wifi_info.next_ev = E_STA;
            g_wifi_info.ms_step_wpa  = 120*1000;
            g_wifi_info.ms_step_wait = 10*1000;
            ms_clock_reset(&g_wifi_info.ms_clock_wait);
        } else {
            DBG("FSM @%s left %d secs\n", g_wifi_event[g_wifi_info.curr_ev], sec_left);
        }
        break;
    case E_WPA_RESET_OOM:
    case E_DRV_RESET:
        if (ms_clock_is_timeup2(&g_wifi_info.ms_clock_drv, g_wifi_info.ms_step_drv>>acc, &sec_left)) {
            if (is_inc_modc(g_wifi_info.drv_reset, 4)) {
                DBG("____\n");
                g_wifi_info.curr_ev = E_DRV_RESET_OOM;
                break;
            }
            SYSLOG("wifi: @%s\n", g_wifi_event[g_wifi_info.curr_ev]);
            // wifi 驱动异常走 drv_reset 修复
            UtilSystemCmd("wifi drv reset");
            g_wifi_info.prev_ev = g_wifi_info.curr_ev;
            g_wifi_info.curr_ev = E_WAITING;
            g_wifi_info.next_ev = E_STA;
            g_wifi_info.ms_step_drv = 120*1000;
            g_wifi_info.ms_step_wait = 10*1000;
            ms_clock_reset(&g_wifi_info.ms_clock_wait);
        } else {
            DBG("FSM @%s left %d sec\n", g_wifi_event[g_wifi_info.curr_ev], sec_left);
        }
        break;
    case E_DRV_RESET_OOM:
    case E_LNX_RESET:
        if (g_wifi_info.lnx_reset < 4) {
            ++g_wifi_info.lnx_reset;
            char nr[8] = {0};
            SYSLOG("FSM @%s\n", g_wifi_event[g_wifi_info.curr_ev]);
            DumpFile(WIFI_LNX_REBOOT, itoa10(MIN(4,g_wifi_info.lnx_reset), nr), strlen(nr));
            DELAY_REBOOT_LINUX();
            g_wifi_info.prev_ev = g_wifi_info.curr_ev;
            g_wifi_info.curr_ev = E_WAITING;
            g_wifi_info.next_ev = E_STA;
            g_wifi_info.ms_step_wait = 10*1000;
        } else {
            DBG("FSM @W_EVENT_LNX_OOM, go back to W_EVENT_DRV_RESET\n");
            g_wifi_info.curr_ev = E_DRV_RESET;
        }
        break;
    case E_WAITING:
        if (ms_clock_is_timeup2(&g_wifi_info.ms_clock_wait, g_wifi_info.ms_step_wait, &sec_left)) {
            g_wifi_info.curr_ev = g_wifi_info.next_ev;
            DBG("FSM @waiting %s over go %s\n",  g_wifi_event[g_wifi_info.prev_ev], g_wifi_event[g_wifi_info.next_ev]);
        } else {
            DBG("FSM @waiting %s left %d sec\n", g_wifi_event[g_wifi_info.prev_ev], sec_left);
        }
        break;
    default:
        dbg_wifi("FSM @%s\n", g_wifi_event[g_wifi_info.curr_ev]);
        break;
    }

    return;
}

int wifi_startup(void)
{
    if (!get_g_sys(usb_wifi)) {
        DBG("wifi module not detected \n");
        return 0;
    }

    SYSLOG("init wifi...\n");
    static struct cmdstat cmdstat_wifi;
    struct cmdstat *ctx = &cmdstat_wifi;
    cmdstat_wifi.diff_cfg2cmd = diff_cfg2cmd;

    ble_services_start();

    if (is_okey(WIFI_LNX_REBOOT)) {
        char nr[8] = {0};
        LoadFile(WIFI_LNX_REBOOT, nr, sizeof(nr)-1);
        g_wifi_info.lnx_reset = MAX(0, atoi(nr));           // 防止负数
    }

    if (is_okey(WIFI_ACC_SHIFT)) {
        char nr[8] = {0};
        LoadFile(WIFI_ACC_SHIFT, nr, sizeof(nr)-1);
        g_wifi_info.acc_shift = MAX(0, MIN(2,atoi(nr)));    // 0,1,2  1X 2X 4X 时间
    }

    g_wifi_info.sch = js_create_scheduler((char *)__func__);

    g_wifi_info.curr_ev = E_AP_STA;
    g_wifi_info.eth_up = net_link_status("eth0");
    ms_clock_reset(&g_wifi_info.ms_clock_wpa);
    ms_clock_reset(&g_wifi_info.ms_clock_drv);
    ms_clock_reset(&g_wifi_info.ms_clock_wait);
    g_wifi_info.ms_step_wpa = 0;       // 第一次允许立即执行
    g_wifi_info.ms_step_drv = 0;       // 第一次允许立即执行

    DevConfS devconf = {0};
    conf_get_devconf_cfg(&devconf);
    conf_get_ethcfg(&g_cfg_wifi->ethcfg);
    conf_get_wificfg(&g_cfg_wifi->wificfg);

    // 设备已配网且绑定，uboot 备份 SUPPLICANT_OK_CONF 检查
    if (devconf.devicebind && g_cfg_wifi->wificfg.mode == WifiModeE_AP_STATION) {
        wifi_uboot_conf_repair();
    }

    if ((!(get_g_sys(factest) && is_okey(FACTORY_SSID_FILE))) && (!is_okey(SUPPLICANT_OK_CONF))) {
        wifi_start_ap();
        SYSLOG("start ap mode\n");
    }

    DBG("eth_up = %d\n", g_wifi_info.eth_up);
    g_wifi_info.p_ctx = ctx;

    attach_config(JEvent_WifiCfgChg           , cb_wifi_cfg , (void *)ctx);
    attach_config(JEvent_EthcfgChg            , cb_eth_cfg  , (void *)ctx);
    attach_config (JEvent_UpdateBegin         , cb_update   , (void *)ctx);
    attach_event_async(JEvent_AlarmCabDis     , cb_cable_out, (void *)ctx);
    attach_event_async(JEvent_AlarmCableNormal, cb_cable_in , (void *)ctx);

    js_create_timer_r(g_wifi_info.sch, 500, 1000, (JSTCFunc)wifi_loop, (void *)ctx, &g_wifi_info.hdl);

    return 0;
}

int wifi_stop()
{
    if (!get_g_sys(usb_wifi)) {
        return 0;
    }

    DBG("wifi_stop...\n");

    ble_services_stop();

    if (g_wifi_info.hdl != NULL) {
        js_delete_timer_r(&g_wifi_info.hdl);
    }

    if (g_wifi_info.sch != NULL) {
        js_delete_scheduler(g_wifi_info.sch);
        g_wifi_info.sch = NULL;
    }

    detach_config(JEvent_WifiCfgChg     , cb_wifi_cfg , g_wifi_info.p_ctx);
    detach_config(JEvent_EthcfgChg      , cb_eth_cfg  , g_wifi_info.p_ctx);
    detach_config(JEvent_UpdateBegin    , cb_update   , g_wifi_info.p_ctx);
    detach_event(JEvent_AlarmCabDis     , cb_cable_out, g_wifi_info.p_ctx);
    detach_event(JEvent_AlarmCableNormal, cb_cable_in , g_wifi_info.p_ctx);

    UtilSystemCmd("wifi ap off; wifi sta off; wifi drv off");
    DropCache(__func__);
    CompactMemo(__func__);
    usleep(1000*1000);

    return 0;
}

