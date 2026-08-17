/* 
 *       Filename:  lamp_main.c
 *    Description:  
 *        Version:  1.0
 *        Created:  11/07/2025 11:38:34 AM
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
#include "lamp_hal.h"
#include "lamp_sal.h"
#include "lamp_main.h"
#include "jpwm.h"
#include "stepless_ev.h"

#include "utils.h"
#include "g_sys.h"
#include "g_run.h"
#include "g_log.h"
#include <limits.h>
#include "cmdstat.h"
#include "confapi.h"
#include "system_sch.h"
#include "system_ctrl.h"
#include "encode_audio_queue.h"
#include "encode_audio_output.h"
#include "lamp_smart_photo_sens.h"
#include "encode_ivp_aidetect.h"
#include "encodeapi.h"
#include "encode_videomask.h"

//最大 MS_ADJUST_NIGHTLED_MAX 内必须调整完亮度
//#define MS_ADJUST_NIGHTLED_MAX (1000)
//#define DUTY_ADJUST_ONCE    (DUTY_CYCLE_RATE * DUTY_MAX / (MS_ADJUST_NIGHTLED_MAX / MS_LAMP))
#define PWM_CYCLE_RATE         (240)
#define DUTY_ADJUST_ONCE       (120)    //实际情况修改
enum {
    E_CMD_LAMP_MODE           = 1 << 0,
    E_CMD_AUDIO_ALARM         = 1 << 1,
    E_CMD_LIGHT_ALARM         = 1 << 2,
    E_CMD_LIGHT_ALARM_TIME    = 1 << 3,  // 灯光报警时长
    E_CMD_IO_ALARM            = 1 << 4,
    E_CMD_DRIVEOUT            = 1 << 5,
    E_CMD_LAMP_ALARM          = 1 << 6,
    E_CMD_LAMP_FREEZ          = 1 << 7,
    E_CMD_LAMP_LIGHTEXT       = 1 << 8,
};

static struct sLampCfg cfg = {0};
static struct sLampCfg raw = {0};
static struct sLampRun run = {0};
static struct sLampCfg *g_cfg_lamp = &cfg;
static struct sLampCfg *g_raw_lamp = &raw;
static struct sLampRun *g_run_lamp = &run;

static int exec_cmd_lightextcfg(void);

#define MODE_COLOR (LAMP_WHITE == g_cfg_lamp->lightext.lamptype || \
                    g_cfg_lamp->lightext.nightled > 0 ||            \
                    g_run_lamp->nightled_prev > 0)    //全彩模式

static void cb_ioalarm_cfg(int id, void *p_src, int size, void *ctx)
{
    CPY2CMDCFG(E_CMD_IO_ALARM, &g_raw_lamp->ioalarm, p_src, size);
}

static void cb_lamp_mode_cfg(int id, void *p_src, int size, void *ctx)
{
    CPY2CMDCFG(E_CMD_LAMP_MODE, &g_raw_lamp->daynight, p_src, size);
}

static void cb_lamp_irctrl_cfg(int id, void *p_src, int size, void *ctx)
{
    CPY2CMDCFG(E_CMD_LAMP_LIGHTEXT, &g_raw_lamp->lightext, p_src, size);
}

static void cb_driveout_cfg(int id, void *p_src, int size, void *ctx)
{
    CPY2CMDCFG(E_CMD_DRIVEOUT, &g_raw_lamp->driveout, p_src, size);
}

static void cb_lightalarm_cfg(int id, void *p_src, int size, void *ctx)
{
    CPY2CMDCFG(E_CMD_LIGHT_ALARM, &g_raw_lamp->lightalarm, p_src, size);
}

static void cb_audioalarm_cfg(int id, void *p_src, int size, void *ctx)
{
    CPY2CMDCFG(E_CMD_AUDIO_ALARM, &g_raw_lamp->audioalarm, p_src, size);
}

static void cb_lamp_freez(int id, void *p_src, int size, void *ctx)
{
    CPY2CMD(E_CMD_LAMP_FREEZ);
}

static void cb_lamp_alarm(int id, void *p_src, int size, void *ctx)
{
    CPY2CMD(E_CMD_LAMP_ALARM);
}

static void cb_led_test(int id, void *p_src, int size, void *ctx)
{
    int chn = 0;
    if (p_src) chn = *(int *)p_src;

    DBG("chn: %d\n", chn);
    switch (chn) {
    case 0:
        g_run_lamp->test = FALSE;
        break;
    case 1:
        g_run_lamp->test = TRUE;
        break;
    case 2:
        set_lamp_status(GPIO_WHITE, GPIO_WHITE_ON);
        g_run_lamp->test = TRUE;
        break;
    case 3:
        set_lamp_status(GPIO_WHITE, GPIO_WHITE_OFF);
        g_run_lamp->test = FALSE;
        break;
    case 4:
        //set_infrared_light_intensity(LIGHT_INTENSITY_FULL);
        lamp_set_night();
        g_run_lamp->test = TRUE;
        usleep(500*1000);
        break;
    case 5:
        //set_infrared_light_intensity(LIGHT_OFF);
        lamp_set_day(TRUE);
        g_run_lamp->test = FALSE;
        usleep(500*1000);
        break;
    case 6:
        lamp_set_day(TRUE);
        break;
    case 7:
        lamp_set_night();
        break;
    case 8:
        lamp_set_color(TRUE);
        break;
    default:
        break;
    }

    return;
}

int is_lamp_testing(void)
{
    return g_run_lamp->test;
}

int get_day_curr(void)
{
    return g_run_lamp->is_day_curr;
}

int get_lamptype(void)
{
    return g_cfg_lamp->lightext.lamptype;
}

time_t get_lamp_switch_time(void)
{
    return g_run_lamp->switch_time;
}

void set_lamp_switch_time(void)
{
    g_run_lamp->switch_time = mono_sec();
    return;
}

static int is_day_in_timer(TimeSeg timeseg)
{
    time_t tNow = time(NULL);
    struct tm timeinfo = {0};
    localtime_r(&tNow,&timeinfo);
    int time_now = timeinfo.tm_hour * 60 + timeinfo.tm_min;

    int is_in_time = -1;
    int beginhour  = 0;
    int beginmin   = 0;
    int endhour    = 0;
    int endmin     = 0;

    beginhour = timeseg.beginhour;
    beginmin  = timeseg.beginmin;
    endhour   = timeseg.endhour;
    endmin    = timeseg.endmin;

    if ((beginhour*60 + beginmin) <= (endhour*60 + endmin)) {
        if ((time_now >= (beginhour*60 + beginmin)) && (time_now <= (endhour*60 + endmin)) )
            is_in_time = TRUE;
        else
            is_in_time = FALSE;
    } else {
        if ((time_now >= (beginhour*60 + beginmin)) || (time_now <= (endhour*60 + endmin)) )
            is_in_time = TRUE;
        else
            is_in_time = FALSE;
    }

    dbg_lamp("is_in_time=%d tm_now=%d [%02d:%02d] TimeSeg[%02d:%02d-%02d:%02d]\n",
        is_in_time, time_now, timeinfo.tm_hour, timeinfo.tm_min,
        timeseg.beginhour, timeseg.beginmin, timeseg.endhour, timeseg.endmin);

    return is_in_time;
}

static int lamp_check_daytime(LightExtCfg *lamp)
{
    int is_in_time = FALSE;

    time_t tNow = time(NULL);
    struct tm timeinfo = {0};
    localtime_r(&tNow, &timeinfo);
    int time_now = timeinfo.tm_hour * 60 + timeinfo.tm_min;

    if ((lamp->beginhour * 60 + lamp->beginmin) <= (lamp->endhour * 60 + lamp->endmin)) {
        if ((time_now >= (lamp->beginhour * 60 + lamp->beginmin)) &&
            (time_now <= (lamp->endhour * 60 + lamp->endmin))) {
            is_in_time = TRUE;
        }
    } else {
        if ((time_now >= (lamp->beginhour * 60 + lamp->beginmin)) ||
            (time_now <= (lamp->endhour * 60 + lamp->endmin))) {
            is_in_time = TRUE;
        }
    }

    dbg_lamp("is_in_time=%d time_now=%d [%02d:%02d] TimeSeg[%02d:%02d-%02d:%02d]\n",
        is_in_time, time_now, timeinfo.tm_hour, timeinfo.tm_min,
        lamp->beginhour, lamp->beginmin, lamp->endhour, lamp->endmin);

    return (!is_in_time);
}

static int lamp_check_lum_debug(int algo, int is_day_prev, int is_day_curr)
{
    char buf_algo[16] = {0};
    char buf_pre[16] = {0};
    char buf_cur[16] = {0};

    switch (algo) {
    case ALG_HARD:
        strlcpy(buf_algo, "HARD", sizeof(buf_algo));
        break;
    case ALG_SOFT:
        strlcpy(buf_algo, "SOFT", sizeof(buf_algo));
        break;
    case ALG_TIME:
        strlcpy(buf_algo, "TIME", sizeof(buf_algo));
        break;
    default:
        strlcpy(buf_algo, "INVALID", sizeof(buf_algo));
        break;
    }

    switch (is_day_prev) {
    case E_STATUS_NIGHT:
        strlcpy(buf_pre, "NIGHT", sizeof(buf_pre));
        break;
    case E_STATUS_DAY:
        strlcpy(buf_pre, "DAY", sizeof(buf_pre));
        break;
    case E_STATUS_COLOR:
        strlcpy(buf_pre, "COLOR", sizeof(buf_pre));
        break;
    default:
        strlcpy(buf_pre, "INVALID", sizeof(buf_pre));
        break;
    }

    switch (is_day_curr) {
    case E_STATUS_NIGHT:
        strlcpy(buf_cur, "NIGHT", sizeof(buf_cur));
        break;
    case E_STATUS_DAY:
        strlcpy(buf_cur, "DAY", sizeof(buf_cur));
        break;
    case E_STATUS_COLOR:
        strlcpy(buf_cur, "COLOR", sizeof(buf_cur));
        break;
    default:
        strlcpy(buf_cur, "INVALID", sizeof(buf_cur));
        break;
    }

    SYSLOG("[LAMP] [%s] [%s TO %s]\n", buf_algo, buf_pre, buf_cur);

    return SUCCESS;
}

/**
 * 夜晚 + 全彩 = 白光 
 * 1. 时间表算法下，全彩模式检查全彩时间，非全彩模式(包括智能模式)检查黑白时间
 * 2. 白光状态下，必须采用全彩算法，目前软光敏和硬光敏只适用于白天和夜晚
 * 3. 优先级: 时间表算法 > 白光状态全彩算法 > 软光敏算法 = 硬光敏算法
 * 4. 四种算法接口统一输出`白天`和`夜晚`两种状态，夜晚状态时会通过状态判断转为白光
 */
