/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : system_ctrl.c
 * @Created Time : 2014.04.03
 * @Version      : 1.0
 * @Author       : cheby
 * @Description  :
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/ioctl.h>
#include <sys/sysinfo.h>
#include <sys/reboot.h>
#include <linux/reboot.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <math.h>
#include <sys/sysinfo.h>

#include "net_check.h"
#include "system_ctrl.h"
#include "net_config.h"
#include "debug.h"
#include "delay_exec.h"
#include "conf_list.h"
#include "jconfig.h"
#include "logapi.h"
#include "utils.h"
#include "libsecurity.h"
#include "soft_check.h"
#include "conf_nand.h"
#include "logapi.h"
#include "http_main.h"
#include "js_scheduler.h"
#include "airlink.h"
#include "system_sch.h"
#include "jcpService.h"
#include "encodeapi.h"
#include "resolution.h"
#include "factory_db.h"
#include "jevent.h"
#include "g_log.h"
#include "record_watch.h"
#include "confapi.h"
#include "ethtool.h"
#include "net_qrcode.h"
#include "net_check.h"
#include "shm_buf_pool.h"
#include "aging8h.h"

#include "ot_wtdg.h"

#define T10_NAME  "T10"
#define T15_NAME  "T15"
#define T20_NAME  "T20"
#define T21_NAME  "T21"
#define T31L_NAME  "T31"
#define T31X_NAME  "T31X"
#define AX620U_NAME  "AX620U"
#define AX620Q_NAME  "AX620Q"
#define HI3516CV610_NAME  "hi3516cv610"
#define HI3516CV608_NAME  "hi3516cv608"

#define TICK_CPU_60SEC (6)
#define AVAIABLE_MEM_SIZE (8*1024)

#define INTV_CPU_USAGE (10*1000)
#define INTV_MEMINFO   (10 * 60 * 1000)
#define WATCH_DOG_TIME_FIRST_INTERVAL     (3*1000)
#define WATCH_DOG_TIME_INTERVAL     (20*1000)
#define WATCH_ATUO_REBOOT_TIME_INTERVAL (20*1000)

#define WATCH_DOG_TIMEOUT 60

#define WATCH_DOG_DEVICE        "/dev/watchdog"
#define NET_WATCH       (21)

#define INTV_QUALITY  (6*1000)
#define INTV_SCAN     (5*1000)
typedef struct {
    BOOL                bSecurity;
    ESecurityType       eSecurityType;
    char                szDevID[16];
    ECPUType            eCPUType;
    ESensorType         eSensor;                 // sensor的类型
    int                 nCpuFreq;
    int                 bSupportHD;              // 是否支持算子
} SystemType;

struct sensor_map {
    char             snr_name[12];
    ESensorType      snr_type;
    VideoIdxE        idx;
    char             product_str[12];
};

int eth0_status_init = -1;
static float        cpu_use_percent = 50.0;
static long         total_time1 = 0;
static long         total_time2 = 0;
static long         usage_time1 = 0;
static long         usage_time2 = 0;
static int          sys_uptime = 0;
static AutoRebootS  ar = {0, 0, 1};
static JSScheduler sch_autorbt = NULL;
static JSTCHandle  hdl_autorbt = NULL;
static JSScheduler sch_wdt = NULL;
static JSTCHandle  hdl_wdt = NULL;
static int g_dog_fd = 0;
static SystemType  systemType;
static int         upgrade_begin = FALSE;
static int  g_net_reticleold = -1;

int is_test_ver()
{
    static int got = FALSE;

    if (got) {
        goto __exit;
    }

    got = TRUE;

    if (is_okey("/tmp/tag.test_ver")) {
        SYSLOG("gsys testing\n");
        set_g_sys(testing);
    }

__exit:
    return get_g_sys(testing);
}

int system_get_uptime()
{
    struct sysinfo info;
    if (-1 == sysinfo(&info)) {
        return 0;
    }
    return info.uptime;
}

