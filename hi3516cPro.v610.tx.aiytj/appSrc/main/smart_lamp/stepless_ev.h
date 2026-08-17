
#ifndef _STEPLESS_EV_H
#define _STEPLESS_EV_H
#ifdef __cplusplus
extern "C" {
#endif

#include "jconfstruct.h"

#ifdef STEPLESS_PWM
#define MS_LAMP 200
#else
#define MS_LAMP 400
#endif

#define DUTY_CYCLE_RATE         24
#define LIGHT_OFF               0
#define LIGHT_INTENSITY_FACE    300
#define LIGHT_INTENSITY_FULL    1000

#define N2SEC       (2*1000/MS_LAMP + 1)
#define MASK2SEC    ((1<<(N2SEC+1))-1)

#define DIFF_P(a,b) ((MAX(a,b)-MIN(a,b))*100.0/((a+b)/2))

#define DUTY_MAX    (1000)
#define DUTY_FAINT  (3)
#define DUTY_DECI   (20)
#define DUTY_INIT   (5*DUTY_DECI)
#define is_duty_deci(duty) (duty == DUTY_DECI || duty == (DUTY_DECI+1))


/*
| NO. | secen                       | EV      |
| :-- | :------                     | :------ |
| 1   | 1m白墙 开灯 deci            |         |
| 2   | 1m白墙 开灯 faint           |         |
| 3   | 2m高度 开灯 deci            | 180     |
| 4   | 2m高度 开灯 deci，人脸 30cm | 190     |
| 5   |                             |         |
*/

// 环境亮度 BV 极限值场景，不同 sensor 需修改此值
#define RAW_EV_MAX  (2400000)   // 办公室对着大灯场景
#define RAW_EV_MIN  (700000)    // 暗房小房间全黑场景
#define BV_FACEAE   (1600000)   // 人脸收光阈值

// 194/1030000  364/1320000
#define EV_LUX5     (364)   // 作为 target_ev，小于此 EV，人来了要开灯，1为警示，2为防止拖影
#define EV_LUX2     (194)   // 最低开灯阈值

/*
 * EV > 波动系数(越小表明距离)，则 __reset2deci
 * 3m 高度, K = 5% =(190-180)/180, deci 180 人脸 190, K= 10/(l(200)/l(10)) = 10/2.3 = 4
 * 1m 白墙, K =
 **/
#define K_FLUT_RESET    (10/log10(MAX(10, sir.ev_deci)/*1~3*/))
#define K_FLUT_FAINT    2.5
#define K_FLUT_DECI     1.5 // 更严格

/* 镜面系数，越大则表明越近，背景越光滑(光利用率高)
 * 1m 白墙 (deci,faint) 600,300
 * 大于此值，只能走补光后，灯变亮差值判断
 * 小于此值，远距离，低反射，走 duty==0 判断
 **/
#define K_MIRROR    1.8

/*
 * ev 与 Lux 成正比
 **/
typedef struct {
    int   d_duty;         // 占空比
    int   cnt_deci;
    int   cnt_faint;
    float d_ev;           // target-ev
    float ev_deci;        //
    float ev_faint;       //
    float ev_darkest;     //
                          // ---------------------- reset tag
    int   duty;           // 10, 极夜 1000, 极昼  
    int   a_duty[10];
    float k;
    float k_break;        // 稳定范围系数
    float ev0;            // 原始环境亮度值 BV
    float ev;             // 映射值
    float ev_force_night; // 频切稳定
    float ev_tbl[10];     // 300ms*10, ev_tbl[1] = ev_prev
    float ev_lux2;        // 黄昏, 2Lux，开始开灯
    float ev_lux5;        // 黎明, 4Lux，开始走关灯逻辑，人脸靠近，镜头转移到高反射
    float ev_max;       // 当前sensor所支持的最高ev实际值，超过最高上限之后的ev值统一换算成0
    float ev_min;       // 当前sensor所支持的最低ev实际值，超过最低下限之后的ev值统一换算成1000
    float ev_light_off;   // 关灯阈值，根据 target 目标亮度的百分比
    float bv_faceae_off;  // 人脸收光阈值    
    float fctargetratio;

    int   adjustable;     // 自动调光开关
    int   pwm_percent;    // 灯光亮度等级，0~100
    struct timespec ms_force_night;
                          // ---------------------- 只 init 一次
    int   nr;             // tick 计数
    float k_lux;          // 开灯系数
} _sir;

int pwm_stepless_adjust(int alg, int *is_force_night);
float get_invert_ev();
int get_light_condition(void);
float log_base_max(void);

void sir_init(LightExtCfg light);

#ifdef __cplusplus
}
#endif
#endif