static int lamp_check_lum(void)
{
    static int is_day = TRUE;
    int alg = g_cfg_lamp->lightext.alg;

    //星光夜视走软光敏逻辑
    if (g_cfg_lamp->lightext.lamptype == LAMP_STAR) {
        alg = ALG_SOFT;
    } else if (g_cfg_lamp->lightext.nightled > 0 || g_run_lamp->nightled_prev > 0) {
        alg = ALG_SOFT;
    } else if (g_cfg_lamp->lightext.gt_1_forceday == 0) {
        alg = ALG_NIGHT;
    } else if (g_cfg_lamp->lightext.gt_1_forceday == 2) {
        alg = ALG_DAY;
    }

#ifdef STEPLESS_PWM
    get_invert_ev();
#endif

    switch (alg) {
    case ALG_TIME: {
        is_day = lamp_check_daytime(&g_cfg_lamp->lightext);
#ifdef STEPLESS_PWM
        if (MODE_COLOR && !is_day) {
            is_day = pwm_stepless_adjust(alg, &g_run_lamp->is_force_night);
        }
#endif
        break;
    }
    case ALG_SOFT: {
        if (videomask_enabled()) {
            break;
        }
#ifdef STEPLESS_PWM
        is_day = pwm_stepless_adjust(alg, &g_run_lamp->is_force_night); 
#else
        lamp_photosens_get_daynight(&is_day, &g_run_lamp->is_force_night);
#endif
        break;
    }
    case ALG_HARD: {
        //is_day = encode_lamp_photosensitive_luma();
        break;
    }
    case ALG_DAY: {
        is_day = TRUE;
        break;
    }
    case ALG_NIGHT: {
        is_day = FALSE;
        break;
    }
    default:
        ERR("[LAMP] Invalid Alg!!!\n");
        break;
    }

    return is_day;
}

