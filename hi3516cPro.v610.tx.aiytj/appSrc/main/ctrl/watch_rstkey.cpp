#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/watchdog.h>

#include "debug.h"
#include "watch_rstkey.h"
#include "js_scheduler.h"
#include "delay_exec.h"
#include "airlink.h"
#include "utils.h"
#include "factory_db.h"
#include "ptz_ctrl.h"
#include "encode_audio_queue.h"
#include "confapi.h"
#include "key_scan.h"

#ifdef PLATFORM_TENCENT
#include "tencent_http_service.h"
#endif

#define CHK_TIMES           7
#define INTV_RESET          (50)
#define GPIO_RESET          (8*1+3) //复位键 GPIO0_0
#define RST_PRESSING        0

static JSScheduler sch_reset = NULL;
static JSTCHandle  hdl_reset = NULL;
static JSTCHandle  g_free_handle = NULL;

static void watch_rstkey(void *data)
{
    static timespec clk_prt = {0};
    static key_config_t cfg_reset = {
        .param = {
            .gpio           = GPIO_RESET,
            .active_level   = RST_PRESSING,
            .debounce_ms    = 100,
            .long_press_ms  = 7000,
        },
    };
    static key_sta_t sta_reset = {
        .dbg_on = LVL_DBG,
    };

    if (get_g_sys(factest) || 0 == access(FACTORY_SSID_FILE, F_OK)) {
        cfg_reset.event = KEY_EVENT_SHORT_PRESS;

        if (key_triggered(&cfg_reset, &sta_reset)) {
            WAR("factory mode, ignore key pressed\n");
            encode_audio_queue_push_amr(AUDIO_FACTORY_TEST, TRUE);
        }
    } else {
        cfg_reset.event = KEY_EVENT_LONG_PRESS;

        if (is_okey("/tmp/disable_reset")) {
            SYSLOG("disable_reset TRUE\n");
            return;
        }

        if (ms_clock_is_timeup(&clk_prt, 1000) && KST_PRESSED == sta_reset.st) {
            WAR("since reset key pressed: %lldms\n",
                ms_since_previous(&sta_reset.t_press));
        }

        if (key_triggered(&cfg_reset, &sta_reset)) {
            SYSLOG("-RESTORE FACTORY DEFAULT --\n");
            wifi_reset_factory();
            ptz_config_default();
            encode_audio_queue_push_amr(AUDIO_KEY_RESETING, TRUE);
            encode_audio_queue_push_amr(AUDIO_EXEC_RESETING, FALSE);

            if (is_okey("/opt/etc/reset_no_unbind")) {
                //条件不解绑，防止客户的设备自己解绑
                SYSLOG("/opt/etc/reset_no_unbind exit,no unbind\n");
            } else {
#if defined(PLATFORM_TENCENT)
                report_dev_unbind();
#endif
                DevConfS devconf = {0};
                conf_get_devconf_cfg(&devconf);
                devconf.devicebind = 0;
                conf_set_devconf_cfg(devconf);
            }

            //sleep(3); // unbind ping spend >3s
            delay_ctrl_exec(DELAY_CMD_DEFAULT, NULL, 0);
        }
    }

    return;
}

int init_client_rst_key(void *data)
{
    //recover_init();
    //gpio_set_export(GPIO_RESET);
    //gpio_set_direction(GPIO_RESET, 1);
    //gpio_set_value(GPIO_RESET, 1);
    gpio_open_export(GPIO_RESET);
    gpio_open_set_direction(GPIO_RESET, "in");

    sch_reset = data;

    js_create_timer_r(sch_reset, INTV_RESET, INTV_RESET, watch_rstkey, NULL, &hdl_reset);

    return 0;
}

void uninit_client_rst_key(void)
{
    if (!sch_reset) {
        return;
    }

    //recover_uninit();
    if(hdl_reset){
        js_delete_timer_r(&hdl_reset);
    }

    sch_reset = NULL;
}

int rst_get_stat(int *rststat)
{
    if (NULL == rststat) {
        return -1;
    }

    int value = -1;

    gpio_open_get_value(GPIO_RESET, &value);
    *rststat = (RST_PRESSING == value);

    return 0;
}

void uninit_watch_free()
{
    if (!g_free_handle) {
        return;
    }

    js_delete_timer_r(&g_free_handle);
}

