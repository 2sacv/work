/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : g_sys.h
 * @Created Time : 2023-03-24
 * @Version      : 1.0
 * @Author       : hul zhangj
 * @Description  : 最常用基本状态，被 log run stat 依赖
 */
#ifndef _G_SYS_H
#define _G_SYS_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

// 模块定义
typedef struct Mod {
    int venc;       // 编码
    int audio;      // 音频
    int record;     // 录像
    int jcp;        // JCP
    int sim4g;      // 4G
    int wifi;       // WiFi
    int rtsp;       // stream 流转发
    int search;     // 搜索服务
    int upgrade;    // 升级
    int http;       // WEB
    int onvif;      // ONVIF
    int tencent;    // Tencent P2P
    int lamp;       // 三光
    int alarm;      // 告警
    int md;         // 移动侦测
    int hd;         // 人形侦测
    int mask;       // mask
    int osd;        // OSD
    int ptz;        // 云台+马达
    int gb28181;    // 国标
    int netcheck;   // netcheck
    int gpio;       // gpio
    int isp;        // isp
    int pwm;        // pwm模块
    int cry;        // cry detect
    int vidcall;    // video call
    int od;         // 遮挡告警
    int vidmask;    // 隐私遮挡
    int ivx;        // ivx
    int mmi;        // 阿里云百炼MMI
    int asr;        // 音频处理芯片
    int dbg;        // 日志重定向，调试杂项
    int code;       // 供 grun 命令带参数使用
    int crop;       // 裁剪状态
    int face;       // face
} sMod;

// 全布尔类型，自动探测，高可移植性的代码
typedef struct Sys {
    int usb_4g;     // 4g类设备
    int usb_asix;   // usb转eth模式
    int usb_wifi;   // WiFi类设备
    int eth;        // eth=0 无 ethernet，或禁用

    int jz;         // 君正方案
    int df;         // 多方方案
    int fh;         // 富瀚方案
    int ax;         // 爱芯方案
    int hs;         // 海思方案

    int maxheight;  // 拉升后幅面高度
    int testing;    // 形态开 is_test_ver，防止动态改变，导致 version 改变
    int factest;    // 产测，三个高频繁特性，会有上电设置，也潜在未来中间设置
    int agingtest;  // 老化
    int upgrading;  // 升级
} sSys;

#define __FF__ 0xFFFFFFFF

// sys
#define set_g_sys(key)  __set_g_sys(offsetof(sSys, key), 1)
#define clr_g_sys(key)  __set_g_sys(offsetof(sSys, key), 0)
#define get_g_sys(key)  __get_g_sys(offsetof(sSys, key))

// log
#define set_g_log(key)  __set_g_log(offsetof(sMod, key), 1)
#define clr_g_log(key)  __set_g_log(offsetof(sMod, key), 0)
#define get_g_log(key)  __get_g_log(offsetof(sMod, key))

// run
#define fet_g_run(key)       __get_g_run(offsetof(sMod, key), __FF__)
#define set_g_run(key, bit)  __set_g_run(offsetof(sMod, key), bit)
#define clr_g_run(key, bit)  __clr_g_run(offsetof(sMod, key), bit)
#define get_g_run(key, bit)  __get_g_run(offsetof(sMod, key), bit)
#define pop_g_run(key, bit)  __pop_g_run(offsetof(sMod, key), bit)

// stat
#define fet_g_stat(key)       __get_g_stat(offsetof(sMod, key), __FF__)
#define set_g_stat(key, bit)  __set_g_stat(offsetof(sMod, key), bit)
#define clr_g_stat(key, bit)  __clr_g_stat(offsetof(sMod, key), bit)
#define get_g_stat(key, bit)  __get_g_stat(offsetof(sMod, key), bit)
#define pop_g_stat(key, bit)  __pop_g_stat(offsetof(sMod, key), bit)

void __set_g_sys(long offset, int val);
int  __get_g_sys(long offset);

void __set_g_log(long offset, int val);
int  __get_g_log(long offset);

void __set_g_stat(long offset, int bit);
void __clr_g_stat(long offset, int bit);
int  __get_g_stat(long offset, int bit);
int  __pop_g_stat(long offset, int bit);

void __set_g_run(long offset, int bit);
void __clr_g_run(long offset, int bit);
int  __get_g_run(long offset, int bit);
int  __pop_g_run(long offset, int bit);

void load_g_sys(void *ptr);
void dump_g_sys(void *ptr);
void load_g_log(void *ptr);
void dump_g_log(void *ptr);
void load_g_stat(void *ptr);
void dump_g_stat(void *ptr);
void load_g_run(void *ptr);
void dump_g_run(void *ptr);

void init_g_sys(void);

#ifdef __cplusplus
}
#endif
#endif
