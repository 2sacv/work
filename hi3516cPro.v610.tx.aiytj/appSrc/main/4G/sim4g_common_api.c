/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : sim4g_common_api.c
 * @Created Time : 2023-3-10
 * @Version      : 3.0
 * @Author       : hul
 * @Description  :
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <regex.h>
#include <sys/stat.h>
#include <netdb.h>
#include <time.h>
#include <openssl/crypto.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <cJSON.h>
/* socket() */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <openssl/md5.h>

/* file */
#include <fcntl.h>
#include <sys/file.h>
#include <termios.h>
#include "debug.h"
#include "utils.h"
#include "jconfig.h"
#include "confapi.h"
#include "conf_list.h"
#include "conf_nand.h"
#include "net_check.h"
#include "net_config.h"
#include "system_ctrl.h"
#include "encode_audio_queue.h"
#include "delay_exec.h"

#include "url.h"
#include "sim4g.h"
#include "sim4g_800e.h"
#include "sim4g_yuga.h"
#include "sim4g_adapter.h"
#include "sim4g_common_api.h"

#ifdef PLATFORM_TENCENT
#include "tencent_server.h"
#include "tencent_http_service.h"
#endif

#include "js_http_client.h"
#include "factory_db.h"

#define APP_KEY            "32652341"
#define MD5_DIGEST_LENGTH 16
#define MD5SUM_BUF_SIZE   (1024*16)
#define NO_ID             201
#define IMEI_ILLEGAL      202

int g_burn_ok = FALSE;

int get_burn_result(int *burn_ok)
{
    *burn_ok = g_burn_ok;
    return 0;
}

//设置波特率
static void serial_set_speed(int fd, int speed)
{
    int speed_arr[] = {B50, B75, B110, B134, B150, B200, B300, B600, B1200, B1800, B2400, B4800, B9600,
                       B19200, B38400, B57600, B115200
                      };
    int name_arr[] = {50,  75,  110,  134,  150,  200,  300,  600,  1200,  1800,  2400,  4800,  9600,
                      19200,  38400,  57600,  115200
                     };
    int i = 0;
    struct termios Opt;

    tcgetattr(fd, &Opt);

    for (i = 0;  i < ARRAY_SIZE(speed_arr);  i++) {
        if  (speed == name_arr[i]) {
            tcflush(fd, TCIOFLUSH);
            cfsetispeed(&Opt, speed_arr[i]);
            cfsetospeed(&Opt, speed_arr[i]);
            if (tcsetattr(fd, TCSANOW, &Opt)) {
                perror("tcsetattr fd1");
            }
            return;
        }
        tcflush(fd,TCIOFLUSH);
    }
}

static int serial_set_parity(int fd, int databits, int stopbits, int parity)
{
    struct termios options;

    if (tcgetattr(fd, &options) != 0) {
        perror("SetupSerial 1");
        return FAILURE;
    }
    options.c_cflag &= ~CSIZE;

    // 设置数据位数
    switch (databits) {
        case 5: {
            options.c_cflag |= CS5;
            break;
        }

        case 6: {
            options.c_cflag |= CS6;
            break;
        }

        case 7: {
            options.c_cflag |= CS7;
            break;
        }

        case 8: {
            options.c_cflag |= CS8;
            break;
        }

        default: {
            printf("Unsupported data size\n");
            return FAILURE;
        }
    }

    // 设 置停止位
    switch (stopbits) {
        case 1: {
            options.c_cflag &= ~CSTOPB;
            break;
        }

        case 2: {
            options.c_cflag |= CSTOPB;
            break;
        }

        default: {
            printf("Unsupported stop bits\n");
            return FAILURE;
        }
    }

    // 设置奇偶校验位
    switch (parity) {
        case 'n':
        case 'N': {
            options.c_cflag &= ~PARENB; // Clear parity enable
            options.c_iflag &= ~INPCK;  // Enable parity checking
            break;
        }

        case 'o':
        case 'O': { // 设置为奇效验
            options.c_cflag |= (PARODD | PARENB);
            options.c_iflag |= INPCK;   // Disnable parity checking
            break;
        }

        case 'e':
        case 'E': { // 设置为偶效验
            options.c_cflag |= PARENB;  // Enable parity
            options.c_cflag &= ~PARODD;
            options.c_iflag |= INPCK;   // Disnable parity checking
            break;
        }

        case 'S':
        case 's': { // as no parity
            options.c_cflag &= ~PARENB;
            options.c_cflag &= ~CSTOPB;
            break;
        }

        default: {
            printf("Unsupported parity\n");
            return FAILURE;
        }
    }

    // Set input parity option
    if (parity != 'n') {
        options.c_iflag |= INPCK;
    }

    options.c_iflag &= ~(IXON | IXOFF | IXANY); // avoid 0x13 stop the termios
    options.c_iflag &= ~(ICRNL | IGNCR | INLCR);
    options.c_iflag &= ~(IGNBRK | BRKINT | PARMRK);
    options.c_oflag &= ~(ONLCR|OCRNL);
    //raw input mode
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    //raw output
    options.c_oflag &= ~OPOST;

    tcflush(fd, TCIFLUSH); // Update the options and do it NOW
    options.c_cc[VTIME] = 1;
    options.c_cc[VMIN] = 0;

    if (tcsetattr(fd, TCSANOW, &options) != 0) {
        perror("SetupSerial 3");
        return FAILURE;
    }

    return SUCCESS;
}