static int get_cpu_usage_info(long *totalTime, long *usageTime)
{
    long user = 0;
    long nice = 0;
    long system = 0;
    long idle = 0;
    long iowait =0;
    long irq = 0;
    long softirq = 0;

    FILE *fp = NULL;

    fp = fopen("/proc/stat", "r");
    if (fp == NULL) {
        return -1;
    }

    fscanf(fp, "cpu %ld %ld %ld %ld %ld %ld %ld\n", &user, &nice, &system,
           &idle, &iowait, &irq, &softirq);
    *totalTime = user + nice + system + idle + iowait + irq + softirq;
    *usageTime = user + nice + system ;

    fclose(fp);
    return 0;
}

#if 0
static int system_get_buffer()
{
    struct sysinfo info;
    if (-1 == sysinfo(&info)) {
        return 0;
    }
    return info.bufferram>>10;
}
#endif

long get_available_memory_kb(void) {
    FILE *fp = fopen("/proc/meminfo", "r");
    if (!fp) return -1;

    long memfree = 0, buffers = 0, cached = 0;
    char line[128];

    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "MemFree: %ld kB", &memfree) == 1) continue;
        if (sscanf(line, "Buffers: %ld kB", &buffers) == 1) continue;
        if (sscanf(line, "Cached: %ld kB", &cached) == 1) continue;
    }
    fclose(fp);
    return memfree + buffers + cached;
}

void calc_cpu_usage(void *data)
{
    //DBG("watch cpu usage start\n");
    static int tick = 0;
    if (is_inc_modc(tick, TICK_CPU_60SEC/2)) {
        long available_mem = get_available_memory_kb();
        if(available_mem > 0 && available_mem < AVAIABLE_MEM_SIZE) {
            DropCache(__func__);
            ms_sleep(2*1000);
            CompactMemo(__func__);
        }

        if (get_num_of_kbytes(7) < 2) {
            DropCache(__func__);
            ms_sleep(2*1000);
            CompactMemo(__func__);
            UtilSystemCmd((char *)"cat /proc/buddyinfo");
        }
    }

    static int useage95s = 0;

    total_time1 = total_time2;
    usage_time1 = usage_time2;

    get_cpu_usage_info(&total_time2, &usage_time2);

    if(total_time2 != total_time1)
        cpu_use_percent = (usage_time2 - usage_time1) * 100.0 /
                          (total_time2 - total_time1);
    //DBG("usage:%ld total:%ld cpu usage:%f\n", usage_time2, total_time2, cpu_use_percent);
    //system_status_info(buf);

    if (cpu_use_percent > 90) {
        SYSLOG("got a percent@%f useage95s@%d\n", cpu_use_percent, useage95s);
        if (cpu_use_percent > 95) {
            useage95s++;
            if (FALSE == upgrade_begin) {
                if (useage95s > 8) {
                    SYSLOG("got BUG: cpu load is too high\n");
                    UtilSystemCmd("lzbox vm s");
                    DELAY_REBOOT_LINUX();
                }
            } else {
                DBG("cpu load is high when upgrade\n");
                if (useage95s >= 10) {
                    SYSLOG("got BUG: cpu load is too high when upgrade\n");
                    UtilSystemCmd("lzbox vm s");
                    DELAY_REBOOT_LINUX();
                }
            }
        }
    } else {
        useage95s = 0;
    }
    //DBG("watch cpu usage end\n");
}

static int system_get_meminfo(int *total, int *free)
{
    int totalkb = 0;
    int freekb = 0;

    struct sysinfo info;

    if (NULL == total || NULL ==  free) {
        return FAILURE;
    }

    if (-1 == sysinfo(&info)) {
        return FAILURE;
    }

    totalkb = info.totalram >> 10;
    freekb =  info.freeram >> 10;

    *total = totalkb;
    *free = freekb;

    return SUCCESS;
}

int time_to_string(char *buf, int data)
{
    int day = 0;
    int hour = 0;
    int minutes = 0;
    int second = 0;

    second = data;
    day = second / 24 / 3600;
    second -= day * 24 * 3600;
    hour = second / 3600;
    second -= hour * 3600;
    minutes = second / 60;
    second -= minutes * 60;
    sprintf(buf, "%d days %02d:%02d:%02d", day, hour, minutes, second);
    return 0;
}

