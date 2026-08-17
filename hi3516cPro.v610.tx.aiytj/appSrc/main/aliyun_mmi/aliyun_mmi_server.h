/*
 *       Filename:  aliyun_mmi_server.h
 *    Description:  阿里云百炼MMI系统级初始化接口
 *        Version:  1.0
 *         Author:  xiangyp
 */

#ifndef _ALIYUN_MMI_SERVER_H
#define _ALIYUN_MMI_SERVER_H
#ifdef __cplusplus
extern "C" {
#endif

#include "js_scheduler.h"

#define MMI_WSS_HOST     "dashscope.aliyuncs.com"
#define MMI_WSS_PORT     (443)

typedef enum {
    E_STEP_INIT_NONE    = 0,
    E_STEP_INITIALIZING = 1,
    E_STEP_INIT_OK      = 2,
    E_STEP_DEINIT_START = 3,
} eInitStep;

typedef struct {
    int           tz_synced;
    int           triple_key_loaded;
    eInitStep     init_step;
    JSScheduler   sch_evt;
    JSScheduler   sch_pack_aud;
    JSScheduler   sch_rx_aud;
    JSTCHandle    hdl_wss;
    JSTCHandle    hdl_pack_aud;
    JSTCHandle    hdl_rx_aud;
    pthread_t     init_thread;
    struct cmdstat *ctx;
} sMmiRun;

extern sMmiRun g_run_mmi;

/* 网络就绪后初始化MMI模块(License全托管模式)，返回0成功 */
int init_aliyun_mmi(void);
int uninit_aliyun_mmi(void);

int aliyun_mmi_tz_synced(void);

int ali_mmi_system_inited(void);

void ali_mmi_system_start_dialog(void);

#ifdef __cplusplus
}
#endif
#endif /* _ALIYUN_MMI_SERVER_H */
