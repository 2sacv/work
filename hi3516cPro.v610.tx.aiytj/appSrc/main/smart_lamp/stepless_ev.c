#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

/* file */
#include <fcntl.h>
#include <sys/file.h>

#include "debug.h"
#include "lamp_main.h"
#include "stepless_ev.h"
#include "jconfstruct.h"
#include "utils.h"
#include "jpwm.h"
#include "lamp_smart_photo_sens.h"
#include "encode_video.h"
#include "jconfig.h"
#include "confapi.h"
#include "ot_common_isp.h"
#include "ss_mpi_ae.h"

#include "lamp_hal.h"
#include "lamp_main.h"
#include "stepless_ev.h"

#define TBL_TAIL (ARRAY_SIZE(sir.ev_tbl)-1)

static _sir sir = {0};

void sir_reset(int line)
{
    if (line) SYSLOG("reset @%d\n", line);
    memset(&sir, 0, (&sir.duty-&sir.d_duty)*sizeof(int));
}

void sir_init(LightExtCfg light)
{
    dbg_lamp("sir init\n");
    memset(&sir, 0, (&sir.nr-&sir.d_duty)*sizeof(int));
    sir.fctargetratio = light.fctargetratio;
    sir.pwm_percent   = light.pwm_percent;
    sir.adjustable    = light.adjustable;
    sir.k_break       = light.fctargetratio;
    sir.ev_max        = light.fclightmaxev;
    sir.ev_min        = light.fclightminev;
    sir.ev_lux2       = 1000.0 * log2(sir.ev_max/light.fcopenevst  + 0.0102) / log_base_max();
    sir.ev_lux5       = 1000.0 * log2(sir.ev_max/light.fctargeevst + 0.0102) / log_base_max();
    sir.ev_tbl[0]     = sir.ev_lux5;
    dbg_lamp("k_break:%f, ev_max:%.2f, ev_min:%.2f, ev_lux2:%.2f, ev_lux5:%.2f\n",
              sir.k_break, sir.ev_max, sir.ev_min, sir.ev_lux2, sir.ev_lux5);
}

static float d_duty_from_target(float ev, float ev_target, int curr_duty)
{
    float s = (ev_target > ev) ? 1 : -1;
    int d_duty = 1;
    switch (curr_duty) {
    case 0 ... 10: d_duty = 1;
        break;
    case 11 ... 1000: d_duty = (curr_duty+5/*15~24使用2*/)/10;
        break;
    default:
        break;
    }

    /* 越接近 delta 越小，防止来回闪烁 */
    float k = fabs(1 - ev/ev_target);
    if (k < 2*sir.k_break) {
        d_duty /= 4;
    } else if (k < 3*sir.k_break) {
        d_duty /= 3;
    } else if (k < 4*sir.k_break) {
        d_duty /= 2;
    }

    return s*MAX(1, d_duty);
}

float log_base_max()
{
    static float base = 0;
    if (is_float0(base)) {
        base = log2(sir.ev_max/sir.ev_min);
    }
    return base;
}

float get_invert_ev()
{
    int ret = 0;
    int vi_pipe = 0;
    ot_isp_exp_info exp_info = {0};

    ret = ss_mpi_isp_query_exposure_info(vi_pipe, &exp_info);

    if (SUCCESS == ret) {
        sir.ev = sir.ev0 = exp_info.exposure / (1000.0);

        if (sir.ev > sir.ev_max) {
            sir.ev = sir.ev_max;
        } else if (sir.ev < sir.ev_min) {
            sir.ev = sir.ev_min;
        }
        /* 0.0102 避免 0 */
        sir.ev = 1000.0 * log2(sir.ev_max/sir.ev + 0.0102) / log_base_max();
    } else {
        sir.ev = 1;
        ERR("hisi get exp_info failed, ret=0x%x.\n", ret);
    }

    if (++sir.nr % 5 == 0) {
        if (get_g_run(pwm, RUN_PWM_PRINT_EV)) {
            DBG("ev org: %.2f ev: %.2f\n", sir.ev0, sir.ev);
        }
    }

    return sir.ev;
}

