/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    : delay_e04-15
 * Version      : 1.0
 * Author       : cheby
 * Description  :
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <utime.h>
#include <sys/ioctl.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <linux/reboot.h>
#include <netinet/in.h>
#include <openssl/md5.h>

#include "g_sys.h"
#include "g_stat.h"
#include "g_run.h"
#include "g_log.h"
#include "system_main.h"
#include "delay_exec.h"
#include "utils.h"
#include "confapi.h"
#include "fifo_queue.h"
#include "conf_list.h"
#include "pthread_manage.h"
#include "logapi.h"
#include "system_ctrl.h"
#include "confutils.h"
#include "time_config.h"
#include "jconfig.h"
#include "system_sch.h"
#include "alarm_log.h"
#include "alarmapi.h"
#include "system_ctrl.h"
#include "jevent.h"
#include "airlink.h"
#include "net_check.h"

#ifdef PLATFORM_TENCENT
#include "tencent_server.h"
#endif

static queue_t * sDelayExecQueue = NULL;
static JSTCHandle  hdl_reboot = NULL;

#define DELAY_REBOOT_TIME (15*1000)

void sync_syslog()
{
    UtilSystemCmd("/ipc/bin/toggle tar");
}

static void kill_all_app()
{
    exit(99);
}

void exit_jco_server(void)
{
    //remove_sdcrad();
    sleep (1);
    _exit(99); // 重启操作在 auto_run 中进行
    //reboot(LINUX_REBOOT_CMD_RESTART);
}

static void reboot_linux()
{
    char epoch[32] = {0};
    snprintf(epoch, sizeof(epoch), "date @%lld\n", time(NULL)+ 14 /* start up spend */);
    DumpFile("/opt/conf/reboot.epoch", epoch, strlen(epoch));
    sync();

    SYSLOG("uninit watchdog avoid following steps block\n");
    uninit_client_watchdog_feed();

#if __WIFI__
    if(get_g_sys(usb_wifi)) {
        set_g_run(wifi, RUN_PAUSE_WIFI);
        wifi_stop();
    }
#endif

    uninit_server_config();
    alarm_log_sync();
    log_sync();
    sync_syslog();      // roll syslog only when reboot
    alarm_sync_log();

    sleep(3);
    sync();
    sleep(1);

    UtilSystemCmd("/ipc/bin/lzbox wait_cache_sync");

    COLOR_Y("---reboot_linux -----\n");
    //reboot(LINUX_REBOOT_CMD_RESTART);
    UtilSystemCmd("echo 1 > /sys/class/mmc_host/mmc0/reboot");
    reboot(LINUX_REBOOT_CMD_RESTART);
}

static void delay_cmd_handle__reboot(DELAY_EXEC_S* delayCmd)
{
    LOG("------ REBOOT from %s -----------\n", (char *)delayCmd->param);
    SYSLOG("------ REBOOT from %s -----------\n", (char *)delayCmd->param);
    reboot_linux();
}

static void delay_cmd_handle__rebootconf(DELAY_EXEC_S* delayCmd)
{
    LOG("------ REBOOT from %s -----------\n", (char *)delayCmd->param);
    SYSLOG("------ REBOOT from %s -----------\n", (char *)delayCmd->param);

    alarm_log_sync();
    log_sync();
    sync_syslog();      // roll syslog only when reboot

    sync();
    sleep(1);

    exit_jco_server();
}

static void  delay_cmd_handle__resetAllApp(DELAY_EXEC_S* delayCmd)
{
    LOG("------ RESETAPP from %s -----------\n", (char *)delayCmd->param);
    SYSLOG("------ RESETAPP from %s -----------\n", (char *)delayCmd->param);
    uninit_server_config();
    log_sync();
    sync();
    sleep(1);
    kill_all_app();
    reboot(LINUX_REBOOT_CMD_RESTART);
}

