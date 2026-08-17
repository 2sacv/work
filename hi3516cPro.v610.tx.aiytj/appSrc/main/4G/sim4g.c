/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : sim4g.c
 * @Created Time : 2023-3-2
 * @Version      : 3.0
 * @Author       : hul
 * @Description  :
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <errno.h>
#include <time.h>
#include <openssl/crypto.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <time.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <math.h>
#include "io.h"
#include "debug.h"
#include "utils.h"
#include "jevent.h"
#include "confapi.h"
#include "jconfig.h"
#include "airlink.h"
#include "conf_list.h"
#include "net_check.h"
#include "net_config.h"
#include "delay_exec.h"
#include "system_sch.h"
#include "system_ctrl.h"
#include "jconfstruct.h"
#include "js_scheduler.h"
#include "encode_audio_queue.h"
#include "cmdstat.h"
#include "factory_db.h"
#include "aging8h.h"

#include "sim4g.h"
#include "sim4g_800e.h"
#include "sim4g_nt26.h"
#include "sim4g_yuga.h"
#include "sim4g_common_api.h"
#include "sim4g_adapter.h"

int usbdev_busy = FALSE;

static sim_4g_t g_info_4g = {.fly_reset = 0,};

static char *g_sim4g_event[EVENT_MAX] = {
    "EVENT_INIT",
    "EVENT_REFRESH",
    "EVENT_FLY_RESET",
    "EVENT_FLY_OOM",
    "EVENT_PWR_RESET",
    "EVENT_PWR_OOM",
    "EVENT_LNX_RESET",
    "EVENT_WAITING",
};

typedef struct {
    const char*   product_name;
    sim_init      cb_init;          //初始化，确认跟模块通讯正常。
    sim_flymode   cb_flymode;       //飞行模式
    sim_pwr_reset cb_pwr_reset;     //硬复位
    const char*   idVendor;
    const char*   idProduct;
    eModuleType   type;
} sDev4gType;

/* eModuleType需要和dev4g_list顺序保持一致 */
static sDev4gType dev4g_list[] = {
    {"NULL"   , sim4g_800e_init , sim4g_800e_flyreset , sim4g_800e_pwr_reset , "0000", "0000", E_UNINIT }, // 默认走ec800e
    {"EC800E" , sim4g_800e_init , sim4g_800e_flyreset , sim4g_800e_pwr_reset , "2c7c", "0903", E_EC800E },
    {"EC801E" , sim4g_800e_init , sim4g_800e_flyreset , sim4g_800e_pwr_reset , "2c7c", "0903", E_EC801E }, // 降成本，通过 ATI 区别
    {"USBBOOT", sim4g_800e_init , sim4g_800e_flyreset , sim4g_800e_pwr_reset , "17d1", "0903", E_USBBOOT}, // 17d1为ec800e的下载模式
    {"NT26"   , sim4g_nt26_init , sim4g_nt26_flyreset , sim4g_nt26_pwr_reset , "3505", "0001", E_NT26   },
    {"YGX09"  , sim4g_ygx09_init, sim4g_ygx09_flyreset, sim4g_ygx09_pwr_reset, "19d1", "1003", E_YGX09  },
};

enum {
    CMD_REFRESH      = 1 << 0,
    CMD_CABLE_CHANGE = 1 << 1,
    CMD_CABLE_IN     = 1 << 2,
    CMD_CABLE_OUT    = 1 << 3,
    CMD_UPDATING     = 1 << 4,
    CMD_LOCATION     = 1 << 5,
};

typedef struct {
    char iccid[32];
    eOperatorsType operator;
} sIccid2Operator;

static sIccid2Operator g_iccid_maps[] = {
    {"898603", E_TELECOM},
    {"898611", E_TELECOM},

    {"898600", E_MOBILE},
    {"898602", E_MOBILE},
    {"898604", E_MOBILE},
    {"898607", E_MOBILE},
    {"898608", E_MOBILE},

    {"898601", E_UNICOM},
    {"898606", E_UNICOM},
    {"898609", E_UNICOM}
};

