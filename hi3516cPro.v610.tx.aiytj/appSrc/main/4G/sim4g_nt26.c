/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : sim4g_nt26.c
 * @Created Time : 2023-01-31
 * @Version      : 3.0
 * @Author       : hul
 * @Description  :
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "io.h"
#include "debug.h"
#include "utils.h"
#include "confapi.h"
#include "jconfig.h"

#include "sim4g.h"
#include "sim4g_nt26.h"
#include "sim4g_adapter.h"
#include "sim4g_common_api.h"

static int sim4g_cfun_off(void)
{
    int ret = SUCCESS;
    char buf[256] = {0};

    ret = sim4g_run_AT_clr("AT+CFUN=0", buf, sizeof(buf));
    usleep(1000*1000);
    if (ret != SUCCESS) {
        sim4g_run_AT_clr("AT+CFUN=0", buf, sizeof(buf));
        usleep(1000*1000);
    }
    SYSLOG("cfun_off success\n");

    return SUCCESS;
}

static int sim4g_cfun_on(void)
{
    int ret = SUCCESS;
    char buf[256] = {0};

    ret = sim4g_run_AT_clr("AT+CFUN=1", buf, sizeof(buf));
    usleep(500*1000);
    if (ret != SUCCESS) {
        sim4g_run_AT_clr("AT+CFUN=1", buf, sizeof(buf));
        usleep(500*1000);
    }
    SYSLOG("cfun_on success\n");

    return SUCCESS;
}

static int sim4g_get_ati(sim_4g_t *info)
{
    int result = 0;
    char ati[64] = {0};
    char buf[1024] = {0};

    sim4g_run_AT_clr("ATI", buf, sizeof(buf));
    result = scanf_AT_result2(buf,  "Revision:.([a-zA-Z0-9_]*)", ati, sizeof(ati));
    goto_if_4gfail(result == SUCCESS, err_ati);

    WriteFile(F_ATI, ati);
    memset(info->SimInfo.fw_version, 0, sizeof(info->SimInfo.fw_version));
    memcpy(info->SimInfo.fw_version, ati, sizeof(info->SimInfo.fw_version));
    return 0;

err_ati:
    return -1;
}

static int sim4g_get_iccid(char *iccid_file)
{
    char iccid[32] = {0};
    char buf[1024] = {0};

    if (!is_okey(iccid_file)) {
        sim4g_run_AT_expect("AT+LCCID", buf, AT_TIMES, sizeof(buf));
        scanf_AT_result2(buf, "LCCID:.([0-9a-zA-Z]*)", iccid, sizeof(iccid));
        if (iccid == NULL || strlen(iccid) != 20) {
            SYSLOG("[iccid=%s] error\n", iccid);
            return FAILURE;
        }
        WriteFile(iccid_file, iccid);
    } else {
        LoadFile2(iccid_file, "%s", iccid);
        SYSLOG("iccid:[%s]\n", iccid);
    }

    return SUCCESS;
}

static int sim4g_get_imei(void)
{
    char imei[32] = {0};
    char buf[1024] = {0};

    if (!is_okey(F_IMEI)) {
        sim4g_run_AT_expect("AT+LGSN=1", buf, AT_TIMES, sizeof(buf));
        scanf_AT_result2(buf, "LGSN:.([0-9]*)", imei, sizeof(imei));
        if (imei == NULL || strlen(imei) != 15) {
            SYSLOG("[imei=%s] error\n", imei);
            return FAILURE;
        }
        WriteFile(F_IMEI, imei);
    } else {
        LoadFile2(F_IMEI, "%s", imei);
        SYSLOG("imei:[%s]\n", imei);
    }

    return SUCCESS;
}

static int sim4g_get_cimi(char *imsi_file)
{
    char imsi[32] = {0};
    char buf[1024] = {0};

    if (!is_okey(imsi_file)) {
        sim4g_run_AT_expect("AT+CIMI", buf, AT_TIMES, sizeof(buf));
        scanf_AT_result2(buf, "([0-9][0-9]*)", imsi, sizeof(imsi));
        if (imsi == NULL || strlen(imsi) != 15) {
            SYSLOG("cimi=[%s] error\n", imsi);
            return FAILURE;
        }
        WriteFile(imsi_file, imsi);
    } else {
        LoadFile2(imsi_file, "%s", imsi);
        SYSLOG("imsi:[%s]\n", imsi);
    }

    return SUCCESS;
}

static int sim4g_set_dial_mode(void)
{
    int result = 0;
    char ati[64] = {0};
    char buf[1024] = {0};

    sim4g_run_AT_clr("AT+ECPCFG?", buf, sizeof(buf));
    result = scanf_AT_result(buf, "usbNet..1");
    if (result != SUCCESS) {
        sim4g_run_AT_clr("AT+ECPCFG=\"usbNet\",1", buf, sizeof(buf));
    }

    sim4g_run_AT_clr("AT+ECNETCFG?", buf, sizeof(buf));

    LoadFile2(F_ATI, "%s", ati);
    //固定 IP 192.168.10.2
    if (scanf_AT_result(ati, "Lierda6180316") == 0) {
        result = scanf_AT_result(buf, "ECNETCFG..1");
        if (result != SUCCESS) {
            result = sim4g_run_AT_clr("AT+ECNETCFG=1", buf, sizeof(buf));
            if (result != SUCCESS) {
                result = scanf_AT_result(buf, "ECNETCFG..1");
            }
        }
    } else {
        result = scanf_AT_result(buf, "nat..1");
        if (result != SUCCESS) {
            result = sim4g_run_AT_clr("AT+ECNETCFG=\"nat\",1", buf, sizeof(buf));
            if (result != SUCCESS) {
                result = scanf_AT_result(buf, "nat..1");
            }
        }
    }
    SYSLOG("success nat\n");

    return result;
}