static void  delay_cmd_handle__setethip(DELAY_EXEC_S* delayCmd)
{
    DBG("delay execute cmd=%d ethip set!\n", delayCmd->cmd);
    sleep(1);
    UtilSystemCmd("/ipc/bin/networking eth0 ip");
    sleep(1);
    send_event(JEvent_TencentReset);
}

static void delay_cmd_handle__default(DELAY_EXEC_S* delayCmd)
{
    DBG("delay execute cmd=%d, xml default!\n", delayCmd->cmd);
    LOG("delay execute cmd=%d, xml default!\n", delayCmd->cmd);
#if defined(PLATFORM_TENCENT)
        if (platform_on_line() && www_reachable()) {
            uninit_tencent_async();
        }
#endif

    uninit_client_config_sync();
    if (delayCmd->cmd == DELAY_CMD_DEFAULT_KEEP_NET) {
        UtilSystemCmd("/ipc/bin/reset2factory keep_net");
    } else {
        UtilSystemCmd("/ipc/bin/reset2factory");
    }
    reboot_linux();
}

static void delay_cmd_handle_dns(DELAY_EXEC_S* delayCmd)
{
    DBG("delay execute cmd=%d dns set!\n", delayCmd->cmd);
    sleep(1);

    UtilSystemCmd("/ipc/bin/networking eth0 dns &");
}

static void delay_cmd_handle_setethmac(DELAY_EXEC_S* delayCmd)
{
    //char *szMAC = (char *)delayCmd->param;
    DBG("delay execute cmd=%d set mac!\n", delayCmd->cmd);
    LOG("delay execute cmd=%d set mac!\n", delayCmd->cmd);

    //processEthMac(ConfSet, "eth0", szMAC);
    delay_one_minute_reboot();
    //reboot_linux();
}

static void delay_cmd_handle__setUpdateRestart(DELAY_EXEC_S* delayCmd)
{
#ifdef __migrate_conf_over__
    DBG("delay execute cmd=%d set update port!\n", delayCmd->cmd);
    LOG("delay execute cmd=%d set update port!\n", delayCmd->cmd);
    return restart_server_upgrade();
#endif
}

int handle_delay_exec(DELAY_EXEC_S* delayCmd)
{
    /*
       if (UPDATE_CMD_BEGIN < GetUpdateCmd() && UPDATE_CMD_END >  GetUpdateCmd())
        {
            // 升级过程中不允许执行重启等可以中断升级的指令
            if (DELAY_CMD_REBOOT == delayCmd->cmd ||
                DELAY_CMD_KILL_ALL_APP == delayCmd->cmd ||
                DELAY_CMD_SETETHMAC == delayCmd->cmd)
            {
                goto _delayExecCleanup;
            }
        }
    */
    switch (delayCmd->cmd) {
        case DELAY_CMD_REBOOT: {
            // 用DELAY_REBOOT_LINUX() 替代 delay_ctrl_exec(DELAY_CMD_REBOOT);
            delay_cmd_handle__reboot(delayCmd);
            break;
        }

        case DELAY_CMD_REBOOTCONF: {
            // 用DELAY_REBOOT_LINUX() 替代 delay_ctrl_exec(DELAY_CMD_REBOOT);
            delay_cmd_handle__rebootconf(delayCmd);
            break;
        }

        case DELAY_CMD_KILL_ALL_APP: {
            // 用DELAY_RESET_APPS() 替代 delay_ctrl_exec(DELAY_CMD_KILL_ALL_APP);
            delay_cmd_handle__resetAllApp(delayCmd);
            break;
        }

        case DELAY_CMD_SETETHIP: {
            delay_cmd_handle__setethip(delayCmd);
            break;
        }

        case DELAY_CMD_DEFAULT:
        case DELAY_CMD_DEFAULT_KEEP_NET: {
            delay_cmd_handle__default(delayCmd);
            break;
        }
        case DELAY_CMD_SETDNS: {
            delay_cmd_handle_dns(delayCmd);
            break;
        }
        case DELAY_CMD_SETETHMAC: {
            delay_cmd_handle_setethmac(delayCmd);
            break;
        }
        case DELAY_CMD_SETUPDATEPORT: {
            delay_cmd_handle__setUpdateRestart(delayCmd);
            break;
        }
        default: {
            DBG("delay execute cmd=%d unknown cmd type!\n", delayCmd->cmd);
            break;
        }
    }

// _delayExecCleanup:

    if (delayCmd->param) {
        free(delayCmd->param);
    }

    free(delayCmd);
    return SUCCESS;
}

