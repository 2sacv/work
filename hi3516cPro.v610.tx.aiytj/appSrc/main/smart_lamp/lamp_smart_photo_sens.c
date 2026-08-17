/*
 *       Filename:  lamp_smart_photo_sens.c
 *    Description:  
 *        Version:  1.0 2.0
 *        Created:  05/05/2022 08:53:52 AM 2025-03-05
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  xiangyp (), wangr zhangj
 *   Organization:  
*/

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "jconfstruct.h"
#include "system_ctrl.h"
#include "lamp_smart_photo_sens.h"
#include "g_sys.h"
#include "g_stat.h"
#include "g_run.h"
#include "g_log.h"
#include "confapi.h"
#include "jconfig.h"
#include "encode_common.h"
#include "lamp_sal.h"
#include "lamp_main.h"

//初始化为白天
static PhotoSensRun g_run_ps = {
    .EV0            = 0,
    .EV             = 0,
    .EV_prev        = 0,
    .EV_avg         = 0,
    .EV_avg_prev    = 0,
    .EV_force9t     = 0,
    .rgain          = 0,
    .bgain          = 0,
    .color_temp     = 1,
    .is_day         = 1,
    .is_day_prev    = 0
};

static PhotoSensThreshold thr_ir[SENSOR_MAX] = {
    //                on   off   max            min  deci bias9t stable
    [SENSOR_SC235] = {342, 530, (MAX_EXPOSURE), 100, 5,   0.1,   0.01},
};

static PhotoSensThreshold thr_wh[SENSOR_MAX] = {
    //                on   off  max             min  deci bias9t stable
    [SENSOR_SC235] = {342, 483, (MAX_EXPOSURE), 100, 5,   0.1,   0.01},
};

static PhotoSensThreshold *g_thr = &thr_wh[SENSOR_SC235];

static void lamp_osd(uint32_t ExposureValue)
{
    int osd_change = 0;
    static int osdinit = 0;
    static OsdExpandS osdExpand = {0};

    if (is_okey("/opt/lamp_osd")) {
        if (!osdinit) {
            osdinit = 1;
            conf_get_osdexpandcfg(&osdExpand);
        }

        snprintf(osdExpand.cusosd[2].content, sizeof(osdExpand.cusosd[2].content) - 1, 
            "EV [%u/%u] on [%d] off [%d]", 
            g_run_ps.EV, ExposureValue, g_thr->EV_on, g_thr->EV_off);
        osdExpand.cusosd[2].enable = 1;
        osdExpand.cusosd[2].x = 10;
        osdExpand.cusosd[2].y = 300;
        osd_change = 1;
    } else {
        if (osdinit) {
            osdinit = 0;
            osdExpand.cusosd[2].enable = 0;
            osd_change = 1;
        }
    }

    if (osd_change) {
        send_conf_data(JEvent_OsdExpandCfgChg, &osdExpand, sizeof(OsdExpandS));
    }
}

static float log_base_max_ev(PhotoSensThreshold *p_thr, float *base)
{
    if (is_float0(*base)) {
        *base = log2(p_thr->EV_max/p_thr->EV_min);
    }

    return *base;
}

static int get_invert_exposure_value(void) //EV 与亮度值正相关处理
{
    static float base = 0;

    int ret = SUCCESS;

    uint32_t ev = get_invert_exposure();
    g_run_ps.EV0 = g_run_ps.EV = ev;
    if (g_run_ps.EV > g_thr->EV_max) {
        g_run_ps.EV = g_thr->EV_max;
    } else if (g_run_ps.EV < g_thr->EV_min) {
        g_run_ps.EV = g_thr->EV_min;
    }

    dbg_lamp("g_run_ps.EV: %u, g_thr->EV_max: %u\n", g_run_ps.EV, g_thr->EV_max);
    g_run_ps.EV = 1000.0 * log2(g_thr->EV_max/g_run_ps.EV + 0.0102) /
                  log_base_max_ev(g_thr, &base);
    dbg_lamp("g_run_ps.EV: %u\n", g_run_ps.EV);
    lamp_osd(ev);

    return ret;
}

int is_photosens_day(void)
{
    static int is_day = TRUE;
    static float base = 0;

    PhotoSensThreshold *p_thr = NULL;
    ESensorType sensor_type = system_get_snsr_type();
    uint32_t ev = get_invert_exposure();

    if (lamp_is_color_mode()) {
        p_thr = &thr_wh[sensor_type];
    } else {
        p_thr = &thr_ir[sensor_type];
    }

    ev = RANGE(ev, p_thr->EV_min, p_thr->EV_max);
    ev = 1000.0 * log2(p_thr->EV_max / ev + 0.0102) / log_base_max_ev(p_thr, &base);

    if (is_day) {
        is_day = (ev >= p_thr->EV_on);
    } else {
        is_day = (ev > p_thr->EV_off);
    }

    pri_lamp(LVL_LOOP, "photosens is %s, EV: %u, EV_on: %u, EV_off: %u\n",
             is_day ? "day" : "night", ev, p_thr->EV_on, p_thr->EV_off);

    return is_day;
}

