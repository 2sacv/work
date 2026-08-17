#ifdef PLATFORM_TENCENT
#ifndef _TENCENT_SERVER_H_
#define _TENCENT_SERVER_H_


#ifdef __cplusplus
extern "C" {
#endif

#include "tencent_param_conf.h"
#include "g_sys.h"
#include "g_run.h"
#include "g_log.h"
#include "g_stat.h"
typedef struct {
    int id;             //设备id
    int online;         //设备是否上线
    int inited;         //设备是否初始化
} TXDevParamS;

typedef enum {
    CONNECTING = 0,
    ONLINE     = 1,
    OFFLINE    = 2,
} TXNetworkStatus;

enum {
    DEV_QIU    = 0,
    DEV_QIANG  = 1,
    DEV_MAX    = 2,
};

typedef enum {
    E_TX_INIT_PROG_NONE    = 0,
    E_TX_INIT_PROG_START   = 1,
    E_TX_INIT_PROG_DONE    = 2,
} eTXInitProg;

#define MAX_SENSOR_NUM 1    // sensor 数量
#define MAX_CONNECT_NUM 3   // 最大拉流人数
#define MAX_VENC_NUM 3      // 0 流畅, 1 高清, 2 超清
#define MAX_STREAM_CHANNEL_NUM 2
#define TENCENT_REPLAY_MAX_CHN MAX_SENSOR_NUM*MAX_CONNECT_NUM

#define MAX_SIZE_OF_DEV_SECRET   (64)
#define SYS_NULL_OF_DEV_SERCERT  "YOUR_DEVICE_SERCERT"

#define PID_OF_DHCP_WLAN        "/var/network/dhcpc.pid.wlan"
#define PID_OF_DHCP_ETH0        "/var/network/dhcpc.pid.eth0"

int is_tencent_eth0_linked(void);
int tencent_format_sd_card(void);
TripleInfoS* tencent_get_triple_info(void);
int is_tencent_on_line(void);
void tencent_reconnect_cb(int id, void *p_src, int size, void *ctx);
void tencent_set_device_bind(BOOL status);
void do_cs_reset(void);
int init_tencent(void);
int uninit_tencent(void);
void uninit_tencent_async(void);
void mlock_xp2p_code(void);
void mfree_xp2p_code(void);

#ifdef __cplusplus
}
#endif

#endif //_TENCENT_SERVER_H_
#endif //PLATFORM_TENCENT
