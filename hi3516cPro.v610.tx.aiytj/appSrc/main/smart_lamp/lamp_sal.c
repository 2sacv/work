/* 
 *       Filename:  lamp_sal.c
 *    Description:  
 *        Version:  1.0
 *        Created:  11/07/2025 11:38:56 AM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  wangr (), 
 *   Organization:  
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/* file */
#include <fcntl.h>
#include <sys/file.h>

/* socket() */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "debug.h"
#include "utils.h"
#include "jevent.h"
#include "lamp_sal.h"
#include "lamp_hal.h"
#include "lamp_main.h"
#include "encodeapi.h"

#include "ss_mpi_ae.h"
#include "jpwm.h"
#include "stepless_ev.h"
#include "encode_od.h"

int lamp_get_ev_lux(LightExtCfg *lamp)
{
    int ret = 0;
    int vi_pipe = 0;
    ot_isp_exp_info exp_info = {0};

    ret = ss_mpi_isp_query_exposure_info(vi_pipe, &exp_info);
    if(ret != SUCCESS) {
        ERR("ss_mpi_isp_get_exposure_attr failed, ret=0x%x.\n", ret);
    }

    lamp->fccurev = exp_info.exposure / 1000.0;
    return ret;
}

uint32_t get_invert_exposure(void)
{
    int ret = SUCCESS;
    int vi_pipe = 0;
    ot_isp_exp_info exp_info = {0};

    ret = ss_mpi_isp_query_exposure_info(vi_pipe, &exp_info);
    if (SUCCESS != ret) {
        ERR("ss_mpi_isp_query_exposure_info failed, ret: 0x%x\n", ret);
        return 0;
    }

    return exp_info.exposure;
}

/**
 * 全彩夜视切白天: 
 * 红外夜视切白天: 
 */
int lamp_set_day(int change_light)
{
    SYSLOG("SET LAMP DAY!!! @ %.2lf\n", mono_stamp());

    encode_od_freeze();

    set_lamp_status(GPIO_INFRARED, GPIO_WHITE_OFF); // 关红外灯

    if (change_light) {
#ifdef STEPLESS_PWM
        pwm_set_duty_cycle(PWM_WHITE, 0);
#else
        set_lamp_status(GPIO_WHITE, GPIO_WHITE_OFF);    // 关白光灯
#endif
    }

    set_ircut_status(1);                            // ircut 红外截止
    send_event_chn(JEvent_RunIspColor, ISP_DAY);    // 切换为白天的效果
    set_lamp_switch_time();

    return SUCCESS;
}

/**
 * 白天切全彩夜视: 
 * 红外夜视切全彩夜视: 
 */
int lamp_set_color(int change_light)
{
    SYSLOG("SET LAMP NIGHT COLOR!!! @ %.2lf\n", mono_stamp());

    encode_od_freeze();

    set_ircut_status(1);                                    // ircut 红外截止

    if (change_light) {
#ifdef STEPLESS_PWM
        pwm_set_duty_cycle(PWM_WHITE, DUTY_FAINT * DUTY_CYCLE_RATE);
#else
        set_lamp_status(GPIO_WHITE, GPIO_WHITE_ON);             // 开白光灯
#endif
    }

    set_lamp_status(GPIO_INFRARED, GPIO_WHITE_OFF);         // 关红外灯
    send_event_chn(JEvent_RunIspColor, ISP_COLOR_NIGHT);    // 切换为全彩夜视的效果
    set_lamp_switch_time();

    return SUCCESS;
}

/**
 * 白天切红外夜视: 
 * 全彩夜视切红外夜视: 
 */
int lamp_set_night(void)
{
    SYSLOG("SET LAMP NIGHT IR!!! @%.2lf\n", mono_stamp());

    encode_od_freeze();

    send_event_chn(JEvent_RunIspColor, ISP_INFRARED_NIGHT); // 切换为红外夜视的效果
#ifdef STEPLESS_PWM
    pwm_set_duty_cycle(PWM_WHITE, 0);
#else
    set_lamp_status(GPIO_WHITE, GPIO_WHITE_OFF);    // 关白光灯
#endif
    set_ircut_status(0);                                    // ircut 全透
    set_lamp_status(GPIO_INFRARED, GPIO_INFRARED_ON);          // 开红外灯
    set_lamp_switch_time();

    return SUCCESS;
}