static void print_photo_sens_message(void)
{
    char out[512] = {0};
    static char out0[512] = {0};
    char *p = out;

    p += sprintf(p, "rgain: %d  bgain: %d\n", g_run_ps.rgain, g_run_ps.bgain);
    p += sprintf(p, "EV: [%u/%llu], EV_prev: %u, EV_avg: %u, EV_avg_prev: %u, EV_force9t: %u\n", 
        g_run_ps.EV, g_run_ps.EV0, g_run_ps.EV_prev, g_run_ps.EV_avg, 
        g_run_ps.EV_avg_prev, g_run_ps.EV_force9t);
    p += sprintf(p, "rgain: %d, bgain: %d, color_temp: %d, is_day: %d, is_day_prev: %d\n", 
        g_run_ps.rgain, g_run_ps.bgain, g_run_ps.color_temp, 
        g_run_ps.is_day, g_run_ps.is_day_prev);
    p += sprintf(p, "EV_on: %u, EV_off: %u\n", 
        g_thr->EV_on, g_thr->EV_off);
    p += sprintf(p, "EV_deci: %.2f, bias_force9t: %.2f, bias_stable: %.2f\n\n", 
        g_thr->EV_deci, g_thr->bias_force9t, g_thr->bias_stable);

    if(get_g_log(lamp)) {
        if (0 != strcmp(out0, out)) {
            DBG("%s", out);
            strcpy(out0, out);
        }
    }

    return;
}

//建议将 light_on 阈值设置为 2lux、light_off 阈值设置为 4~6lux 时的 EV
int lamp_photosens_full_color_init(void)
{
    int ret = 0;
    ESensorType sensor_type = system_get_snsr_type();
    dbg_lamp("sensor_type = %d\n", sensor_type);

    g_thr = &thr_wh[sensor_type];

    do {
        ret = get_invert_exposure_value();
        ENCODE_RET_BREAK(ret, "get_exposure_value failed");

        g_run_ps.EV_prev = g_run_ps.EV_avg = g_run_ps.EV;
    } while(0);

    return SUCCESS;
}

//建议将 light_on 阈值设置为 2lux、light_off 阈值设置为 4~6lux 时的 EV
int lamp_photosens_infrared_init(void)
{
    int ret = 0;
    ESensorType sensor_type = system_get_snsr_type();
    dbg_lamp("sensor_type = %d\n", sensor_type);

    g_thr = &thr_ir[sensor_type];

    do {
        ret = get_invert_exposure_value();
        ENCODE_RET_BREAK(ret, "get_exposure_value failed");

        g_run_ps.EV_prev = g_run_ps.EV_avg = g_run_ps.EV;
    } while(0);

    return ret;
}

int lamp_photosens_get_daynight(int *is_day, int *is_force_night)
{
    return_val_if_fail(NULL != is_day && NULL != is_force_night, FAILURE);
    return_val_if_fail(NULL != g_thr, FAILURE);

    int ret = 0;

    do {
         ret = get_invert_exposure_value();
        ENCODE_RET_BREAK(ret, "get_exposure_value failed");

        g_run_ps.EV_avg = 
            (g_run_ps.EV_avg*(10-g_thr->EV_deci) + g_run_ps.EV * g_thr->EV_deci) / 10;

        print_photo_sens_message();
        g_run_ps.is_day_prev = g_run_ps.is_day;

        if (get_g_run(lamp, E_RUN_LAMP_FORCE_DAY)) {
            g_run_ps.is_day = TRUE;
            clr_g_run(lamp, E_RUN_LAMP_FORCE_NIGHT);
            break;
        } else if (get_g_run(lamp, E_RUN_LAMP_FORCE_NIGHT)) {
            g_run_ps.is_day = FALSE;
            clr_g_run(lamp, E_RUN_LAMP_FORCE_DAY);
            break;
        }

        //防反复切, 只有偏离足够大 才解冻，bias_force9t 通常是 bias_stable 10 倍以上。
        //办公室环境突然开灯, EV_force9t 比较大, 当 EV_force9t 与 EV_avg 相差较大时退出防频切
        if (g_run_ps.EV_force9t > 0) {
            if (BIAS_P(g_run_ps.EV_avg, g_run_ps.EV_force9t) < g_thr->bias_force9t && 
                BIAS_P(g_run_ps.EV_avg, g_thr->EV_off) < g_thr->bias_force9t*2) {
                dbg_lamp("detected frequent switch & EV stable, ignore switch case\n");
                g_run_ps.is_day = FALSE;
                break;
            } else {
                dbg_lamp("EV out of fluctuating rate, quit ignore switch case\n");
                g_run_ps.EV_force9t = 0;
            }
        }

        /* 等待 EV 值稳定到一定程度后才进入日夜判断 */
        /* 环境突变, 彩切黑不等 EV 值稳定 */
        if ((BIAS_P(g_run_ps.EV, g_run_ps.EV_prev) > g_thr->bias_stable || 
            BIAS_P(g_run_ps.EV_avg, g_run_ps.EV_avg_prev) > g_thr->bias_stable/2) &&
            g_run_ps.EV > g_thr->EV_on/2) {
            break;
        }

        if (TRUE == g_run_ps.is_day) { // day
            if (g_run_ps.EV < g_thr->EV_on) {
                g_run_ps.is_day = FALSE;
            } else {
                g_run_ps.is_day = TRUE;
            }
        } else { // night
            if (g_run_ps.EV > g_thr->EV_off) {
                g_run_ps.is_day = TRUE;

                //防反复切记录稳定后的 EV 值
                if (TRUE == *is_force_night) {
                    *is_force_night = FALSE;
                    g_run_ps.EV_force9t = g_run_ps.EV_avg;
                    g_run_ps.is_day = FALSE;
                }
            } else {
                g_run_ps.is_day = FALSE;
            }
        }
    } while(0);

    g_run_ps.EV_prev = g_run_ps.EV;
    g_run_ps.EV_avg_prev = g_run_ps.EV_avg;

    *is_day = g_run_ps.is_day;

    return ret;
}

void lamp_photosens_clean_status(void)
{
    g_run_ps.EV_force9t = 0;
}