/**
 *  判断是否频切?
 *  返回值结果：TRUE  表示已频切
 *              FALSE 表示正常切换
 */
static int judge_abnormal_switch(void)
{
    static time_t preTime = 0;       // 日夜切换间隔时长
    static int quickSwitchCount = 0; // 日夜切换计数

    time_t curTime   = time(NULL);
    /* 时间是可以手动调节的，需要对手动调节时间的情况进行判断 */
    int intervalTime = curTime > preTime ? curTime - preTime : INT_MAX;

    if (g_cfg_lamp->lightext.lamptype == LAMP_STAR) {
        return FALSE;
    }

    if (g_run_lamp->is_lamp_change == TRUE) {
        DBG("lamp is change, renew quick switch\n");
        quickSwitchCount = 0;
        g_run_lamp->is_lamp_change = FALSE;
        return FALSE;
    }

    /* 只在状态切换时进行判断 */
    if (g_run_lamp->is_day_prev != g_run_lamp->is_day_curr) {
        /* 一定时间段内记录到多次切换，判断为出现频切 */
        if (quickSwitchCount >= SWITCH_LIMITE) {
            if (intervalTime > PAUSE_TIME) {
                DBG("switch recover\n");
                quickSwitchCount = 0;
                return FALSE;
            }
            g_run_lamp->is_force_night = TRUE;
            return TRUE;
        }

        /* 对一定时间段内的切换进累加 */
        if (intervalTime < INTERVAL_TIME) {
            quickSwitchCount++;
        } else {
            quickSwitchCount = 0;
            preTime = curTime;
        }
        DBG("quick switch count[%d] intervalTime[%d s]\n", quickSwitchCount, intervalTime);
    }
    return FALSE;
}

/**
 *  日夜状态切换:
 *  @param[is_force]  是否强制切换
 */
static void day_night_switch(int is_day_curr, int is_force)
{
    if (g_run_lamp->is_day_prev != is_day_curr || is_force) {
        /* debug */
        lamp_check_lum_debug(g_cfg_lamp->lightext.alg,
                            g_run_lamp->is_day_prev, g_run_lamp->is_day_curr);

        send_event_chn(JEvent_ALGO_Forzen, 1);
        ms_sleep(200); // 需要大于 ivs loop 的时间

        switch (is_day_curr) {
        case E_STATUS_NIGHT:
            lamp_set_night();
            break;
        case E_STATUS_DAY:
            lamp_set_day(0 == g_run_lamp->nightled_prev &&
                         0 == g_cfg_lamp->lightext.nightled);
            break;
        case E_STATUS_COLOR:
            lamp_set_color(0 == g_run_lamp->nightled_prev &&
                           0 == g_cfg_lamp->lightext.nightled);
            break;
        default:
            WAR("illegal lamp type\n");
            break;
        }

        send_event(JEvent_RunFreezLampCtrl);
        g_run_lamp->hard_cnt = 0;
        g_run_lamp->frozen_cnt = 5000/MS_LAMP;
        g_run_lamp->is_day_prev = is_day_curr;
    }

    return;
}

static int smoothly_change_whtlgt(int target_duty)
{
    int  duty_set = 0;

#ifdef STEPLESS_PWM
    int  duty_curr = 0;

    if (pwm_get_duty_cycle(PWM_WHITE, &duty_curr) != SUCCESS) {
        duty_curr = 0;
    }
    // 当目标亮度为0时，直接熄灭
    if (target_duty == 0) {
        pwm_set_duty_cycle(PWM_WHITE, 0);
        return 1;
    }

    if (duty_curr < target_duty) {
        duty_set = MIN(duty_curr + DUTY_ADJUST_ONCE, target_duty);
        pwm_set_duty_cycle(PWM_WHITE, duty_set);
    }
#else
    set_lamp_status(GPIO_WHITE, !!target_duty); //GPIO_WHITE_ON
    duty_set = target_duty;
#endif

    return (duty_set == target_duty);
}

/**
 * 1. 画面光状态[白天/夜晚/白光]切换和获取日夜状态统一接口 lampctrl_process
 * 2. 触发灯光报警时，在其他回调中打开/关闭白光灯，此回调空跑，等 is_alarm=FALSE 继续调用
 * 3. 切换时，告警过滤消息发送后需要 usleep，防止偶然情况下 JEvent_ALGO_Forzen 来不及回调导致的告警触发
 * 4. 切换时，硬/软光敏稳定计数重置 cnt 必要操作，防止切换算法导致 cnt 残留，此 cnt 只给硬光敏算法使用
 */
