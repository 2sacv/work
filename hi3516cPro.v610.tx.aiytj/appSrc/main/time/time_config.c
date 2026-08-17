#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <linux/rtc.h>
#include <linux/reboot.h>

#include "time_config.h"
#include "utils.h"
#include "debug.h"
#include "conf_list.h"
#include "jconfig.h"
#include "js_scheduler.h"
#include "cmdstat.h"
#include "jdns.h"
#include "net_check.h"

//NTP protocol parameters.
#define NTP_VERSION     (3) // current version number
#define MODE_CLIENT     3   // client mode

#define JAN_1970        0x83AA7E80  // The most signficant 32b word of NTP time on Jan. 1, 1970
#define DEFAULT_TIME    1596240000  //2020-8-1 0:0:0

static int iRtcStat = 0; //rtc是否正常 1:正常 0:不正常
                         //
static int g_tz_idx = 29;
static TzoneS g_tzs[] = {
    {0 , -43200, "UTC+12:00"},
    {1 , -39600, "UTC+11:00"},
    {2 , -36000, "UTC+10:00"},
    {3 , -32400, "UTC+09:00"},
    {4 , -28800, "UTC+08:00"},
    {5 , -25200, "UTC+07:00"},
    {6 , -21600, "UTC+06:00"},
    {7 , -18000, "UTC+05:00"},
    {8 , -18000, "UTC+05:00"},
    {9 , -14400, "UTC+04:00"},
    {10, -12600, "UTC+03:30"},
    {11, -10800, "UTC+03:00"},
    {12, -7200 , "UTC+02:00"},
    {13, -3600 , "UTC+01:00"},
    {14, 0     , "UTC+00:00"},
    {15, 3600  , "UTC-01:00"},
    {16, 3600  , "UTC-01:00"},
    {17, 3600  , "UTC-01:00"},
    {18, 3600  , "UTC-01:00"},
    {19, 7200  , "UTC-02:00"},
    {20, 7200  , "UTC-02:00"},
    {21, 10800 , "UTC-03:00"},
    {22, 12600 , "UTC-03:30"},
    {23, 14400 , "UTC-04:00"},
    {24, 16200 , "UTC-04:30"},
    {25, 18000 , "UTC-05:00"},
    {26, 19800 , "UTC-05:30"},
    {27, 21600 , "UTC-06:00"},
    {28, 25200 , "UTC-07:00"},
    {29, 28800 , "CST-08:00"},
    {30, 32400 , "UTC-09:00"},
    {31, 34200 , "UTC-09:30"},
    {32, 36000 , "UTC-10:00"},
    {33, 39600 , "UTC-11:00"},
    {34, 43200 , "UTC-12:00"},
};

struct ntp_run {
    JSScheduler sch;
    JSTCHandle  hdl;
    struct cmdstat *p_ctx;
    int skip;
};

struct ntp_cfg {
    SysNtpS ntp;
};

enum {
    CMD_NTP_CFG         = 1 << 0,
};

static struct ntp_cfg cfg_ntp = {{0}};
static struct ntp_run run_ntp = {0};
static struct ntp_cfg *g_cfg_ntp = &cfg_ntp;
static struct ntp_run *g_run_ntp = &run_ntp;

