/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : g_run.h
 * @Created Time : 2023-03-24
 * @Version      : 1.0
 * @Author       : hul zhangj
 * @Description  : 代码内部，不会有对变量设置，
 *               : e.g. 老版 RUN_AUDIO_PLAY_ON RUN_AUDIO_PLAY_OFF
 *               : e.g. 新版 AUDIO_PLAY_ON AUDIO_PLAY_OFF
 * 编码规则      :
 *               : 1. 使用 UTF-8 编码
 *               : 2. 不使用 TAB，而使用 4空格代替
 *               : 3. g_run.h g_stat.h enum 格式如下，方便自动生成 Help 文档
 *               :    typedef enum RunMod {
 *               :        RUN_XXX = 1<<0,
 *               :    } eRunMod;
 */
#ifndef _G_RUN_H
#define _G_RUN_H
#ifdef __cplusplus
extern "C" {
#endif

typedef enum RunCommon {
    RUN_GET = 1<<30,
    RUN_POP = 1<<31,
} eRunCommon;
typedef enum RunVenc {
    RUN_VENC0  = 1<<0,          // 打印 chn-[0~9] 参数: gok, iFr, iMax, fps, itv
    RUN_VENC1  = 1<<1,
    RUN_VENC2  = 1<<2,
    RUN_VENC3  = 1<<3,
    RUN_VENC4  = 1<<4,
    RUN_VENC5  = 1<<5,
    RUN_VENC_BEFORE_SAVE  = 1<<6,      // 开启抓拍
    RUN_VENC_AFTER_SAVE   = 1<<7,      // 开启抓拍
} eRunVenc;

typedef enum RunAudio {
    RUN_AUDIO_PLAY_ON    = 1<<0,  // 音频开启
    RUN_AUDIO_PLAY_OFF   = 1<<1,  // 音频关闭
    RUN_AUDIO_SAVE       = 1<<2,  // 保存输出音频流
    RUN_AUDIOIN_SAVE     = 1<<3,  // 保存输入音频流
    RUN_AUDIO_PLAY_FORCE = 1<<4,  // 播放指定 g_run.code 的提示音
    RUN_AUDIO_CAPTURE    = 1<<5,  // 捕获音频准备
    RUN_AUDIO_PLAY_NUM   = 1<<6,
    RUN_AUDIO_CONTINUE   = 1<<9,  /* 继续执行 jco_server */
    RUN_AUDIO_AAC        = 1<<19,
} eRunAudio;

typedef enum RunSim4g {
    RUN_DBM_WEAK    = 1<<0, //模拟4G信号差
    RUN_WWW_FAIL    = 1<<1, //模拟阿里连接失败
    RUN_PAUSE_4G    = 1<<2, //暂停4G策略
    RUN_PRINT_4G    = 1<<3, //立即打印4G信息
    RUN_RESTART_TX  = 1<<4, //模拟tx重连
    RUN_ESIM_FAIL   = 1<<5, //模拟内置卡失败
    RUN_CHIP_FAIL   = 1<<6, //模拟4G模块失败
    RUN_SIM4G_LOG   = 1<<7,
    RUN_REPORT_FAIL          = 1<<8, //模拟4G上报失败
    RUN_REPORT_LOCATION_FAIL = 1<<9, //模拟4G定位上报失败
    RUN_CPIN_FAIL            = 1<<10, //模拟4G cpin FAIL
} eRunSim4g;

typedef enum RunWifi {
    RUN_PAUSE_WIFI  = 1<<0, //暂停WIFI策略
    RUN_PRINT_WIFI  = 1<<1, //立即打印WIFI信息
    RUN_W_CHIP_FAIL = 1<<2, //模拟wifi模块失效
    RUN_BLE_START   = 1<<3, // 开启ble
    RUN_BLE_INIT    = 1<<4, // cmd_init(code) 0:反初始化 1:初始化
    RUN_BLE_ADD     = 1<<5, // cmd_addservice
    RUN_BLE_ADVERT  = 1<<6, // cmd_advert(1,code,12) code为ble_name
    RUN_BLE_SEND    = 1<<7, // cmd_send(code, "ABCD") code为ble中UUID
} eRunWifi;

typedef enum RunDbg {
    RUN_DBG_DUP2FILE = 1<<0,   // stdio 日志重定向到 /tmp/messages.dot 或 sdcard
    RUN_DBG_REBOOT   = 1<<10,   // 模拟一次 reboot 前 sleep 300s 
    RUN_DBG_SIGSEGV  = 1<<11,   // 模拟一次 SEGV, 11 是 SEGV 的信号代码
    RUN_SAVE_TMPJPEG = 1<<12,   // 将JPEG抓拍的图片存储在/tmp目录下
    RUN_DBG_WATCH    = 1<<19,   // print /dev/kmsg size, always 64KB
} eRunDbg;

typedef enum RunTencent {
    RUN_SDK_LOG_LEVEL   = 1<<0, // code-> IV_eLOG_DEBUG:4, INFO:3,WARN:2,ERROR:1,DISABLE:0
    RUN_P2P_STATUS     = 1<<1,
    RUN_CS_QURAY       = 1<<2,
    RUN_XP2P_LOG_LEVEL = 1<<3, // iv_sys_log_level_type_e: {4,3,2,1,0}
    RUN_OTA_BLOCK      = 1<<4, // 模拟 ota_download_size is block
} eRunTencent;

typedef enum RunAliMmi {
    RUN_MMI_LOG_LEVEL   = 1<<0,
} eRunAliMmi;

typedef enum RunPwm {
    RUN_PWM_PRINT_EV       = 1<<0, // always print ev
    RUN_PWM_LOAD_X         = 1<<1, // load /tmp/x
    RUN_PWM_CHN0_ENABLE    = 1<<2, // 0-disable 1-enable
    RUN_PWM_CHN_ENABLE_GET = 1<<3, // 0-chn0  1-chn1
    RUN_PWM_DUTY_CYCLE_GET = 1<<4, // get duty
    RUN_PWM_DUTY_CYCLE_SET = 1<<5, // set duty  0~1000
} eRunPwm;

typedef enum RunJcp {
    RUN_JCP_BATCH_STEP   = 1<<0, // print batch debug
    RUN_JCP_BATCH_TIME   = 1<<1, // print batch time
    RUN_JCP_SINGL_TIME   = 1<<2, // print singl time
    RUN_JCP_P2P_TIME     = 1<<3, // print _p2p_ time
    RUN_JCP_PRI_OUTPUT   = 1<<8, // print jcp RAW output
} eRunJcp;

typedef enum RunRecord {
    RUN_RECORD_DENTRY    = 1<<0, // lookupdir() 打印列表
    RUN_RECORD_ERR_WR    = 1<<1, //
} eRunRecord;

typedef enum RunLamp {
    E_RUN_LAMP_FORCE_DAY        = 1<<0, // 自动模式调试，强制白天
    E_RUN_LAMP_FORCE_NIGHT      = 1<<1, // 自动模式调试，强制晚上
    E_RUN_LAMP_GET_EV           = 1<<2,
    E_RUN_IRCUT_DAY             = 1<<3, // ircut 切到白天状态
    E_RUN_IRCUT_NIGHT           = 1<<4, // ircut 切到夜视状态
    E_RUN_LAMP_VERBOSE          = 1<<9, // 减少打印
} eRunLamp;

typedef enum RunIvx {
    RUN_IVX_PRINT_EVERY_INFO       = 1<<0, // print every frame information
    RUN_IVX_PRINT_STATI            = 1<<1, // print statistics information
    RUN_IVX_PRINT_NOISE_INFO_ONCE  = 1<<2, // print noise box info once
    RUN_IVX_PERSON_FILTER          = 1<<8, // 人形过滤
    RUN_IVX_DBG                    = 1<<9,
    RUN_IVX_TRACK                  = 1<<11,
    RUN_IVX_FOLLOW_OSD             = 1<<12,
    RUN_IVX_PRINT_FOLLOW_STAT      = 1<<13,
} eRunIvx;

#ifdef __cplusplus
}
#endif
#endif