int do_md5sum_str(const char *ptr, char *out)
{
    unsigned char md[MD5_DIGEST_LENGTH] = {0};
    MD5_CTX ctx;
    int i = 0;
    char *buf = NULL;

    if (NULL == ptr || NULL == out) {
        return FAILURE;
    }

    buf = (char *)calloc(1, MD5SUM_BUF_SIZE);
    if (NULL == buf) {
        SYSLOG("calloc md5 buf failed!\n");
        return FAILURE;
    }

    MD5_Init(&ctx);
    snprintf(buf, MD5SUM_BUF_SIZE-1, "%s", ptr);
    MD5_Update(&ctx, buf, (size_t)strlen(buf));
    MD5_Final(md, &ctx);

    for (i = 0; i < MD5_DIGEST_LENGTH; i++) {
        sprintf(out+i*2, "%02x", md[i]);
    }
    DBG("md5sum: %s\r\n", out);

    free(buf);

    return SUCCESS;
}

int sim4g_set_serial(int fd,int baud,int nbits,int parity,int stop)
{
    serial_set_speed(fd, baud);
    if (SUCCESS != serial_set_parity(fd,nbits, stop, parity)) {
        ERR("set serial fail\n");
        return FAILURE;
    }

    return SUCCESS;
}

static int sim4g_at_readfully(int fd, void* buf, int nbytes, struct timeval *tv)
{
    int nr = 0;
    int nread = 0;
    fd_set rw_set;
    FD_ZERO(&rw_set);
    FD_SET(fd, &rw_set);
    struct timeval tv0;

    while ( nread < nbytes ) {
        memcpy(&tv0, tv, sizeof(tv0));
        nr = select(fd+1, &rw_set/*read*/, NULL, NULL, &tv0);
        if (nr < 0) {
            if ((errno == EINTR || errno == EAGAIN)) {
                usleep(1000);
                continue;
            } else {
                break;
            }
        } else if (nr == 0) {
            return nread;
        }

        int r = read(fd, (char*) buf + nread, nbytes - nread);
        if (0 > r) {
            if (errno == EINTR || errno == EAGAIN) {
                usleep( 10 * 1000 );
                continue;
            } else {
                return r;
            }
        } else if (0 == r) {
            break;
        }
        nread += r;
    }

    return nread;
}

int ha_readfully(int fd, void* buf, int nbytes, struct timeval *tv)
{
    int nr = 0;
    nr = sim4g_at_readfully(fd, buf, nbytes-1, tv);

    // 超时再重试一次
    if (nr <= 0) {
        tv->tv_sec += 1;
        tv->tv_usec = 0;
        nr = sim4g_at_readfully(fd, buf, nbytes-1, tv);
    }
    return nr;
}

/*
 * 采用shell命令去解析usb0的IP，而不是采用net_get_ipaddr()去解析获得usb0的IP，
 * 是考虑到usb0 实际存在，但又是处于down掉状态的情况。此时通讯是有问题的，应该要走相应的硬复位或者飞行模式逻辑。
 *
 * 考虑使用 /sys/class/net/usb0/operstate  -> down
 */