time_t get_ntp_epoche(char *server_ip, int server_port)
{
    time_t epoche = 0;
    int socked = -1;

    if (server_ip == NULL || server_port == 0) {
        DBG("IP is NULL\n");
        goto __exit;
    }

    NtpPacket ntppkt;
    memset(&ntppkt, 0, sizeof(NtpPacket));
    int recvlen = 0;
    fd_set fdset;
    int ret = 0;
    char *pstr = NULL;
    socked = socket(AF_INET, SOCK_DGRAM, 0);
    fcntl(socked, F_SETFL, fcntl(socked, F_GETFL, 0)|O_NONBLOCK);
    if (socked <= 0) {
        SYSLOG("ntp socked error!\n");
        goto __exit;
    }
    char ip[64] = {0};
    pstr = get_ip_by_domain(server_ip, ip, sizeof(ip));
    if (NULL == pstr) {
        ERR("fix ntp server fail, please check you network setting\n");
        goto __exit;
    }

    do {
        memset(&ntppkt, 0, sizeof(NtpPacket));
        ntppkt.li_vn_mode = PKT_LI_VN_MODE(0, NTP_VERSION, MODE_CLIENT);

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(struct sockaddr_in));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr(ip);
        addr.sin_port = htons(server_port);

        sendto(socked, &ntppkt, sizeof(ntppkt), 0, (struct sockaddr*)&addr,
               sizeof(struct sockaddr_in));

        struct timeval tv = {1, 0};
        FD_ZERO(&fdset);
        FD_SET(socked, &fdset);
        ret = select(socked+1, &fdset, NULL, NULL, &tv);
        if (ret <= 0) {
            goto __exit;
        }

        if (!FD_ISSET(socked, &fdset)) {
            goto __exit;
        }

        recvlen = recvfrom(socked, &ntppkt, sizeof(ntppkt), 0, NULL, NULL);
        if (recvlen != (int)sizeof(ntppkt)) {
            ERR("recvlen = %d\n", recvlen);
            //连接失败需要尝试
            goto __exit;
        }

        unsigned int upper = ntohl(ntppkt.xmt.Xl_i);
        epoche = MAX(upper, JAN_1970) - JAN_1970;
    } while (0);

__exit:

    if (socked > 0) {
        close(socked);
    }

    return epoche;
}

void pri_ntp_epoche(void *data)
{
    if (!platform_on_line()) {
        return;
    }

    time_t epo = get_ntp_epoche(g_cfg_ntp->ntp.ntpserver, g_cfg_ntp->ntp.ntpport);
    SYSLOG("ntp epoche %lld @uptime %lld\n", epo, mono_sec());
    return;
}

//设置时钟芯片时间
int hwclock_set(time_t tmSet)
{
    return 0;
    int fd;
    struct rtc_time timedate;
    struct tm settm;

    memset(&settm, 0, sizeof(struct tm));
    memset(&timedate, 0, sizeof(struct rtc_time));

    if (NULL == gmtime_r(&tmSet, &settm)) {
        return FAILURE;
    }

    fd = open("/dev/rtc0", O_NONBLOCK);
    if (0 > fd) {
        ERR("open fail\n");
        return FAILURE;
    }

    timedate.tm_mday = settm.tm_mday;
    timedate.tm_wday = settm.tm_wday;
    timedate.tm_mon  = settm.tm_mon;
    timedate.tm_hour = settm.tm_hour;
    timedate.tm_min  = settm.tm_min;
    timedate.tm_sec  = settm.tm_sec;
    timedate.tm_year = settm.tm_year;

    printf("year:%d\n",   timedate.tm_year);
    printf("month:%d\n",  timedate.tm_mon);
    printf("date:%d\n",   timedate.tm_mday);
    printf("hour:%d\n",   timedate.tm_hour);
    printf("minute:%d\n", timedate.tm_min);
    printf("second:%d\n", timedate.tm_sec);

    ioctl(fd, RTC_SET_TIME, &timedate);

    close(fd);
    return SUCCESS;
}


