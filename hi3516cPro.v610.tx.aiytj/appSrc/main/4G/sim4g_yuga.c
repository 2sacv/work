/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : sim4g_yuga.c
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
#include <stdbool.h>

#include "io.h"
#include "debug.h"
#include "utils.h"
#include "jevent.h"
#include "confapi.h"
#include "jconfig.h"
#include "conf_list.h"
#include "coordinate_system.h"

#include "sim4g.h"
#include "sim4g_yuga.h"
#include "sim4g_adapter.h"
#include "sim4g_common_api.h"

static int sim4g_cfun_off(void)
{
    int ret = SUCCESS;
    char buf[256] = {0};

    ret = sim4g_run_AT_clr("AT+CFUN=0", buf, sizeof(buf));
    usleep(500*1000);
    if (ret != SUCCESS) {
        sim4g_run_AT_clr("AT+CFUN=0", buf, sizeof(buf));
        usleep(500*1000);
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
    result = scanf_AT_result2(buf,  "(^Y[a-zA-Z0-9_.]*)", ati, sizeof(ati));
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
        sim4g_run_AT_expect("AT+ICCID", buf, AT_TWO_TIMES, sizeof(buf));
        scanf_AT_result2(buf, "ICCID:.([0-9a-zA-Z]*)", iccid, sizeof(iccid));
        if (strlen(iccid) != 20) {
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

static int sim4g_get_cpin()
{
    char buf[256] = {0};
    sim4g_run_AT_expect("AT+CPIN?", buf, AT_TIMES, sizeof(buf));
    if (scanf_AT_result(buf, "READY") != SUCCESS) {
        SYSLOG("cpin fail\n");
        return FAILURE;
    }

    SYSLOG("cpin success\n");
    return SUCCESS;
}

static int sim4g_get_imei(void)
{
    char imei[32] = {0};
    char buf[1024] = {0};

    if (!is_okey(F_IMEI)) {
        sim4g_run_AT_expect("AT+CGSN", buf, AT_TIMES, sizeof(buf));
        scanf_AT_result2(buf, "([0-9]{15})", imei, sizeof(imei));
        if (strlen(imei) != 15) {
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

static int sim4g_mobile_card_band_fdd(sim_4g_t *info)
{
    char buf[1024] = {0};
    Sim4g sim4gcfg = {0};

    get_config(handleSim4gCfg, sim4gcfg);
    info->SimInfo.fdd = sim4gcfg.fdd;

    if (info->SimInfo.fdd == 1 && info->SimInfo.operators == E_MOBILE) { // 移动卡且FDD优先
        DBG(">>sim4g band FDD\n");
        sim4g_run_AT_clr("AT+ECBAND=1,3,5,8", buf, sizeof(buf));
    } else {
        DBG(">>sim4g band all\n");
        sim4g_run_AT_clr("AT+ECBAND=1,3,5,8,34,38,39,40,41", buf, sizeof(buf));
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
        if (strlen(imsi) != 15) {
            SYSLOG("cimi=[%s] error\n", imsi);
            return FAILURE;
        }
        WriteFile(imsi_file, imsi);

        DBG(">>sim4g detect card imsi: %s \n", imsi);
    } else {
        LoadFile2(imsi_file, "%s", imsi);
        SYSLOG("imsi:[%s]\n", imsi);
    }

    return SUCCESS;
}

static int sim4g_set_inside_sim(void)
{
    int ret = SUCCESS;
    char buf[256] = {0};

    ret = sim4g_run_AT_expect("AT+SIMCROSS=1", buf, AT_TIMES, sizeof(buf));
    usleep(500*1000);
    SYSLOG("set_inside_sim success\n");

    return ret;
}

/* 兼容产测模式内置卡检测成功率,以获取iccid判断存在内置卡 */
static int sim4g_detect_inside_sim(void)
{
    int esim_exist = TRUE;

    SYSLOG("detect inside_sim\n");
    sim4g_cfun_off();
    sim4g_set_inside_sim();
    sim4g_cfun_on();

    if (!get_g_run(sim4g, RUN_ESIM_FAIL)) {
        esim_exist = (sim4g_get_iccid(F_EICCID) == SUCCESS)?TRUE:FALSE;
    }

    if (get_g_run(sim4g, RUN_ESIM_FAIL)) {  //模拟内置卡失败
        SYSLOG("simulation inside_sim not exist\n");
        esim_exist = FALSE;
    }

    return esim_exist;
}

static int sim4g_set_outside_sim(void)
{
    int ret = SUCCESS;
    char buf[256] = {0};

    ret = sim4g_run_AT_expect("AT+SIMCROSS=0", buf, AT_TIMES, sizeof(buf));
    usleep(500*1000);
    SYSLOG("set_outside_sim success\n");

    return ret;
}

static int sim4g_detect_outside_sim(void)
{
    int sim_exist = TRUE;
    char buf[128] = {0};

    SYSLOG("detect outside_sim\n");
    sim4g_cfun_off();
    sim4g_set_outside_sim();
    sim4g_cfun_on();

    sim_exist = (sim4g_get_iccid(F_ICCID) == SUCCESS)?TRUE:FALSE;
    if (sim_exist != TRUE) {
        sim4g_run_AT_expect("AT+CPIN?", buf, AT_TWO_TIMES, sizeof(buf));
        scanf_AT_result(buf, "READY");
        sim_exist = (sim4g_get_iccid(F_ICCID) == SUCCESS)?TRUE:FALSE;
    }

    return sim_exist;
}

static int sim4g_soft_reboot(void)
{
    sim4g_cfun_off();
    sim4g_cfun_on();

    return SUCCESS;
}

static int sim4g_get_sysinfo()
{
    char srv_status[32] = {0};
    char buf[1024] = {0};

    sim4g_run_AT_expect("AT^SYSINFO", buf, AT_TWO_TIMES, sizeof(buf));
    scanf_AT_result2(buf, "SYSINFO:.([0-9]*)", srv_status, sizeof(srv_status));

    int status = atoi(srv_status);
    WAR("sysinfo status:%d\n", status);

    if (status == 2) {
        return SUCCESS;
    }

    return FAILURE;
}

static int sim4g_env_init(sim_4g_t *info)
{
    int ret = SUCCESS;
    char buf[1024] = {0};
    int sim_exist = FALSE;
    int esim_exist = FALSE;
    int sim_imsi = FAILURE;
    int esim_imsi = FAILURE;

    eOperatorsType eoperator_type = E_UNKNOWN;
    eOperatorsType operator_type = E_UNKNOWN;

    sim4g_get_ati(info);
    sim4g_run_AT_expect("AT+CIPGSMLOC=?", buf, AT_TIMES, sizeof(buf));
    ret = scanf_AT_result(buf, "ERROR");
    if (ret == SUCCESS) {
        info->SimInfo.location_enable = 0;
    } else {
        info->SimInfo.location_enable = 1;
    }
    sim4g_run_AT_expect("AT+ECPCFG=\"usbNet\",0", buf, AT_TIMES, sizeof(buf));
    sim4g_run_AT_expect("AT+ECPCFG?", buf, AT_TIMES, sizeof(buf));
    ret = scanf_AT_result(buf, "\"usbNet\":0");
    if (ret != SUCCESS) {
        SYSLOG("usbNet ERROR\n");
    }

    do {
        sim_exist = sim4g_detect_outside_sim();
        if (TRUE != sim_exist) {
            ERR("sim is not exist\n");
            break;
        }

        sim_imsi = sim4g_get_cimi(F_IMSI);
        if (SUCCESS != sim_imsi) {
            ERR("sim_imsi get error\n");
        }
        sim4g_is_mobile_card(F_ICCID, &operator_type);
    } while(0);

    do {
        esim_exist = sim4g_detect_inside_sim();
        if (TRUE != esim_exist) {
            ERR("sim is not exist\n");
            break;
        }

        esim_imsi = sim4g_get_cimi(F_EIMSI);
        if (SUCCESS != esim_imsi) {
            ERR("esim_imsi get error\n");
        }
        sim4g_is_mobile_card(F_EICCID, &eoperator_type);
    } while(0);
    goto_if_4gfail(((TRUE == esim_exist) || (TRUE == sim_exist)),  hard_err);
    goto_if_4gfail(((SUCCESS == sim_imsi) || (SUCCESS == esim_imsi)),  hard_err);

    ret = sim4g_get_imei();
    goto_if_4gfail(ret == SUCCESS, hard_err);

    if (operator_type == E_MOBILE && eoperator_type == E_TELECOM) {
        info->SimInfo.sim_status = E_CARD_NORMAL;
    } else {
        info->SimInfo.sim_status = E_CARD_UNNORMAL;
    }

    if ((E_UNKNOWN == eoperator_type || info->SimInfo.operators != eoperator_type) && TRUE == sim_exist) {
        info->SimInfo.operators = operator_type;
        info->inside_sim = FALSE;
        sim4g_detect_outside_sim();
    } else {
        info->SimInfo.operators = eoperator_type;
        info->inside_sim = TRUE;
    }

    sim4g_mobile_card_band_fdd(info);
    DBG("operators_type:%d, eoperator_type:%d\n", info->SimInfo.operators, eoperator_type);

    return SUCCESS;

// soft_err:
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

static int sim4g_is_fdd_query()
{
    char *p = NULL;
    int ret = SUCCESS;
    char cmdbuf[256] = {0};
    char buf[256] = {0};
    char model[8] = {0,};

    ret = sim4g_get_sysinfo();

    if (ret != SUCCESS) {
        return MODLE_NULL;
    }
    ret = sim4g_run_AT_expect("AT*BANDIND?", buf, AT_TIMES, sizeof(buf));
    goto_if_4gfail(ret == SUCCESS, __fail);

    p = strstr(buf, "*BANDIND:");
    if (p) {
        strtok(p, "\r\n");
        WriteFile("/tmp/bandind", p);
        sprintf(cmdbuf, "awk -F \",\" '{print $2}' /tmp/bandind");
        ret = ReadCmdResult(cmdbuf, model, sizeof(model));
        goto_if_4gfail(ret > 0, __fail);
    }

    if (strstr(model, "1") || strstr(model, "3") || strstr(model, "5") || strstr(model, "8")) {
        return MODLE_FDD;
    } else {
        return MODLE_TDD;
    }

__fail:
    return 0;
}

static int sim4g_change_fdd_tdd(sim_4g_t *info)
{
    char buf[1024] = {0};

    info->SimInfo.is_fdd = sim4g_is_fdd_query();
    if (info->SimInfo.is_fdd == 0) {    //未注网
        sim4g_run_AT_clr("AT+ECBAND=1,3,5,8,34,38,39,40,41", buf, sizeof(buf));
    }

    return SUCCESS;
}

static void sim4g_network_check(void)
{
    int ret = 0;
    char buf[256] = {0};

    sim4g_get_cpin();
    ret = sim4g_get_sysinfo();
    if (ret != SUCCESS) {
        sleep(3);
        sim4g_get_sysinfo();
    }
    sim4g_run_AT_expect("AT+CEREG?", buf, AT_TWO_TIMES, sizeof(buf));
    return;
}

int sim4g_ygx09_init(void *data)
{
    int ret = 0;
    sim_4g_t *info = (sim_4g_t *)data;

    ret = sim4g_env_init(info);
    goto_if_4gfail(ret == SUCCESS, init_err);

    UtilSystemCmd("/ipc/bin/4g udhcpc");
    sim4g_network_check();
    sim4g_change_fdd_tdd(info);
    goto_if_4gfail(ret == SUCCESS, init_err);

    return SUCCESS;

init_err:
    return ret;
}

int sim4g_ygx09_flyreset(void)
{
    SYSLOG("fly_reset\n");

    sim4g_soft_reboot();

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
    UtilSystemCmd("rmmod -f /ipc/drv/option.ko; rmmod -f /ipc/drv/usb_wwan.ko; rmmod -f /ipc/drv/rndis_host.ko; rmmod -f /ipc/drv/cdc_ether.ko");
    SYSLOG("stop 4g succ\n");

    return SUCCESS;
}

static int sim4g_insmod(void)
{
    int count = 0;
    UtilSystemCmd("insmod /ipc/drv/usb/cdc_ether.ko; insmod /ipc/drv/usb/rndis_host.ko; insmod /ipc/drv/usb/usb_wwan.ko; insmod /ipc/drv/usb/option.ko;");
    sleep(1);
    while (count < 5 && (!is_okey(USBDEV_IDVENDOR))) {
        count++;
        sleep(1);
        DBG("wait @insmod add ttyusb count = %d\n", count);
    }

    UtilSystemCmd("test -e /dev/tty4G || ln -sf /dev/ttyACM0 /dev/tty4G");

    return SUCCESS;
}

int sim4g_ygx09_pwr_reset(void)
{
    /* 解决 ping 过程中 rmmod 导致内核 Oops
     * usleep 防止 ping 进程启动慢
     **/
    usbdev_busy = TRUE;
    usleep(500*1000);
    UtilSystemCmd("killall -9 ping");

    sim4g_rmmod();
    sim4g_pwr(E_ACT_OFF);
    sim4g_pwr(E_ACT_ON);
    sim4g_insmod();

    usbdev_busy = FALSE;

    return SUCCESS;
}

int sim4g_ygx09_location(Sim4g *info)
{
    if (info == NULL) {
        return FAILURE;
    }

    int step = 0;
    char *p = NULL;
    char *str = NULL;
    char buf[256] = {0};
    char at_buf[128] = {0};
    char latitude[32] = {0};
    char longitude[32] = {0};
    double wgs_lat, wgs_lng;

    // 配置网络
    sim4g_run_AT_expect("AT+SAPBR=3,1,\"Contype\",\"GPRS\"", buf, AT_TIMES, sizeof(buf));
    sim4g_run_AT_expect("AT+SAPBR=3,1,\"APN\",\"\"", buf, AT_TIMES, sizeof(buf));
    // 查询承载状态
    sim4g_run_AT_clr("AT+SAPBR=2,1", buf, sizeof(buf));
    if (scanf_AT_result(buf, "0.0.0.0") == SUCCESS) {
        // 激活该承载的 GPRS PDP 上下文
        sim4g_run_AT_clr("AT+SAPBR=1,1", buf, sizeof(buf));
    }
    snprintf(at_buf, sizeof(at_buf)-1, "AT+CIPGSMLOC=1,1,\"%s\"", info->token);
    if (sim4g_run_AT_expect_sec(AT_TIMEOUT, at_buf, buf, AT_TWO_TIMES, sizeof(buf)) != SUCCESS) {
        return FAILURE;
    }

    // +CIPGSMLOC: 0,26.876325,112.537748,2025/06/19,11:43:30
    p = strstr(buf, "+CIPGSMLOC:");
    if (p) {
        // 根据 ',' 分隔整个字符串，取中间两个字符串 latitude 和 longitude
        while ((str = strtok(p, ","))) {
            switch(step++) {
            case 0:
                p = NULL;
                break;
            case 1:     // 纬度 26.876325
                DBG("latitude: %s\n", str);
                snprintf(latitude, sizeof(latitude)-1, "%s", str);
                latitude[strlen(latitude)] = '\0';
                break;
            case 2:     // 经度 112.537748
                DBG("longitude: %s\n", str);
                snprintf(longitude, sizeof(longitude)-1, "%s", str);
                longitude[strlen(longitude)] = '\0';
                break;
            default:
                break;
            }
        }

        if (strlen(longitude) > 0 && strlen(latitude) > 0) {
            // 域格基站定位返回的是 GCJ02 坐标系
            gcj02_to_wgs84(atof(longitude), atof(latitude), &wgs_lng, &wgs_lat);

            sprintf(latitude, "%f", wgs_lat);
            sprintf(longitude, "%f", wgs_lng);
            DBG("[GCJ-02 -> WGS-84] longitude: %s, latitude: %s\n", longitude, latitude);
            if ((strlen(longitude) > 0 && 0 != strcmp(info->longitude, longitude)) ||
                (strlen(latitude) > 0 && 0 != strcmp(info->latitude, latitude))) {
                strncpy(info->longitude, longitude, sizeof(info->longitude));
                strncpy(info->latitude, latitude, sizeof(info->latitude));
                DumpFile2(F_LONGITUDE, "%s", longitude);
                DumpFile2(F_LATITUDE, "%s", latitude);
            }
        } else {
            ERR("Failed to obtain longitude and latitude\n");
            return FAILURE;
        }
    }

    return SUCCESS;
}