int system_status_info(char *buf)
{
    if (NULL == buf) {
        return FAILURE;
    }

    int len = 0;
    int total = 0;
    int free = 0;
    char strbuf[64] = {0};

    float  fuse = cpu_use_percent;
    fuse = fuse > 100 ? 100:fuse;
    fuse = fuse < 0 ? 0:fuse;
    len += sprintf(buf + len, "Cpu_Usage#CPU-%.2f%%,", fuse);
    len += sprintf(buf + len, "Clock_Info#CPU-%dMHz,", systemType.nCpuFreq);

    if (SUCCESS == system_get_meminfo(&total, &free)) {
        len += sprintf(buf + len, "Memory_Info#Total-%dKB Used-%dKB Free-%dKB,",
                       total, total - free, free);
    }

    int apptime = system_get_uptime();
    time_to_string(strbuf, apptime);
    len += sprintf(buf + len, "System_Uptime#%s,", strbuf);
    apptime = apptime - sys_uptime;
    time_to_string(strbuf, apptime);
    len += sprintf(buf + len, "NvsApp_Uptime#%s,", strbuf);

    memset(strbuf, 0, sizeof(strbuf));
    net_get_dnsaddr(strbuf);

    len += sprintf(buf + len, "Dns_Server#%s,", strbuf);
    memset(strbuf, 0, sizeof(strbuf));
    net_get_ipaddr("eth0", strbuf, sizeof(strbuf));
    len += sprintf(buf + len, "Eth_ip#%s,", strbuf);

    return len;
}


static JSScheduler sch_cpu = NULL;
static JSTCHandle  hdl_cpu = NULL;
static JSTCHandle  hdl_meminfo = NULL;

// 独立定时器：每 60 秒打印内存调试信息
static void cb_debug_meminfo(void *data)
{
    char buf[2048] = {0};

    DBG("=== buddyinfo ===\n");
    CatFile("/proc/buddyinfo", buf, sizeof(buf));

    DBG("=== meminfo ===\n");
    CatFile("/proc/meminfo", buf, sizeof(buf));

    DBG("=== pid_status ===\n");
    CatFile("/proc/self/status", buf, sizeof(buf));
}

int init_client_cpu_usage(void *data)
{
#ifdef __DISABLE_OOM
    char cmd[128] = {0};
    sprintf(cmd, "echo -17 > /proc/%d/oom_adj; echo oom_adj return $?", getpid());
    UtilSystemCmd(cmd);
#endif
    sch_cpu = data;
    sys_uptime = system_get_uptime();

    js_create_timer_r(sch_cpu, INTV_CPU_USAGE, INTV_CPU_USAGE, calc_cpu_usage, NULL, &hdl_cpu);
    if (is_okey(F_DBG_MEMINFO)) {
        js_create_timer_r(sch_cpu, INTV_MEMINFO,  INTV_MEMINFO,  cb_debug_meminfo, NULL, &hdl_meminfo);
    }

    return 0;
}

void uninit_client_cpu_usage()
{
    js_delete_timer_r(&hdl_cpu);
    js_delete_timer_r(&hdl_meminfo);

    sch_cpu = NULL;
}

static void watch_dog_func(void *data)
{
    static int tick = 0, wdt_stopped = FALSE;

    int ret = 0;
    time_t now;
    char szTime[64] = {0};
    time(&now);
    struct tm tmnow ;
    localtime_r(&now, &tmnow);
    strftime(szTime, 20, "%F %H:%M:%S", &tmnow);

    if (tick % (3600*1000/WATCH_DOG_TIME_INTERVAL) == 0) {  // 1h 更新一次状态
        set_aging8h();
    } else {
        if (get_g_sys(usb_4g) && !system_get_security()) {
            if (tick > (BURNED_SIGN*3600*1000/WATCH_DOG_TIME_INTERVAL)) {
                SYSLOG("No security after more than %d hours, reboot\n", BURNED_SIGN);
                DELAY_REBOOT_LINUX();
            }
        }
    }

    // block 300s, to stop feed wdt
    if (tick && (tick%3) == 0) {
        static int cnt_block = 0;
        int num = js_scheduler_blocked_nums(cnt_block*2000);
        if (num > 0) {
            SYSLOG("\n_____ %d js blocked @%d _____\n", num, cnt_block);
            UtilSystemCmd("free");
            UtilSystemCmd("top -b -n 1");
            if (++cnt_block >= 5 && !is_test_ver()) {
                wdt_stopped = TRUE;
                SYSLOG("js_scheduler_blocked_nums: %d\n", num);
                DELAY_REBOOT_LINUX();
            }
        } else {
            cnt_block = 0;
        }
    }

    if (0 < g_dog_fd && !wdt_stopped) {
        ret = ioctl(g_dog_fd, WDIOC_KEEPALIVE, NULL);
        DBG("feed dog szTime:%s!, ret:%d\n", szTime, ret);
    }

    if (++tick >= 12 && (tick%12) == 0) { // avoid no buffers, and enlarge check period
        //watch_rec_timer();
    }
}

