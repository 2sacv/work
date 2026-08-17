/*
 *       Filename:  encode_noise.h
 *    Description:  
 *        Version:  1.0
 *        Created:  09/11/2024 10:39:12 AM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  wangr (), 
 *   Organization:  
 */
#ifndef __ENCODE_NOISE_H__
#define __ENCODE_NOISE_H__
#ifdef __cplusplus 
extern "C" {
#endif

#include "ot_common_aidetect.h"
#include "encode_base_ivx.h"
#include "encode_ivp_aidetect.h"

#define HD_ALARM_ALLOW_RANGE_X        0.15  //x 轴容差系数
#define HD_ALARM_ALLOW_RANGE_Y        0.15  //y 轴容差系数
#define HD_ALARM_ALLOW_RANGE_DXY      0.50
#define PIXEL_MIN_MOVED               5
#define AIDET_MAX_TYPE                4     //最多支持侦测类型数量（35 实际只有三个）
#define AIDET_FILTER_MAX_NUM          10    //最多支持同时10个目标过滤
#define CNT_TOLENRENCE_COUNT          50    //连续静止帧数

#define HD_ALARM_ALLOW_OVERLAP        0.70  //重合阈值
#define HD_BLACKLIST_LEAVE_OVERLAP    0.50  //解开黑名单阈值

typedef struct {
    int w;
    int h;
    int x_center;
    int y_center;
    int cnt_appeared;       //出现次数
    int cnt_tolenrence;     //连续满足位移在一定范围内的次数
    int is_black;           //标记黑名单
    int black_list_idx;     //黑名单数组内的索引,每个目标只与一个位置匹配,防止多个黑名单位置挨在一起
} sRepeatedObj;

typedef struct {
    int x;
    int y;
    int w;
    int h;
    int existence;      //是否有记录
    int age;            //数据保留时长
    int match_count;    //被命中次数
} sBlackList;

void encode_ivp_object_filter(ot_aidetect_result_array *p_result, struct ivx_cfg* ivx_info);

#ifdef __cplusplus
}
#endif
#endif

