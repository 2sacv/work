/*
 *       Filename:  tencent_video_call.h
 *    Description:  
 *        Version:  1.0
 *        Created:  03/19/2026 03:29:15 PM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  xiangyp (), 
 *   Organization:  
 */

#ifndef _TENCENT_VIDEO_CALL_H
#define _TENCENT_VIDEO_CALL_H
#ifdef __cplusplus 
extern "C" {
#endif

#include "js_scheduler.h"

typedef enum {
    CMD_VC_VIDCALLCFG = 1 << 0 , //video call 配置改变
    CMD_VC_VIDEOCFG   = 1 << 1 , //video 配置改变
    CMD_VC_DO_VIDCALL = 1 << 2 , //外部触发 video call
} eTXVidCallCmd;

typedef enum {
    E_STAT_IDLE        = 0, //未通话，空闲状态
    E_STAT_WAITHANGUP  = 1, //通话时意外断联，等待通话挂断/指令超时
    E_STAT_RINGING     = 2, //呼叫中
    E_STAT_CALLING     = 3, //通话中
    E_STAT_COUNT,
} eCallStatus;

typedef enum {
    E_EVT_CALL_NONE     = -1, //接听方未执行任何操作
    E_EVT_CALL_HANGUP   = 0,  //拨打方或者接听方挂断
    E_EVT_CALL_REJECT   = 0,  //接听方拒接
    E_EVT_CALL_PICKUP   = 1,  //接听方接听
    E_EVT_CALL_DISCON   = 2,  //接听方意外断联
    E_EVT_CALL_TORESP   = 3,  //接听方超时未操作
    E_EVT_CALL_LAUNCH   = 4,  //拨打方发起通话请求
    E_EVT_CALL_CANCEL   = 5,  //拨打方呼叫时取消/通话时挂断通话
    E_EVT_CALL_TODROP   = 6,  //接听方意外断联后超时未发送挂断请求
    E_EVT_CALL_COUNT,
} eCallEvt;

int init_tencent_video_call(JSScheduler sch);

void uninit_tencent_video_call(void);

bool call_evt_enqueue(eCallEvt evt_call);

void wechat_call_evt_enqueue(int cmd);

void toggle_video_call(void);

void set_video_call_ready(int is_ready);

int get_video_call_status(void);

#ifdef __cplusplus
}
#endif
#endif
