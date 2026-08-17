/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : system_main.c
 * @Created Time : 2013-12-11
 * @Version      : 1.0
 * @Author       : zengy
 * @Description  :
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <pthread.h>

#include <logapi.h>

#include "g_sys.h"
#include "g_log.h"
#include "debug.h"

#include "pthread_manage.h"
#include "jconfig.h"
#include "jevent.h"

#include "jcpService.h"
#include "upnp.h"
#include "ddnsstrategy.h"
#include "time_config.h"
#include "conf_list.h"
#include "net_check.h"
#include "search_service.h"
#include "system_ctrl.h"
#include "utils.h"
#include "hm_api.h"
#include "logapi.h"
#include "alarm_service.h"
#include "encodeapi.h"
#include "http_main.h"
#include "js_rtsp_server.h"
#include "aging_test.h"
#include "update.h"
#include "gpio.h"
#include "ptz_ctrl.h"

#include "system_main.h"

#if defined(CUST_PROT_ZNUPNP)
#include "znpnp.h"
#endif

#ifdef PLATFORM_TENCENT
#include "tencent_server.h"
#include "tencent_media_manage.h"
#endif

#if defined(PLATFORM_GB)
#include "gb_api.h"
#endif

#include "valgrind.h"
#include "js_scheduler.h"
#include "pwm.h"
#include "lamp_main.h"
#include "record_main.h"

#include "sim4g.h"
#include "airlink.h"
#include "encode_audio_queue.h"
#include "encode_audio_input.h"
#include "encode_audio_output.h"
#include "wifi_factory_config.h"
#include "net_qrcode.h"
#include "id_protect.h"
#include "system_ctrl.h"
#include "ble_services.h"
#include "sd_recovery.h"
#include "factory_db.h"
#include "shm_buf_pool.h"
#include "encode_audio.h"
#include "conf_nand.h"
#include "encode_main.h"
#include "watch_rstkey.h"
#include "tencent_video_call.h"
#include "encode_videomask.h"
#include "aliyun_mmi_server.h"

#ifdef PLATFORM_TENCENT
#include "tencent_server.h"
#include "tencent_media_manage.h"
#endif

static char gMainQuit = 0;

JSScheduler sch_sock = NULL, sch_fast = NULL, sch_slow = NULL, sch_disk = NULL;

static void signalINTHandler(int signum)
{
    DBG("Set quit flag!!\n");
    gMainQuit = 1;

    http_signal_handler();

    return ;
}

/*
__const unsigned short int *__ctype_b;
__const __int32_t *__ctype_tolower;
__const __int32_t *__ctype_toupper;
void ctSetup()
{
__ctype_b = *(__ctype_b_loc());
__ctype_toupper = (const __int32_t*)(__ctype_toupper_loc());
__ctype_tolower = (const __int32_t*)(__ctype_tolower_loc());
}
*/

int main()
{
    /*
     * Insure a clean shutdown if user types ctrl-c
     **/
    struct sigaction sigAction;

    sigAction.sa_handler = signalINTHandler;
    sigAction.sa_flags = 0;
    sigemptyset(&sigAction.sa_mask);
    sigaddset(&sigAction.sa_mask, SIGTERM);
    sigaddset(&sigAction.sa_mask, SIGINT);

    sigaction(SIGINT, &sigAction, NULL);
    sigaction(SIGTERM, &sigAction, NULL);

    sigAction.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sigAction, NULL);
    sigAction.sa_handler = SIG_IGN;
    sigAction.sa_flags = 0;
    sigemptyset(&sigAction.sa_mask);
    sigaddset(&sigAction.sa_mask, SIGALRM);
    sigaction(SIGALRM, &sigAction, NULL);

    //SIGABRT

    sigAction.sa_handler = SIG_IGN;
    sigAction.sa_flags = 0;
    sigemptyset(&sigAction.sa_mask);
    sigaddset(&sigAction.sa_mask, SIGABRT);
    sigaction(SIGABRT, &sigAction, NULL);

    //ctSetup();

#if __VALGRIND
    vm_wrap_start();
#endif

    gpio_init();
