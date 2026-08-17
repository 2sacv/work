/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : airlink.h
 * @Created Time : 2023-04-11
 * @Version      : 2.0
 * @Author       : hul
 * @Description  :
 */

#ifndef __AIRLINK_H__
#define __AIRLINK_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "debug.h"
#include "js_scheduler.h"
#include "jconfstruct.h"

#define F_BLE                   "/tmp/ble"
#define PID_OF_DHCP_WLAN        "/var/network/dhcpc.pid.wlan"
#define PID_OF_DHCP_ETH0        "/var/network/dhcpc.pid.eth0"
#define SUPPLICANT_OK_CONF      "/opt/conf/airlink/supplicantOK.conf"
#define TMP_SUPPLICANT_CONF     "/tmp/supplicant.conf"
#define WLAN0_SUPPLICAN         "/var/run/wlan0_supplicant"
#define WIFI_LNX_REBOOT         "/opt/conf/wifi/lnx_reboot"
#define WIFI_ACC_SHIFT          "/opt/conf/wifi/acc_shift"
#define USBDEV_IDVENDOR         "/sys/bus/usb/devices/1-1/idVendor"
#define USBDEV_IDPRODUCT        "/sys/bus/usb/devices/1-1/idProduct"

typedef enum {
    E_AP_STA        = 0,    // ap+station
    E_STA           = 1,    // station
    E_WPA_RESET     = 2,    // wpa_reset
    E_WPA_RESET_OOM = 3,    // wpa_reset_oom
    E_DRV_RESET     = 4,    // drv_reset
    E_DRV_RESET_OOM = 5,    // drv_reset_oom
    E_LNX_RESET     = 6,    // lnx_reset
    E_REFRESH       = 7,    // refresh
    E_WAITING       = 8,    // waiting
    E_WMAX
} eWIFIEvent;

typedef struct {
    JSScheduler      sch;
    JSTCHandle       hdl;
    struct cmdstat   *p_ctx;
    pthread_mutex_t  mutex;

    int              wpa_reset;         // wpa复位次数
    int              drv_reset;         // 硬复位次数
    int              lnx_reset;         // linux Reboot
    int              acc_shift;         // 加速
    struct timespec  ms_clock_wpa;      // wpa 重启闹钟。
    struct timespec  ms_clock_drv;      // 硬复位模式闹钟。
    struct timespec  ms_clock_wait;     // 飞行模式闹钟。
    int              ms_step_www;       //
    int              ms_step_wpa;       // 执行间隔，第一次可以立即执行，后面要满足间隔 120s
    int              ms_step_drv;       //
    int              ms_step_wait;      // 

    eWIFIEvent       prev_ev;           // wait 耗时打印
    eWIFIEvent       curr_ev;           // WIFI 状态
    eWIFIEvent       next_ev;           // wait 时使用

    int              ripe;              // 已执行 wifi sta reset
    int              chip_ever;         // wifi 模块存在标识

    int              quality;           // wifi 信号质量
    char             wifiscan[4096];    // wifi 列表
    int              quality_flag;      // wifi 质量更新标志
    int              wifiscan_flag;     // wifi 列表更新标志
    int              initialized;       // 初始化
    int              updating;          // 升级中
    int              eth_up;            // 是否有线插入
    int              www_fail_cnt;      // p2p_plat4m 失败次数
    int              www_alive;         // 外网可 ping 通
} sWIFIDev;

typedef enum {
    ASSOCIATING = 1,
    DISCONNECTED = 2,
    SCANNING = 3,
    _4WAY_HANDSHAKE = 4,
    COMPLETED = 5,
    INTERFACE_DISABLED = 6,
} eWFIFIStat;

typedef enum {
    _VLD_COMPLTED = 0,
    _VLD_ERRPASSWD,
    _VLD_WEAKSIGNAL,
    _VLD_UNKNOW,
} eVLDStat;

struct wifi_cfg {
    NetWifiS wificfg;
    NetEthS  ethcfg;
};

int get_wifiexist(void);
int get_wifi_status(void);
int get_wifi_quality(void);
int get_wifi_hotspot_list(char *data);
char *get_ssid(const char *file, char ssid[]);
int wifi_startup(void);
int wifi_stop(void);
void reset_udhcpc(char nic);
int wifi_reset_factory(void);
const char *supplicant_conf(void);

#ifdef __cplusplus
}
#endif

#endif