//获取时钟芯片时间
int hwclock_get(time_t *tmGet)
{
    int fd;
    struct tm gettm;
    struct rtc_time timedate;
    int retry = 0;
    return 0;

    if (NULL == tmGet) {
        return FAILURE;
    }

    do {
        fd = open("/dev/rtc0", O_NONBLOCK);
        if (fd < 0) {
            DBG("open sysclock failed, retry=%d\n", retry++);
            sleep(1);
            continue;
        }
    } while ((0 > fd) && (3 > retry));

    if (0 > fd) {
        return FAILURE;
    }

    memset(&gettm, 0, sizeof(struct tm));
    memset(&timedate, 0, sizeof(struct rtc_time));

    ioctl(fd, RTC_RD_TIME, &timedate);

    gettm.tm_mday = timedate.tm_mday;
    gettm.tm_wday = timedate.tm_wday;
    gettm.tm_mon  = timedate.tm_mon;
    gettm.tm_hour = timedate.tm_hour;
    gettm.tm_min  = timedate.tm_min;
    gettm.tm_sec  = timedate.tm_sec;
    gettm.tm_year = timedate.tm_year;

    *tmGet = mktime(&gettm);

    printf("year:%d\n",   gettm.tm_year);
    printf("month:%d\n",  gettm.tm_mon);
    printf("date:%d\n",   gettm.tm_mday);
    printf("hour:%d\n",   gettm.tm_hour);
    printf("minute:%d\n", gettm.tm_min);
    printf("second:%d\n", gettm.tm_sec);

    close(fd);
    return SUCCESS;
}

int set_clock_time(int fd, time_t tmSet)
{
    struct rtc_time timedate;
    struct tm settm;

    memset(&settm, 0, sizeof(struct tm));
    memset(&timedate, 0, sizeof(struct rtc_time));

    if (NULL == gmtime_r(&tmSet, &settm)) {
        return FAILURE;
    }

    timedate.tm_mday = settm.tm_mday;
    timedate.tm_wday = settm.tm_wday;
    timedate.tm_mon  = settm.tm_mon;
    timedate.tm_hour = settm.tm_hour;
    timedate.tm_min  = settm.tm_min;
    timedate.tm_sec  = settm.tm_sec;
    timedate.tm_year = settm.tm_year;

    ioctl(fd, RTC_SET_TIME, &timedate);

    return 0;
}

int clean_rtc_vl_status()
{
    int fd;
    int retry = 0;
    int status = -1;
    int ret = 0;

    do {
        fd = open("/dev/rtc0", O_NONBLOCK);
        if (fd < 0) {
            DBG("open sysclock failed, retry=%d\n", retry++);
            sleep(1);
            continue;
        }
    } while ((0 > fd) && (10 > retry));

    if (0 > fd) {
        return FAILURE;
    }

    ret = ioctl(fd, RTC_VL_READ, &status);
    DBG("clean_rtc_vl_status ret:%d, status:%d\n", ret, status);


    if (status == 1) {
        set_clock_time(fd, 1546344501);
        ret = ioctl(fd, RTC_VL_CLR, &status);
        ret = ioctl(fd, RTC_VL_READ, &status);
        DBG("clean_rtc_vl_status ret:%d, status:%d\n", ret, status);
    }

    close(fd);

    return 0;
}

int dump_tz_idx(int idx)
{
    if (idx >= 0 && idx <= ARRAY_SIZE(g_tzs)) {
        g_tz_idx = idx;

        char load_buf[16] = {0};
        LoadFile(TZ_FILE, load_buf, sizeof(load_buf)-1);
        drop_tail_space(load_buf);
        if (0 != strcmp(g_tzs[idx].str, load_buf)) {
            DBG("Dump TZ file\n");
            DumpFile2(TZ_FILE, "%s\n", g_tzs[idx].str);
        }
        DBG("Dump TZ var: %s\n", g_tzs[idx].str);
        setenv("TZ", g_tzs[idx].str, 1);
        tzset();

        return SUCCESS;
    } else {
        ERR("bad idx %d\n", idx);
        return FAILURE;
    }
}

int get_tz_seceast(void)
{
    return g_tzs[g_tz_idx].sec_east;
}

int get_tz_idx_by_seceast(int sec_east)
{
    for (int i = 0; i < ARRAY_SIZE(g_tzs); i++) {
        if (g_tzs[i].sec_east == sec_east) {
            return i;
        }
    }

    return 29; // CST-8
}

time_t mktime_utc(struct tm *ts)
{
    ts->tm_sec += get_tz_seceast();
    return mktime(ts);
}