int init_client_watchdog_feed(void *data)
{

    int timeout = 0;
    int i = 0;

    g_dog_fd = open(WATCH_DOG_DEVICE, O_RDWR);
    if (g_dog_fd <= 0) {
        perror("open WDT device");
        return -1;
    }

    ioctl(g_dog_fd, WDIOC_GETTIMEOUT, &timeout);
    DBG("timeout = %d\n", timeout);
    timeout = WATCH_DOG_TIMEOUT;
    ioctl(g_dog_fd, WDIOC_SETTIMEOUT, &timeout);
    ioctl(g_dog_fd, WDIOC_GETTIMEOUT, &timeout);
    DBG("timeout = %d\n", timeout);

    i = WDIOS_DISABLECARD;
    DBG("%d\n",ioctl(g_dog_fd, WDIOC_SETOPTIONS, &i));

    i = WDIOS_ENABLECARD;
    DBG("%d\n",ioctl(g_dog_fd, WDIOC_SETOPTIONS, &i));

    sch_wdt = data;
    js_create_timer_r(sch_wdt, WATCH_DOG_TIME_FIRST_INTERVAL, WATCH_DOG_TIME_INTERVAL,watch_dog_func, NULL, &hdl_wdt);

    return 0;
}

void uninit_client_watchdog_feed()
{
    if (g_dog_fd > 0) {
        int nValue = WDIOS_DISABLECARD;
        ioctl(g_dog_fd, WDIOC_SETOPTIONS, &nValue);
        close(g_dog_fd);
        g_dog_fd = -1;
    }

    /* call before updateExt.sh, avoid sync() blocking */
    js_delete_timer_r(&hdl_wdt);

    sch_wdt = NULL;
}

static void auto_do_timereboot()
{
    struct sysinfo info;
    sysinfo(&info);
    if (info.uptime > 120) {
        DELAY_REBOOT_LINUX();
    }
}

//每天凌晨2点判断腾讯不在线时重启设备
static void p2p_do_timereboot()
{
    //DBG("auto_do_timereboot\n");
    time_t timenow;
    struct tm nowtm = {0};
    struct sysinfo info;
    DevConfS devconf = {0};
    get_config(handleDevConf, devconf);

    timenow = time(NULL);
    localtime_r(&timenow, &nowtm);

    if ((2 == nowtm.tm_hour) && (0 == nowtm.tm_min)) {
        if (platform_on_line()) {
            DBG("p2p is online\n");
        } else {
            sysinfo(&info);
            if (info.uptime > 120) {
                if (www_reachable()) {
                    // 绑定文件不存在且devicebind不为1:  设备未绑定,不走广电策略
                    if ((!devconf.devicebind) && get_g_sys(usb_4g)) {
                        return;
                    }
                    // 未配置三元组的不走策略
                    if (!is_okey(F_P2P_TRIPLE)){
                        return;
                    }
                    if (!is_test_ver()) {
                        SYSLOG("reboot from watch tencent abnormal disconnect");
                        DELAY_REBOOT_LINUX();
                    }
                } else {
                    DBG("Network Disconnected");
                }
            }
        }
    }
}