static int sim4g_soft_reboot(void)
{
    sim4g_cfun_off();
    sim4g_cfun_on();

    return SUCCESS;
}

int sim4g_nt26_flyreset(void)
{
    SYSLOG("fly_reset\n");
    sim4g_soft_reboot();
    sim4g_set_dial_mode();
    sim4g_cfun_off();
    sim4g_cfun_on();

    return SUCCESS;
}

static int sim4g_pwr(eAction gpio_status)
{
    if (gpio_status == E_ACT_OFF) {
        pin_write(E_IO_4G, gpio_status);
        sleep(5);                           //经实测保持下电5s，以电容放完电
    } else if (gpio_status == E_ACT_ON) {
        pin_write(E_IO_4G, gpio_status);
        sleep(3);
    }

    return SUCCESS;
}


static int sim4g_rmmod(void)
{
    DumpFile(USB_MOVE, "1", 1);
    sleep(2);
    UtilSystemCmd("rmmod -f /ipc/drv/rndis_host.ko; rmmod -f /ipc/drv/cdc_ether.ko; rmmod -f /ipc/drv/usbnet.ko; rmmod -f /ipc/drv/cdc-acm.ko");
    SYSLOG("stop 4g succ\n");

    return SUCCESS;
}

static int sim4g_insmod(void)
{
    int count = 0;
    UtilSystemCmd("insmod /ipc/drv/usb/cdc-acm.ko; insmod /ipc/drv/usb/usbnet.ko; insmod /ipc/drv/usb/cdc_ether.ko; insmod /ipc/drv/usb/rndis_host.ko;");
    sleep(1);
    while (count < 5 && (!is_okey(USBDEV_IDVENDOR))) {
        count++;
        sleep(1);
        DBG("wait @insmod add ttyusb count = %d\n", count);
    }
    UtilSystemCmd("test -e /dev/tty4G || ln -sf /dev/ttyACM0 /dev/tty4G");

    return SUCCESS;
}

int sim4g_nt26_pwr_reset(void)
{
    sim4g_rmmod();
    sim4g_pwr(E_ACT_OFF);
    sim4g_pwr(E_ACT_ON);
    sim4g_insmod();

    return SUCCESS;
}

/* 兼容产测模式内置卡检测成功率,以获取iccid判断存在内置卡 */
static int sim4g_detect_inside_sim(void)
{
    int esim_exist = TRUE;
    char eiccid[32] = {0};

    if (!is_okey(F_INSIDE_SIM)) {
        SYSLOG("detect inside_sim\n");
        sim4g_cfun_off();
        pin_write(E_IO_INSIDE_SIM, E_ACT_ON);
        sim4g_cfun_on();

        if (!get_g_run(sim4g, RUN_ESIM_FAIL)) {
            esim_exist = (sim4g_get_iccid(F_EICCID) == SUCCESS)?TRUE:FALSE;
        }
    }  else {
        LoadFile2(F_EICCID, "%s", eiccid);
        SYSLOG("eiccid:[%s]\n", eiccid);
    }

    if (get_g_run(sim4g, RUN_ESIM_FAIL)) {  //模拟内置卡失败
        SYSLOG("simulation inside_sim not exist\n");
        esim_exist = FALSE;
    }

    return esim_exist;
}

static int sim4g_detect_outside_sim(void)
{
    int sim_exist = TRUE;
    char iccid[32] = {0};

    if (!is_okey(F_INSIDE_SIM)) {
        SYSLOG("detect outside_sim\n");
        sim4g_cfun_off();
        pin_write(E_IO_OUTSIDE_SIM, E_ACT_ON);
        sim4g_cfun_on();
        sim_exist = (sim4g_get_iccid(F_ICCID) == SUCCESS)?TRUE:FALSE;
        if (!get_g_sys(factest)) {  //非产测模式未检测到外置卡,产测模式加速处理
            sim_exist = (sim4g_get_iccid(F_ICCID) == SUCCESS)?TRUE:FALSE;
            if (sim_exist != TRUE) {
                sim4g_nt26_pwr_reset();
                sim_exist = (sim4g_get_iccid(F_ICCID) == SUCCESS)?TRUE:FALSE;
            }
        }
    }  else {
        LoadFile2(F_ICCID, "%s", iccid);
        SYSLOG("iccid:[%s]\n", iccid);
    }

    return sim_exist;
}