/*
函数功能:同步系统时间。
参数说明:
返回值: 0 SUCCESS,   -1 FAIL;
*/
int init_system_zone()
{
    TzoneS tz = {0};
    get_config(handleTimeZoneCfg, tz);
    dump_tz_idx(tz.idx);

    return SUCCESS;
}

int init_system_time()
{
    time_t now;
    time_t testtime;
    return 0;
    clean_rtc_vl_status();

    if (SUCCESS != hwclock_get(&now)) {
        SYSLOG("time check fail \n");
        iRtcStat = 0;
        return FAILURE;
    }

    if (0 > stime(&now)) {
        SYSLOG("time check: stime error %lld\n", now);
        iRtcStat = 0;
        return FAILURE;
    }

    sleep(2);
    if (SUCCESS != hwclock_get(&testtime)) {
        SYSLOG("time check fail \n");
        iRtcStat = 0;
        return FAILURE;
    } else {
        if (((testtime - now) >= 1 ) && ((testtime - now) <= 3)) {
            iRtcStat = 1;
        }
    }

    return SUCCESS;
}

/*
函数功能: 设置系统时间
参数说明: ttime : 需设置的时间，
返回值: 0 SUCCESS,   -1 FAIL;
*/
int dump_system_time(time_t epoch)
{
    time_t diff = epoch - time(NULL);

    if (diff >= -2 && diff <= 2) {
        DBG("ignore time set @diff: %lld\n", diff);
        return SUCCESS; 
    }

    hwclock_set(epoch);

    if (stime(&epoch) > 0) {
        SYSLOG("FAIL epoche %lld, diff: %lld @uptime %lld\n", epoch, diff, mono_sec());
        return FAILURE;
    }

    SYSLOG("set epoche %lld, diff: %lld @uptime %lld\n", epoch, diff, mono_sec());

    send_conf_nake(JEvent_TimeChange);
    DumpFile2("/opt/conf/reboot.epoch", "date @%lld\n", time(NULL));

    if (abs(diff) > 3) {
        js_run_function(g_run_ntp->sch, pri_ntp_epoche, NULL, 0);
    }

    return SUCCESS;
}

/*
函数功能: 同步一次指定服务器的时间。
            并设置到板内。
参数说明:
    server_ip : NTP服务器地址 如: 192.168.2.27
    server_port : NTP服务器端口 如 :123
返回值: 0 SUCCESS,   -1 FAIL;
*/
int time_ntp_server_time(char *server_ip, int server_port)
{
    if (server_ip == NULL || server_port == 0) {
        DBG("IP is NULL\n");
        return -1;
    }

    int socked = -1;
    NtpPacket ntppkt;
    memset(&ntppkt, 0, sizeof(NtpPacket));
    int recvlen = 0;
    int times = 2;
    fd_set fdset;
    int ret = 0;
    //const char *pstr = NULL;
    socked = socket(AF_INET, SOCK_DGRAM, 0);
    fcntl(socked, F_SETFL, fcntl(socked, F_GETFL, 0)|O_NONBLOCK);
    if (socked <= 0) {
        socked = -1;
        DBG("ntp socked error!\n");
        return FAILURE;
    }

    char ip[16] = {0};
    if (NULL == get_ip_by_domain(server_ip, ip, sizeof(ip))) {
        ERR("fix ntp server fail, please check you network setting\n");
        close(socked);
        socked = -1;
        return FAILURE;
    }

    DBG("got server ip %s %s\n", server_ip, ip);

    do {
        memset(&ntppkt, 0, sizeof(NtpPacket));
        ntppkt.li_vn_mode = PKT_LI_VN_MODE(0, NTP_VERSION, MODE_CLIENT);

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(struct sockaddr_in));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr(ip);
        addr.sin_port = htons(server_port);

        sendto(socked, &ntppkt, sizeof(ntppkt), 0, (struct sockaddr*)&addr,
               sizeof(struct sockaddr_in));

        struct timeval tv = {1, 0};
        FD_ZERO(&fdset);
        FD_SET(socked, &fdset);
        ret = select(socked+1, &fdset, NULL, NULL, &tv);
        if (ret <= 0) {
            times--;
            continue;
        }

        if (!FD_ISSET(socked, &fdset)) {
            times--;
            continue;
        }

        recvlen = recvfrom(socked, &ntppkt, sizeof(ntppkt), 0, NULL, NULL);
        if (recvlen != (int)sizeof(ntppkt)) {
            ERR("recvlen = %d\n", recvlen);
            //连接失败需要尝试
            times--;
            continue;
        }

        unsigned int upper = ntohl(ntppkt.xmt.Xl_i);
        if (upper > JAN_1970) {
            time_t epo = upper - JAN_1970;
            if (abs(epo - time(NULL)) >= 3) {
                dump_system_time(epo+1);
            }
            break;
        } else {
            //DBG("------- Ntpdate fail\n");
            times--;
            continue;
        }
    } while (times > 0);

    close(socked);
    socked = -1;

    if (times <= 0) {
        return -1;
    }

    return 0;
}