static void watch_auto_reboot(void *data)
{
    time_t timenow;
    struct tm nowtm = {0};

    p2p_do_timereboot();

    if (0 == ar.enable) {
        return ;
    }

    timenow = time(NULL);
    localtime_r(&timenow, &nowtm);

    if (ar.alarmday == 7) {
        if ((ar.alarmhour == nowtm.tm_hour) && (nowtm.tm_min == 0)) {
            // log_record(1, 1, 1, MODULE, "auto reboot at %dday %dhour %dmin", ar.alarmday,ar.alarmhour, nowtm.tm_min);
            auto_do_timereboot();
        }
    } else {
        if ((ar.alarmday == nowtm.tm_wday) && (ar.alarmhour == nowtm.tm_hour)
            && (nowtm.tm_min == 0)) {
            // log_record(1, 1, 1, MODULE, "auto reboot at %dday %dhour %dmin", ar.alarmday,ar.alarmhour, nowtm.tm_min);
            auto_do_timereboot();
        }
    }

}

int chanage_auto_reboot(AutoRebootS ars)
{
    memcpy(&ar, &ars, sizeof(AutoRebootS));
    return 0;
}

int init_client_watch_auto_reboot(void *data)
{
    sch_autorbt = data;
    get_config(handleAutoRebootCfg, ar);

    js_create_timer_r(sch_autorbt, WATCH_ATUO_REBOOT_TIME_INTERVAL, WATCH_ATUO_REBOOT_TIME_INTERVAL,
                watch_auto_reboot, NULL, &hdl_autorbt);
    return 0;
}

void uninit_client_watch_auto_reboot()
{
    if (!sch_autorbt) {
        return;
    }

    js_delete_timer_r(&hdl_autorbt);

    sch_autorbt = NULL;
}

void save_record_before_reboot(void)
{
    uninit_record_watch();
    sync();
}

int system_get_sdexist(int *sdexist)
{
    FILE* stream;
    char FileBuf[512];
    char *ptr = NULL;
    int Length = 0;

    if (NULL == sdexist)
        return -1;

    memset(FileBuf,0,sizeof(FileBuf));
    stream = vpopen("mount | grep mmcblk","r");
    if (!stream) {
        ERR("vpopen error\n");
        *sdexist = 0;
        return FAILURE;
    }
    Length = fread(FileBuf, 1,sizeof(FileBuf), stream);
    vpclose(stream);

    // 如果读不到数据
    if (0 >= Length) {
        //ERR("Length:%d <= 0\n", Length);
        *sdexist = 0;
        return FAILURE;
    }

    if (NULL != (ptr = strstr(FileBuf,"mmcblk0"))) {
        *sdexist = 1;
    } else {
        *sdexist = 0;
    }

    return 0;
}


int init_networking()
{
    // 设置初始状态, 防止 net_check 启动之前，拔插网线导致 dhcp 不分配ip
    net_status(NULL);
    if(!get_g_sys(eth)) {
        DBG("ifconfig down\n");
        return UtilSystemCmd("ifconfig eth0 down");
    } else {
        if (!(system_get_security() || is_okey(F_P2P_TRIPLE))) {
            DBG(" devid is null and F_P2P_TRIPLE is not exist\n");
            return 0;
        }
        DBG("ifconfig _up_\n");
        return UtilSystemCmd("/ipc/bin/networking init");
    }
}

void uninit_networking()
{
    if(0 == access("/tmp/fknfs", F_OK)) {
        return;
    }
    if (0 < GetAppCount("udhcpc")) {
        char shellcmd[128] = {0};
        sprintf(shellcmd, "%s", "killall -9 udhcpc");
        UtilSystemCmd(shellcmd);
    }
}

void exec_redirect_dbgout()
{
    int ret;
    FILE *retp;
    char buf[128];

    ReadCmdResult("ps | grep [s]d_sync", buf, sizeof(buf));
    if (NULL == strstr(buf, "sd_sync")) {
        DBG("no sd_sync match\n");
        return;
    }

    DBG("____________ redirect stdout ___________\n");

    retp = freopen("/tmp/messages.dot", "a", stderr);
    if (!retp) {
        DBG("fail ret@ %p\n", retp);
        return;
    }

    ret = dup2(fileno(stderr), fileno(stdout));
    if (ret == -1) {
        DBG("errno %d: %s\n", errno, strerror(errno));
    }

    setvbuf(stdout, (char *)NULL, _IONBF, 0);
    setvbuf(stderr, (char *)NULL, _IONBF, 0);

    return;
}

