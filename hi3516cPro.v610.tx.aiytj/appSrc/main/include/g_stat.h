/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : g_stat.h
 * @Created Time : 2023-03-24
 * @Version      : 1.0
 * @Author       : hul zhangj
 * @Description  : 代码内部，会被其它模块引用
 */
#ifndef _G_STAT_H
#define _G_STAT_H
#ifdef __cplusplus
extern "C" {
#endif

typedef enum StatRecord {
    SD_ERR_READ          = 1<<0, // 读错误
    SD_ERR_WRITE         = 1<<1, // 写错误
    SD_ERR_UNAUTH        = 1<<2,  // 机卡未绑定, auto_mount.sh 会引用此值
    SD_ERR_MOUNT         = 1<<4,  // 有节点但 mount 失败
    SD_ERR_MMCNODE       = 1<<5,  // 无节点
    SD_ERR_WRITE_PROTECT = 1<<6,  // 写保护
    SD_ERR_STOP          = 1<<7,  // 停止T卡业务
    SD_REC_FORMAT        = 1<<11, // 格式化
    SD_REC_RENAME        = 1<<13, // rename 录像标记，用来查询录像列表的时候快速刷新列表
    SD_REC_TMP_REPAIR    = 1<<15, // 修复临时录像文件
    SD_REC_CLOSEING      = 1<<16, // 关闭录像文件中
    SD_CD_IN             = 1<<19, // 存在SD卡
    SD_REPORTED          = 1<<20, // 上报
    SD_ERR_ACCESS3       = (SD_ERR_MOUNT|SD_ERR_READ|SD_ERR_WRITE),  // 访问异常，可RESCAN
} eStatRecord;

typedef enum StatWifi {
    WIFI_STA        = 1<<0, // station模式
    WIFI_AP         = 1<<1, // ap模式
    WIFI_PASSWDERR  = 1<<2, // 密码错误
    WIFI_DHCPSUCC   = 1<<3, // DHCP成功
    WIFI_DHCPONCE   = 1<<4, // DHCP成功过一次
    WIFI_TESTING    = 1<<5, // 测试模式
    WIFI_WEP        = 1<<6, // 老式五位加密方式WEP
    WIFI_MATCHGOT   = 1<<7, // 对码
} eStatWifi;

typedef enum StatSim4g {
    SIM4G_DHCPONCE  = 1<<0, // DHCP成功过一次
} eStatSim4g;

typedef enum StatCrop {
    CROP_SC         = 1<<0, // 开启裁剪
    CROP_ZOOM       = 1<<1, // 数字变倍中
} eStatCrop;

typedef enum StatTencent {
    TENCENT_INVALID  = 1<<0, // P2P 非法
    TENCENT_BAND     = 1<<1, // 绑定
} eStatTencent;
#ifdef __cplusplus
}
#endif
#endif
