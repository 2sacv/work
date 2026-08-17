/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : sim4g_800e.c
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
#include "conf_list.h"
#include "jevent.h"

#include "sim4g.h"
#include "sim4g_800e.h"
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
        sim4g_run_AT_expect("AT+QCCID", buf, AT_TWO_TIMES, sizeof(buf));
        scanf_AT_result2(buf, "QCCID:.([0-9a-zA-Z]*)", iccid, sizeof(iccid));
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

static int sim4g_get_imei(void)
{
    char imei[32] = {0};
    char buf[1024] = {0};

    if (!is_okey(F_IMEI)) {
        sim4g_run_AT_expect("AT+CGSN=1", buf, AT_TIMES, sizeof(buf));
        scanf_AT_result2(buf, "CGSN:..([0-9]*)", imei, sizeof(imei));
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

int sim4g_mobile_card_band_fdd(sim_4g_t *info)
{
    char buf[1024] = {0};
    if (info->SimInfo.fdd == MODLE_FDD && info->SimInfo.operators == E_MOBILE) { // 移动卡且FDD优先
        DBG(">>sim4g band FDD\n");
        sim4g_run_AT_clr("AT+QCFG=\"band\",0,95", buf, sizeof(buf));
    } else {
        DBG(">>sim4g band all\n");
        sim4g_run_AT_clr("AT+QCFG=\"band\",0,7FFFFFFFFFFFFFFF", buf, sizeof(buf));
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

static int sim4g_set_dial_mode(void)
{
    int result = 0;
    char buf[256] = {0};

    do {
        sim4g_run_AT_clr("AT+QCFG=\"usbnet\"", buf, sizeof(buf));
        result = scanf_AT_result(buf, "usbnet..1");
        if (result != SUCCESS) {
            result = sim4g_run_AT_clr("AT+QCFG=\"usbNet\",1", buf, sizeof(buf));
            sleep(5);
            result = sim4g_run_AT_clr("AT+QCFG=\"usbNet\"", buf, sizeof(buf));
            if (result != SUCCESS) {
                SYSLOG("AT+QCFG set fail\n");
            }
        } else {
            break;
        }
    } while(0);

    return result;
}

static int sim4g_set_inside_sim(void)
{
    int ret = SUCCESS;
    char buf[256] = {0};

    ret = sim4g_run_AT_expect("AT+QDSIM=1", buf, AT_TIMES, sizeof(buf));
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
    } else {    // 模拟内置卡失败
        SYSLOG("simulation inside_sim not exist\n");
        esim_exist = FALSE;
    }

    return esim_exist;
}

static int sim4g_set_outside_sim(void)
{
    int ret = SUCCESS;
    char buf[256] = {0};

    ret = sim4g_run_AT_expect("AT+QDSIM=0", buf, AT_TIMES, sizeof(buf));
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

static int sim4g_env_init(sim_4g_t *info)
{
    int ret = SUCCESS;
    char buf[1024] = {0};
    char iccid[32] = {0};
    int sim_exist = FALSE;
    int esim_exist = FALSE;
    int sim_imsi = FAILURE;
    int esim_imsi = FAILURE;

    eOperatorsType eoperator_type = E_UNKNOWN;
    eOperatorsType operator_type = E_UNKNOWN;

    sim4g_get_ati(info);
    sim4g_run_AT_expect("AT+QLBSCFG=?", buf, AT_TIMES, sizeof(buf));
    ret = scanf_AT_result(buf, "ERROR");
    if (ret == SUCCESS) {
        info->SimInfo.location_enable = 0;
    } else {
        info->SimInfo.location_enable = 1;
    }
    void detect_outside_sim(void)
    {
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

            if (is_okey(F_ICCID)) {
                LoadFile2(F_ICCID, "%s", iccid);
                sim4g_get_operator_by_iccid(iccid, &operator_type);
            }
        } while(0);
    }

    void detect_inside_sim(void)
    {
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

            if (is_okey(F_EICCID)) {
                LoadFile2(F_EICCID, "%s", iccid);
                sim4g_get_operator_by_iccid(iccid, &eoperator_type);
            }
        } while(0);
    }

    if (info->SimInfo.operators == E_TELECOM) {
        DBG("primary SIM1 mode\n");
        detect_outside_sim();
        detect_inside_sim();
        info->inside_sim = TRUE;
    } else {
        DBG("primary SIM2 mode\n");
        detect_inside_sim();
        detect_outside_sim();
        info->inside_sim = FALSE;
        // sim_reverse 需要在这里确定卡类型
        info->SimInfo.operators = operator_type;
    }
    goto_if_4gfail(((TRUE == esim_exist) || (TRUE == sim_exist)),  hard_err);
    goto_if_4gfail(((SUCCESS == sim_imsi) || (SUCCESS == esim_imsi)),  hard_err);

    info->SimInfo.ecard_type = eoperator_type;
    info->SimInfo.card_type = operator_type;

    ret = sim4g_get_imei();
    goto_if_4gfail(ret == SUCCESS, hard_err);

    // 检查双卡状态
    if ((operator_type == E_MOBILE || operator_type == E_UNICOM) && eoperator_type == E_TELECOM) {
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

    if (E_MOBILE == info->SimInfo.operators) {
        sim4g_mobile_card_band_fdd(info);
    }

    DBG("operators_type:%d, eoperator_type:%d\n", info->SimInfo.operators, eoperator_type);

    ret = sim4g_set_dial_mode();
    goto_if_4gfail(ret == SUCCESS, soft_err);

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
    int count = 0;
    int ret = SUCCESS;
    char buf[256] = {0};
    ret = sim4g_run_AT_clr("AT+QNETDEVCTL=3,1", buf, sizeof(buf));
    if (ret != SUCCESS) {
        sim4g_run_AT_clr("AT+QNETDEVCTL=3,1", buf, sizeof(buf));
    }

    while (count < 5) {
        sim4g_run_AT_expect("AT+CEREG?", buf, AT_TIMES, sizeof(buf));
        ret = scanf_AT_result(buf, "0.1");
        if (ret == SUCCESS) {
            break;
        }
        sleep(1);
        count++;
    }

    goto_if_4gfail(ret == SUCCESS, err_autocall);
    return SUCCESS;

err_autocall:
    info->prev_ev = info->curr_ev;
    info->curr_ev = EVENT_REFRESH;
    SYSLOG("__soft_err from line: %d\n", __LINE__);
    return FAILURE;
}

static int sim4g_is_fdd_query(void)
{
    int ret = SUCCESS;
    char buf[256] = {0};

    ret = sim4g_run_AT_expect("AT+QENG=\"servingcell\"", buf, AT_TIMES, sizeof(buf));
    goto_if_4gfail(ret == SUCCESS, __fail);

    if (strstr(buf, "SEARCH") != NULL || strstr(buf, "LIMSRV") != NULL) {
        return MODLE_NULL;
    }

    if (strstr(buf, "FDD")) {
        return MODLE_FDD;
    } else {
        return MODLE_TDD;
    }

    //+QENG: "servingcell","NOCONN","LTE","FDD",460,11,47ADF04,160,100,1,5,5,F274,-76,-7,-48,28,46
__fail:
    return 0;
}

static int sim4g_change_fdd_tdd(sim_4g_t *info)
{
    char buf[1024] = {0};

    info->SimInfo.is_fdd = sim4g_is_fdd_query();
    if (info->SimInfo.is_fdd == MODLE_NULL) {    //未注网
        sim4g_run_AT_clr("AT+QCFG=\"band\",0,7FFFFFFFFFFFFFFF", buf, sizeof(buf));
    }

    return SUCCESS;
}

int sim4g_800e_init(void *data)
{
    int ret = 0;
    sim_4g_t *info = (sim_4g_t *)data;

    ret = sim4g_env_init(info);
    goto_if_4gfail(ret == SUCCESS, init_err);

    ret = sim4g_autocall(info);
    UtilSystemCmd("/ipc/bin/4g udhcpc");

    sim4g_change_fdd_tdd(info);

    return SUCCESS;

init_err:
    return ret;
}

int sim4g_800e_flyreset(void)
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
    SYSLOG("stop 4g...\n");
    DumpFile(USB_MOVE, "1", 1);
    sleep(2);
    UtilSystemCmd("rmmod -f option; rmmod -f usb_wwan; rmmod -f rndis_host; rmmod -f cdc_ether");
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

    UtilSystemCmd("test -e /dev/tty4G || ln -sf /dev/ttyUSB2 /dev/tty4G");

    return SUCCESS;
}

int sim4g_800e_pwr_reset(void)
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

int sim4g_800e_location(Sim4g *info)
{
    if (info == NULL) {
        return FAILURE;
    }

    int ret = SUCCESS;
    char buf[256] = {0};
    char cmdbuf[256] = {0};
    char *p = NULL;
    char at_buf[128] = {0};
    char longitude[32] = {0};
    char latitude[32] = {0};
    Sim4g sim4gcfg = {0};
    get_config(handleSim4gCfg, sim4gcfg);
    if (strlen(info->token) <= 0) {    // 防止策略清空全局变量或者平台下发失败
        strncpy(info->token, sim4gcfg.token, sizeof(info->token));
    }

    snprintf(at_buf, sizeof(at_buf), "AT+QLBSCFG=\"token\",\"%s\"", info->token);
    sim4g_run_AT_expect(at_buf, buf, AT_TIMES, sizeof(buf));
    sim4g_run_AT_expect("AT+QLBSCFG=\"token\"", buf, AT_TIMES, sizeof(buf));
    sim4g_run_AT_expect("AT+QLBSCFG=\"latorder\",1", buf, AT_TIMES, sizeof(buf));
    if (sim4g_run_AT_expect_sec(AT_TIMEOUT, "AT+QLBS", buf, AT_TWO_TIMES, sizeof(buf)) != SUCCESS) {
        return FAILURE;
    }

    p = strstr(buf, "+QLBS:");
    if (p) {
        strtok(p, "\r\n");

        WriteFile("/tmp/location", p);

        sprintf(cmdbuf, "awk -F \",\" '{print $2}' /tmp/location");

        ret = ReadCmdResult(cmdbuf, latitude, sizeof(latitude));
        goto_if_4gfail(ret > 0, fail);

        if (latitude[strlen(latitude) - 1] == '\n') {
            latitude[strlen(latitude) - 1] = '\0';
        }

        DBG("latitude:%s\n", latitude);

        memset(cmdbuf, 0, sizeof(cmdbuf));

        sprintf(cmdbuf, "awk -F \",\" '{print $3}' /tmp/location");

        ret = ReadCmdResult(cmdbuf, longitude, sizeof(longitude));
        goto_if_4gfail(ret > 0, fail);

        if (longitude[strlen(longitude) - 1] == '\n') {
            longitude[strlen(longitude) - 1] = '\0';
        }

        DBG("longitude:%s\n", longitude);
    }

    if (strlen(longitude) > 0 && strlen(latitude) > 0) {
        strncpy(info->latitude, latitude, sizeof(info->latitude));
        strncpy(info->longitude, longitude, sizeof(info->longitude));
        WriteFile(F_LATITUDE, info->latitude);
        WriteFile(F_LONGITUDE, info->longitude);
    }

    remove("/tmp/location");

    return SUCCESS;

fail:
    if (is_okey(F_EICCID)) {
        remove("/tmp/location");
    }

    return FAILURE;
}