int system_init_cmdline(void)
{
    // 将得到的信息存到文件中
    FILE* stream;
    char FileBuf[1024];
    char *ptr = NULL;
    int Length = 0;
    ESensorType snsrtype = SENSOR_NONE;
    ECPUType cpuType = CPU_NONE;
    int cpuFreq = 0;

    memset(&systemType, 0, sizeof(systemType));
    systemType.eSecurityType = Security_SoftWare;

    memset(FileBuf,0,sizeof(FileBuf));
    stream = vpopen("cat /proc/cmdline","r");
    if(!stream) {
        return FAILURE;
    }
    Length = fread(FileBuf, 1,sizeof(FileBuf), stream);
    vpclose(stream);

    // 如果读不到数据
    if(0 >= Length) {
        return FAILURE;
    }

    // 获取感光器型号
    if(NULL != (ptr = strstr(FileBuf,"sensor="))) {
        ptr += strlen("sensor=");
        if (strncmp(ptr,"NONE",strlen("NONE")) == 0) {
            snsrtype = SENSOR_NONE;
        } else if (strncmp(ptr,"GC4663",strlen("GC4663")) == 0) {
            snsrtype = SENSOR_GC4663;
        } else if (strncmp(ptr,"GC3003",strlen("GC3003")) == 0) {
            snsrtype = SENSOR_GC3003;
        } else if (strncmp(ptr,"GC5603",strlen("GC5603")) == 0) {
            snsrtype = SENSOR_GC5603;
        } else if (strncmp(ptr,"SC230AI",strlen("SC230AI")) == 0) {
            snsrtype = SENSOR_SC230AI;
        } else if (strncmp(ptr,"SC200AI",strlen("SC200AI")) == 0) {
            snsrtype = SENSOR_SC200AI;
        } else if (strncmp(ptr,"OS04D10",strlen("OS04D10")) == 0) {
            snsrtype = SENSOR_OS04D10;
        } else if (strncmp(ptr,"SP4329",strlen("SP4329")) == 0) {
            snsrtype = SENSOR_SP4329;
        } else if (strncmp(ptr,"SC465SL",strlen("SC465SL")) == 0) {
            snsrtype = SENSOR_SC465SL;
        } else if (strncmp(ptr,"SC4336P",strlen("SC4336P")) == 0) {
            snsrtype = SENSOR_SC4336P;
        } else if (strncmp(ptr,"SC235",strlen("SC235")) == 0) {
            snsrtype = SENSOR_SC235;
        }
    }

    systemType.eSensor = snsrtype;

    // 获取CPU型号
    if(NULL != (ptr = strstr(FileBuf,"cpu="))) {
        ptr += strlen("cpu=");
        if (strncmp(ptr,T20_NAME,strlen(T20_NAME)) == 0) {
            cpuType = CPU_T20;
            cpuFreq = 860;
        } else if (strncmp(ptr,T21_NAME,strlen(T21_NAME)) == 0) {
            cpuType = CPU_T21;
            cpuFreq = 854;
        } else if (strncmp(ptr,T31X_NAME,strlen(T31X_NAME)) == 0) {
            cpuType = CPU_T31X;
            cpuFreq = 854;
        } else if (strncmp(ptr,T31L_NAME,strlen(T31L_NAME)) == 0) {
            cpuType = CPU_T40X;
            cpuFreq = 854;
        } else if (strncmp(ptr,AX620U_NAME,strlen(AX620U_NAME)) == 0) {
            cpuType = CPU_AX620U;
            cpuFreq = 854;
        } else if (strncmp(ptr,AX620Q_NAME,strlen(AX620Q_NAME)) == 0) {
            cpuType = CPU_AX620Q;
            cpuFreq = 854;
        } else if (strncmp(ptr,HI3516CV610_NAME,strlen(HI3516CV610_NAME)) == 0) {
            cpuType = CPU_HI3516CV610;
            cpuFreq = 950;
        } else if (strncmp(ptr,HI3516CV608_NAME,strlen(HI3516CV608_NAME)) == 0) {
            cpuType = CPU_HI3516CV608;
            cpuFreq = 950;
        }
    }

    if(NULL != (ptr = strstr(FileBuf,"cputype="))) {
        ptr += strlen("cputype=");
        if (0 == strncmp(ptr,"T31X",strlen("T31X")))
            systemType.bSupportHD = 1;
        else if (0 == strncmp(ptr,"T31N",strlen("T31N")))
            systemType .bSupportHD = 1;
        else
            systemType.bSupportHD = 1;
    }
    systemType.eCPUType = cpuType;
    systemType.nCpuFreq = cpuFreq;

    if(system_get_security() && NULL != (ptr = strstr(FileBuf,"device_id="))) {
        ptr += strlen("device_id=");
        strncpy(systemType.szDevID, ptr, MAX_ID_LEN);   // id固定11位
    } else {
        strcpy(systemType.szDevID, "00000000000");
    }

    // 获取CPU频率
    do {
        int icpuFreq = 0;
        char scpuFreq[8];

        ptr = NULL;
        memset(FileBuf, 0, sizeof(FileBuf));
        memset(scpuFreq, 0, sizeof(scpuFreq));
        stream = vpopen("cat /proc/cpuinfo","r");
        if(!stream) {
            break;
        }
        Length = fread(FileBuf, 1, sizeof(FileBuf) - 1, stream);
        vpclose(stream);

        // 如果读不到数据
        if(0 >= Length) {
            break;
        }
        // BogoMIPS                : 709.42
        if(NULL != (ptr = strstr(FileBuf, "BogoMIPS"))) {
            ptr += strlen("BogoMIPS");
            ptr = strstr(ptr, ":");
            ptr += strlen(":");
            if(1 == sscanf(ptr, "%[^.]", scpuFreq)) {
                icpuFreq = atoi(scpuFreq);
            }
        }
        systemType.nCpuFreq = icpuFreq;
    } while (0);

    DBG("systemType:secure=%d sectype=%d devid=%s cpu=%d sensor=%d cpufreq=%d bSupportHD=%d\n",
        system_get_security(), systemType.eSecurityType, systemType.szDevID,
        systemType.eCPUType, systemType.eSensor, systemType.nCpuFreq, systemType.bSupportHD );

    /* 启动更新 aging8h，空中烧录可以参考 jcpcmd prienv __FRESH_SECU__ 实时更新 */
    set_aging8h();

    return SUCCESS;
}