static void *delay_exec_service(void* data)
{
    DELAY_EXEC_S *delayCmd = NULL;

    SYSLOG("delay exec loop [ppid:%ld pid:%d]pthread_self(%p)...\n", syscall(SYS_gettid), getpid(), pthread_self());
    while(0 == system_get_quit()) {
        DBG("delay_exec_service\n");
        if(sDelayExecQueue == NULL) {
            usleep(50*1000);
            continue;
        }

        delayCmd = (DELAY_EXEC_S *)fifo_queue_pop(sDelayExecQueue);

        if(delayCmd == NULL)
            continue;

        handle_delay_exec(delayCmd);

        usleep(10*1000); // pretect cpu too high when much delay task
    }

    return NULL;
}

int init_delay_exec_env(void)
{
    sDelayExecQueue = create_fifo_queue();
    if(sDelayExecQueue == NULL)
        return -1;

    if (pthread_namecreate("thrd_delay_cmd", delay_exec_service, NULL) == 0) {
        DBG("create pthread delay execute is error \n");
        return FAILURE;
    }

    return 0;
}


int delay_ctrl_exec(DELAY_CMD_E cmd, void *param, int len)
{
    static int sDealyExecInitFlag = 0;

    DBG("JCODelayExec cmd:%d\n", cmd);

    if( !sDealyExecInitFlag ) {
        if(init_delay_exec_env() == 0) {
            sDealyExecInitFlag = 1;
            DBG("init_delay_exec_env success!\n");
        }
    }

    if(sDelayExecQueue == NULL)
        return FAILURE;

    if (DELAY_CMD_BEGIN >= cmd || DELAY_CMD_END <= cmd) {
        return FAILURE;
    }

    DELAY_EXEC_S *delayCmd = NULL;
    if (NULL == (delayCmd = malloc(sizeof(DELAY_EXEC_S)))) {
        return FAILURE;
    }
    memset(delayCmd, 0, sizeof(DELAY_EXEC_S));
    delayCmd->cmd = cmd;

    if (0 < len && param) {
        if (NULL == (delayCmd->param = malloc(len + 1))) {
            free(delayCmd);
            return FAILURE;
        }

        memcpy(delayCmd->param, param, len);
        ((char *)(delayCmd->param))[len] = '\0';
        delayCmd->len = len;
    }

    fifo_queue_push(sDelayExecQueue, (void *) delayCmd);

    return SUCCESS;
}

static void delay_reboot_linux()
{
    reboot_linux();
}

void secs_delay_reboot(int sec, const char *func)
{
    SYSLOG("------ delay %ds REBOOT from %s -----------\n", sec, func);
    js_create_once(hdl_reboot, sch_slow, sec*1000, delay_reboot_linux, NULL);
}

void delay_one_minute_reboot()
{
    if (hdl_reboot){
        COLOR_Y("delay_one_minute_reboot doing , can not retry!\n");
        return;
    }
    if (get_g_sys(factest)) {
        DBG("delay_one_minute_reboot is factest\n");
        return;
    }

    js_create_once(hdl_reboot, sch_slow, DELAY_REBOOT_TIME, delay_reboot_linux, NULL);
}

void now_rbeoot_linux()
{
    reboot_linux();
}

