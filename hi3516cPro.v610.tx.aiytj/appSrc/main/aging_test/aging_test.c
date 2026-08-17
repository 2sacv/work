#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "debug.h"
#include "utils.h"
#include "js_scheduler.h"
#include "aging_test.h"
#include "jconfstruct.h"
#include "jcpService.h"
#include "jevent.h"
#include "system_sch.h"
#include "encode_audio_queue.h"
#include "lamp_hal.h"

typedef struct {
    int ptz_speed;
    int zoom_time;
    int digit_zoom_enable;
} AGI_InitParams_st;

static JSScheduler sch_agtest = NULL;
static JSTCHandle  hdl_zoom = NULL;
static int g_s_ptz_aging_test_dir = 0;

AGI_InitParams_st g_init_params;

int start_agingtest_opt(void)
{
    if (is_okey(AGING_TEST_OPT)) {
        DBG("start aging test opt\n");
        chmod(AGING_TEST_OPT, S_IXUSR | S_IXGRP | S_IXOTH);
        UtilSystemCmd(AGING_TEST_OPT "&");
        set_g_sys(agingtest);
    }
    return 0;
}

int start_agingtest_mnt(void)
{
    if (is_okey(AGING_TEST_MNT)) {
        DBG("start aging test mnt\n");
        chmod(AGING_TEST_MNT, S_IXUSR | S_IXGRP | S_IXOTH);
        UtilSystemCmd(AGING_TEST_MNT "&");
        set_g_sys(agingtest);
    }
    return 0;
}

void stop_agingtest_opt(void)
{
    DBG("**********stop aging test**********\n");
    UtilSystemCmd(AGING_TEST_KILL);
    usleep(500*1000);
    UtilSystemCmd(AGING_TEST_KILL);

    // 设置白天，防止烤机有红屏
    send_event_chn(JEvent_LedTest, led_white_close);
    send_event_chn(JEvent_LedTest, led_infrared_close);
    send_event_chn(JEvent_LedTest, led_ircut_redglass_open);

    clr_g_sys(agingtest);
}

static void do_aging_test_ptz(void *data)
{
    char cmdline[256] = {0};
    char respbuf[64] = {0};

    jcpcmd_sendrecv( "pelcod20ctrl -type 1 -cmd 9", respbuf,  sizeof(respbuf));
    DBG("aging_test_ptz, %s\n",cmdline);

    memset(cmdline, 0, sizeof(cmdline));

    if (1 == g_s_ptz_aging_test_dir) {//左
        sprintf(cmdline, "pelcod20ctrl -type 1 -cmd %d -data%d %d", 3, 1, g_init_params.ptz_speed);
    } else if (2 == g_s_ptz_aging_test_dir) {//右
        sprintf(cmdline, "pelcod20ctrl -type 1 -cmd %d -data%d %d", 4, 1, g_init_params.ptz_speed);
    } else if (3 == g_s_ptz_aging_test_dir) {//上
        sprintf(cmdline, "pelcod20ctrl -type 1 -cmd %d -data%d %d", 1, 2, g_init_params.ptz_speed);
    }else if (4 == g_s_ptz_aging_test_dir) {//下
        sprintf(cmdline, "pelcod20ctrl -type 1 -cmd %d -data%d %d", 2, 2, g_init_params.ptz_speed);
    }
    DBG("ptz cmdline=[%s]\n", cmdline);
    jcpcmd_sendrecv(cmdline, respbuf,  sizeof(respbuf));
}

static void aging_test_zoom(void *data)
{
    static int zoom_cmd  = -1;
    static int zoom_old  = -1;
    int delay_sec = g_init_params.digit_zoom_enable ? 9000 : 4000;
    char cmdline[64] = {0};

    if (-1 == zoom_cmd) {
        zoom_cmd = 10;
    } else if (10 == zoom_cmd) {
        zoom_cmd = 12;
        zoom_old = 10;
    } else if (11 == zoom_cmd) {
        zoom_cmd = 12;
        zoom_old = 11;
    } else if (12 == zoom_cmd) {
        if (10 == zoom_old) {
            zoom_cmd = 11;
        } else {
            zoom_cmd = 10;
        }
    }

    js_modify_timer_time_r(&hdl_zoom, zoom_cmd > 11 ? (g_init_params.zoom_time*1000 - delay_sec) : delay_sec);

    DBG("aging_test_zoom, cmd %d\n", zoom_cmd);
    sprintf(cmdline, "pelcod20ctrl -type 1 -cmd %d", zoom_cmd);
    jcpcmd_sendrecv(cmdline, 0, 0);
}

void run_agtest_ircut_cb(void *data)
{
    set_ircut_status((int)data);
}

int aging_test_process(int type, int value)
{
    if (NULL == sch_agtest) {
        sch_agtest = js_create_scheduler("sch_agtest");
    }

    switch (type) {
    case AGTEST_ENABLE: {
        static int cnt_proc_done = 0;   // 两个线程每个都会发一个 -enable 4
        if (1 == value) {               // 老化工具下发或启动时判断需要继续老化
            start_agingtest_opt();
        } else if (2 == value) {        // 由老化工具下发的结束老化命令
            stop_agingtest_opt();
            remove(AGING_TEST_KEEP);    // AGING_TEST_OPT 在 reset2factory 中删除，方便 debug
        } else if (value == 4 && (++cnt_proc_done) == 2) {
            stop_agingtest_opt();
            cnt_proc_done = 0;
        }
        break;
        }
    case AGTEST_IRCUT: {
        js_run_function(sch_agtest, run_agtest_ircut_cb, (void *)(value-1) , 0); // enable
        break;
        }
    case AGTEST_AUDIO:
        encode_audio_queue_push_amr(AUDIO_FACTORY_TEST, TRUE);
        break;
    case AGTEST_LED_RED:
        send_event_chn(JEvent_LedTest, (value == 1) ? led_infrared_open : led_infrared_close);
        break;
    case AGTEST_LED_WHITE:
        send_event_chn(JEvent_LedTest, (1 == value) ? led_white_open : led_white_close);
        break;
    case AGTEST_PTZ:
        g_s_ptz_aging_test_dir = value;
        js_run_function(sch_agtest,  do_aging_test_ptz, NULL , 0);
        break;
    case AGTEST_PTZ_SPEED:
        g_init_params.ptz_speed = value;
        DBG("ptz_speed=%d\n", g_init_params.ptz_speed);
        break;
    case AGTEST_ZOOM:
        js_run_function(sch_agtest,  aging_test_zoom, NULL , 0);
        break;
    case AGTEST_KEEP:
        if (1 == value) {
            if (!is_okey(AGING_TEST_KEEP)) {
                TouchFile(AGING_TEST_KEEP);
            }
        } else if (2 == value) {
            if (is_okey(AGING_TEST_KEEP)) {
                remove(AGING_TEST_KEEP);
            }
        }
        break;
    default:
        break;
    }

    return 0;
}

void init_aging_test(void)
{
    if (is_okey(AGING_TEST_MNT)) { //SD卡启动老化脚本
        start_agingtest_mnt();
        return;
    }

    if (is_okey(AGING_TEST_OPT) && is_okey(AGING_TEST_KEEP)) { //keep
        start_agingtest_opt();
        return;
    }
}

void uninit_aging_test(void)
{
    js_delete_scheduler(sch_agtest);
    sch_agtest = NULL;
}