ECPUType system_get_cpu_type(void)
{
    return systemType.eCPUType;
}

ESensorType system_get_snsr_type(void)
{
    return systemType.eSensor;
}

static int is_auth = 0xBADF00D;

void system_clr_security(void)
{
    is_auth = 0xBADF00D;
    SYSLOG("clear security\n");
}

int system_get_security(void)
{
    if (is_auth == 0xBADF00D) {
        int ret = 0;
        const char *p = NULL;
        char mac[32] = {0,};
        char devid[32] = {0,};
        char devinfo[128] = {0,};

        is_auth = FALSE;

        p = uboot_devid_get(devid, sizeof(devid));
        goto_tag_if_fail(p != NULL, __sec_exit);
        p = uboot_mac_get(mac, sizeof(mac));
        goto_tag_if_fail(p != NULL, __sec_exit);
        p = uboot_devinfo_get(devinfo, sizeof(devinfo));
        goto_tag_if_fail(p != NULL, __sec_exit);

        if (0 == check_device_info_isok(get_uid(), mac, devid, devinfo)) {
            is_auth = TRUE;
        } else {
            // 一次自动算法提升 jcoxa -> jcoxb
            if (strlen(devinfo) > strlen("jcoxa") && devinfo[4] == 'a') {
                memset(devinfo, 0, sizeof(devinfo));
                ret = get_device_info_new(get_uid(), mac, devid, devinfo, sizeof(devinfo));
                SYSLOG("up version soft_check to %s, ret:%d\n", devinfo, ret);
                if (0 == ret) {
                    if(uboot_devinfo_set() != SUCCESS) {
                        LOG("uboot_devid_set device info failed\n");
                    }
                    DELAY_REBOOT_LINUX();
                    sleep(15);
                }
            }
        }

__sec_exit:
        SYSLOG("devid security %s %s@%s %s\n", is_auth ? "TRUE" : "FALSE", devid, mac, devinfo);
    }

    return is_auth;
}