//  pwm_init();

    init_g_sys();
    system_init_cmdline();
    init_server_log();
    init_server_config();
    init_server_event();

    sch_sock = js_create_scheduler((char *)"sch_sock");
    sch_fast = js_create_scheduler((char *)"sch_fast");
    sch_slow = js_create_scheduler((char *)"sch_slow");
    sch_disk = js_create_scheduler((char *)"sch_disk");
    js_scheduler_set_skip_blocked_detect(sch_disk, 1); // 防止概率出现 sd_recovery 及删 sd 卡文件时卡住
                                                       // 根据先入后出原则，record 中不进行 delete(sch_disk)

    SYSLOG("init timezone...\n");
    init_system_zone();
    init_system_time();

    init_g_log(); /* system_init_cmdline() debug devid depend on me */

    SYSLOG("cpuid %s\n", get_cpuid());

    SYSLOG("init JCPCMD...\n");
    init_server_jcpcmd(sch_sock);

    init_factory_testing();

    SYSLOG("init client wdt log config\n");
    init_client_watchdog_feed(sch_fast);

    init_shm_buf_pool();

    videomask_init();

    encode_audio_init();

    /* ota 升级不播报升级语音 */
    if (is_okey(F_UPGRADE_WIN)) {
        if (is_okey(F_UPGRADE_OTA)) {
            remove(F_UPGRADE_OTA);
        } else {
            encode_audio_queue_push_amr(AUDIO_UPGRAD_SUCCESS_REBOOT, FALSE);
        }
        remove(F_UPGRADE_WIN);
        sync();
    }

    if (get_g_sys(usb_4g)) {
        DBG("play setup 4G audio...\n");
        encode_audio_queue_push_amr(AUDIO_SETUP_4G, FALSE);
    } else if (get_g_sys(usb_wifi)) {
        DBG("play setup wifi audio...\n");
        encode_audio_queue_push_amr(AUDIO_AIRLINK_MODE, FALSE);
        encode_audio_queue_push_amr(AUDIO_SETUP_WIFI, FALSE);
    }

    init_factory_upgrading();

    SYSLOG("init Encode...\n");
    init_encode_server();

    SYSLOG("init ptz...\n");
    ptz_init();

    init_sd_recovery(sch_disk);

    if (!get_g_sys(upgrading)) {
        SYSLOG("init networking...\n");
        init_networking();

        system_eth_rate_init();

        SYSLOG("init 4g...\n");
        sim4g_startup();
#if __WIFI__
        SYSLOG("init wifi...\n");
        wifi_startup();
#endif
    }

    lamp_server_init();
    start_qrcode_server();

    if (!get_g_sys(upgrading)) {
#ifdef WITHRTSP
        SYSLOG("init Stream...\n");
        rtsp_server_init();
#endif

#ifdef WITHHTTP
        SYSLOG("init HttpOnvif...\n");
        init_http_onvif_server();
#endif
    }

    /*
     *  servers -- a listen socket on schedule
     *  TaskScheduler for backgroundhanding
     **/

    record_module_init();
    /*
     *  clients -- a timer task
     *  sch_fast get response accurately, sch_slow something long or delay
     **/

    SYSLOG("init Alarm...\n");
    init_alarm_server(sch_slow);    // 报警模块放在最后启动，以防止调用录像等服务错误

    init_client_log_sync(sch_fast);
    init_client_config_sync(sch_fast);

    SYSLOG("init client autoreboot\n");
    init_client_watch_auto_reboot(sch_slow);

    SYSLOG("init_client_rst_key\n");
    init_client_rst_key(sch_fast);

    SYSLOG("init_tencent_video_call\n");
    init_tencent_video_call(sch_fast);

    SYSLOG("init client iplink\n");
    init_client_iplink_check(sch_slow);

    SYSLOG("init client ntp\n");
    init_client_ntp_update(sch_slow);

    SYSLOG("init client upnp\n");
    init_client_upnp_check(sch_slow);

    SYSLOG("init client ddns\n");
    init_client_ddns(sch_slow);

    init_client_cpu_usage(sch_fast);

    SYSLOG("init server update...\n");
    init_server_upgrade(sch_sock);

    SYSLOG("init server search...\n");
    init_server_search();

    init_server_upnp_discovery(sch_sock);

    init_aging_test();

    validate_devid();

#if defined(ETH_CONF)
    init_ip_adaptive(sch_sock);
#endif

#if defined(CUST_PROT_ZNUPNP)
    init_znpnp_server(sch_sock, sch_slow);
#endif

#if defined(PLATFORM_TENCENT)
    mlock_xp2p_code();
    init_tencent();
#endif

    init_aliyun_mmi();

#if defined(PLATFORM_GB)
    gb_init_server();
#endif

    DropCache(__func__); // 清缓存
    while (!gMainQuit) { sleep(1); }

// EXIT:

    gMainQuit = 1;          // avoid create_pthread() fail

    quit_all_pthread();

    if (!get_g_sys(upgrading)) {
#ifdef WITHRTSP
        rtsp_server_uninit();
#endif
#ifdef WITHHTTP
        uninit_http_onvif_server();
#endif
    }

    uninit_aging_test();

    uninit_server_search();

    js_delete_scheduler(sch_sock);

    uninit_server_upnp_discovery();
    uninit_client_upnp_check();

    uninit_client_ddns();
    uninit_client_ntp_update();

    uninit_client_iplink_check();
    uninit_ip_adaptive();
    uninit_client_cpu_usage();

    uninit_server_log();
    uninit_server_config();
    uninit_server_jcpcmd();
    uninit_alarm_server();
    uninit_qrcode_server();

    videomask_uninit();
    lamp_server_uninit();
    uninit_encode_background();