static void ev_pri(int line, int flat, int state, float dif_p, const char *tag)
{
    float diff = sir.ev-sir.ev_darkest;
    float k_dark = DIFF_P(sir.ev_darkest, sir.ev_faint);

    if (get_g_run(pwm, RUN_PWM_PRINT_EV)) {
        printf(
            "\tline:%4d   △ :%6.1f   ev:%6.2f ev_p:%6.2f   ev0:%.2f __tag:%s\n"
            "\tduty:%4d   △ :%5d   diff:%6.2f  pct:%6.2f k_rst:%.2f k_break:%.2f\n"
            "\t  nr:%4d deci:%6.2f fain:%6.2f dark:%6.2f state:0x%0X\n"
            "\tflat:%4d difp:%6.1f lux2:%6.2f lux5:%6.2f k_mir:%.2f\n\n",
            line    , sir.d_ev   , sir.ev_tbl[0], sir.ev_tbl[1], sir.ev0, tag,
            sir.duty, sir.d_duty , diff         , k_dark, K_FLUT_RESET, sir.k_break,
            sir.nr%1000, sir.ev_deci, sir.ev_faint , sir.ev_darkest, state,
            flat    ,       dif_p, sir.ev_lux2  , sir.ev_lux5 , (sir.ev_deci/sir.ev_darkest)
        );
    }

    return;
}

//3分钟之内如果ev值在lux5来回波动，代表此时的k_break不适合当前环境，需要增大k_break值
//如果3分钟之内ev值稳定在lux5左右，说明此时环境相对稳定，可将k_break值回调
static void do_auto_k_break(int d)
{
    static time_t arr[3] = {0};
    static int d_prev = 0;
    if (d * d_prev < 0) {
        if (mono_sec() - arr[2] < 3*60) {
            sir.k_break = MIN(sir.k_break * 1.26, sir.fctargetratio * (2+(sir.duty <= 2*DUTY_FAINT)));
            arr[2] = 0; 
        } else {
            arr[2] = arr[1]; 
        }
        arr[1] = arr[0]; 
        arr[0] = mono_sec();
    } else if (sir.k_break > sir.fctargetratio) {
        if (mono_sec() - arr[0] > (2+(sir.duty <= 2*DUTY_FAINT))*60*60) {    // 每(两+1)小时缩小
            sir.k_break = MAX(sir.k_break / 1.26, sir.fctargetratio);
            arr[0] = mono_sec();
        }
    }

    d_prev = d;
}

/*
 * 自动调光
 * @alg: 模式 2-自动 3-定时
 * @is_force_night: 是否已进入防频切
 *
 * Return Value:
 *  1: is white day and duty=0
 *  0: night
 **/