int sim4g_get_usb0_ip(char ip[])
{
	char *ptr = NULL;
    char cmdbuf[64] = {0};
    char result[64] = {0};

    sprintf(cmdbuf,"%s","ifconfig | grep -r usb0 -A 1 | grep -r inet | awk '{print $2}'");
    ReadCmdResult(cmdbuf,result, sizeof(result));

    if (strlen(result) == 0) {
        ERR("result is NULL\n");
    } else {
        ptr = strstr(result, "addr:");
        if (ptr == NULL) {
           ERR("ptr is NULL\n");
        } else {
            ptr = ptr + 5;
            sscanf(ptr, "%s", ip);
            return SUCCESS;
        }
    }

    return FAILURE;
}

int sim4g_run_AT_expect(char *AT, char *buf, int times, int size)
{
    int ret = 0;

    while (times--) {
        ret = sim4g_run_AT_clr(AT, buf, size);
        if (ret == 0) {
            break;
        } else {
            SYSLOG("_AT_ error[%s] @%d\n", buf, times);
            usleep(500*1000);
            continue;
        }
    }

	return ret;
}

int sim4g_run_AT_expect_sec(int sec, char *AT, char *buf, int times, int size)
{
    int ret = 0;

    while (times--) {
        ret = sim4g_run_AT_clr_sec(sec, AT, buf, size);
        if (ret == 0) {
            break;
        } else {
            SYSLOG("_AT_ error[%s] @%d\n", buf, times);
            usleep(500*1000);
            continue;
        }
    }

	return ret;
}

int scanf_AT_result(char *buf, char *tag)
{
    int ret = 0;
    int cflags = 0;
    char ebuff[256] = {0};
    regex_t regex = {0};
    regmatch_t pmatch = {0,};

    cflags = REG_EXTENDED | REG_ICASE;
    ret = regcomp(&regex, tag, cflags);
    goto_if_4gfail(ret == SUCCESS, err_exit);

    ret = regexec(&regex, buf, 1, &pmatch, REG_NOTBOL);
    goto_if_4gfail(ret == SUCCESS, err_exit);

    if(pmatch.rm_so == -1) {
        goto err_exit;
    }

    goto nomal_eixt;
err_exit:
    SYSLOG("ret = %d, tag = %s, buf = %s", ret, tag, buf);
    regerror(ret, &regex, ebuff, sizeof(ebuff));
    regfree(&regex);
    return FAILURE;
nomal_eixt:
    regfree(&regex);
    return SUCCESS;
}

//正则表达式接口说明:https://www.cnblogs.com/zhidongjian/p/10097856.html或使用man查询接口说明
int scanf_AT_result2(char *buf,  char *tag, char *result, int result_len)
{
    int ret = 0;
    int len = 0;
    int cflags = 0;
    char ebuff[256] = {0};
    regex_t regex = {0};
    regmatch_t pmatch[2] = {0,};

    cflags = REG_EXTENDED | REG_ICASE | REG_NEWLINE;
    ret = regcomp(&regex, tag, cflags);
    goto_if_4gfail(ret == SUCCESS, err_exit);

    ret = regexec(&regex, buf, 2, pmatch, REG_NOTBOL);
    goto_if_4gfail(ret == SUCCESS, err_exit);

    if(pmatch[1].rm_so == -1) {
        SYSLOG("re_nsub = %d\n", regex.re_nsub);
        goto err_exit;
    }

    len = pmatch[1].rm_eo - pmatch[1].rm_so;
    if (len < result_len) {
        memcpy(result, buf + pmatch[1].rm_so, len);
    } else {
        memcpy(result, buf + pmatch[1].rm_so, result_len-1);
    }
    regfree(&regex);

    return SUCCESS;

err_exit:
    regerror(ret, &regex, ebuff, sizeof(ebuff));
    SYSLOG("ret = %d, tag = %s, buf = %s\n", ret, tag, buf);
    regfree(&regex);

    return FAILURE;
}

int air_generate_triple_file(TripleInfoS triple)
{
    char id_buf[256]     = {0};
    if (strlen(triple.product_key) <= 0 || strlen(triple.device_name) <= 0 || strlen(triple.device_secret) <= 0) {
        SYSLOG("txconf strlen small than zero!\n");
        return FAILURE;
    }

    if (strlen(triple.product_key) > 20 || strlen(triple.device_name) > 32 || strlen(triple.device_secret) > 64) {
        SYSLOG("txconf strlen bigger than max!\n");
        return FAILURE;
    }

    sprintf(id_buf, "%s;%s;%s;", triple.product_key, triple.device_name, triple.device_secret);

    SYSLOG("settings txconf to %s\n", id_buf);
    if (uboot_txconf_set((char *)"txconf", id_buf) != SUCCESS) {
        LOG("uboot_txconf_set txconf failed\n");
        DBG("uboot_txconf_set txconf failed\n");
        return FAILURE;
    }

    DumpFile(F_P2P_TRIPLE, id_buf, strlen(id_buf));
    LOG("settings tx_conf to pk:%s dn:%s\n", triple.product_key, triple.device_name);

    return SUCCESS;
}