static void cb_ntpcfg_sync(int id, void *p_src, int size, void *ctx)
{
    CPY2CMDCFG(CMD_NTP_CFG, &g_cfg_ntp->ntp, p_src, size);
}

static void loop_ntp(void *ctx)
{
    int cmd = cmd_get_command((struct cmdstat *)ctx);
    static int ntp_flag = 1;

    if (cmd & CMD_NTP_CFG) {
        if (g_cfg_ntp->ntp.enable == 1) {
            ntp_flag = 1;
        }
    }

    g_run_ntp->skip++;
    if (g_run_ntp->skip == g_cfg_ntp->ntp.interval*60 && g_cfg_ntp->ntp.enable == 1) {
        ntp_flag = 1;
    }

    if (ntp_flag == 1) {
        time_ntp_server_time(g_cfg_ntp->ntp.ntpserver, g_cfg_ntp->ntp.ntpport);
        g_run_ntp->skip = 0;
        ntp_flag = 0;
    }

    return;
}

int init_client_ntp_update(void *data)
{
    static struct cmdstat cmdstat_ntp;
    struct cmdstat *ctx = &cmdstat_ntp;

    g_run_ntp->sch = data;
    g_run_ntp->p_ctx = ctx;

    get_config(handleNtpcfg, g_cfg_ntp->ntp);
    DBG("PORT = %d\n", g_cfg_ntp->ntp.ntpport);

    attach_config(JEvent_NtpcfgChg, cb_ntpcfg_sync, ctx);

    js_create_timer_r(g_run_ntp->sch, 1000, 1000, loop_ntp, ctx, &g_run_ntp->hdl);

    return SUCCESS;
}

void uninit_client_ntp_update(void)
{
    if (!g_run_ntp->sch) {
        return;
    }

    if(g_run_ntp->hdl){
        js_delete_timer_r(&g_run_ntp->hdl);
    }

    detach_config(JEvent_NtpcfgChg, cb_ntpcfg_sync, g_run_ntp->p_ctx);

    return;
}

int is_board_cost_effective()
{
    static int flag =  0xBADF00D;
    char buf[128];
    int  nr;

    if (flag != 0xBADF00D) {
        return flag;
    }

    // hwclock -s ; echo $? > /var/run/hwclock

    nr = LoadFile("/var/run/hwclock", buf, sizeof(buf));

    if (nr < 1) {
        SYSLOG("read /var/run/hwclock fail\n");
        flag = FALSE;
        goto __exit;
    }

    if (buf[0] == '0') {
        flag = FALSE;
    } else {
        flag = TRUE;
    }

__exit:
    return flag;
}

int get_rtcstat(int *rtcstat)
{
    *rtcstat = iRtcStat;
    return 0;
}