ESecurityType system_get_security_type(void)
{
    return systemType.eSecurityType = Security_SoftWare;
}

int system_get_supportHD(void)
{
#if defined(OUR_IVS)
    return 0;
#else
    return 1;
#endif
    return systemType.bSupportHD;
}

int system_get_dev_id(char *szDevID)
{
    if (!szDevID) {
        return FAILURE;
    }

    strncpy(szDevID, systemType.szDevID, MAX_ID_LEN);
    szDevID[MAX_ID_LEN] = '\0';              // id固定11位
    return SUCCESS;
}

int system_set_dev_id(char *dev_id)
{
    if (dev_id == NULL) {
        return FAILURE;
    }

    memcpy(systemType.szDevID, dev_id, sizeof(systemType.szDevID));
    return SUCCESS;
}

int system_set_upgrade_begin(void)
{
    upgrade_begin = TRUE;
    return SUCCESS;
}

int system_get_cpu_udid(unsigned long long *id)
{
    if(id == NULL)
        return -1;

    *id = 0; //systemType.eCpuid;
    return 0;
}

int get_enc_max_fps(void)
{
    VideoIdxE idx = encode_max_idx(CH_FS_MAIN0);
    int max_fps = 0;

    switch (idx) {
    case VideoIdxE_8M:
        max_fps = 15;
        break;
    case VideoIdxE_4M:
    case VideoIdxE_3M_16_9:
    case VideoIdxE_1080P:
        max_fps = 15;
        break;
    default:
        max_fps = 15;
        break;
    }

    return max_fps;
}

int get_valid_fps(int fps)
{
    return MIN(MAX(5, fps), get_enc_max_fps());
}

VideoIdxE get_valid_vidx(VideoIdxE vidx, VideoIdxE min, VideoIdxE max)
{
    if (vidx < min) {
        WAR("small vidx: %d, reset to %d\n", vidx, min);
        return min;
    } else if (vidx > max) {
        WAR("_big_ vidx: %d, reset to %d\n", vidx, max);
        return max;
    } else if (vidx == VideoIdxE_960P) {
        WAR("_960P vidx: %d, reset to 720P\n", vidx);
        return VideoIdxE_720P;
    } else {
        return vidx;
    }
}

const char *system_get_product_name(const char *devtype)
{
    static char product[16] = {0};
    static int got = FALSE;

    if (got) {
        return product;
    }

    strncpy(product, devtype, sizeof(product)-1);

    got = TRUE;

    return product;
}

int system_eth_rate_init()
{
    NetEthS eth = {{0,},};

    conf_get_ethcfg(&eth);

    do {
        if (g_net_reticleold == eth.reticle)
            break;

        if (eth.reticle) { //10M
            net_set_speed("eth0", SPEED_10, DUPLEX_FULL, AUTONEG_ENABLE);
        }

        g_net_reticleold = eth.reticle;
    } while (0);

    if(eth.enable) {
        UtilSystemCmd("ifconfig eth0 up");
        DBG("ifconfig eth0 up\n");
    } else if(!eth.enable){
        UtilSystemCmd("ifconfig eth0 down");
        DBG("ifconfig eth0 down\n");
    }
    UtilSystemCmd("ifconfig");

    return 0;
}

int system_set_eth_rate(int reticle)
{
    do {
        if (g_net_reticleold == reticle)
            break;

        if (reticle) { //10M
            UtilSystemCmd((char *) "echo 2 > /sys/class/net/eth0/phydev/adc");
            net_set_speed("eth0", SPEED_10, DUPLEX_FULL, AUTONEG_ENABLE);
        } else { // 恢复自适应
            UtilSystemCmd((char *) "echo 0 > /sys/class/net/eth0/phydev/adc");
            net_set_speed("eth0", SPEED_100, DUPLEX_FULL, AUTONEG_ENABLE);
        }

        g_net_reticleold = reticle;
    } while (0);

    return 0;
}
