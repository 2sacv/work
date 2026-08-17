/*
 *       Filename:  lamp_main.h
 *    Description:  
 *        Version:  1.0
 *        Created:  11/07/2025 11:39:21 AM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  wangr (), 
 *   Organization:  
 */
#ifndef __LAMP_MAIN_H__
#define __LAMP_MAIN_H__
#ifdef __cplusplus 
extern "C" {
#endif

#include "jconfstruct.h"
#include "time.h"

#define ADC_PATH    "/dev/ingenic_adc_aux_"     /* adc channal 0-3 */
#define STD_VAL_VOLTAGE 1800    /* The unit is mv/1000. T10/T20 VREF=3300; T30/T21/T31/T40 VREF=18000 */

/* define ioctl command. they are fixed. don't modify! */
#define ADC_ENABLE      0
#define ADC_DISABLE     1
#define ADC_SET_VREF    2

/* Hard Photosensitive Algorithm Map Function Parameters */
#define ADC_HARD_MAX    500     //灯板硬件adc最大电压值
#define ADC_HARD_MIN    0       //灯板硬件adc最小电压值
#define ADC_MAP_MAX     1000    //映射后adc最大电压值,实际判断时会使用映射后的值,可能进行正反比转换
#define ADC_MAP_MIN     0       //映射后adc最小电压值

#define INTERVAL_TIME   (3*60)   //判断的时间段，单位：s
#define SWITCH_LIMITE   (5)      //在判断时间段内，切换次数达到该值，认定为频切
#define PAUSE_TIME      (15*60)  //出现频切后琐死的时间，单位：s
#define W_FACEAE        (60)

typedef enum {
    E_STATUS_INVALID = -1,
    E_STATUS_NIGHT   =  0,    // 夜晚 - 红外夜视
    E_STATUS_DAY     =  1,    // 白天 
    E_STATUS_COLOR   =  2,    // 白光 - 全彩夜视
} eLampStatus;

// 灯光控制模式
typedef enum {
    E_LAMP_AUTO     = 0,
    E_LAMP_TIMING   = 1,
} eLampCtrl;

// 灯光模式
typedef enum {
    E_LAMP_INFRARED     = 0,
    E_LAMP_WHITE        = 1,
    E_LAMP_SMART        = 2,
    E_LAMP_MANUAL       = 3,
} eLampMode;

// 手动灯光模式下可以选择的控制模式
typedef enum {
    E_MANUAL_FORCENIGHT     = 0,
    E_MANUAL_FORCEDAY       = 1,
} eManualMode;

struct sLampCfg {
    LightExtCfg  lightext;
    DaynightCfgS daynight;
    AudioAlarmS  audioalarm;
    LightAlarmS  lightalarm;
    IOAlarmS     ioalarm;
    DriveOut     driveout;
};

struct sLampRun {
    JSScheduler   sch;
    JSTCHandle    hdl_loop;
    JSTCHandle    hdl_shineoff;
    JSTCHandle    hdl_io;
    JSTCHandle    hdl_drvout;
    JSTCHandle    hdl_audio;
    struct cmdstat *p_ctx;

    int           nightled_prev;
    int           is_alarm;        // 是否触发告警
    int           is_driveout;     // 现在是否是驱赶报警
    int           is_day_curr;     // 当前设备状态，0-夜晚 1-白天 2-白光
    int           is_day_prev;     // 上一个设备状态
    int           hard_cnt;        // 硬光敏日夜切换计数
    int           is_force_night;  // 是否启用防反复切
    int           is_lamp_change;  // 模式或算法是否有改变
    int           test;
    int           frozen_cnt;

    int           atComp;
    int           is_faceae;
    int           do_alm_faceae;
    time_t        switch_time;     // 红外白光灯切换时间，用来做报警过滤，单位:s
    struct timespec ms_clock_lampwh;
};

int is_lamp_testing(void);
int get_day_curr(void);
int get_lamptype(void);
time_t get_lamp_switch_time(void);
void set_lamp_switch_time(void);
int lamp_server_init(void);
int lamp_server_uninit(void);
int lamp_is_color_mode(void);

#ifdef __cplusplus
}
#endif
#endif // __LAMP_MAIN_H__

