/**
 * Copyright (C) by Jabsco Company
 * 
 * @File Name    : alarm_event.h
 * @Created Time : 2014-03-12
 * @Version      : 1.0
 * @Author       : zengy
 * @Description  : 
 */

#ifndef _ALARM_EVENT_H_
#define _ALARM_EVENT_H_

#define NEED_TIME_CHECK    1
#define NO_NEED_TIME_CHECK 0

#define NEED_TIME_CHECK 1
#define DONT_TIME_CHECK 0

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

typedef enum
{
    JALARM_TYPE_BEGIN = 0,

    JALARM_TYPE_MD              =  1, // 1 移动
    JALARM_TYPE_VGLINE          =  2, // 2 越线
    JALARM_TYPE_VGRECT          =  3, // 3
    JALARM_TYPE_VL              =  4, // 4
    JALARM_TYPE_DISK_FULL       =  5, // 5

    JALARM_TYPE_DISK_ERR        =  6, // 6
    JALARM_TYPE_CABLE_DISC      =  7, // 7
    JALARM_TYPE_IP_CONFLICT     =  8, // 8
    JALARM_TYPE_ILLEGAL_ACCESS  =  9, // 9
    JALARM_TYPE_AI              = 10, // 10

    JALARM_TYPE_EXP             = 11, // 11
    JALARM_TYPE_MASK            = 12, // 12
    JALARM_TYPE_HUMAN_DETECT    = 13, // 13 人形识别
    JALARM_TYPE_FACE            = 14, // 14
    JALARM_TYPE_PLATE           = 15, // 15

    JALARM_TYPE_PET             = 16, // 16
    JALARM_TYPE_HUMAN_MIX       = 17, // 17
    JALARM_TYPE_EBIKE           = 18, // 18
    JALARM_TYPE_THROW           = 19, // 19 高空抛物
    JALARM_TYPE_CAR             = 20, // 20 车型

    JALARM_TYPE_CIGARETTE       = 21, // 21 香烟
    JALARM_TYPE_PASSENGER_FS    = 22, // 22 客流
    JALARM_TYPE_FALL            = 23, // 23 跌倒
    JALARM_TYPE_BARCODE         = 24, // 24 条形码
    JALARM_TYPE_FACESNAP        = 25, // 25
    JALARM_TYPE_SCENE_CHANGE    = 26, // 26 画面变化
    JALARM_TYPE_CRY             = 27, // 27哭声检测

    JALARM_TYPE_END,
    JALARM_TYPE_CHECK = 0x10000,
}JALARM_TYPE;

typedef struct {
    int channel;
    int itemindex;
    int itemnum;
    char stime[64];
    char etime[64];
    JALARM_TYPE type;
}QueryInfoS;

typedef struct {
    int chn;
    JALARM_TYPE type;
    int filter;
    char desc[128];
}ALARM_INFOS;

typedef struct {
    int           channel;
    JALARM_TYPE   type;
}ALARM_RECORD_EVENTS;

typedef struct {
    JALARM_TYPE   type;
    int recordEn;
    int captureEn;
    int ftpEn;
    int emailEn;
}ALARM_RECORD_EVENTS_MB;

/*
* 1. 顺序和数值要跟云平台物模型保持一致,且新产品物模型需要继承之前的事件类型定义
* 2. 32 已用于上报无效事件(例如:双目事件云存设备,如果枪机触发报警,需要上报两路云存,
* 腾讯就有两条报警事件, 使用 32 将球机的事件类型上报给app将球机报警过滤)
* 3. 物模型只会对真实报警通道上报报警, 事件云存有多少路就有多少次报警事件,
* 通过app过滤后保持和物模型一致
* 4. 请勿使用32作为实际报警类型
*/
typedef enum {
    ALARM_MD  = 1,
    ALARM_VGR = 2,
    ALARM_HD  = 3,
    ALARM_VGL = 4,
    ALARM_CAR = 5,
    ALARM_VMASK = 7,
    ALARM_SCENE = 14,
    ALARM_PET = 15,
    ALARM_CRY = 16,
    ALARM_CALL_CANCEL   = 100,//视频通话已取消
    ALARM_CALLING       = 101,//视频通话来电
    ALARM_CALL_END      = 102,//正常呼叫结束
    ALARM_CALL_MISSED   = 103,//视频通话未接听
    ALARM_CALL_REJECT   = 104,//拒绝来电
    ALARM_END,
} AlarmTypeE;

#endif