#if defined(CUST_PROT_ZNUPNP)
    uninit_znpnp_server();
#endif

#if defined(PLATFORM_GB)
    gb_uninit_server();
#endif

#if defined(PLATFORM_TENCENT)
    uninit_tencent();
#endif

    uninit_client_watchdog_feed();

    //gpio_uninit();

    return 0;
}

int system_get_quit(void)
{
    return gMainQuit;
}

/**
 * part1: upgrade 读 shmbuf 所有引用 uninit, 耗时 11秒
 * -----------
 * set_g_sys(upgrading) 禁用 reboot 策略: no_frame,i2c,record,rtsp,wifi,4g // 不反初始化资源，提前通知
 * 释放media引用: record, 及协议 rtsp,ali,onvif,guobiao
 * -----------
 * 远程调试客户设备时防打扰 先调用 gpio -act set -ao_prompt 1
 */
int system_upmedia_uninit(void)
{
    set_g_sys(upgrading);
    // send_conf_nake(JEvent_UpdateBegin);  // do @JCOUpdateBegin, DONT uncomment me

    SYSLOG("uninit_qrcode_server\n");
    uninit_qrcode_server();

    SYSLOG("uninit_aging_test\n");
    uninit_aging_test();

    SYSLOG("uninit_ip_adaptive\n");
    uninit_ip_adaptive();

#ifdef WITHONVIF
    SYSLOG("uninit_onvif_only_http\n");
    uninit_onvif_only_http();
#endif

    SYSLOG("record_module_uninit\n");
    record_module_uninit();

#ifdef WITHRTSP
    SYSLOG("rtsp_server_uninit\n");
    rtsp_server_uninit();
#endif

#ifdef PLATFORM_TENCENT
    SYSLOG("tencent_media_manage_uninit\n");
    tencent_media_manage_uninit();
#endif

#if defined(PLATFORM_GB)
    SYSLOG("gb_uninit_server\n");
    gb_uninit_server();
#endif

    SYSLOG("upgrade lamp_server_uninit\n");
    lamp_server_uninit();

    SYSLOG("uninit_encode_background\n");
    mfree_xp2p_code();
    uninit_encode_background();

    UtilSystemCmd((char *)"/usr/bin/swapoff /dev/zram0");
    UtilSystemCmd((char *)"echo 1 > /sys/block/zram0/reset");
    DropCache(__func__);
    CompactMemo(__func__);
    UtilSystemCmd((char *)"free");

    return SUCCESS;
}

/**
 * part2: encode+其它
 * NVR 有 send.timeout = 15S 的限制，因此
 * uninit_encode_wait() & audio 不在 media 中释放
 * record uninit 后即有足够内存存包
 */
int system_upgrade_uninit(void)
{
    /* transporter: http, p2p platform */
    /* php 自动脚本需要依赖 http 发送 jcp 命令，需要保留 httf 服务 */
    // SYSLOG("uninit_http_onvif_server\n");
    // uninit_http_onvif_server();

#if defined(PLATFORM_TENCENT)
    // SYSLOG("upgrade p2p platform stop\n");
    // uninit_tencent();
#endif
    /* transporter end */

    SYSLOG("upgrade check encode\n");
    uninit_encode_wait();

    uninit_sd_recovery();

#if __VALGRIND
    vm_statistic();
#endif

    SYSLOG("upgrade uninit_client_config_sync\n");
    uninit_client_config_sync();

    SYSLOG("upgrade uninit_client_iplink_check\n");
    uninit_client_iplink_check();

    SYSLOG("upgrade uninit_networking\n");
    uninit_networking();

    SYSLOG("upgrade uninit_ip_adaptive\n");
    uninit_ip_adaptive();

    SYSLOG("upgrade uninit_alarm_server\n");
    uninit_alarm_server();

    SYSLOG("upgrade uninit auto reboot\n");
    uninit_client_watch_auto_reboot();

    // SYSLOG("upgrade uninit_server_search\n");
    // uninit_server_search();

    // SYSLOG("upgrade uninit_client_cpu_usage\n");
    // uninit_client_cpu_usage();

    SYSLOG("uninit_tencent_video_call\n");
    uninit_tencent_video_call();

    SYSLOG("upgrade uninit_client_rst_key\n");
    uninit_client_rst_key();

    //一旦开启就进入 60s 倒计时，只在 REBOOT 接口中使用
    //uninit_client_watchdog_feed();

    return SUCCESS;
}