static void lamp_process(void)
{
    int is_day = TRUE;

    if (g_run_lamp->frozen_cnt > 0 && !g_run_lamp->is_alarm) {
        g_run_lamp->frozen_cnt--;
        dbg_lamp("[lamp] skip frozen Cnt=[%d]\n", g_run_lamp->frozen_cnt);
        goto exit;
    }

#ifdef STEPLESS_PWM
    if (g_run_lamp->is_alarm || get_facial_convergence_status()) {
        // 保证在人脸收光期间也能更新 EV 值
        get_invert_ev();
#else
    // 灯光报警的时候不检测当前状态
    // 在晚上一直告警到白天, 软光敏不会切，持续告警时，隔一段时间让软光敏可以执行
    if ((g_run_lamp->is_alarm || get_facial_convergence_status())
        && ms_since_previous(&g_run_lamp->ms_clock_lampwh) < 300*1000) {
#endif
        dbg_lamp("[%d/%d] is alarm or faceae\n",
                g_run_lamp->is_alarm, get_facial_convergence_status());
        goto exit;
    }

    is_day = lamp_check_lum();

    if (g_cfg_lamp->lightext.nightled > 0) {
        int target_duty = (g_cfg_lamp->lightext.nightled * PWM_CYCLE_RATE);
    
        smoothly_change_whtlgt(target_duty);
    } else if (g_run_lamp->nightled_prev > 0) {
        int chg_complete = smoothly_change_whtlgt(0);
    
        if (chg_complete) {
            g_run_lamp->nightled_prev = 0;

            //恢复之前的灯光配置，等待一段时间之后再判断
            exec_cmd_lightextcfg();
            g_run_lamp->frozen_cnt = 5000 / MS_LAMP;

            goto exit;
        }
    }

    if (system_get_security() != TRUE) {
        is_day = TRUE;
    }

    // is_day_curr 需要在频切判断之前更新
    if (is_day) {
        g_run_lamp->is_day_curr = E_STATUS_DAY;
    } else {
        g_run_lamp->is_day_curr = MODE_COLOR ? E_STATUS_COLOR : E_STATUS_NIGHT;
    
    }

    /* 频切判断，如果短时间内出现多次切换就琐死为黑白一段时间 */
    if (!get_g_sys(factest)) {
        if (judge_abnormal_switch() == TRUE) {
            dbg_lamp("judge_abnormal_switch == TRUE\n");
            g_run_lamp->is_day_curr = MODE_COLOR ? E_STATUS_COLOR : E_STATUS_NIGHT;
        }
    }

    if (g_cfg_lamp->lightext.lamptype == LAMP_STAR) {
        day_night_switch(E_STATUS_DAY, FALSE);
    } else {
        day_night_switch(g_run_lamp->is_day_curr, FALSE);
    }

exit:

    return;
}

static void play_audio(void *ueserdata)
{
    if (get_audioout_status() == GPIO_AUDIO_OFF) {
        if (SoundSelectE_DEFAULT == g_cfg_lamp->audioalarm.type) {
            encode_audio_queue_push_amr(AUDIO_ALARM_ALARM, TRUE);
        } else if (SoundSelectE_DOG == g_cfg_lamp->audioalarm.type) {
            encode_audio_queue_push_amr(AUDIO_ALARM_DOG, TRUE);
        } else if (SoundSelectE_OTHER == g_cfg_lamp->audioalarm.type) {
            encode_audio_queue_push_amr(AUDIO_ALARM_OTHER, TRUE);
        } else {
            encode_audio_queue_push_amr(AUDIO_ALARM_CUSTOM, TRUE);
        }
    }

    return;
}

static void stop_driveout(void *userdata)
{
    DBG("%s enable: %d, time: %d\n",
        __func__, g_cfg_lamp->driveout.enable, g_cfg_lamp->driveout.time);

    if (g_cfg_lamp->driveout.enable) {
        g_cfg_lamp->driveout.enable = 0;
        conf_set_driveout_cfg(g_cfg_lamp->driveout);
    }

    return;
}

static void do_shineoff(void *userdata)
{
    DBG("do_shineoff\n");
    day_night_switch(g_run_lamp->is_day_curr, TRUE);

    g_run_lamp->is_alarm = FALSE;
    g_run_lamp->is_faceae = FALSE;
    return;
}

static int exec_cmd_driveout(void)
{
    int openwhite = TRUE;

    /* 1.双光源模式下已经开启白光灯，就不再开白光灯
     * 2.红外灯板
     * 3.白光灯板晚上
     * 4.双光灯板全彩模式晚上
     * 以上不再需要开白光灯 - 不适用于自动调光
     */
#ifndef STEPLESS_PWM
    if (g_cfg_lamp->lightext.lightboard == 0 ||
        (g_cfg_lamp->lightext.lightboard == 1 && g_run_lamp->is_day_curr == FALSE) ||
        (g_cfg_lamp->lightext.lightboard == 2 && g_cfg_lamp->lightext.lamptype == 1 && g_run_lamp->is_day_curr == FALSE)) {
        openwhite = FALSE;
    }
#endif
    if (g_cfg_lamp->driveout.enable == 1) {
        g_run_lamp->is_driveout = TRUE;
    } else if (g_cfg_lamp->driveout.enable == 0) {
        if (openwhite == TRUE) {
            do_shineoff(NULL);
        }
        js_delete_timer_r(&g_run_lamp->hdl_audio);
        g_run_lamp->is_driveout = FALSE;
        return 0;
    } else {
        return 0;
    }

    if (openwhite == TRUE && g_cfg_lamp->driveout.whiteen == 1) {
        DBG("SET COLOR!!!, run->is_day_curr = %d\n", g_run_lamp->is_day_curr);
        g_run_lamp->is_alarm = TRUE;
        lamp_set_color(TRUE);
    }

    if (g_cfg_lamp->driveout.audioen == 1) {
        if (g_run_lamp->hdl_audio == NULL) {
            js_create_timer_r(g_run_lamp->sch, 100, 500,
                    play_audio, NULL, &g_run_lamp->hdl_audio);
        }
    }

    // 到时间后，停止驱赶
    js_create_once(g_run_lamp->hdl_drvout, g_run_lamp->sch,
            g_cfg_lamp->driveout.time * 1000, stop_driveout, NULL);

    return SUCCESS;
}

static int exec_cmd_lightextcfg(void)
{
    //小夜灯开启时，走软光敏切换日夜状态，亮度不变
    if (g_cfg_lamp->lightext.nightled > 0) {
        //小夜灯开启，根据日夜切换效果文件
        if (g_run_lamp->nightled_prev != g_cfg_lamp->lightext.nightled) {
            if (g_run_lamp->is_day_curr == E_STATUS_DAY) {
                lamp_set_day(FALSE);
            } else {
                lamp_set_color(FALSE);
            }
        }
    } else if (g_run_lamp->nightled_prev > 0) {
    //非小夜灯开启时，模式切换时，关闭所有灯，并获取算法需要的参数
    } else {
        if (MODE_COLOR) {
            DBG("color mode\n");
            lamp_photosens_full_color_init();
            // set_infrared_light_intensity(LIGHT_OFF);
        } else {
            DBG("smart mode || infa mode\n");
            lamp_photosens_infrared_init();
            // set_white_light_intensity(LIGHT_OFF);
        }

        // 默认白天
        g_run_lamp->is_day_curr = E_STATUS_DAY;
        g_run_lamp->is_day_prev = E_STATUS_DAY;
        lamp_set_day(TRUE);
    }

    g_run_lamp->is_alarm = FALSE;       // 重置灯光告警状态
    g_run_lamp->hard_cnt = 0;           // 硬光敏稳定计数重置
    g_run_lamp->is_lamp_change = TRUE;  // 模式或算法状态改变
    lamp_photosens_clean_status();

    // 避免调光过程中修改开灯阈值时出现闪灯
    DBG("is_day_curr: %d, lamptype: %d, alg: %d\n", g_run_lamp->is_day_curr,
        g_cfg_lamp->lightext.lamptype, g_cfg_lamp->lightext.alg);

#ifdef STEPLESS_PWM
    // 避免固定亮度后开启调光灯光会突变
    sir_init(g_cfg_lamp->lightext);
#endif

    return SUCCESS;
}

static AUDIO_PROMPT xslt_type2name(SoundSelectE type)
{
    if (SoundSelectE_DEFAULT == type) {
        return AUDIO_ALARM_ALARM;
    } else if (SoundSelectE_DOG == type) {
        return AUDIO_ALARM_DOG;
    } else if (SoundSelectE_OTHER == type) {
        return AUDIO_ALARM_OTHER;
    } else {
        return AUDIO_ALARM_CUSTOM;
    }
}

static int do_audio_alarm(void)
{
    int playaudio = FALSE;
    static struct timespec time_pre = {0};

    if (g_run_lamp->is_driveout == TRUE || g_cfg_lamp->audioalarm.enable == 0) {
        return 0;
    }

    // 全彩设备靠时间判断白天晚上，不以是否开关灯判断白天或晚上
#ifdef STEPLESS_PWM
    TimeSeg t_day = {6, 0, 17, 59};
#endif

    switch (g_cfg_lamp->audioalarm.place) {
    case 0:
#ifdef STEPLESS_PWM
        if (!is_day_in_timer(t_day)) {
            playaudio = TRUE;
        }
#else
        if (g_run_lamp->is_day_curr != E_STATUS_DAY) {
            playaudio = TRUE;
        }
#endif
        break;
    case 1:
#ifdef STEPLESS_PWM
        if (is_day_in_timer(t_day)) {
            playaudio = TRUE;
        }
#else
        if (g_run_lamp->is_day_curr == E_STATUS_DAY) {
            playaudio = TRUE;
        }
#endif
        break;
    case 2:
        playaudio = TRUE;
        break;
    case 3:
        playaudio = is_day_in_timer(g_cfg_lamp->audioalarm.timeseg);
        break;
    default:
        break;
    }

    if (playaudio == FALSE) {
        return 0;
    }

    AUDIO_PROMPT audio_name = xslt_type2name(g_cfg_lamp->audioalarm.type);
    int ms_total = get_amr_ms_by_name(audio_name) * g_cfg_lamp->audioalarm.times;

    if (!ms_clock_is_timeup(&time_pre, MAX(ms_total, 15*1000))) {
        // 2024-12-23 李明、曾祥富再次确认
        // 1. 连续告警间隔小于 15s， 不进行音频报警播报，防止播报太多。
        // 2. W45A 连播次数选10，连续卡 16S 也会积压播报，因此当前使用 MAX()
        // 3. 可适当降 cpu
        DBG("ignore audioplay bcz intv < MAX(%d,15)s\n", ms_total/1000);
        ms_clock_reset(&time_pre);
        return 0;
    }

    DBG("ms_total: %d, one: %d, times: %d\n",
        ms_total, get_amr_ms_by_name(audio_name), g_cfg_lamp->audioalarm.times);

    for (int i = 0; i < g_cfg_lamp->audioalarm.times; i++) {
        encode_audio_queue_push_amr(xslt_type2name(g_cfg_lamp->audioalarm.type), TRUE);
    }

    return 0;
}

static int set_lamp_bright(void)
{
    unsigned int grade = LIGHT_INTENSITY_FULL;
    /* manual tunning light intensity */
    if (g_run_lamp->is_alarm || g_run_lamp->is_faceae) {    // 灯光告警打到 30%
        grade = LIGHT_INTENSITY_FACE;
        goto __exit;
    }

    if (g_cfg_lamp->lightext.alg != ALG_SOFT) {
        if (TRUE == g_cfg_lamp->lightext.adjustable) {
            grade = DUTY_DECI;
        } else {
            grade = g_cfg_lamp->lightext.pwm_percent*LIGHT_INTENSITY_FULL/100;
        }
    } else {
        if (g_cfg_lamp->lightext.adjustable) {
            grade = DUTY_FAINT;
        } else {
            grade = g_cfg_lamp->lightext.pwm_percent*LIGHT_INTENSITY_FULL/100;
        }
    }

__exit:
    pwm_set_duty_cycle(PWM_WHITE, grade*DUTY_CYCLE_RATE);

    return SUCCESS;
}

static int do_light_alarm()
{
    int is_day = TRUE;// 1-白天 0-晚上
    int openwhite = FALSE;

    if (g_run_lamp->is_driveout == TRUE || g_cfg_lamp->lightalarm.enable == 0 || g_cfg_lamp->lightext.lamptype == LAMP_STAR) {
        return 0;
    }

    // 全彩设备靠时间判断白天晚上，不以是否开关灯判断白天或晚上
#ifdef STEPLESS_PWM
    TimeSeg t_day = {6, 0, 17, 59};
#endif

    switch (g_cfg_lamp->lightalarm.place) {
    case 0:
#ifdef STEPLESS_PWM
        if (!is_day_in_timer(t_day)) {
            openwhite = TRUE;
        }
#else
        if (g_cfg_lamp->lightext.alg == ALG_TIME) {
            is_day = lamp_check_daytime(&g_cfg_lamp->lightext);
            if (!is_day && g_run_lamp->is_day_curr != E_STATUS_DAY) {
                openwhite = TRUE;
            }
        } else {
            if (g_run_lamp->is_day_curr != E_STATUS_DAY) {
                openwhite = TRUE;
            }
        }
#endif
        break;
    case 1:
#ifdef STEPLESS_PWM
        if (is_day_in_timer(t_day)) {
            openwhite = TRUE;
        }
#else
        if (g_cfg_lamp->lightext.alg == ALG_TIME) {
            is_day = lamp_check_daytime(&g_cfg_lamp->lightext);
            if (is_day && g_run_lamp->is_day_curr == E_STATUS_DAY) {
                openwhite = TRUE;
            }
        } else {
            if (g_run_lamp->is_day_curr == E_STATUS_DAY) {
                openwhite = TRUE;
            }
        }
#endif
        break;
    case 2:
        openwhite = TRUE;
        break;
    case 3:
        openwhite = is_day_in_timer(g_cfg_lamp->lightalarm.timeseg);
        break;
    default:
        break;
    }

    /* [夜晚/白光] + 双光设备 + 智能模式 = shine 10s */
    if (openwhite == TRUE && g_cfg_lamp->lightext.lamptype != LAMP_IR) {
        if (!g_run_lamp->is_alarm) {
            ms_clock_reset(&g_run_lamp->ms_clock_lampwh);
            SYSLOG("set color !!!!!!!!!!!!!\n");
            g_run_lamp->is_alarm = TRUE;
#ifdef STEPLESS_PWM
            lamp_set_color(TRUE);
            set_lamp_bright(); // 告警亮灯 (占空比调节)
#else
            if (g_run_lamp->is_day_curr == E_STATUS_NIGHT) {
                lamp_set_color(TRUE);
            } else {
                set_lamp_status(GPIO_WHITE, GPIO_WHITE_ON);
            }
#endif
        }
        js_create_once(g_run_lamp->hdl_shineoff, g_run_lamp->sch, 
            g_cfg_lamp->lightalarm.time * 1000, do_shineoff, NULL);
    }

    // 处理全彩模式, 晚上一直告警到白天, 软光敏不会切的情况
    if (!g_run_lamp->is_alarm && g_run_lamp->is_day_curr == E_STATUS_COLOR) {
        ms_clock_reset(&g_run_lamp->ms_clock_lampwh);
        g_run_lamp->is_alarm = TRUE;
    }

    return 0;
}

static void do_io_off(void *userdata)
{
    DBG("%s\n", __func__);
    set_lamp_status(GPIO_RED_BLUE, GPIO_RED_BLUE_OFF);

    return;
}

static int do_io_alarm()
{
    int openio = FALSE;

    if (g_cfg_lamp->ioalarm.enable == 0) {
        return 0;
    }

    // 全彩设备靠时间判断白天晚上，不以是否开关灯判断白天或晚上
#ifdef STEPLESS_PWM
    TimeSeg t_day = {6, 0, 17, 59};
#endif

    switch (g_cfg_lamp->ioalarm.place) {
    case 0:
#ifdef STEPLESS_PWM
        if (!is_day_in_timer(t_day)) {
            openio = TRUE;
        }
#else
        if (g_run_lamp->is_day_curr != E_STATUS_DAY) {
            openio = TRUE;
        }
#endif
        break;
    case 1:
#ifdef STEPLESS_PWM
        if (is_day_in_timer(t_day)) {
            openio = TRUE;
        }
#else
        if (g_run_lamp->is_day_curr == E_STATUS_DAY) {
            openio = TRUE;
        }
#endif
        break;
    case 2:
        openio = TRUE;
        break;
    case 3:
        openio = is_day_in_timer(g_cfg_lamp->ioalarm.timeseg);
        break;
    default:
        break;
    }

    if (openio == TRUE) {
        set_lamp_status(GPIO_RED_BLUE, GPIO_RED_BLUE_ON);
        js_create_once(g_run_lamp->hdl_io, g_run_lamp->sch, 
                g_cfg_lamp->ioalarm.time * 1000, do_io_off, NULL);
    }

    return 0;
}

static int exec_cmd_alarm(void)
{
    // 产测模式下，不进行声光报警
    if (get_g_sys(factest)) {
        ERR("test mode, no __play__\n");
        return 0;
    }

    // 声音报警
    do_audio_alarm();

    // 灯光报警
    do_light_alarm();

    // 红蓝灯报警
#ifdef LIGHT_IO_ALARM
    do_io_alarm();
#endif

    return SUCCESS;
}

static int exec_cmd_lamp_frozen(int ms)
{
    int frz_cnt = g_run_lamp->frozen_cnt;
    g_run_lamp->frozen_cnt = ms/MS_LAMP;
    if (frz_cnt <= 0) {
        DBG("Day And Night Switch Frozen Cnt=[%d -> %d]...\n", frz_cnt, g_run_lamp->frozen_cnt);
    } else {
        if (get_g_run(lamp, E_RUN_LAMP_VERBOSE)) {
            DBG("sequence frozen Cnt=[%d -> %d], [ grun lamp 9 on ] to disable this prompt\n",
                frz_cnt, g_run_lamp->frozen_cnt);
        }
    }

    return SUCCESS;
}

static void diff_cfg2cmd(void *ctx)
{
    struct cmdstat *p_cmd = (struct cmdstat *)ctx;

    if (p_cmd->cmd_stage) {
        if (p_cmd->cmd_stage & E_CMD_LAMP_LIGHTEXT) {
            if (g_run_lamp->nightled_prev != g_cfg_lamp->lightext.nightled) {
                g_run_lamp->nightled_prev = g_cfg_lamp->lightext.nightled;
            }
            memcpy(&g_cfg_lamp->lightext, &g_raw_lamp->lightext, sizeof(g_cfg_lamp->lightext));
        }

        if (p_cmd->cmd_stage & E_CMD_AUDIO_ALARM) {
            memcpy(&g_cfg_lamp->audioalarm, &g_raw_lamp->audioalarm, sizeof(g_cfg_lamp->audioalarm));
        }

        if (p_cmd->cmd_stage & E_CMD_LIGHT_ALARM) {
            if (g_cfg_lamp->lightalarm.time != g_raw_lamp->lightalarm.time) {
                cmd_set_command(p_cmd, E_CMD_LIGHT_ALARM_TIME);
            }
            memcpy(&g_cfg_lamp->lightalarm, &g_raw_lamp->lightalarm, sizeof(g_cfg_lamp->lightalarm));
        }

        if (p_cmd->cmd_stage & E_CMD_IO_ALARM) {
            memcpy(&g_cfg_lamp->ioalarm, &g_raw_lamp->ioalarm, sizeof(g_cfg_lamp->ioalarm));
        }

        if (p_cmd->cmd_stage & E_CMD_DRIVEOUT) {
            memcpy(&g_cfg_lamp->driveout, &g_raw_lamp->driveout, sizeof(g_cfg_lamp->driveout));
        }

        if (p_cmd->cmd_stage & E_CMD_LAMP_MODE) {
            memcpy(&g_cfg_lamp->daynight, &g_raw_lamp->daynight, sizeof(g_cfg_lamp->daynight));
        }
    }

    return;
}

static void loop_lamp(void *ctx)
{
    int cmd = cmd_get_command((struct cmdstat *)ctx);

    if (TRUE == g_run_lamp->test) {
        return;
    }

    if (cmd) {
        if (cmd & E_CMD_DRIVEOUT) {
            exec_cmd_driveout();
        }

        if (cmd & E_CMD_LAMP_LIGHTEXT) {
            exec_cmd_lightextcfg();
        }

        // 告警事件处理
        if (cmd & E_CMD_LAMP_ALARM) {
            exec_cmd_alarm();
        }

        if (cmd & E_CMD_LAMP_FREEZ) {
            exec_cmd_lamp_frozen(4 * 1000);
        }
    }

    lamp_process();

    return;
}

static int lamp_init(void)
{
    lamp_gpio_init();

#ifdef STEPLESS_PWM
    // pwm chn create, duty=0
    pwm_open_export(PWM_WHITE, TRUE);
    pwm_set_period(PWM_WHITE, PWM_PERIOD);
    pwm_set_polarity(PWM_WHITE, POLARITY_NORMAL); 
    pwm_set_duty_cycle(PWM_WHITE, 0*24);
    pwm_enable_chn(PWM_WHITE, TRUE);

    sir_init(g_cfg_lamp->lightext);
#endif

    // 判断灯板类型
    if (g_cfg_lamp->lightext.lightboard == 0) {
        g_cfg_lamp->lightext.lamptype = LAMP_IR;
    } else if (g_cfg_lamp->lightext.lightboard == 1) {
        g_cfg_lamp->lightext.lamptype = LAMP_WHITE;
    }

    conf_set_lightext_cfg(g_cfg_lamp->lightext);

    // 默认白天
    g_run_lamp->is_day_curr = E_STATUS_DAY;
    g_run_lamp->is_day_prev = E_STATUS_DAY;
    lamp_set_day(TRUE);

    // 初始化软光敏算法参数
    if (MODE_COLOR) {
        lamp_photosens_full_color_init();
    } else {
        lamp_photosens_infrared_init();
    }

    // 驱赶
    if (g_cfg_lamp->driveout.enable == 1) {
        g_cfg_lamp->driveout.enable = 0;
        g_cfg_lamp->driveout.rest_time = 0;
        conf_set_driveout_cfg(g_cfg_lamp->driveout);
    }

    return SUCCESS;
}

int lamp_is_color_mode(void)
{
    return LAMP_WHITE == g_cfg_lamp->lightext.lamptype;
}

int lamp_server_init(void)
{
    DBG("lamp_server_init\n");
    static struct cmdstat cmdstat_lamp;
    struct cmdstat *ctx = &cmdstat_lamp;
    cmdstat_lamp.diff_cfg2cmd = diff_cfg2cmd;
    g_run_lamp->p_ctx = ctx;

    conf_get_IOalarm_cfg(&g_cfg_lamp->ioalarm);
    conf_get_daynightcfg(&g_cfg_lamp->daynight);
    conf_get_lightext_cfg(&g_cfg_lamp->lightext);
    conf_get_driveout_cfg(&g_cfg_lamp->driveout);
    conf_get_audioalarm_cfg(&g_cfg_lamp->audioalarm);
    conf_get_lightalarm_cfg(&g_cfg_lamp->lightalarm);

    lamp_init();

    attach_config(JEvent_IOAlarmCfg             , cb_ioalarm_cfg    , (void *)ctx);
    attach_config(JEvent_Daynightcfg            , cb_lamp_mode_cfg  , (void *)ctx);
    attach_config(JEvent_LightExtCfgChg         , cb_lamp_irctrl_cfg, (void *)ctx);
    attach_config(JEvent_DriveOutCfg            , cb_driveout_cfg   , (void *)ctx);
    attach_config(JEvent_AudioAlarmCfg          , cb_audioalarm_cfg , (void *)ctx);
    attach_config(JEvent_LightAlarmCfg          , cb_lightalarm_cfg , (void *)ctx);
    attach_event_async(JEvent_RunFreezLampCtrl  , cb_lamp_freez     , (void *)ctx);
    attach_event_async(JEvent_Alarmhumadetect   , cb_lamp_alarm     , (void *)ctx);
    attach_event_async(JEvent_AlarmCar          , cb_lamp_alarm     , (void *)ctx);
    attach_event_async(JEvent_AlarmPet          , cb_lamp_alarm     , (void *)ctx);
    attach_event_async(JEvent_AlarmCry          , cb_lamp_alarm     , (void *)ctx);
    attach_event_async(JEvent_AlarmVgline       , cb_lamp_alarm     , (void *)ctx);
    attach_event_async(JEvent_AlarmVgrect       , cb_lamp_alarm     , (void *)ctx);
    attach_event_async(JEvent_AlarmMD           , cb_lamp_alarm     , (void *)ctx);
    attach_event_async(JEvent_LedTest           , cb_led_test       , (void *)ctx);

    if (NULL == g_run_lamp->sch) {
        g_run_lamp->sch = js_create_scheduler("lamp_server");
        return_val_if_fail(NULL != g_run_lamp->sch, FAILURE);
    }

    if (NULL == g_run_lamp->hdl_loop) {
        js_create_timer_r(g_run_lamp->sch, 400, MS_LAMP, loop_lamp, ctx, &g_run_lamp->hdl_loop);
        return_val_if_fail(NULL != g_run_lamp->hdl_loop, FAILURE);
    }

    return SUCCESS;
}

int lamp_server_uninit(void)
{
    DBG("lamp_server_uninit\n");
    detach_config(JEvent_LightExtCfgChg , cb_lamp_irctrl_cfg, g_run_lamp->p_ctx);
    detach_config(JEvent_Daynightcfg    , cb_lamp_mode_cfg  , g_run_lamp->p_ctx);
    detach_config(JEvent_AudioAlarmCfg  , cb_audioalarm_cfg , g_run_lamp->p_ctx);
    detach_config(JEvent_LightAlarmCfg  , cb_lightalarm_cfg , g_run_lamp->p_ctx);
    detach_config(JEvent_IOAlarmCfg     , cb_ioalarm_cfg    , g_run_lamp->p_ctx);
    detach_config(JEvent_DriveOutCfg    , cb_driveout_cfg   , g_run_lamp->p_ctx);
    detach_event(JEvent_RunFreezLampCtrl, cb_lamp_freez     , g_run_lamp->p_ctx);
    detach_event(JEvent_Alarmhumadetect , cb_lamp_alarm     , g_run_lamp->p_ctx);
    detach_event(JEvent_AlarmCar        , cb_lamp_alarm     , g_run_lamp->p_ctx);
    detach_event(JEvent_AlarmPet        , cb_lamp_alarm     , g_run_lamp->p_ctx);
    detach_event(JEvent_AlarmCry        , cb_lamp_alarm     , g_run_lamp->p_ctx);
    detach_event(JEvent_AlarmVgline     , cb_lamp_alarm     , g_run_lamp->p_ctx);
    detach_event(JEvent_AlarmVgrect     , cb_lamp_alarm     , g_run_lamp->p_ctx);
    detach_event(JEvent_AlarmMD         , cb_lamp_alarm     , g_run_lamp->p_ctx);
    detach_event(JEvent_LedTest         , cb_led_test       , g_run_lamp->p_ctx);

    if (NULL != g_run_lamp->hdl_loop) {
        js_delete_timer_r(&g_run_lamp->hdl_loop);
    }

    if (NULL != g_run_lamp->sch) {
        js_delete_scheduler(g_run_lamp->sch);
        g_run_lamp->sch = NULL;
    }

    return SUCCESS;
}