static int sim4g_env_init(sim_4g_t *info)
{
    int ret = 0;
    int sim_exist = FALSE;
    int esim_exist = FALSE;

    sim4g_get_ati(info);

    do {
        //外置卡优先上网,外置卡未检测到切换到内置卡,兼容单卡和双卡
        if (pin_exist(E_IO_OUTSIDE_SIM)) {
            sim_exist = sim4g_detect_outside_sim();
        }

        if (pin_exist(E_IO_INSIDE_SIM) && (!sim_exist)) { //内置卡存在且外置卡未检测到
            esim_exist = sim4g_detect_inside_sim();
        }
        

        if (is_okey(F_ICCID)) {
            WriteFile(F_INSIDE_SIM, "0");
        } else {
            WriteFile(F_INSIDE_SIM, "1");
        }

        sim4g_get_imei();
        sim4g_get_cimi(F_IMSI);
        goto_if_4gfail((esim_exist || sim_exist),  hard_err);

        ret += sim4g_set_dial_mode();
        goto_if_4gfail(ret == SUCCESS,  soft_err);
    } while(0);

    return SUCCESS;

soft_err:
    info->prev_ev = info->curr_ev;
    info->curr_ev = EVENT_FLY_RESET;
    SYSLOG("__soft_err from line: %d\n", __LINE__);

    return FAILURE;

hard_err:
    info->prev_ev = info->curr_ev;
    info->curr_ev = EVENT_PWR_RESET;
    SYSLOG("__hard_err from line: %d\n", __LINE__);

    return FAILURE;
}

static int sim4g_autocall(sim_4g_t *info)
{
    int ntry = 0;
    int count = 0;
    int ret  = SUCCESS;
    char buf[1024] = {0};

    sim4g_run_AT_clr("AT+ECNETDEVCTL=3,0,0", buf, sizeof(buf));
    sim4g_run_AT_clr("AT+CGDCONT=0,\"IP\",\"\"", buf, sizeof(buf));

    do {
        sim4g_run_AT_clr("AT+CGACT=1,0", buf, sizeof(buf));
        sim4g_run_AT_clr("AT+CGACT?", buf, sizeof(buf));
        ret = scanf_AT_result(buf, "CGACT:.0.1");
        if (ret == SUCCESS) {
           	break;
        }
    } while(ntry++ < 2);

    while (count < 5) {
        sim4g_run_AT_clr("AT+CEREG?", buf, sizeof(buf));
        ret = scanf_AT_result(buf, "0.1");
        if (ret == SUCCESS) {
            break;
        }
        sleep(1);
        count++;
    }

    sim4g_run_AT_clr("AT+CGPADDR", buf, sizeof(buf));
    goto_if_4gfail(ret == SUCCESS, err_autocall);

    return SUCCESS;

err_autocall:
    info->prev_ev = info->curr_ev;
    info->curr_ev = EVENT_REFRESH;
    SYSLOG("__soft_err from line: %d\n", __LINE__);
    return FAILURE;
}

//产测模式:内置卡优先上网,IMSI不检测
int sim4g_nt26_factory(void *data)
{
    int ret = SUCCESS;
    int sim_exist = FALSE;
    int esim_exist = FALSE;
    sim_4g_t *info = (sim_4g_t*)data;

    //兼容双卡,单外置卡,单内置卡检测
    if (pin_exist(E_IO_OUTSIDE_SIM)) {
        sim_exist  = sim4g_detect_outside_sim();
    }
    if (pin_exist(E_IO_INSIDE_SIM)) {
        esim_exist = sim4g_detect_inside_sim();
    }
    goto_if_4gfail((esim_exist == TRUE || sim_exist == TRUE), hard_err);

    ret = sim4g_get_imei();
    goto_if_4gfail(ret == SUCCESS, hard_err);

    //选择sim卡
    if ((!is_okey(F_EICCID)) && is_okey(F_ICCID)) {
        if (pin_exist(E_IO_INSIDE_SIM) && pin_exist(E_IO_OUTSIDE_SIM)) { //双卡模式,内置卡不存在或损坏
            sim4g_cfun_off();
            pin_write(E_IO_OUTSIDE_SIM, E_ACT_ON);
            sim4g_cfun_on();
        }
        WriteFile(F_INSIDE_SIM, "0");
    } else {
        WriteFile(F_INSIDE_SIM, "1");
    }

    sim4g_set_dial_mode();
    sim4g_autocall(info);
    UtilSystemCmd("/ipc/bin/4g udhcpc");

    return SUCCESS;

hard_err:
    info->prev_ev = info->curr_ev;
    info->curr_ev = EVENT_PWR_RESET;
    SYSLOG("__hard_err from line: %d\n", __LINE__);

    return FAILURE;
}

int sim4g_nt26_init(void *data)
{
    int ret = 0;
    sim_4g_t *info = (sim_4g_t *)data;

    ret = sim4g_env_init(info);
    goto_if_4gfail(ret == SUCCESS, init_err);

    ret = sim4g_autocall(info);
    UtilSystemCmd("/ipc/bin/4g udhcpc");
    goto_if_4gfail(ret == SUCCESS, init_err);

    return SUCCESS;
init_err:
    return ret;
}

