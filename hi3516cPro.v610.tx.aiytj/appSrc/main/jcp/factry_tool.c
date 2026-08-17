/* 
 *       Filename:  factry_tool.c
 *    Description:  
 *        Version:  1.0
 *        Created:  10/30/2018 08:00:54 PM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  zhangjian (), 
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
#include "factry_tool.h"
#include "system_ctrl.h"
#include "system_sch.h"
#include "utils.h"
#include "conf_list.h"
#include "jconfig.h"

#include "jevent.h"
#include "confapi.h"
#include "encode_osd.h"
#include "ptz_ctrl.h"
#include "lamp_hal.h"

typedef enum {
    FAC_UNDEFINE            = 0,
    FAC_MIC_SPEAKER         = 1,
    FAC_LED_BLINK           = 2,
    FAC_LED_CLOSE           = 3,
    FAC_LED_WHITE_ON        = 4,
    FAC_LED_WHITE_OFF       = 5,
    FAC_LED_DOUBLEFLASF_ON  = 6,
    FAC_LED_DOUBLEFLASF_OFF = 7,
    FAC_LED_RED_ON          = 8,
    FAC_LED_RED_OFF         = 9,
} FACTRY_TOOL_E;

enum CHN {
    led_white_open            = 2,
    led_white_close           = 3,
    led_infrared_open         = 4,
    led_infrared_close        = 5,
    led_ircut_redglass_open   = 6,
    led_ircut_whiteglass_open = 7,
};

#define MOTOR_TEST_MAX_STEPS_PAN 20000
#define MOTOR_TEST_MAX_STEPS_TILT 10000

typedef struct {
    JSTCHandle  hdl_step;
    BOOL        sign;      // first create step timer
    MotorStatus pre_status;   // last position(P,T)
    OsdExpandS  osdExpand; // use 拓展字幕 0 1
} sCalcStep;

static JSTCHandle  hdl_mic = NULL;
static sCalcStep   g_step  = {0};

static void stop_mic_speaker(void *data)
{
    usleep(200*1000);
    UtilSystemCmd("nc 127.0.0.1 8004 < /tmp/mic.pcm &");
}

static void routin_mic_speaker(void)
{
    js_create_once(hdl_mic, sch_slow, 5*1000, stop_mic_speaker, NULL);
}

static void routin_led_close(void)
{
    //light_ctrl(LED_RED_OFF);
    //light_ctrl(LED_BLUE_OFF);
}

static void routin_led(void)
{
    //light_ctrl(LED_RB_BLINK);
    //create_ms_delaytask(g_cancel_scher, 5*1000, routin_led_close, NULL);
}

static void routin_led_white_open(void)
{
    send_event_chn(JEvent_LedTest, led_white_open);
}

static void routin_led_white_close(void)
{
    send_event_chn(JEvent_LedTest, led_white_close);
}

int set_light_doubleflash_value(int value)
{
#ifdef LIGHT_IO_ALARM
    int ret_value = 0;

    gpio_open_set_value(GPIO_RED_BLUE, value);
    gpio_open_get_value(GPIO_RED_BLUE, &ret_value);
    DBG("set_light_doubleflash_value: %d\n",ret_value);
#endif

    return 0;
}

static void routin_led_red_open(void)
{
#if defined(OPTICS_ZOOM)|| defined(DIGITAL_ZOOM)
    send_event_chn(JEvent_LedTest, led_infrared_open);          // 开红外灯
#endif
}

static void routin_led_red_close(void)
{
#if defined(OPTICS_ZOOM)|| defined(DIGITAL_ZOOM)
    send_event_chn(JEvent_LedTest, led_infrared_close);       // 关红外灯
#endif
}

int get_step_debug_status(void)
{
    return (g_step.hdl_step == NULL?FALSE:TRUE);
}

void encode_osd_expand_update_cb(void *data)
{
    MotorStatus status;
    get_motor_status(&status);

    if (g_step.sign || status.steps[PAN_MOTOR] != g_step.pre_status.steps[PAN_MOTOR] || status.steps[TILT_MOTOR] != g_step.pre_status.steps[TILT_MOTOR]) {
        char string[128] = {0};
        snprintf(string, sizeof(string) - 1, "水平步数: %d", status.steps[PAN_MOTOR]);
        memcpy(g_step.osdExpand.cusosd[E_OSD_EXPEND_PAN].content, string, sizeof(string));

        memset(string, 0, sizeof(string));
        snprintf(string, sizeof(string) - 1, "垂直步数: %d", status.steps[TILT_MOTOR]);
        memcpy(g_step.osdExpand.cusosd[E_OSD_EXPEND_TILT].content, string, sizeof(string));

        //send_conf_data(JEvent_OsdExpandCfgChg, &g_step.osdExpand, sizeof(OsdExpandS));
        conf_set_osdexpandcfg(g_step.osdExpand);
        memcpy(&g_step.pre_status, &status, sizeof(MotorStatus));
    }


    g_step.sign = FALSE;
}

int init_step_param(void)
{
    conf_get_osdexpandcfg(&g_step.osdExpand);

    // 显示字幕
    if (g_step.osdExpand.cusosd[E_OSD_EXPEND_PAN].enable != TRUE || g_step.osdExpand.cusosd[E_OSD_EXPEND_PAN].x != 50 ||
       g_step.osdExpand.cusosd[E_OSD_EXPEND_PAN].y != 900 ||
       g_step.osdExpand.cusosd[E_OSD_EXPEND_TILT].enable != TRUE || g_step.osdExpand.cusosd[E_OSD_EXPEND_TILT].x != 50 ||
       g_step.osdExpand.cusosd[E_OSD_EXPEND_TILT].y != 1000) {
        g_step.osdExpand.cusosd[E_OSD_EXPEND_PAN].enable = TRUE;
        g_step.osdExpand.cusosd[E_OSD_EXPEND_PAN].x = 50;
        g_step.osdExpand.cusosd[E_OSD_EXPEND_PAN].y = 920;
        g_step.osdExpand.cusosd[E_OSD_EXPEND_TILT].enable = TRUE;
        g_step.osdExpand.cusosd[E_OSD_EXPEND_TILT].x = 50;
        g_step.osdExpand.cusosd[E_OSD_EXPEND_TILT].y = 1000;
//        send_conf_data(JEvent_OsdExpandCfgChg, &g_step.osdExpand, sizeof(OsdExpandS));
        conf_set_osdexpandcfg(g_step.osdExpand);
    }

    // 限制最大步数
    motor_t motor = {0};
    conf_get_motorcfg(&motor);

    motor.h_maxstep = MOTOR_TEST_MAX_STEPS_PAN;
    motor.v_maxstep = MOTOR_TEST_MAX_STEPS_TILT;
    send_conf_data(JEvent_MotorCfg, &motor, sizeof(motor_t));

    return 0;
}

static int start_test_step(void)
{
    SYSLOG("start step debug\n");

    // 刷新 P T 信息
    if (g_step.hdl_step == NULL) {
        g_step.sign = TRUE;
        init_step_param();
        js_create_timer_r(sch_slow, 200, 200, encode_osd_expand_update_cb, NULL, &g_step.hdl_step);
    } else {
        DBG("test step timer is exist\n");
    }

    return 0;
}

static int end_test_step(void)
{
    SYSLOG("end step debug\n");
    
    memset(&g_step.pre_status, 0, sizeof(MotorStatus));
    js_delete_timer_r(&g_step.hdl_step);

    // 恢复成原来的最大步数
    motor_t motor = {0};
    conf_get_motorcfg(&motor);

    send_conf_data(JEvent_MotorCfg, &motor, sizeof(motor_t));

    // 清除字幕
    conf_get_osdexpandcfg(&g_step.osdExpand);
    g_step.osdExpand.cusosd[E_OSD_EXPEND_PAN].enable = FALSE;
    g_step.osdExpand.cusosd[E_OSD_EXPEND_TILT].enable = FALSE;

//    send_conf_data(JEvent_OsdExpandCfgChg, &g_step.osdExpand, sizeof(OsdExpandS));
    conf_set_osdexpandcfg(g_step.osdExpand);
    return 0;
}

int factry_tool_handle_step(int step)
{
    switch(step) {
        case 0:  end_test_step();                    break;
        case 1:  start_test_step();                  break;
        case 2:  set_motor_zero();                   break;
        default: printf("__unknow__ factry step\n"); break;
    }

    return 0;
}

int factry_tool_main(int code)
{
    switch (code) {
        case FAC_MIC_SPEAKER:           routin_mic_speaker();               break;
        case FAC_LED_BLINK:             routin_led();                       break;
        case FAC_LED_CLOSE:             routin_led_close();                 break;
        case FAC_LED_WHITE_ON:          routin_led_white_open();            break;
        case FAC_LED_WHITE_OFF:         routin_led_white_close();           break;
        case FAC_LED_DOUBLEFLASF_ON:    set_light_doubleflash_value(1);     break;
        case FAC_LED_DOUBLEFLASF_OFF:   set_light_doubleflash_value(0);     break;
        case FAC_LED_RED_ON:            routin_led_red_open();              break;
        case FAC_LED_RED_OFF:           routin_led_red_close();             break;
        default: printf("__unknow__ factry code\n");                        break;
    }
    return 0;
}