int pwm_stepless_adjust(int alg, int *is_force_night)
{
    static int flat_state = 0;
    int line = __LINE__;

    float dif_p3 = 0, dif_p2 = 0, dif_p1 = 0;
    float ev = sir.ev;

    memmove(&sir.a_duty[1], &sir.a_duty[0], sizeof(sir.a_duty)-sizeof(sir.a_duty[0]));
    sir.a_duty[0] = sir.duty;

    memmove(&sir.ev_tbl[1], &sir.ev_tbl[0], sizeof(sir.ev_tbl)-sizeof(sir.ev_tbl[0]));
    sir.ev_tbl[0] = ev;

    dif_p3 = DIFF_P(ev, sir.ev_tbl[TBL_TAIL]);  // 3s 
    dif_p2 = DIFF_P(ev, sir.ev_tbl[6]);         // 2s 
    dif_p1 = DIFF_P(ev, sir.ev_tbl[3]);         // 1s 

    int flat = (dif_p2 <= K_FLUT_FAINT);      // 波动范围

    if (pop_g_run(pwm, RUN_PWM_LOAD_X)) {
        //sir_init();
        goto __silent;
    }

    flat_state <<= 1;
    flat_state |= flat;

    if (sir.duty == 0) {
        if (alg == ALG_SOFT) {
            if (ev <= sir.ev_lux2) { // 开灯
                if (sir.adjustable) {
                    WAR("enter auto adjust, ev = %.2f\n", ev);
                    sir.duty = DUTY_FAINT;
                } else {
                    sir.duty = sir.pwm_percent*LIGHT_INTENSITY_FULL/100;
                }

                line = __LINE__; goto __exit;
            }
        } else { // ALG_TIME
            if (sir.adjustable) {
                WAR("enter timer adjust, ev = %.2f\n", ev);
                sir.duty = DUTY_DECI;
            } else {
                sir.duty = sir.pwm_percent*LIGHT_INTENSITY_FULL/100;
            }

            line = __LINE__; goto __exit;
        }
        goto __silent;
    } else {
        line = __LINE__;
        /*                        target=5Lux
         *                          施密特范围 ± K_BREAK*5Lux
         *        2lux              __^__
         *         +-----+------+-----+
         *         2                  5Lux
         **/
        /* ev 会有 3 个 sample 的迟延，在 target 附近降低频率 */
        float k = fabs(1 - ev/sir.ev_lux5);
        if (k < sir.k_break) {
            dbg_lamp("__bingo__ EV, ev:%.2f @%.2f ± @%.2f%%\n", ev, sir.ev_lux5, 100*sir.k_break);
            line = __LINE__; goto __skip;
        } else if (k < sir.k_break*2) {
            dbg_lamp("in sir.k_break*2 %d\n", sir.nr);
            if ((sir.nr % (4 << (sir.duty <= 2*DUTY_FAINT))) != 0) {
                line = __LINE__; goto __skip;
            }
        } else if (k < sir.k_break*3) {
            dbg_lamp("in sir.k_break*3 %d\n", sir.nr);
            if ((sir.nr % (2 << (sir.duty <= 2*DUTY_FAINT))) != 0) {
                line = __LINE__; goto __skip;
            }
        }

        sir.d_ev = sir.ev_lux5 - ev;
        sir.d_duty = d_duty_from_target(ev, sir.ev_lux5, sir.duty);

        do_auto_k_break(sir.d_ev);

        // 定时模式下(灯光常亮)，只调光不判断开关灯
        if (alg != ALG_SOFT) { // ALG_TIME 限制最低亮度为 20‰
            if (sir.adjustable) {
                sir.duty += sir.d_duty;
                if (sir.duty < DUTY_DECI) {
                    sir.duty = DUTY_DECI;
                } else if (sir.duty > DUTY_MAX) {
                    sir.duty = DUTY_MAX;
                }
            }
            line = __LINE__; goto __exit;
        }

        static int tick = 0;
        static int prev_force = FALSE;
        if (*is_force_night) {
            // 防反复切后，获取稳定后的 EV 值
            static int cnt = 0;
            if (!prev_force) {
                prev_force = TRUE;
                cnt = 2000/MS_LAMP;
                ms_clock_reset(&sir.ms_force_night);
            } else {
                if (cnt > 0) {
                    cnt--;
                    if (!cnt) {
                        DBG("got force night, ev = %.2f\n", ev);
                        sir.ev_force_night = ev;
                    } else {
                        DBG("[%d] skip, ev = %.2f\n", cnt, ev);
                    }
                    line = __LINE__; goto __silent;
                }

                if (!is_float0(sir.ev_force_night)) {
                    // 时间到了，退出锁死状态
                    if (ms_clock_is_timeup(&sir.ms_force_night, PAUSE_TIME)) {
                        WAR("clock is timeup\n");
                        sir.ev_force_night = 0;
                        prev_force = FALSE;
                        *is_force_night = FALSE;
                    }

                    // BV 波动范围超过 10% 持续 2s 则退出锁死状态
                    if (DIFF_P(sir.ev_force_night, ev) > 10) {
                        tick++;
                    } else {
                        tick = 0;
                    }

                    if (tick >= 2000/MS_LAMP) {
                        WAR("BV flush cover, ev = %.2f\n", ev);
                        sir.ev_force_night = 0;
                        prev_force = FALSE;
                        *is_force_night = FALSE;
                    }
                } 
            }
            line = __LINE__ ; goto __exit;
        } else {
            prev_force = FALSE;
            if (!sir.adjustable) {
                if (ev > sir.ev_light_off) { // 大于关灯阈值
                    tick++;
                    DBG("[%d] [%.2f/%.2f] skip light-off\n", tick, ev, sir.ev_light_off);
                } else {
                    tick = 0;
                }

                if (tick >= 2000/MS_LAMP) {
                    tick = 0;
                    line = __LINE__; goto __day;
                }
                line = __LINE__ ; goto __exit;
            }
        }

        /* 已经获取 deci，或当前不是 duty_deci，才开始变动 */
        if (!is_duty_deci(sir.duty) || sir.ev_deci) {
            sir.duty += sir.d_duty;
        }

        // 保证 duty 值在灯板的亮度范围之内
        if (sir.duty < DUTY_FAINT) {
            sir.duty = DUTY_FAINT;
        } else if (sir.duty > DUTY_MAX) {
            sir.duty = DUTY_MAX;
        }

        /* 采集 deci */
        if (is_duty_deci(sir.duty) && !sir.ev_deci) {
            DBG("wait deci stable %d d_duty:%d\n", sir.cnt_deci, sir.d_duty);
            if (dif_p3 <= K_FLUT_DECI &&
                    (++sir.cnt_deci) >= N2SEC && eq_bit_and(flat_state, MASK2SEC)) {
                DBG("got deci, ev=%.2f\n", ev);
                sir.ev_deci = ev;               // deci 采样结束，开始采 faint
            } else {
                line = __LINE__; goto __exit;
            }
        } else {
            sir.cnt_deci = 0;
        }

        /* have got deci 有波动
         * ev突然变大:开灯
         * ev突然变小:人主动移动到更暗处
         */ 
        if (sir.ev_deci) {
            if (sir.duty > DUTY_DECI+1) { // 高Lux波动，duty 不跳变
                line = __LINE__; goto __reset2deci;
            } else if (sir.duty == sir.a_duty[3] && sir.duty == sir.a_duty[6] &&
                dif_p1 > K_FLUT_RESET) {   //低Lux波动，duty 立即重采
                line = __LINE__; goto __reset2deci;
            }
        }

        /* 采集 faint，区别于 ev_deci, 即便获取到后，会每 5 个 sample 更新一次 */
        if (sir.duty == DUTY_FAINT) {
            DBG("wait faint stable %d\n", sir.cnt_faint);
            if (++sir.cnt_faint >= N2SEC && eq_bit_and(flat_state, MASK2SEC)) {
                DBG("got faint, ev=%.2f\n", ev);
                sir.ev_faint = ev;
                sir.cnt_faint = 0;

                if (is_float0(sir.ev_darkest) || ev < sir.ev_darkest) {
                    sir.ev_darkest = ev;    // dark  采样
                }
            }
        } else {
            sir.cnt_faint = 0;
        }

        if (is_float0(sir.ev_faint)) {
            line = __LINE__; goto __exit;
        }

        /*
         *         faint       deci
         *        +--+-----------+---oo
         *  duty  0  2          20
         *
         *  关灯条件：
         *  1. duty==0 && 不是高反射 K_MIRROR
         *  2. 高反射场景，ev 从 ev_darkest 反弹
         */
        if (sir.ev_darkest && sir.ev_deci / sir.ev_darkest >= K_MIRROR) {
            // 要灭灯，必然有 ev_faint >= ev_deci 的过程
            if (ev > sir.ev_deci ||
                ev - sir.ev_darkest > sir.ev_lux2/log10(MAX(10, sir.ev_darkest))) {
                line = __LINE__; goto __day;
            } else {
                line = __LINE__; goto __skip;
            }
        } else if (sir.duty == DUTY_FAINT) {
            line = __LINE__; goto __day;
        }
    }

__exit:
    ev_pri(line, flat, flat_state, dif_p3, "exit");
    pwm_set_duty_cycle(PWM_WHITE, sir.duty*DUTY_CYCLE_RATE);
    return (sir.duty == 0);

__skip:
    sir.d_duty = 0;
    ev_pri(line, flat, flat_state, dif_p3, "skip");
__silent:
    return (sir.duty == 0);

__day:
    sir.duty = 0;
    ev_pri(line, flat, flat_state, dif_p3, "day");
    pwm_set_duty_cycle(PWM_WHITE, sir.duty*DUTY_CYCLE_RATE);
    printf("__________ day _________\n");
    sir_reset(__LINE__);
    return (sir.duty == 0);

__reset2deci:
    ev_pri(line, flat, flat_state, dif_p3, "reset");
    sir_reset(__LINE__);
    sir.duty = MAX(sir.duty, DUTY_DECI+1);
    pwm_set_duty_cycle(PWM_WHITE, sir.duty*DUTY_CYCLE_RATE);
    return (sir.duty == 0);
}

