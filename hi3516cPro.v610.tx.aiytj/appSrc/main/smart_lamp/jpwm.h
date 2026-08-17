/*
 *       Filename:  jpwm.h
 *    Description:  
 *        Version:  1.0
 *        Created:  12/08/2022 09:28:07 PM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  xiangyp (), 
 *   Organization:  
 */

#ifndef _JPWM_H_
#define _JPWM_H_
#ifdef __cplusplus 
extern "C" {
#endif
typedef enum {
    PWM_CHN0      = 0,
    PWM_CHN1      = 1,
    PWM_CHN2      = 2,
    PWM_CHN3      = 3,
    PWM_CHN4      = 4,
    PWM_CHN5      = 5,
    PWM_CHN6      = 6,
    PWM_CHN7      = 7,
    PWM_CHN_CNT   = 8,
} ePwmChn;

#define PWM_WHITE PWM_CHN1
#define PWM_WAYTRONIC_CLK PWM_CHN0

#define POLARITY_NORMAL     "normal"
#define POLARITY_INVERSED   "inversed"
#define PWM_PERIOD          (24000)  //根据实际情况设定
#define PWM_MIN_DUTY_CYCLE  (24)     //根据实际调整

/*
 * @desc: 打开或关闭指定 pwm 设备
 * @param: chn，0~7，分别对应 pwm0~pwm7
 * @param: exports，0|1，0 关闭，1 打开
 * @return: SUCCESS|FAILURE
 */
int pwm_open_export(int chn, int exports);

/*
 * @desc: 使能指定 pwm 通道
 * @param: chn，0~7，分别对应 pwm0~pwm7
 * @param: enable，0|1，0 关闭，1 打开
 * @return: SUCCESS|FAILURE
 */
int pwm_enable_chn(int chn, int enable);

/*
 * @desc: 获取指定 pwm 通道是否使能
 * @param: chn，0~7，分别对应 pwm0~pwm7
 * @param: enable，0|1，0 未使能，1 已使能
 * @return: SUCCESS|FAILURE
 */
int pwm_get_chn_enable(int chn, int *enable);

/*
 * @desc: 设置指定 pwm 通道频率
 * @param: chn，0~7，分别对应 pwm0~pwm7
 * @param: period，pwm 频率，如果此时 duty_cycle 已大于 0，不能小于 duty_cycle
 * @return: SUCCESS|FAILURE
 */
int pwm_set_period(int chn, int period);

/*
 * @desc: 获取指定 pwm 通道频率
 * @param: chn，0~7，分别对应 pwm0~pwm7
 * @param: period, 返回的 pwm 频率
 * @return: SUCCESS|FAILURE
 */
int pwm_get_period(int chn, int *period);

/*
 * @desc: 设置指定 pwm 通道极性
 * @param: chn，0~7，分别对应 pwm0~pwm7
 * @param: polarity, 设置的极性，POLARITY_NORMAL|POLARITY_INVERSED，分别代表正极和负极
 * @return: SUCCESS|FAILURE
 */
int pwm_set_polarity(int chn, const char *polarity);

/*
 * @desc: 获取指定 pwm 通道极性
 * @param: chn，0~7，分别对应 pwm0~pwm7
 * @param: polarity, 返回的极性，POLARITY_NORMAL|POLARITY_INVERSED，分别代表正极和负极
 * @param: size，polarity 数组的大小
 * @return: SUCCESS|FAILURE
 */
int pwm_get_polarity(int chn, char *polarity, int size);

/*
 * @desc: 设置指定 pwm 通道占空比
 * @param: chn，0~7，分别对应 pwm0~pwm7
 * @param: duty_cycle, 占空比，可设置为 0~period 的大小，不能超过 period
 * @return: SUCCESS|FAILURE
 */
int pwm_set_duty_cycle(int chn, int duty_cycle);

/*
 * @desc: 获取指定 pwm 通道占空比
 * @param: chn，0~7，分别对应 pwm0~pwm7
 * @param: duty_cycle, 返回的占空比大小
 * @return: SUCCESS|FAILURE
 */
int pwm_get_duty_cycle(int chn, int *duty_cycle);

#ifdef __cplusplus
}
#endif
#endif