static void cb_4g_refresh(void *date)
{
    do {
        if (NULL == date) {
            break;
        }

        eOperatorsType operators = (int)date;
        if (g_info_4g.SimInfo.operators == operators) {
            break;
        }

        g_info_4g.curr_ev = EVENT_INIT;
        g_info_4g.SimInfo.operators = operators;
        DBG("operators:%d\n", g_info_4g.SimInfo.operators);
    } while(0);
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

static void cb_location(int id, void *p_src, int size, void *ctx)
{
    CPY2CMD(CMD_LOCATION);
}

static void cb_refresh(int id, void *p_src, int size, void *ctx)
{
    CPY2CMD(CMD_REFRESH);
    if (p_src != NULL) {
        Sim4g *sim4g_info = (Sim4g *)p_src;
        g_info_4g.SimInfo.fdd = sim4g_info->fdd;
        g_info_4g.SimInfo.card = sim4g_info->card;
        DBG("card:%d fdd:%d\n", g_info_4g.SimInfo.card, g_info_4g.SimInfo.fdd);
        js_run_function(g_info_4g.sch, cb_4g_refresh, (void *)sim4g_info->card, 0);
    }
}

static void cb_update(int id, void *p_src, int size, void *ctx)
{
    CPY2CMD(CMD_UPDATING);
}

int sim4g_get_operator_by_iccid(char *iccid, eOperatorsType *operator_type)
{
    return_val_if_fail(NULL != iccid, FAILURE);
    return_val_if_fail(NULL != operator_type, FAILURE);

    int idx = 0;

    DBG("imsi:[%s]\n", iccid);

    *operator_type = E_UNKNOWN;

    for (idx = 0; idx < ARRAY_SIZE(g_iccid_maps); idx++) {
        if (0 == strncmp(iccid, g_iccid_maps[idx].iccid,
                         strlen(g_iccid_maps[idx].iccid))) {
            *operator_type = g_iccid_maps[idx].operator;
            break;
        }
    }

    if (E_UNKNOWN == *operator_type) {
        ERR("unknown operator type\n");
        goto err;
    }

    return SUCCESS;
err:
    return FAILURE;
}

static void diff_cfg2cmd(void *ctx)
{
    struct cmdstat *p_cmd = (struct cmdstat *)ctx;
    if (p_cmd->cmd_stage) {
        if (p_cmd->cmd_stage & CMD_CABLE_CHANGE) {
            cmd_set_command(p_cmd, net_link_status("eth0") == 1 ? CMD_CABLE_IN : CMD_CABLE_OUT); 
        }
    }
    return;
}

// 防止接串口时，电气干扰导致设备重启
// 即便是接上网线，使用 4g route，不响应网线的 in out 事件 dhcp，不打断策略执行
static int is_u4g_forced()
{
    return (access("/mnt/u4g.txt", F_OK) == 0);
}

// 手动停止，进行命令行调试
static int loop_killed()
{
    if (is_okey(TAG_KILL_Q)){
        if (!is_okey(TAG_KILL_ACK)) {
            WriteFile(TAG_KILL_ACK, "ACK");
        }
        dbg_4g("%s stopped\n", __func__);
        return TRUE;
    } else {
        return FALSE;
    }
}

void exe_ev_command(void *ctx)
{
    int cmd = cmd_get_command((struct cmdstat *)ctx);

    if (!cmd) {
        return;
    }

    if (get_g_sys(factest) || !(system_get_security() || is_okey(F_P2P_TRIPLE))) {
        SYSLOG("it is factory test, no-resp cmd or devid is null and F_P2P_TRIPLE is not exist\n");
        return;
    }

    if (cmd & CMD_REFRESH) {
        g_info_4g.refresh = TRUE;         // dBm
        g_info_4g.ms_step_www = 0;
    }

    if (cmd & CMD_UPDATING) {
        g_info_4g.updating = TRUE;        // 正在升级，依赖 4G 传输
    }

    if (cmd & CMD_LOCATION) {
        sim4g_report_location(&g_info_4g.SimInfo);
    }

    if ((cmd & CMD_CABLE_OUT) && get_g_sys(eth)) {
        g_info_4g.www_tick = 0;
        if (g_info_4g.eth_up == FALSE) {
            SYSLOG("ignore inital event %s\n", "CABLE_OUT");
            return;
        }

        // 立即激活策略
        g_info_4g.ms_step_www = g_info_4g.ms_step_fly = g_info_4g.ms_step_pwr = 0;
        g_info_4g.eth_up = FALSE;
        SYSLOG("____ eth0 breaking, 4g %s\n", g_info_4g.ripe?"ripe":"raw");
        // 无串口时的有线调试模式
        if (is_u4g_forced()) {
            SYSLOG("u4g.txt, keep ROUTE TABLE though %s\n", "CABLE_IN");
            return;
        }

        if (g_info_4g.ripe && (is_okey(F_ICCID) || is_okey(F_EICCID))) {
            UtilSystemCmd("route del default gw 0.0.0.0 dev eth0");
            UtilSystemCmd("/ipc/bin/4g udhcpc");
            sleep(5);
        } else {
            SYSLOG("sim card is not exist, ignore event %s\n", "CABLE_OUT");
            return;
        }
    }

    if ((cmd & CMD_CABLE_IN) && get_g_sys(eth)) {
        g_info_4g.www_tick = 0;
        if (g_info_4g.eth_up == TRUE) {
            DBG("ignore inital event %s\n", "CABLE_IN");
        }

        g_info_4g.eth_up = TRUE;
        SYSLOG("____ eth0 connecting\n");

        // 无串口时的有线调试模式
        if (is_u4g_forced()) {
            SYSLOG("u4g.txt, keep ROUTE TABLE though %s\n", "CABLE_IN");
            return;
        }

        UtilSystemCmd("route del default gw 0.0.0.0 dev usb0");

        NetEthS netcfg = {{0}};
        conf_get_ethcfg(&netcfg);

        if (netcfg.dhcpen) {
            reset_udhcpc('e');
            UtilSystemCmd("route -n");
        } else {
            UtilSystemCmd("route del default gw 0.0.0.0 dev eth0");

            char cmdline[128] = {0};
            snprintf(cmdline, sizeof(cmdline)-1, "ifconfig eth0 %s", netcfg.ip);
            UtilSystemCmd(cmdline);
            memset(cmdline,0,sizeof(cmdline));
            snprintf(cmdline, sizeof(cmdline)-1, "route add default gw %s dev eth0 metric 0; route -n", netcfg.gw);
            UtilSystemCmd(cmdline);
        }
    }
}

int sim4g_chip_ok()
{
    int i = 0;
    char id_ven[8]    = {0};
    char vendor[8]    = {0};
    char idVendor[8]  = {0};
    char idProduct[8] = {0};

    //避免4g模块探测不到时,dev4g_list未初始化
    if (LoadFile(USBDEV_IDVENDOR, idVendor, sizeof(idVendor)-1) <= 0 ||
       LoadFile(USBDEV_IDPRODUCT, idProduct, sizeof(idProduct)-1) <= 0) {
        SYSLOG("USBDEV_IDVENDOR or USBDEV_IDVENDOR not exist\n");
        if (LoadFile(F_ID_VEN, idVendor, sizeof(idVendor)-1) <= 0) {
            strncpy(idVendor, dev4g_list[E_UNINIT].idVendor, sizeof(idVendor));
        }
    } else {
        if (LoadFile(F_ID_VEN, id_ven, sizeof(id_ven)-1) <= 0) {
            WriteFile(F_ID_VEN, idVendor);
        } else {
            if (strncmp(id_ven, idVendor, strlen(idVendor)) != 0) {
                WriteFile(F_ID_VEN, idVendor);
                SYSLOG("model change, update idVendor\n");
            }
        }
    }

    //模拟4G模块不存在的情况
    if (get_g_run(sim4g, RUN_CHIP_FAIL)) {
        strncpy(idVendor, dev4g_list[E_UNINIT].idVendor, sizeof(idVendor));
        SYSLOG("4G module failed, idVendor = %s\n", idVendor);
    }

    for (i = 0; i < ARRAY_SIZE(dev4g_list); i++) {
        if ((strncmp(idVendor, dev4g_list[i].idVendor, 4) == 0) &&
            (strncmp(idProduct, dev4g_list[i].idProduct, 4) == 0)) {
            g_info_4g.model_type = dev4g_list[i].type;
            g_info_4g.cb_init = dev4g_list[i].cb_init;
            g_info_4g.cb_flymode = dev4g_list[i].cb_flymode;
            g_info_4g.cb_pwr_reset = dev4g_list[i].cb_pwr_reset;
            SYSLOG("product name:%s, type = %d\n", dev4g_list[i].product_name, dev4g_list[i].type);
            break;
        }
    }

    if (g_info_4g.model_type != E_UNINIT) {
        switch (g_info_4g.model_type) {
        case E_NT26:
            strncpy(vendor, "L6", sizeof(vendor));
            break;
        case E_EC800E:
        case E_EC801E:
            strncpy(vendor, "EC", sizeof(vendor));
            break;
        case E_USBBOOT:
            strncpy(vendor, "EC", sizeof(vendor));
            sleep(16);
        case E_YGX09:
            strncpy(vendor, "YG", sizeof(vendor));
            break;
        default:
            SYSLOG("UNKNOW MODEL\n");
            break;
        }
        WriteFile(OTA_VENDOR, vendor);
    } else {    // 识别的模块不在数组内,函数指针赋初值
        g_info_4g.model_type = dev4g_list[0].type;
        g_info_4g.cb_init = dev4g_list[0].cb_init;
        g_info_4g.cb_flymode = dev4g_list[0].cb_flymode;
        g_info_4g.cb_pwr_reset = dev4g_list[0].cb_pwr_reset;

        SYSLOG("model type error\n");
        goto err_model;
    }

    // 模组检查
    UtilSystemCmd("ifconfig usb0 up");
    UtilSystemCmd("ifconfig usb0");
    if (is_okey(SIM4G_AT_TTY4G) && is_okey(SIM4G_AT_TXBYUGA)) {
        // 拉起成功，播报成功语音，不设置 mtu(ioctl 概率 block)
        g_info_4g.chip_ever = TRUE;
        if (!g_info_4g.eth_up) {
            play_conditionally(AUDIO_CHECK_SUCCESS_4G);
        }
        get_soft4g_chipname();
        SYSLOG("4g usb0 success\n");
    } else {
        SYSLOG("4g usb0 fail\n");
        goto err_model;
    }

    return g_info_4g.chip_ever;

err_model:
    g_info_4g.chip_ever = FALSE;
    if (!g_info_4g.chip_ever && (!g_info_4g.eth_up)) {
        // 播报失败语音
        play_conditionally(AUDIO_CHECK_FAIL_4G);
        play_conditionally(AUDIO_CHECK_FAIL_4G);
    }
    g_info_4g.prev_ev = g_info_4g.curr_ev;
    g_info_4g.curr_ev = EVENT_LNX_RESET;
    syslog4g(__FILE__, __LINE__);
    return g_info_4g.chip_ever;
}

int sim4g_run_AT_clr_sec(int sec, const char *AT_no_CR, char *buf, int len)
{
    int fd = -1;
    int size = 0;
    int ret = -1;
    char cmdbuf[128] = {0};
    char at_reply[1024] = {0,};

    struct timeval timeout;
    timeout.tv_sec = sec;
    timeout.tv_usec = 500*1000;

    do {
        fd = ha_open_ttyusb0();
        return_val_if_fail(fd > 0, FAILURE);

        ret = sim4g_set_serial(fd, 115200, 8, 'S', 1);
        if(ret == -1){
            SYSLOG("set yg at serial fail\n");
            break;
        }

        tcflush(fd, TCIOFLUSH);
        sprintf(cmdbuf, "%s\r\n", AT_no_CR);
        size = write(fd, cmdbuf, strlen(cmdbuf));
        if(size < 0){
            DBG("write at cmd fail\n");
            ret = -1;
            break;
        }
        memset(buf, 0 , len);
        int nr = ha_readfully(fd, buf, len-1, &timeout);
        if (nr <= 0) {
            DBG("read at cmd fail\n");
            ret = -1;
            break;
        }

        snprintf(at_reply, sizeof(at_reply)-1, "%s reply: [%s]\n", AT_no_CR, buf);
        if(strstr(buf, "OK")==NULL){
            ERR("%s", at_reply);
            ret = -1;
            AppendFile("/tmp/messages", at_reply);
        } else {
            DBG("%s", at_reply);
            if (get_g_run(sim4g, RUN_PRINT_4G) || is_test_ver()) { // 调试打开正常日志
                AppendFile("/tmp/messages", at_reply);
            }
            ret = 0;
        }
    }while(0);
    close(fd);
    return ret;
}

int sim4g_run_AT_clr(const char *AT_no_CR, char *buf, int len)
{
    int ret = 0;
    ret = sim4g_run_AT_clr_sec(0, AT_no_CR, buf, len);

    return ret;
}

int simcard_workable()
{
    int i = 2;
    int sim_exit = FALSE;
    char buf[256] = {0};

    if (get_g_run(sim4g, RUN_CPIN_FAIL)) {
        return FALSE;
    }
    while (i--) {
        sim4g_run_AT_clr("AT+CPIN?", buf, sizeof(buf));
        if (strstr(buf, "READY")) {
            sim_exit = TRUE;
            break;
        } else {
            SYSLOG("_sim_ error[%s] @%d\n", buf, i);
            sleep(2);
            sim_exit = FALSE;
            continue;
        }
    }

    return sim_exit;
}

int sim4g_get_stat(Sim4g *data)
{
    pthread_mutex_lock(&(g_info_4g.mutex));
    memcpy(data, &g_info_4g.SimInfo, sizeof(Sim4g));
    pthread_mutex_unlock(&(g_info_4g.mutex));
    return 0;
}

int sim4g_get_sim_4g(sim_4g_t *data)
{
    pthread_mutex_lock(&(g_info_4g.mutex));
    memcpy(data, &g_info_4g, sizeof(sim_4g_t));
    pthread_mutex_unlock(&(g_info_4g.mutex));
    return 0;
}

const char *get_soft4g_chipname()
{
    if (g_info_4g.vendor_name[0] == '\0') {
        LoadFile2(OTA_VENDOR, "%s", g_info_4g.vendor_name);
    }

    return g_info_4g.vendor_name;
}

int sim4g_video_turnon()
{
    g_info_4g.video_workable = TRUE;
    return 0;
}

int sim4g_video_turnoff()
{
    g_info_4g.video_workable = FALSE;
    return 0;
}

int sim4g_video_workable()
{
    return g_info_4g.video_workable;
}

int sim4g_is_mobile_card( char *iccid_file, eOperatorsType *operator_type)
{
    char iccid[32] = {0};

    if (is_okey(iccid_file)) {
        LoadFile2(iccid_file, "%s", iccid);
        if (0 == strncmp(iccid, "898600", 6)
            || 0 == strncmp(iccid, "898602", 6)
            || 0 == strncmp(iccid, "898604", 6)
            || 0 == strncmp(iccid, "898607", 6)
            || 0 == strncmp(iccid, "898608", 6)) {
            DBG(">> sim4g detect mobile card\n");
            *operator_type = E_MOBILE;
            return SUCCESS;
        }
    }
    *operator_type = E_TELECOM;
    return SUCCESS;
}

int ha_open_ttyusb0()
{
    int fd = -1;
    if ((fd = open(SIM4G_AT_TTY4G, O_RDWR)) < 0) {
        sleep(1);
        if ((fd = open(SIM4G_AT_TTY4G, O_RDWR)) < 0) {
            printf("open %s fail: %s\n", SIM4G_AT_TTY4G, strerror(errno));
            remove(SIM4G_AT_TTY4G);
            return FAILURE;
        }
    }
    return fd;
}

char *get_4g_runinfo()
{
    static char buf[1024] = {0};
    snprintf(buf, sizeof(buf)-1, "\n"
                "www_alive     :%d  eth_up     :%2d    www_alive_ever:%d    curr_ev    :%s\n"
                "sim_ever      :%d  chip_ever  :%2d    chipvendor    :%s    acc        :%d\n"
                "fly_reset     :%d  inside_sim :%2d    sim_iccid     :%s\n"
                "pwr_reset     :%d  sim_status :%2d    imsi          :%s\n"
                "lnx_reset     :%d  csq        :%2d    esim_iccid    :%s\n"
                "operators     :%d  dmb[0]     :%2d    eimsi         :%s\n"
                "ripe          :%d  dmb[1]     :%2d    imei          :%s\n"
                "www_fail_cnt  :%d  dmb[2]     :%2d    tx_KBytes     :%lld\n"
                "videoAAA      :%d  p2p_plat4m :%2d    rx_bytes      :%lld\n"
                "sim           :%d  e_sim      :%2d    @%s\n",
            g_info_4g.www_alive        , g_info_4g.eth_up            ,g_info_4g.www_alive_ever  ,g_sim4g_event[g_info_4g.curr_ev],
            g_info_4g.sim_ever         , g_info_4g.chip_ever         ,g_info_4g.vendor_name     ,g_info_4g.acc_shift,
            g_info_4g.fly_reset        , g_info_4g.inside_sim        ,g_info_4g.SimInfo.sim_card,
            g_info_4g.pwr_reset        , g_info_4g.SimInfo.sim_status,g_info_4g.SimInfo.imsi,
            g_info_4g.lnx_reset        , g_info_4g.csq               ,g_info_4g.SimInfo.esim_card,
            g_info_4g.SimInfo.operators, g_info_4g.dbm[0]            ,g_info_4g.SimInfo.eimsi,
            g_info_4g.ripe             , g_info_4g.dbm[1]            ,g_info_4g.SimInfo.imei,
            g_info_4g.www_fail_cnt     , g_info_4g.dbm[2]            ,(g_info_4g.tx_bytes[2]-g_info_4g.tx_bytes[0])/1024,
            g_info_4g.video_workable   , platform_on_line()    ,(g_info_4g.rx_bytes[2]-g_info_4g.rx_bytes[0]), 
            g_info_4g.SimInfo.sim      , g_info_4g.SimInfo.e_sim     , get_timestr());
    return buf;
}

void syslog4g(const char *file, int lineno)
{
    char *p = get_4g_runinfo();
    AppendFile("/tmp/messages", p);
    printf("%s:%d %s\n", file, lineno, p);
}

/*
 * 防止弱网传输时 ping 失败
 * Tx 每分钟有一次 Mqtt ping，TX=200B, RX=100B
 * App 一次请求                TX=30KB, RX=10KB
 *
root@fhan:/tmp# ifconfig usb0 | grep 'RX bytes'
          RX bytes:242006 (236.3 KiB)  TX bytes:4952864 (4.7 MiB)
root@fhan:/tmp# [dbg](21948123) MQTTKeepalive(374): len = MQTTSerialize_pingreq() = 2
[inf](21948124) iotx_mc_keepalive_sub(3015): send MQTT ping...
[inf](21948738) iotx_mc_cycle(1899): receive ping response!
root@fhan:/tmp# ifconfig usb0 | grep 'RX bytes'
          RX bytes:242127 (236.4 KiB)  TX bytes:4953065 (4.7 MiB)
              # App Req
root@fhan:/tmp# ifconfig usb0 | grep 'RX bytes'
          RX bytes:253480 (247.5 KiB)  TX bytes:4991789 (4.7 MiB)
*/
int txrx_alive()
{
    int64_t nr_tx = 0;
    int64_t nr_rx = 0;

    g_info_4g.tx_bytes[0] = g_info_4g.tx_bytes[1];
    g_info_4g.rx_bytes[0] = g_info_4g.rx_bytes[1];
    g_info_4g.tx_bytes[1] = g_info_4g.tx_bytes[2];
    g_info_4g.rx_bytes[1] = g_info_4g.rx_bytes[2];

    nr_tx = LoadFile2("/sys/class/net/usb0/statistics/tx_bytes", "%lld", &g_info_4g.tx_bytes[2]);
    nr_rx = LoadFile2("/sys/class/net/usb0/statistics/rx_bytes", "%lld", &g_info_4g.rx_bytes[2]);

    if (nr_tx <= 0 || nr_rx <= 0) {
        g_info_4g.tx_bytes[0] = g_info_4g.tx_bytes[1] = g_info_4g.tx_bytes[2] = 0;
        g_info_4g.rx_bytes[0] = g_info_4g.rx_bytes[1] = g_info_4g.rx_bytes[2] = 0;
        ERR("get tx rx_bytes fail\n");
        return FALSE;
    }

    if (g_info_4g.tx_bytes[2] <= 0 || g_info_4g.rx_bytes[2] <= 0) {
        DBG("first Run, ignor");
        return FALSE;
    }

    if (g_info_4g.tx_bytes[0] > 0 && 
       (g_info_4g.tx_bytes[2] - g_info_4g.tx_bytes[0] > 10*1000 ||
       (g_info_4g.rx_bytes[2] - g_info_4g.rx_bytes[0] > 120 && platform_on_line()))) {
        dbg_4g("__ translate 1, ignore ping\n"
                "tx012: %lld %lld %lld\n"
                "rx012: %lld %lld %lld\n",
            g_info_4g.tx_bytes[0]%1000000 , g_info_4g.tx_bytes[1]%1000000 , g_info_4g.tx_bytes[2]%1000000,
            g_info_4g.rx_bytes[0]%1000000 , g_info_4g.rx_bytes[1]%1000000 , g_info_4g.rx_bytes[2]%1000000
            );
        return TRUE;
    }

    if (g_info_4g.tx_bytes[1] > 0 &&
       (g_info_4g.tx_bytes[2] - g_info_4g.tx_bytes[0] > 10*1000 ||
       (g_info_4g.rx_bytes[2] - g_info_4g.rx_bytes[0] > 120 && platform_on_line()))) {
        dbg_4g("__ translate 0, ignore ping\n"
                "tx012: %lld %lld %lld\n"
                "rx012: %lld %lld %lld\n",
            g_info_4g.tx_bytes[0]%1000000 , g_info_4g.tx_bytes[1]%1000000 , g_info_4g.tx_bytes[2]%1000000,
            g_info_4g.rx_bytes[0]%1000000 , g_info_4g.rx_bytes[1]%1000000 , g_info_4g.rx_bytes[2]%1000000
            );
        return TRUE;
    }

    return FALSE;
}

int sim4g_update_iccid_imei_imsi(sim_4g_t *info)
{
    if (is_okey(F_ICCID)) {
        LoadFile2(F_ICCID, "%s",  g_info_4g.SimInfo.sim_card);
    }

    if (strlen(g_info_4g.SimInfo.sim_card) <= 0) {
        g_info_4g.SimInfo.sim = FALSE;
    } else {
        g_info_4g.SimInfo.sim = TRUE;
    }

    if (is_okey(F_EICCID)) {
        LoadFile2(F_EICCID, "%s", g_info_4g.SimInfo.esim_card);
    } 

    if (strlen(g_info_4g.SimInfo.esim_card) <= 0) {
        g_info_4g.SimInfo.e_sim = FALSE;
    } else {
        g_info_4g.SimInfo.e_sim = TRUE;
    }

    if (is_okey(F_IMSI)) {
        LoadFile2(F_IMSI, "%s", g_info_4g.SimInfo.imsi);
    }

    if (is_okey(F_EIMSI)) {
        LoadFile2(F_EIMSI, "%s", g_info_4g.SimInfo.eimsi);
    }

    if (is_okey(F_IMEI)) {
        LoadFile2(F_IMEI, "%s", g_info_4g.SimInfo.imei);
        snprintf(g_info_4g.SimInfo.iccid, sizeof(g_info_4g.SimInfo.iccid), "%s%s", SIM_VIRTUAL_CARD_TITLE, g_info_4g.SimInfo.imei);
        snprintf(g_info_4g.SimInfo.esim_iccid, sizeof(g_info_4g.SimInfo.esim_iccid), "%s%s", SIM_VIRTUAL_CARD_TITLE, g_info_4g.SimInfo.imei);
    }

    return SUCCESS;

}

static int sim4g_play_simcard_audio(void)
{
    if (is_okey(F_ICCID) || is_okey(F_EICCID)) {
        play_conditionally(AUDIO_SIM_CHECK_SUCCESS_4G);
        g_info_4g.sim_ever = TRUE;
    } else {
        play_conditionally(AUDIO_SIM_CHECK_FAIL_4G);
        play_conditionally(AUDIO_SIM_CHECK_FAIL_4G);
        g_info_4g.sim_ever = FALSE;
    }

    return 0;
}

/*
 * csq      rssi
 * 0        小于等于-113 dBm
 * 1        -111 dBm
 * 2~30     -109 ~ -53 dBm
 * 31       大于等于-51 d
 * csq取值说明：(113+rssi)/2  取值范围：0~31
 * signal取值说明 0-100
*/
static void sim4g_refresh_signal(void)
{
    g_info_4g.csq   = 0;
    int signal      = 0;
    int csq         = 0;
    int csq_count   = 3;
    char buf[256]   = {0};
    char dbm_buf[8] = {0};

    sim4g_run_AT_clr("AT", buf, sizeof(buf));

    while (csq_count--) {
        sim4g_run_AT_clr("AT+CSQ", buf, sizeof(buf));
        scanf_AT_result2(buf, "CSQ:.([0-9]*)", dbm_buf, sizeof(dbm_buf));
        if (strlen(dbm_buf) <= 0) {
            ERR("read at csq fail @%d\n", csq_count+1);
            sleep(3);
            continue;
        }

        csq = atoi(dbm_buf);
        if (csq > 31) {
            ERR("read at dbm fail @%d\n", csq_count+1);
            sleep(3);
            continue;
        } else {
            signal = (int)((atoi(dbm_buf) / BEST_4G_SIG_VAL) * 100);
            break;
        }
    }

    // 确保刷新数据原子性 g_info_4g.SimInfo.dbm
    pthread_mutex_lock(&(g_info_4g.mutex));
    if (csq > 31) {
        g_info_4g.SimInfo.dbm = 0;
    } else if (csq > 21) {
        g_info_4g.SimInfo.dbm = 4;
    } else if (csq > 18) {
        g_info_4g.SimInfo.dbm = 3;
    } else if (csq > 12) {
        g_info_4g.SimInfo.dbm = 2;
    } else if (csq > 3) {
        g_info_4g.SimInfo.dbm = 1;
    }

    g_info_4g.SimInfo.esim_dbm = g_info_4g.SimInfo.dbm;
    g_info_4g.SimInfo.signal = signal;
    pthread_mutex_unlock(&(g_info_4g.mutex));

    g_info_4g.csq = csq;
    ++g_info_4g.csq_tick;
    if (((g_info_4g.csq_tick)%2 == 0)) {
         dbg_4g("sim_dbm: %d, csq: %d\n", g_info_4g.SimInfo.dbm, g_info_4g.SimInfo.signal);
    }

    g_info_4g.dbm[2] = g_info_4g.dbm[1];
    g_info_4g.dbm[1] = g_info_4g.dbm[0];
    g_info_4g.dbm[0] = g_info_4g.SimInfo.dbm;

    // 刷新小区信息
    if (g_info_4g.model_type == E_YGX09) {
        sim4g_run_AT_clr("AT+CCED=0,1", buf, sizeof(buf));
    } else {
        sim4g_run_AT_clr("AT+QENG=\"servingcell\"", buf, sizeof(buf));
    }

    return;
}

static void sim4g_refresh_status(void)
{
    static int report_times = 0;
    static int report_interval = 0;
    static int report_location_times = 0;
    static int report_location_interval = 0;
    static int log_tick = 0;
    static int csq_tick = 0;
    static int report_ok = FALSE;
    static int report_location_ok = FALSE;
    static int www_alive_prev = FALSE;
    const char *tag = "UNKNOW";

    // 刷新 dbB & www
    if (is_inc_mod0(csq_tick, 3)) {
        sim4g_refresh_signal();
    }

    if (!g_info_4g.initialized) {
        // 首次启动，tid 到 syslog，快速检查 card chip
        SYSLOG("sim4g: thread %d initialized\n", (int)gettid());
        g_info_4g.initialized = TRUE;

        if (!g_info_4g.eth_up) {
            if (g_info_4g.dbm[0] > 2) {
                play_conditionally(AUDIO_CSQ_STRONG_4G);
            } else {
                play_conditionally(AUDIO_CSQ_WEAK_4G);
            }
        }
    }

    if (get_g_run(sim4g, RUN_WWW_FAIL)) {
        g_info_4g.www_alive = FALSE;
        tag = "run_www_fail";
        goto __www_fail;
    }

    // 做 dhcp 检查
    int cnt = 10;
    while (!get_g_stat(sim4g, SIM4G_DHCPONCE) && --cnt > 0) {
       if (0 == (cnt % 3)) DBG("wait a DHCP succ\n");
       sleep (1);
    }

    // WWW 检查，check tx_bytes & rx_bytes, ping p2p_plat4m 
    // 无流量时，概率出现大 TXRX
    if (g_info_4g.www_tick++ >= 6) {    //拔插网线,设备第一次启动使用ping判断网络
        g_info_4g.www_alive = platform_on_line() && txrx_alive();
    }

    if (!g_info_4g.www_alive) {
        g_info_4g.www_alive = g_info_4g.SimInfo.connected = sim4g_www_reachable();
        dbg_4g("www_tick = %d\n", g_info_4g.www_tick);
        if (!platform_on_line() && g_info_4g.www_alive) {
            DBG("plat4m offline but www_alive @cnt: %d\n", g_info_4g.www_fail_cnt);
        }
    }

    if (g_info_4g.www_alive) {
        g_info_4g.SimInfo.online = g_info_4g.SimInfo.esim_is4G = g_info_4g.SimInfo.is4G = TRUE;
        sim4g_burn(&g_info_4g);
        g_info_4g.refresh = FALSE;
        if (!report_ok) {
            if (report_interval == 0) {
                report_ok = (SUCCESS == sim4g_report_cloud_server_simcard(&g_info_4g));
                if (get_g_run(sim4g, RUN_REPORT_FAIL)) {  // 模拟4G上报失败
                    report_ok = 0;
                }
                if (report_ok) {
                    report_times = 0;
                    report_interval = 0;
                    g_info_4g.www_alive_ever = TRUE;
                    syslog4g("report_ok", __LINE__);
                } else {
                    report_interval = (int)pow(2, report_times%8);  // 2^7*30s=64min
                    report_times++;
                    SYSLOG("report fail, report_time = %d\n", report_times);
                }
            } else {
                report_interval --;
            }
        }

        // 设备绑定 阿里在线 非产测
        if (!report_location_ok && g_info_4g.SimInfo.location_enable) {
            if (report_location_interval == 0) {
                DevConfS devconf = {0};
                conf_get_devconf_cfg(&devconf);
                if (devconf.devicebind && platform_on_line() && (!get_g_sys(factest))) {
                    struct sysinfo info;
                    sysinfo(&info);
                    if (info.uptime > 600) {
                        report_location_ok = sim4g_report_location(&g_info_4g.SimInfo);
                        if (get_g_run(sim4g, RUN_REPORT_LOCATION_FAIL)) { // 模拟4G定位上报失败
                            report_location_ok = FAILURE;
                        }

                        if (report_location_ok == SUCCESS) {
                            report_location_ok = TRUE;
                            report_location_times = 0;
                            report_location_interval = 0;
                            syslog4g("report_location_ok", __LINE__);
                        } else {
                            report_location_ok = FALSE;
                            report_location_interval = (int)pow(2, report_location_times%8);
                            report_location_times++;
                            SYSLOG("report location fail, times: %d\n", report_location_times);
                        }
                    }
                }
            } else {
                report_location_interval--;
            }
        }

        g_info_4g.www_fail_cnt = 0;
        if (g_info_4g.www_alive != www_alive_prev) {
            SYSLOG("sim4g clean flag, www_alive=%d, www_alive_prev=%d\n",g_info_4g.www_alive, www_alive_prev);
            g_info_4g.fly_reset = g_info_4g.pwr_reset = g_info_4g.lnx_reset = 0;
            remove(SIM4G_LNX_REBOOT);
        }
    } else {
        g_info_4g.SimInfo.online = g_info_4g.SimInfo.esim_is4G = g_info_4g.SimInfo.is4G = FALSE;
        // usb0 检查，ping 自身
        char self[16] = {0};

        if (SUCCESS != sim4g_get_usb0_ip(self)) {
            tag = "ipaddr_usb0"; goto __soft_err;
        }

        if (!is_alive_ip(self, __func__)) {
            ERR("%s usb0-ip-alive-fail\n", self);
            tag = "alive_usb0"; goto __soft_err;
        }

        tag = "_www_"; goto __www_fail;
    }

    if (is_inc_mod0(log_tick, 180)) {
        syslog4g(__FILE__, __LINE__);
    }
    www_alive_prev = TRUE;
    g_info_4g.ms_step_www = 30*1000;

    return;

__www_fail:
    // 3rd 失败 -> 可以快速判断一次 simcard
    // cfun 会拖慢 sim 掉卡的修复时间
    if (++g_info_4g.www_fail_cnt < 3) {
        g_info_4g.ms_step_www = 30*1000;
        UtilSystemCmd("lsusb; ls /dev/tty*");
        DBG("err_tag: %s %s\n", tag, get_4g_runinfo());
        return;
    }

    if (simcard_workable()) {
        SYSLOG("www_fail_cnt to __soft_err\n");
        goto __soft_err;
    } else {
        // 快速判断sim卡, 未识别走cfun 0/1尝试修复
        SYSLOG("sim _not_ ready __soft_err\n");
        g_info_4g.refresh = FALSE;
        goto __soft_err;
    }

__soft_err:
    // 外网不通，进行软修复
    if (!g_info_4g.eth_up && g_info_4g.SimInfo.operators != g_info_4g.SimInfo.card) {
        play_conditionally(AUDIO_SIM4G_CON_FAIL);
        play_conditionally(AUDIO_SIM4G_CON_FAIL);
    }

    g_info_4g.prev_ev = g_info_4g.curr_ev;
    g_info_4g.curr_ev = EVENT_FLY_RESET;
    www_alive_prev = FALSE;
    UtilSystemCmd("route -n; ifconfig usb0");

    if (!is_okey("/sys/class/net/usb0/statistics/tx_bytes") || (!is_okey("/sys/class/net/usb0/statistics/rx_bytes"))) {
        g_info_4g.curr_ev = EVENT_LNX_RESET;
        UtilSystemCmd("lsusb; ls /dev/tty*");
        SYSLOG("tx_bytes or rx_bytes not exist, to reboot\n");
        return;
    }

    if (!is_okey(SIM4G_AT_TTY4G)) {
        tag = "ttyusb0";
        goto __hard_err;
    }
    SYSLOG("__soft_err from line: %s\n", tag);
    syslog4g(__FILE__, __LINE__);
    if (g_info_4g.refresh == TRUE) {
        if (E_TELECOM == g_info_4g.SimInfo.operators) {
            g_info_4g.SimInfo.operators = E_MOBILE;
        } else {
            g_info_4g.SimInfo.operators = E_TELECOM;
        }
        g_info_4g.refresh = FALSE;
    }
    return;

__hard_err:
    www_alive_prev = FALSE;
    g_info_4g.prev_ev = g_info_4g.curr_ev;
    g_info_4g.curr_ev = EVENT_PWR_RESET;
    syslog4g(__FILE__, __LINE__);
}

/*
 * 1. 软重启路径: fly_reset -> pwr_reset -> lnx_reset，www 连接失败时执行，每个重启达 4 次，时向下个演进
 * 2. 硬重启路径: pwr_reset -> lnx_reset, 适用于 4G 模块和 simcard 检测不到时
 * 3. 全系统重启: lnx_reset, 最多只重启 4 次，保存在 /opt/conf/4g/lnx_reset, 次数超过时会转回到 pwr_reset
 * 4. 时间间隔  : 所有 xxx_reset 都是 2 分钟，但系统第一次异常检测后的 xxx_reset 会立即执行。
 * 5. 系统修复  : www 连接成功，所有标志位，都会清 0
 */
void sim4g_loop(void *ctx)
{
    // 32s 打印一次状态
    int ret = 0;
    int chip_ok = 0;
    static int tick = 0;
    static int time_tick = 0;
    static int sec_left = 0;

    // 处理 grun gsys
    sim4g_send_event(JEvent_TencentReset);

    // 处理事件
    exe_ev_command(ctx);

    if (!is_inc_mod0(time_tick, 5) && g_info_4g.ripe) {
        //DBG("time_tick = %d\n", time_tick);
        return;
    }

    if (g_info_4g.updating) {
        DBG("updating, exit 4g process\n");
        return;
    }

    if (g_info_4g.www_alive_ever && get_g_run(sim4g, RUN_PAUSE_4G)) {
        dbg_4g("pause 4g loop\n");
        return;
    }

    // 信息打印
    if (is_inc_mod0(tick, 6)) {
        dbg_4g("%s", get_4g_runinfo());
    } else if (get_g_run(sim4g, RUN_PRINT_4G)) {
        if (is_okey(SIM4G_ACC_SHIFT)) {
            // 动态加载一次
            char nr[8] = {0};
            LoadFile(SIM4G_ACC_SHIFT, nr, sizeof(nr)-1);
            g_info_4g.acc_shift = MAX(0, MIN(2,atoi(nr)));    // 0,1,2  1X 2X 4X 时间
        }
        clr_g_run(sim4g, RUN_PRINT_4G);
        syslog4g(__FILE__, __LINE__);
    }

    // 无 devid, 不进行任何操作，一直在 EVENT_INIT
    /*if (!sim4g_get_security() && (!get_g_sys(factest))) {
        dbg_4g("! security @%s", __func__) ;
        return;
    }*/
    
    // 手动暂停 4g 策略
    if (loop_killed()) {
        dbg_4g("stopped @%s\n", "manual stop") ;
        return;
    }

    // 非测试模式下，接有线时启动4G，但不跑策略，可快速响应网线插拔事件
    // 烧录模式必须用 sim4g_get_security 规避，不能依赖g_info_4g.eth_up条件
    if (!get_g_sys(factest) && g_info_4g.eth_up && g_info_4g.ripe) {
        dbg_4g("stopped @%s\n", "eth_up");
        return;
    }

    int acc = g_info_4g.acc_shift;
    // main/4G/sim4g.c.i
    switch(g_info_4g.curr_ev) {
    case EVENT_INIT:
        DBG("FSM @%s\n", g_sim4g_event[g_info_4g.curr_ev]);

        g_info_4g.www_fail_cnt = 0;
        g_info_4g.ms_step_www = 0*1000;
        g_info_4g.tx_bytes[0] = g_info_4g.tx_bytes[1] = g_info_4g.tx_bytes[2] = 0;
        g_info_4g.rx_bytes[0] = g_info_4g.rx_bytes[1] = g_info_4g.rx_bytes[2] = 0;
        if (g_info_4g.model_type == E_UNINIT) {
            chip_ok = sim4g_chip_ok();
            goto_if_4gfail(chip_ok == TRUE, err_init);
        }

        ret = dev4g_list[g_info_4g.model_type].cb_init(&g_info_4g);
        g_info_4g.cid_succ = sim4g_update_iccid_imei_imsi(&g_info_4g);
        if (!g_info_4g.eth_up) {
            sim4g_play_simcard_audio();
        }
        goto_if_4gfail((ret == SUCCESS && g_info_4g.cid_succ == SUCCESS), err_init);
        g_info_4g.curr_ev = EVENT_REFRESH;
        break;

    case EVENT_REFRESH:
        if (ms_clock_is_timeup2(&g_info_4g.ms_clock_fly, g_info_4g.ms_step_www, &sec_left)) {
            dbg_4g("FSM @%s\n", g_sim4g_event[g_info_4g.curr_ev]);
            // 事件发生器
            sim4g_refresh_status();
            g_info_4g.ripe = TRUE;
        } else {
            dbg_4g("FSM @%s left %d secs\n", g_sim4g_event[g_info_4g.curr_ev], sec_left);
        }
        break;

    case EVENT_FLY_RESET:
        if (NULL == g_info_4g.cb_flymode) {
            ERR("4G Have not INIT, can not do fly reset");
            break;
        }

        if (ms_clock_is_timeup2(&g_info_4g.ms_clock_fly, g_info_4g.ms_step_fly>>acc, &sec_left)) {
            if (is_inc_modc(g_info_4g.fly_reset, 4)) {
                DBG("____\n");
                g_info_4g.curr_ev = EVENT_FLY_OOM;
                g_info_4g.ms_step_fly= 120*1000;
                break;
            }

            SYSLOG("sim4g: @%s from %s\n", g_sim4g_event[g_info_4g.curr_ev], g_sim4g_event[g_info_4g.prev_ev]);
            g_info_4g.cb_flymode();
            g_info_4g.prev_ev = g_info_4g.curr_ev;
            g_info_4g.curr_ev = EVENT_WAITING;
            g_info_4g.next_ev = EVENT_INIT;
            g_info_4g.ms_step_fly= 120*1000;
            g_info_4g.ms_step_wait = 5*1000;  // 脚本中已经有 10s，
            ms_clock_reset(&g_info_4g.ms_clock_wait);
        } else {
            DBG("FSM @%s left %d secs\n", g_sim4g_event[g_info_4g.curr_ev], sec_left);
        }
        break;

    case EVENT_FLY_OOM:
    case EVENT_PWR_RESET:
        if (NULL == g_info_4g.cb_pwr_reset) {
            ERR("4G Have not INIT, can not do pwr reset");
            break;
        }

        if (ms_clock_is_timeup2(&g_info_4g.ms_clock_pwr, g_info_4g.ms_step_pwr>>acc, &sec_left)) {
            if (is_inc_modc(g_info_4g.pwr_reset, 4)) {
                g_info_4g.curr_ev = EVENT_PWR_OOM;
                DBG("____\n");
                break;
            }
            // 第一次加快到 3 次
            SYSLOG("sim4g: @%s\n", g_sim4g_event[g_info_4g.curr_ev]);
            g_info_4g.cb_pwr_reset();
            Sim4g siminfo = {0};
            memcpy(&siminfo, &g_info_4g.SimInfo, sizeof(Sim4g));
            memset(&g_info_4g.SimInfo, 0, sizeof(Sim4g));
            g_info_4g.SimInfo.fdd = siminfo.fdd;
            g_info_4g.SimInfo.card = siminfo.card;
			g_info_4g.SimInfo.report_status = siminfo.report_status;
            strncpy(g_info_4g.SimInfo.token, siminfo.token, sizeof(g_info_4g.SimInfo.token));
            if (EVENT_FLY_OOM == g_info_4g.curr_ev) {
                if (E_TELECOM == siminfo.operators) {
                    g_info_4g.SimInfo.operators = E_MOBILE;
                } else if (E_MOBILE == siminfo.operators) {
                    g_info_4g.SimInfo.operators = E_TELECOM;
                }
            }
            DBG("operators:%d, SimInfo.operators:%d\n", siminfo.operators, g_info_4g.SimInfo.operators);
            g_info_4g.prev_ev = g_info_4g.curr_ev;
            g_info_4g.curr_ev = EVENT_WAITING;
            g_info_4g.next_ev = EVENT_INIT;
            g_info_4g.ms_step_pwr = 120*1000;
            g_info_4g.ms_step_wait = 10*1000;
            //4G模块异常走pwr_reset修复,标志位和/tmp/4g/*删除处理
            g_info_4g.model_type = E_UNINIT;
            UtilSystemCmd("rm -rf /tmp/4g/*;sync");
            ms_clock_reset(&g_info_4g.ms_clock_wait);
        } else {
            DBG("FSM @%s left %d sec\n", g_sim4g_event[g_info_4g.curr_ev], sec_left);
        }
        break;

    case EVENT_PWR_OOM:
    case EVENT_LNX_RESET:
        if (g_info_4g.lnx_reset < 4) {
            ++g_info_4g.lnx_reset;
            char nr[8] = {0};
            SYSLOG("FSM @%s\n", g_sim4g_event[g_info_4g.curr_ev]);
            DumpFile(SIM4G_LNX_REBOOT, itoa10(MIN(4,g_info_4g.lnx_reset), nr), strlen(nr));
            // 产测模式，或无 devid(SD卡与主板分离情况)，不重启
            if (!get_g_sys(factest) || !sim4g_get_security()) {
                DELAY_REBOOT_LINUX();
            }
            g_info_4g.prev_ev = g_info_4g.curr_ev;
            g_info_4g.curr_ev = EVENT_WAITING;
            g_info_4g.next_ev = EVENT_WAITING;
            g_info_4g.ms_step_wait = 10*1000;
        } else {
            DBG("FSM @EVENT_LNX_OOM, go back to EVENT_PWR_RESET\n");
            g_info_4g.curr_ev = EVENT_PWR_RESET;
        }
        break;

    case EVENT_WAITING: {
        if (ms_clock_is_timeup2(&g_info_4g.ms_clock_wait, g_info_4g.ms_step_wait, &sec_left)) {
            g_info_4g.curr_ev = g_info_4g.next_ev; // EVENT_REFRESH;
            DBG("FSM @waiting %s over go %s\n", g_sim4g_event[g_info_4g.prev_ev], g_sim4g_event[g_info_4g.next_ev]);
        } else {
            DBG("FSM @waiting %s left %d sec\n", g_sim4g_event[g_info_4g.prev_ev], sec_left);
        }
        break;
    }
    default:
        ERR("stat UNKNOW: %d\n", g_info_4g.curr_ev);
        break;
    }
    return;
err_init:
    g_info_4g.ripe = TRUE;
    syslog4g( __FILE__, __LINE__);
    return;
}

int sim4g_startup()
{
    // F_KILL_QUERY 存在时不起 4g, 可以 local.rc 中设置
    if (!get_g_sys(usb_4g) || is_okey(F_KILL_QUERY)) {
        DBG("4G module not detected or kill_q[%d]\n", is_okey(F_KILL_QUERY));
        return 0;
    }

    g_info_4g.eth_up = (net_link_status("eth0") == 1);

    /* 未老化完成，阻止烧录，除非 factest */
    if (!(g_info_4g.eth_up || get_g_sys(factest) || get_aging8h_pass())) {
        SYSLOG("do not enter burn mode cuase not ever aging8h and not factest\n");
        return 0;
    }

    SYSLOG("init 4g...\n");

    static struct cmdstat cmdstat_sim4g; 
    struct cmdstat *ctx = &cmdstat_sim4g;
    cmdstat_sim4g.diff_cfg2cmd = diff_cfg2cmd;

    if (is_okey(SIM4G_LNX_REBOOT)) {
        char nr[8] = {0};
        LoadFile(SIM4G_LNX_REBOOT, nr, sizeof(nr)-1);
        g_info_4g.lnx_reset = MAX(0, atoi(nr));           // 防止负数
    }

    if (is_okey(SIM4G_ACC_SHIFT)) {
        char nr[8] = {0};
        LoadFile(SIM4G_ACC_SHIFT, nr, sizeof(nr)-1);
        g_info_4g.acc_shift = MAX(0, MIN(2,atoi(nr)));    // 0,1,2  1X 2X 4X 时间
    }

    g_info_4g.sch = js_create_scheduler((char *)__func__);

    get_config(handleSim4gCfg, g_info_4g.SimInfo);

    g_info_4g.refresh = 1;
    g_info_4g.csq_tick = 0;
    g_info_4g.inside_sim = 8;
    g_info_4g.SimInfo.dbm = 4;
    g_info_4g.SimInfo.esim_dbm = 4;
    g_info_4g.curr_ev = EVENT_INIT;
    g_info_4g.model_type = E_UNINIT;
    g_info_4g.SimInfo.operators = g_info_4g.SimInfo.card;
    if (is_okey(F_SIM_REVERSE)) {
        if (g_info_4g.SimInfo.operators == E_TELECOM) {
            g_info_4g.SimInfo.operators = E_UNKNOWN;
        } else {
            g_info_4g.SimInfo.operators = E_TELECOM;
        }

        WAR("sim reverse, will use SIM%d\n", g_info_4g.SimInfo.operators);
    }

    g_info_4g.SimInfo.sim_status = E_CARD_UNINIT;
    g_info_4g.SimInfo.report_status = E_REPORT_UNINIT;
    g_info_4g.video_workable = TRUE;

    ms_clock_reset(&g_info_4g.ms_clock_fly);
    ms_clock_reset(&g_info_4g.ms_clock_pwr);
    ms_clock_reset(&g_info_4g.ms_clock_wait);
    g_info_4g.ms_step_fly = 0;       // 第一次允许立即执行
    g_info_4g.ms_step_pwr = 0;       // 第一次允许立即执行

    attach_event_async(JEvent_AlarmCabDis     , cb_cable_out, (void *)ctx); // sim4g_eth0_check_cb
    attach_event_async(JEvent_AlarmCableNormal, cb_cable_in , (void *)ctx);
    attach_event_async(JEvent_Sim4gLocation   , cb_location , (void *)ctx);
    attach_config(JEvent_Sim4g                , cb_refresh  , (void *)ctx); // sim4g -act set -online 有效
    attach_config(JEvent_UpdateBegin          , cb_update   , (void *)ctx);

    js_create_timer_r(g_info_4g.sch, 500, 1000, (JSTCFunc)sim4g_loop, (void *)ctx, &g_info_4g.hdl);

    return 0;
}