static int air_burn_devid(char *devid)
{
    int ret = SUCCESS;
    char old_devid[32] = {0,};

    if (NULL == devid) {
        ERR("devid ptr is NULL\n");
        ret = FAILURE;
        goto __exit;
    }

    if (strlen(devid) != MAX_ID_LEN) {
        ERR("devid len is not equal to 11\n");
        ret = FAILURE;
        goto __exit;
    }

    uboot_devid_get(old_devid, sizeof(old_devid));
    if (!strcmp(old_devid, devid)) {
        SYSLOG("devid and platform issued consistent\n");
        return SUCCESS;
    }

    SYSLOG("air burning set devid : %s\n", devid);
    ret = uboot_devid_set(devid);
    if (ret != SUCCESS) {
        ERR("uboot set devid err\n");
        goto __exit;
    }

    uboot_devinfo_set();

    ret = system_set_dev_id(devid);
    if (ret != SUCCESS) {
        ERR("system set devid err\n");
        goto __exit;
    }

    LOG("settings devid to %s\n", devid);
__exit:
    return ret;
}

static int air_burn_mac(char *mac)
{
    // gw和ip会进行校验,4G设备无eth的gw和ip,需要默认值
    int ret =SUCCESS;

    char old_mac[32] = {0,};
    const char* ip = "192.168.1.217";
    const char* gw = "192.168.1.1";
    NetEthS inner = {0};

    if (NULL == mac) {
        ERR("mac ptr is NULL\n");
        ret = FAILURE;
        goto __exit;
    }

    uboot_mac_get(old_mac, sizeof(old_mac));
    if (!strcmp(old_mac, mac)) {
        SYSLOG("dev mac and platform issued consistent\n");
        return SUCCESS;
    }

    conf_get_ethcfg(&inner);
    memcpy(inner.mac, mac, sizeof(inner.mac));
    memcpy(inner.ip, ip, sizeof(inner.ip));
    memcpy(inner.gw, gw, sizeof(inner.gw));
    ret = conf_set_ethcfg(inner);
    if (ret != SUCCESS) {
        ERR("set mac cfg err\n");
        goto __exit;
    }

__exit:
    return ret;
}

int sim4g_burn_devid_mac_ali_conf(sBurnArg *dev, sim_4g_t *sim4g_info)
{
    int ret = SUCCESS;
    ret += air_burn_mac(dev->mac);
    ret += air_burn_devid(dev->device_id);
    ret += WriteFile(F_OPT_IMEI, sim4g_info->SimInfo.imei);
    ret = air_generate_triple_file(dev->triple);
    SYSLOG("air burn  %s [imei:%s] [cpuid:%s]\n", (ret == SUCCESS) ? "succ" : "fail", sim4g_info->SimInfo.imei, get_cpuid());
    LOG("[imei:%s] [cpuid:%s]", sim4g_info->SimInfo.imei, get_cpuid());

    return ret;
}

int sim4g_burn_devid_mac_tx_conf(sBurnArg *dev, sim_4g_t *sim4g_info)
{
    int ret = SUCCESS;
    
    ret += air_burn_mac(dev->mac);
    ret += air_burn_devid(dev->device_id);
    ret = air_generate_triple_file(dev->triple);
    SYSLOG("air burn  %s [imei:%s]\n", (ret == SUCCESS) ? "succ" : "fail", sim4g_info->SimInfo.imei);
    LOG("[cpuid:%s]", get_cpuid());
    
    return ret;
}

int sim4g_get_trip_devid_from_cloud_server(sim_4g_t *sim4g_info, sBurnArg *dev)
{
    int get_succ = SUCCESS;

#ifdef PLATFORM_TENCENT
    get_succ = report_4g_airburn(sim4g_info, dev);
    if (get_succ == SUCCESS) {
        if (dev->code != 200) {
            get_succ = FAILURE;
            DBG("dev->code = %d\n", dev->code);
        } else if (strcmp(dev->cpu, get_cpuid()) != 0) {
            LOG("cpuid no the same, [platform_cpuid:%s cpuid:%s]\n", dev->cpu, get_cpuid());
            get_succ = FAILURE;
        }
    }
    SYSLOG("get hcciot %s\n", (get_succ == SUCCESS) ? "succ" : "fail");
#endif

    return get_succ;
}

