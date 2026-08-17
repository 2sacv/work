/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : sim4g.h
 * @Created Time : 2023-3-10
 * @Version      : 3.0
 * @Author       : hul
 * @Description  :
 */
#ifndef __SIM4G_H_
#define __SIM4G_H_

#include <pthread.h>
#include "jconfstruct.h"
#include "js_scheduler.h"

#ifdef __cplusplus
extern "C" {
#endif
#define AT_TWO_TIMES        2
#define AT_TIMES            3
#define AT_TIMEOUT          3
#define BEST_4G_SIG_VAL  (31.0)
#define SIM4G_CHK           "/mnt/4g_chk.txt"
#define F_ATI               "/tmp/4g/ATI"
#define F_IMEI              "/tmp/4g/imei"
#define F_ICCID             "/tmp/4g/iccid"
#define F_IMSI              "/tmp/4g/imsi"
#define F_EIMSI             "/tmp/4g/eimsi"
#define F_EICCID            "/tmp/4g/esim_iccid"
#define F_INSIDE_SIM        "/tmp/4g/inside_sim"
#define F_LATITUDE          "/opt/conf/4g/latitude"
#define F_LONGITUDE         "/opt/conf/4g/longitude"
#define F_ID_VEN            "/opt/conf/4g/idVendor"
#define F_OPT_IMEI          "/opt/conf/4g/imei"
#define F_KILL_QUERY        "/tmp/4g/kill_q"
#define F_SIM_REVERSE       "/mnt/sim_reverse.txt"

#define SIM4G_CHECK_USB0    "/sys/class/net/usb0"
#define SIM4G_AT_TTY4G      "/dev/tty4G"
#define TAG_KILL_ACK        "/tmp/4g/kill_a"
#define TAG_KILL_Q          "/tmp/4g/kill_q"
#define OTA_VENDOR          "/tmp/4g/ota_vendor"
#define SIM4G_LNX_REBOOT    "/opt/conf/4g/lnx_reboot"
#define SIM4G_ACC_SHIFT     "/opt/conf/4g/acc_shift"
#define USB_MOVE            "/sys/bus/usb/devices/1-1/remove"
#define USBDEV_IDVENDOR     "/sys/bus/usb/devices/1-1/idVendor"
#define USBDEV_IDPRODUCT    "/sys/bus/usb/devices/1-1/idProduct"
#define SIM4G_AT_TXBYUGA    "/sys/class/net/usb0/statistics/tx_bytes"

typedef enum {
    INSIDE_SIM = 1,         // 只检内卡
    OUTSIDE_SIM,            // 只检外卡
    INSIDE_OUTSIDE_SIM,     // 检测双卡
} cHKName;

typedef enum {
    MODLE_NULL= 0,  // 未注网
    MODLE_FDD,      // FDD制式
    MODLE_TDD,      // TDD制式
} Model;

/*返回值：0：成功， -1：失败 */
typedef int (*sim_init)(void *data);

/*返回值：0：成功， -1：失败 */
typedef int (*sim_flymode)(void);

/*返回值：0：成功， -1：失败 */
typedef int (*sim_pwr_reset)(void);

/*返回值：0：成功， -1：失败 */
typedef int (*sim_factory)(void *data);

typedef enum {
    EVENT_INIT         = 0,         // 初始化状态
    EVENT_REFRESH      = 1,         // 正常循环刷新
    EVENT_FLY_RESET    = 2,         // 所有 www 不通，执行飞行模式
    EVENT_FLY_OOM      = 3,         // FLY 次数达到 12
    EVENT_PWR_RESET    = 4,         // 模块和 simcard 探测失败，GPIO 控制模块重启
    EVENT_PWR_OOM      = 5,         // GPIO 重启次数达到 12
    EVENT_LNX_RESET    = 6,         // Linux 重启
    EVENT_WAITING      = 7,         // 各个事件执行时等待
    EVENT_MAX
} eSim4gEvent;

typedef enum {
    E_UNINIT = 0,
    E_EC800E = 1,
    E_EC801E,
    E_USBBOOT,
    E_NT26,
    E_YGX09,
} eModuleType;

// 卡运营商
typedef enum {
    E_UNKNOWN = 0,
    E_TELECOM = 1,  // 电信
    E_MOBILE  = 2,  // 移动
    E_UNICOM  = 3,  // 联通
} eOperatorsType;

// 卡状态
typedef enum {
    E_CARD_UNINIT = -1,
    E_CARD_UNNORMAL = 0,
    E_CARD_NORMAL = 1,
} eSimStatus;

// 卡上报状态
typedef enum {
    E_REPORT_UNINIT = -1,
    E_REPORT_FAIL = 0,              // 异常
    E_REPORT_SUCCESS = 1,           // 成功
    E_REPORT_CHANGE_CARD = 2,       // 换卡
    E_REPORT_IMEI_EXCEPTION = 3,    // IMEI不在库
    E_REPORT_SIGN_EXCEPTION = 4,    // 签名验证失败
    E_REPORT_OPERATOR_NOT_MATCH = 5,// SIM卡运营商不匹配
    E_REPORT_CARD_NOT_EXIST = 6,    // SIM卡不在库
    E_REPORT_CARD_USED = 7,         // SIM卡已被使用
} eReportStatus;

typedef struct {
    int sim;            // -1 plugin-out, 0, 1
    int e_sim;
    int online;
    int is4G;           // -1 invalid, 0 3G, 1 4G
    int esim_is4G;
    int dbm;            // signal strength sim4g_at_dbm_get()
    int esim_dbm;
    int signal;
    int connected;
    int txBpsec;        // upload
    int rxBpsec;        // download
    char ip[16];        // 0.0.0.0
    char iccid[32];
    char esim_iccid[32];
    char imsi[32];
    char eimsi[32];
    char fw_version[32];
    char spnname[20];
    char imei[20];
    int fdd;
    int is_fdd;
    int card;
    eOperatorsType operators;
    eSimStatus sim_status;
    char sim_card[32];
    char esim_card[32];
    eReportStatus report_status;
    char token[64];     // 移远-16位 域格-32位
    char longitude[32];
    char latitude[32];
    int location_enable;
    eOperatorsType card_type;           // 卡 2 类型
    eOperatorsType ecard_type;          // 卡 1 类型
} Sim4g;

typedef struct {
    JSScheduler         sch;
    JSTCHandle          hdl;
    int                 cmd_stage;
    int                 csq_tick;                   //csq刷新次数
    int                 www_tick;
    pthread_mutex_t     mutex;                      //

    int                 fly_reset;                  //flymode 软复位
    int                 pwr_reset;                  //硬复位次数
    int                 lnx_reset;                  //linux Reboot
    int                 acc_shift;                  //
    struct timespec     ms_clock_fly;               //飞行模式闹钟。
    struct timespec     ms_clock_pwr;               //硬复位模式闹钟。
    struct timespec     ms_clock_wait;              //飞行模式闹钟。
    int                 ms_step_www;                //
    int                 ms_step_fly;                //执行间隔，第一次可以立即执行，后面要满足间隔 300s
    int                 ms_step_pwr;                //
    int                 ms_step_wait;               //

    int                 simcard0;
    int                 chipset0;
    int                 sim_ever;
    int                 inside_sim;
    int                 chip_ever;
    int                 initialized;
    int                 www_alive_ever;
    int                 cid_succ;                   //include iccid,imei,

    eSim4gEvent         prev_ev;                    //wait 耗时打印
    eSim4gEvent         curr_ev;                    //4G状态
    eSim4gEvent         next_ev;                    //wait 时使用

    char                vendor_name[12];

    Sim4g               SimInfo;                    //4G 信息。
    int                 dbm[3];                     //保存3次dbm;
    int                 csq;
    int64_t             tx_bytes[3];
    int64_t             rx_bytes[3];                //3次采样，防止 ping 失效时，误杀自身

    int                 ripe;                       //已执行 /ipc/bin/4g LTE start
    int                 refresh;                    //刷新标识 dBm，弃用
    int                 updating;                   //升级中
    int                 eth_up;                     //是否有线插入
    int                 www_fail_cnt;               //
    int                 www_alive;                  //外网可 ping 通
    int                 video_workable;             //判断绑定卡是否出图， 0：不出图，1：出图
    int                 iccid_check;                //iccid 0代表非定向卡，1代表捷高定向卡
    int                 iccid_changed;              //iccid 0代表iccid没改变, 1代表iccid变化
    int                 imei_check;                 //imei  0代表旧模组，  1代表新模组
    eModuleType         model_type;                 //4G模块类型。
    sim_init            cb_init;                    //初始化，确认跟模块通讯正常。
    sim_flymode         cb_flymode;                 //飞行模式
    sim_pwr_reset       cb_pwr_reset;               //硬复位
    sim_factory         cb_factory;                 //产测模式
} sim_4g_t;

typedef struct {
    int  code;
    char device_id[32];
    char mac[64];
    char cpu[32];
    TripleInfoS triple;
} sBurnArg;

int sim4g_get_operator_by_iccid(char *iccid, eOperatorsType *operator_type);

int sim4g_startup();

int sim4g_get_stat(Sim4g *data);

const char *get_soft4g_chipname();

int dbm_strong();
int sim4g_get_sim_4g(sim_4g_t *data);


int simcard_workable();

int sim4g_run_AT_clr(const char *AT_no_CR, char *buf, int len);
int sim4g_run_AT_clr_sec(int sec, const char *AT_no_CR, char *buf, int len);
int sim4g_video_turnon();
int sim4g_video_turnoff();
int sim4g_video_workable();
int sim4g_is_mobile_card( char *iccid_file, eOperatorsType *operator_type);
void syslog4g(const char *file, int lineno);
int ha_open_ttyusb0();

extern int usbdev_busy;

#ifdef __cplusplus
}
#endif
#endif
