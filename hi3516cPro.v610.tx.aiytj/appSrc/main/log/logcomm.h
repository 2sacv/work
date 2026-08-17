/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : logcomm.h
 * @Created Time : 2013-10-16
 * @Version      : 1.0
 * @Author       : zengy
 * @Description  :
 */

#ifndef __LOGCOMM_H_
#define __LOGCOMM_H_

typedef enum {
    JLogBegain = -1,
    JLogPtz = 0x00,
    JLogAlarm = 0x01,
    JLogPrint       ,  //普通记录消息
} JLogType;

typedef enum {
    LevelBegain = -1,
    JLevelDebug = 0x00,
    JLevelWarning = 0x01,
    JLevelFatal = 0x02,
} JlogLevel;

typedef enum {
    JPtzSubBegain = -1,
    JPtzSubControl = 0x00, //用户操作
    JPtzSubAlarmLink = 0x01, //报警联动
} JPtzSubType;

typedef enum {
    AlarmSubBegain = -1,
    JAlarmSubMotionDec = 0x00,
    JAlarmSubIO = 0x01,
    JAlarmSubVideoLoss = 0x02,
    JAlarmSubExtend = 0x03,
} JAlarmSubtype;

#endif