int sim4g_burn(sim_4g_t *sim4g_info)
{
    int get_trip_ok = FALSE;    // 获取烧录数据标志
    NetEthS ethcfg  = {0,};
    OsdInfoS osd_info = {0,};
    static sBurnArg dev = {.code = 200};

    if (sim4g_info->eth_up || g_burn_ok) {
        dbg_4g("do not enter burn mode\n");
        return 0;
    }

    conf_get_ethcfg(&ethcfg);
    conf_get_osdinfocfg(&osd_info);

    if ((!is_okey(F_P2P_TRIPLE)) || (!sim4g_get_security()) || pop_g_stat(tencent, TENCENT_INVALID)) {
        if (dev.code != NO_ID && dev.code != IMEI_ILLEGAL) {    // 数据库中无ID或IMEI非法的情况下不再访问服务器
            get_trip_ok = (SUCCESS == sim4g_get_trip_devid_from_cloud_server(sim4g_info, &dev));
            if (!get_trip_ok) {
               sleep(10);
               get_trip_ok = (SUCCESS == sim4g_get_trip_devid_from_cloud_server(sim4g_info, &dev));
            }
        }

        if (get_trip_ok) {
            g_burn_ok = (SUCCESS == sim4g_burn_devid_mac_tx_conf(&dev, sim4g_info));
            if (g_burn_ok && get_g_sys(factest)) {    // 产测模式需要在烧录成功以OSD方式显示叠加在直播流上确认烧录成功和失败
                osd_info.nameen   = 1;
                osd_info.nameleft = 840;
                osd_info.nametop  = 440;
                osd_info.osdcolor = 3;
                strcpy(osd_info.name, "SUCCESS");
                conf_set_osdinfocfg(osd_info);
            } else {  
                DELAY_REBOOT_LINUX();
            }
        }
    } else {
        SYSLOG("tx_exist:%d, security:%d, mac:%s cpuid:%s\n", is_okey(F_P2P_TRIPLE), sim4g_get_security(), ethcfg.mac, get_cpuid());
        g_burn_ok = TRUE;
    }

    return 0;
}

int sim4g_report_location(Sim4g *sim4g_info)
{
    int ret = SUCCESS;
    sim_4g_t sim_4g_info = {0};
    sim4g_get_sim_4g(&sim_4g_info);
    if (sim_4g_info.model_type == E_YGX09) {
        ret = sim4g_ygx09_location(sim4g_info);
    } else {
        ret = sim4g_800e_location(sim4g_info);
    }

    if (ret != SUCCESS || strlen(sim4g_info->longitude) == 0 || strlen(sim4g_info->longitude) == 0) {
        SYSLOG("refresh fail, longitude or longitude is null\n");
        return FAILURE;
    }

#if defined(PLATFORM_TENCENT)
    ret = report_4g_location(sim4g_info);
#endif
    DBG("report location %s\n", (ret == SUCCESS) ? "succ" : "fail");
    if (ret != SUCCESS) {
        SYSLOG("report_location failed, ret = %d\n", ret);
        return FAILURE;
    }

    return SUCCESS;
}

/*
 * 上报失败会改变 iccid_changed()，这个是没有必要的，后续再改
 **/
int sim4g_report_cloud_server_simcard(sim_4g_t * info)
{
    int  report_4g_succ = -1;

#if defined(PLATFORM_TENCENT)
    report_4g_succ = report_4g_info(info);
#endif
    DBG("report hcciot %s\n", (report_4g_succ == SUCCESS) ? "succ" : "fail");
    if (report_4g_succ != SUCCESS) {
        SYSLOG("report_4g_succ = %d\n", report_4g_succ);
        return FAILURE;
    }

    if (E_REPORT_CHANGE_CARD == info->SimInfo.report_status) {
        // 提示非原厂卡
        sim4g_video_turnoff();
        encode_audio_queue_push_amr(AUDIO_SIM_CHECK_ICCID_4G, FALSE);
        encode_audio_queue_push_amr(AUDIO_SIM_CHECK_ICCID_4G, FALSE);
        SYSLOG("E_REPORT_CHANGE_CARD\n");
    }

    return SUCCESS;
}

