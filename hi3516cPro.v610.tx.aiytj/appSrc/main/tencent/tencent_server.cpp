#ifdef PLATFORM_TENCENT
#include <errno.h>
#include <dirent.h>
#include <fnmatch.h>
#include <sys/mman.h> 

#include "utils.h"
#include "confapi.h"
#include "conf_list.h"
#include "jconfig.h"
#include "debug.h"
#include "js_scheduler.h"
#include "factory_db.h"
#include "jcpService.h"
#include "net_check.h"
#include "jevent.h"
#include "g_sys.h"
#include "g_stat.h"
#include "sim4g_adapter.h"
#include "record_disk.h"
#include "recordapi.h"
#include "delay_exec.h"
#include "time_config.h"
#include "jdns.h"

#include "tencent_server.h"
#include "tencent_param_conf.h"
#include "tencent_event_handle.h"
#include "tencent_media_manage.h"
#include "tencent_cloud_storage.h"
#include "tencent_talk.h"
#include "tencent_model.h"
#include "tencent_living_stream.h"
#include "tencent_ota_update.h"

#include "iv_config.h"
#include "iv_dm.h"
#include "iv_def.h"
#include "iv_ota.h"
#include "iv_ad.h"
#include "lite-utils.h"
#include "qcloud_iot_export.h"
#include "qcloud_iot_import.h"
#include "tencent_http_service.h"
#ifdef AVT_VOIP_ENABLE
#include "iv_voip.h"
#endif
#include "encode_audio_queue.h"
#include "pthread_manage.h"
#include "system_sch.h"
#include "tencent_video_call.h"
#include "encode_videomask.h"

extern iv_sys_os_impl_t os_impl;

typedef enum {
    E_TX_SYS_INIT = 1<<0,
    E_TX_DM_INIT  = 1<<1,
    E_TX_AVT_INIT = 1<<2,
    E_TX_OTA_INIT = 1<<3,
    E_TX_ALL_INIT,
} eTxSrvInit;

typedef struct {
    int txserver_init;  //腾讯云服务是否初始化
    int init_prog;      //腾讯云初始化进度
    int running;        //腾讯云服务是否运行
    int toreconnect;    //腾讯云服务是否需要重连
    int eth0linked;     //有线网络连接状态
    int exiting;        //腾讯云服务是否退出
    int cs_init;        //云存服务是否初始化
    int ota_init;       //OTA是否初始化
    int twecall_init;   //twecall是否初始化
    int restarting;     //tencent 是否 restart
    int block_cnt;
    TXDevParamS dev;
    TripleInfoS triple; //三元组消息
    JSScheduler sch;
    JSScheduler sch_restart;
    JSScheduler sch_block;
    JSScheduler sch_ntp;
    JSTCHandle  hdl;
    JSScheduler hdl_ntp;
    JSTCHandle  hdl_cs;
    JSTCHandle  hdl_ota;
    JSTCHandle  hdl_block;
} TXServerS;

static TXServerS g_tencent = {0,};
#ifdef AVT_VOIP_ENABLE
static int voip_update_authorize_status(char *open_id, int status)
{
    printf("voip_update_authorize_status open_id:%s, status:%d\n", open_id, status);
    return 0;
}
voip_callback_func_s voip_callback_func = { voip_update_authorize_status };
static const char *twecall_cfg_path = "/tmp";
#endif

/* 判断腾讯云域名情况(stun.iotvideo.tencentcs.com, gateway.iotvideo.tencentcs.com)
 * 1. 两个域名都不通，视为网络不通，继续循环等待
 * 2. global 域名不通，shanghai 域名能通，初始化时需要设置为 shanghai 的域名去连接腾讯云上线
 */
int tencent_reachable()
{
    static size_t i = 0;
    time_t start = 0;

    static const char *list[] = {
        "public.iot-as-mqtt.cn-shanghai.aliyuncs.com",      // 最常用
        "iot-auth-global.aliyuncs.com",                     // 可能在 Singapo
        "gateway.iotvideo.tencentcs.com",       // p2p
        "production.hcciot.com",                // jco server
        "iotx-vision-streaming-rtmp-vpc-sh.aliyuncs.com",
        "iotx-vision-vod-rtmp-vpc-sh.aliyuncs.com",
        "link-vision-picture-sh.oss-cn-shanghai.aliyuncs.com",
        "gateway.iotcloud.tencentdevices.com",  // 信令
    };

    start = mono_sec();

    if (!is_alive_name(list[i%ARRAY_SIZE(list)])) {
        ERR("www[%s] timeout in %lld secs\n", list[i%ARRAY_SIZE(list)], mono_sec()-start);
        i++;
        return FALSE;
    }
    DBG("ping [%s] success \n", list[i%ARRAY_SIZE(list)]);

    return TRUE;
}

/* 判断腾讯云域名情况(stun.iotvideo.tencentcs.com, gateway.iotvideo.tencentcs.com)
 * 1. 两个域名都不通，视为网络不通，继续循环等待
 * 2. global 域名不通，shanghai 域名能通，初始化时需要设置为 shanghai 的域名去连接腾讯云上线
 */
int tencent_get_ip(char *ip, int size)
{
    static size_t i = 0;
    time_t start = 0;

    static const char *list[] = {
        "gateway.iotcloud.tencentdevices.com",   // 信令
        "sz.tencent.com",
        "stun.iotvideo.tencentcs.com",          // p2p
        "gateway.iotvideo.tencentcs.com",       // p2p
        "gateway.iotcloud.tencentiotcloud.com"  // 信令
    };

    start = mono_sec();

    if (get_ip_by_domain(list[i%ARRAY_SIZE(list)], ip, size) == NULL) {
        ERR("www[%s] timeout in %lld secs\n", list[i%ARRAY_SIZE(list)], mono_sec()-start);
        i++;
        return FALSE;
    }
    DBG("get_ip [%s] success \n", list[i%ARRAY_SIZE(list)]);

    return TRUE;
}

int tencent_isalive()
{
    if (tencent_reachable() == TRUE) {
        return TRUE;
    }

    return FALSE;
}

int is_tencent_eth0_linked(void)
{
    return (g_tencent.eth0linked == TRUE);
}

int is_tencent_on_line(void)
{
    if (g_tencent.dev.online == ONLINE) {
        return TRUE;
    }

    return FALSE;
}

TripleInfoS* tencent_get_triple_info(void)
{
    return &g_tencent.triple;
}

int tencent_format_sd_card(void)
{
    if (!get_g_stat(record, SD_CD_IN)) {
        ERR("no sd card, ignore format\n");
        return -1;
    }

    if (get_g_stat(record, SD_ERR_WRITE_PROTECT | SD_ERR_MMCNODE)) {
        ERR("storage get mmcpath failed, ignore format\n");
        return -1;
    }

    return record_request_format("/mnt");
}

int tencent_start_sync_utc_time()
{
    if (g_tencent.sch_ntp == NULL) {
        g_tencent.sch_ntp = js_create_scheduler((char*)"tencent_ntp");
        return_val_if_fail(g_tencent.sch_ntp != NULL, FAILURE);
    }

    js_delete_timer_r(&g_tencent.hdl_ntp);

    SYSLOG("start sync utc time\n");
    js_create_timer_r(g_tencent.sch_ntp, 20, 2*60*1000, cb_tencent_sync_utc_time, NULL, &g_tencent.hdl_ntp);
    return SUCCESS;
}

static void device_online(uint64_t u64NetDateTime)
{
    SYSLOG("online time:%lldms\n", u64NetDateTime);
    DBG("\033[1;33m""online time:%lldms""\033[0m\n", u64NetDateTime);
    int ret = 0;
    DevConfS devconf = {0};
    NetWifiS wifi_info = {0};
    g_tencent.dev.online = ONLINE;

    conf_get_wificfg(&wifi_info);
    conf_get_devconf_cfg(&devconf);

    tencent_start_sync_utc_time();
    if ((devconf.devicebind != 1 || pop_g_stat(tencent, TENCENT_BAND)) &&
        strcmp(wifi_info.token, "anonymous") != 0) {
        ret= report_dev_bind(wifi_info.token);
        if (ret == 0) {
            devconf.devicebind = 1;
            conf_set_devconf_cfg(devconf);
        }
    }

    play_conditionally(AUDIO_AUTH_SUCCESS);
    play_conditionally(AUDIO_AUTH_SUCCESS);

}

static void device_offline(iv_sys_offline_status_type_e status)
{
    SYSLOG("offline %d\n", status);
    if (IV_SYS_DISCONNECT_STATUS == status) {
        g_tencent.dev.online = OFFLINE;
        send_event(JEvent_Tencent_Offline);
    } else {
        g_tencent.dev.online = CONNECTING;
    }
}

static int tencent_get_privctrl_info(void)
{
    int ret = 0;
    priv_ctrl_t info = {0};
    static int get_info_suc = FALSE;

    if (get_info_suc) {
        return SUCCESS;
    }

    if (get_g_sys(factest)) {
        DBG("factest not query privctrl info\n");
        return SUCCESS;
    }

    ret = report_privctrl(info);
    if (ret == SUCCESS) {
        set_config(handlePrivCtrlCfg, info);
        SYSLOG("get private ctrl, video: %d\n", info.video);
        get_info_suc = TRUE;
    } else {
        SYSLOG("get private ctrl, network error: %d\n", ret);
    }

    return ret;
}

int tencent_media_init(void)
{
    int ret = 0;
    NetEthS ethcfg = {{0},};
    SYSLOG("tencent_media_init\n");

    conf_get_ethcfg(&ethcfg);
    iv_avt_init_parm_s stAvtInitParameters;
    memset(&stAvtInitParameters, 0, sizeof(iv_avt_init_parm_s));
    stAvtInitParameters.max_frame_size    = (MAX_FRAME_BYTES+MAX_FRAME_HEAD_BYTES)/1024;// 发送数据的最大
    stAvtInitParameters.max_connect_num   = MAX_CONNECT_NUM;//当前设备支持的最大链接数，即连接的APP数量
    stAvtInitParameters.congestion.enable = true;
    //拥塞控制参数,SDK会根据网络环境和用户设置参数，触发相应的事件通知
    stAvtInitParameters.congestion.low_mark  =  MAX_FRAME_BYTES * 0.5 * MAX_SENSOR_NUM;
    stAvtInitParameters.congestion.warn_mark =  MAX_FRAME_BYTES * 1  * MAX_SENSOR_NUM; //变量
    stAvtInitParameters.congestion.high_mark = MAX_FRAME_BYTES * 1.5 * MAX_SENSOR_NUM;

    stAvtInitParameters.p2p_keep_alive.time_inter_s    = 10;//P2P 保活最大时间间隔，单位s
    stAvtInitParameters.p2p_keep_alive.max_attempt_num = 4;//P2P 保活最大尝试次数，最大10
    stAvtInitParameters.iv_avt_get_av_enc_info_cb      = tencent_talk_get_enc_info;         //获取音视频编码
    stAvtInitParameters.iv_avt_start_real_play_cb      = tencent_talk_start_real_play;      //开始播放
    stAvtInitParameters.iv_avt_stop_real_play_cb       = tencent_talk_stop_real_play;       //停止推流
    stAvtInitParameters.iv_avt_start_recv_stream_cb    = tencent_talk_start_recv_stream;    //开始接收音视频回调
    stAvtInitParameters.iv_avt_stop_recv_stream_cb     = tencent_talk_stop_recv_stream;     //停止接收
    stAvtInitParameters.iv_avt_recv_stream_cb          = tencent_talk_recv_stream;          //接收数据流,并进行解码播放
    stAvtInitParameters.iv_avt_notify_cb               = tencent_talk_notify_process;       //事件通知回调
    stAvtInitParameters.iv_avt_recv_command_cb         = tencent_talk_command_proc;         //接收信令
    stAvtInitParameters.iv_avt_download_file_cb        = tencent_talk_download_proc;        //文件下载请求回调
    //stAvtInitParameters.p2p_init_params.protocol     = IV_AVT_P2P_UDP;
    stAvtInitParameters.p2p_init_params.mtu_size       = ethcfg.mtu;
    stAvtInitParameters.p2p_init_params.sender_interval_ms = 10;
    stAvtInitParameters.p2p_init_params.need_pre_connect = 0;
    // IV_eLOG_DEBUG是全局日志等级，IV_AVT_P2P_LOG_DEBUG是p2p模块的日志等级，属于包含关系
    if (is_okey(F_TENCENT_DEBUG)) {
        stAvtInitParameters.p2p_init_params.log_level = IV_AVT_P2P_LOG_DEBUG;
        stAvtInitParameters.p2p_init_params.log_file_path  = NULL;
        stAvtInitParameters.p2p_init_params.log_file_size  = 10 * 1024 * 1024;// 10M
    } else {
        stAvtInitParameters.p2p_init_params.log_level = IV_AVT_P2P_LOG_DISABLE;
        stAvtInitParameters.p2p_init_params.log_file_path  = NULL;
        stAvtInitParameters.p2p_init_params.log_file_size  = 500 * 1024;// 500K
    }

#ifdef AVT_LAN_ENABLED
    stAvtInitParameters.net_info.probe_port = 3072;
    stAvtInitParameters.net_info.trans_port = 34567;
    stAvtInitParameters.net_info.vendor_id  = NULL;  // use productid
    stAvtInitParameters.net_info.device_id  = NULL;  // use device name
    get_local_ip(stAvtInitParameters.net_info.local_addr, "eth");
#endif

    //音视频对讲模块初始化
    ret = iv_avt_init(&stAvtInitParameters);
    if (ret < 0) {
        ERR("iv_avt_init error:%d\n", ret);
        return ret;
    }

    return ret;
}

// SDK ERR LOG append /tmp/messages
static bool log_handler(const char *message)
{
	if (strstr(message, "ERR|QCIV")) {
    	AppendFile("/tmp/messages", message);
	}
    return SUCCESS;
}

/*
 * 当`auto_connect_enable=1`时,设备自主感知离线后会触发 iv_sys_offline_cb(IV_SYS_RECONNECT_STATUS)
 * 通知用户处于重连状态,设备端会自动重连后台,当自动重连成功后会触发 iv_sys_online_cb;
 * 当自动重连失败后会触发 iv_sys_offline_cb(IV_SYS_DISCONNECT_STATUS) 通知用户连接失败,
 * 需要用户重新去初始化和初始化 IoT Video SDK
 * 当`auto_connect_enable=0`时,设备自主感知离线后会触发iv_sys_offline_cb(IV_SYS_DISCONNECT_STATUS)
 * 通知用户连接失败,需要用户重新去初始化和初始化 IoT Video SDK;
 */
int tencent_sys_init(void)
{
    int ret = 0;
    iv_sys_init_parm_s stSysInitParameters;
    SYSLOG("tencent_sys_init\n");
    TripleInfoS* info = tencent_get_triple_info();
    iv_sys_device_info dev_info     = {0};
    //设置系统打印日志
    if (is_okey(FACTORY_SDFIRELOG) || is_okey(F_TENCENT_DEBUG) || is_okey(F_TX_DBGLOG)) {
        iv_sys_set_log_level(IV_eLOG_DEBUG);
    } else {
        iv_sys_set_log_level(IV_eLOG_ERROR);
    }
	IOT_Log_Set_MessageHandler(log_handler);

    memset(&stSysInitParameters, 0, sizeof(iv_sys_init_parm_s));
    dev_info.product_id             = info->product_key;
    dev_info.device_name            = info->device_name;
    dev_info.device_key             = info->device_secret;

    strcpy(stSysInitParameters.sys_cache_path, "/tmp");
    strcpy(stSysInitParameters.sys_store_path, "/opt/log");
    stSysInitParameters.device_info             = &dev_info;
    stSysInitParameters.iv_sys_online_cb        = device_online;
    stSysInitParameters.iv_sys_offline_cb       = device_offline;
    stSysInitParameters.command_timeout         = 5 * 1000;
    stSysInitParameters.keep_alive_ms           = 60 * 2 * 1000;//保活时间,建议使用240s
    //ping服务器时间间隔 5JY288BMX2.iotcloud.tencentdevices.com
    stSysInitParameters.mqtt_ping_interval_ms   = 30 * 1000;
    stSysInitParameters.auto_connect_enable     = 1;
    stSysInitParameters.mqtt_recv_buf_max_size  = 4096;
    stSysInitParameters.mqtt_write_buf_max_size = 4096;
    stSysInitParameters.max_channel_num         = MAX_SENSOR_NUM;
    memcpy(&stSysInitParameters.os_impl_cb, &os_impl, sizeof(os_impl));
    ret = iv_sys_init(&stSysInitParameters);
    if (ret < 0) {
        ERR("iv_sys_init error:%d\n", ret);
    }

    g_tencent.dev.online = CONNECTING;

    return ret;
}


int file_exists_with_pattern(const char *dir_path, const char *pattern, char *full_path)
{
    DIR *dir;
    struct dirent *entry;

    // Open the directory
    dir = opendir(dir_path);
    if (dir == NULL) {
        perror("opendir");
        return 0;
    }

    // Iterate through the directory entries
    while ((entry = readdir(dir)) != NULL) {
        // Skip "." and ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Construct the full path to the file
        snprintf(full_path, 1024, "%s/%s", dir_path, entry->d_name);

        // Check if the filename matches the pattern
        if (fnmatch(pattern, entry->d_name, 0) == 0) {
            closedir(dir);
            return 1; // File exists with the given pattern
        }
    }

    closedir(dir);
    return 0; // No file matches the pattern
}

int tencent_set_device_triple()
{
    int ret = 0;
    const char* p_triple_file = F_P2P_TRIPLE;
    TripleInfoS* info = tencent_get_triple_info();

    //三元组备份
    tencent_uboot_triple_repair(p_triple_file);
    const char *p_dir_path = "/mnt";
    const char *pattern = "super_*.txt";
    char full_path[1024] = {0,};
    //获取三元组,保存到全局变量中
    ret = tencent_load_triple_info(p_triple_file, info);
    if (is_pattern_exist(p_dir_path, pattern, full_path) && get_g_sys(factest)) {
        //获取产测三元组,保存到全局变量中
        SYSLOG("factest pk use %s\n", full_path);
        ret = tencent_load_triple_info(full_path, info);
    } else {
        SYSLOG("pk use %s\n", p_triple_file);
    }

    return ret;
}

static void tencent_restart_init(void *data)
{
    SYSLOG("tencent_restart_init\n");
    DropCache(__func__);
    CompactMemo(__func__);
    UtilSystemCmd("free");

    do {
        int ret = uninit_tencent();
        if(FAILURE == ret) {
            SYSLOG("uninit_tencent fail\n");
            break;
        }

        ret = init_tencent();
        if(FAILURE == ret) {
            SYSLOG("init_tencent fail\n");
            break;
        }
    } while(0);

    return;
}

static void cs_reset(void *userdata)
{
    WAR("cs_reset\n");
    cs_uninit();
    g_tencent.cs_init = FALSE;
    return;
}

void do_cs_reset(void)
{
    DBG("do_cs_reset\n");
    js_run_function(g_tencent.sch, cs_reset, NULL, 0);
}

static void ota_reset(void *userdata)
{
    WAR("ota_reset\n");
    iv_ota_exit();
    g_tencent.ota_init = FALSE;
    return;
}

static void twecall_init()
{
#ifdef AVT_VOIP_ENABLE
    int ret = 0;
    static int get_succ = -1;
    sVideoCallCfg video_call = {0,};

    conf_get_videocall_cfg(&video_call);

    if (get_succ != SUCCESS) {
        get_succ = report_twecall();
    }
    ret = iv_avt_voip_init_v2(VOIP_WXA_TYPE_RELEASE, twecall_cfg_path, video_call.modelId,
                             video_call.appId, voip_callback_func);
    if (ret) {
        DBG("iv_avt_voip_init_v2 fail(%d)\n", ret);
    }
    
#endif
    return;
}

void tencent_loop(void *data)
{
    if (!g_tencent.restarting) {
        if (g_tencent.toreconnect == TRUE && tencent_isalive() == TRUE) {
            g_tencent.restarting = TRUE;
            js_run_function(g_tencent.sch_restart, tencent_restart_init, NULL, 0);
        }
    }

    //上线后进行 cs 和 ota 的初始化, restart 的时候不能初始化 cs 和 ota
    if (platform_on_line() && !g_tencent.restarting) {
        if (videomask_enabled()) {
            if (g_tencent.cs_init == TRUE) {
                COLOR_G("video mask enable, uninit cs\n");
                cs_uninit();
                g_tencent.cs_init = FALSE;
            }
        } else {
            if (g_tencent.cs_init == FALSE) {
                COLOR_G("init cs\n");
                g_tencent.cs_init = TRUE;
                if (cs_init() != SUCCESS) {
                    ERR("tencent_cloud_storage_init fail\n");
                    js_create_once(g_tencent.hdl_cs, g_tencent.sch, 2*60*1000, cs_reset, NULL);
                }
            }
        }

        //OTA初始化
        if (g_tencent.ota_init == FALSE || tencent_ota_thread_exited()) {
            g_tencent.ota_init = TRUE;
            if (tencent_ota_init() != SUCCESS) {
                ERR("tencent_ota_init fail\n");
                js_create_once(g_tencent.hdl_ota, g_tencent.sch, 60*1000, ota_reset, NULL);
            } else {
                g_tencent.txserver_init |= E_TX_OTA_INIT;
            }
        }

        //twecall初始化
        if (g_tencent.twecall_init == FALSE) {
            g_tencent.twecall_init = TRUE;
            twecall_init();

            set_video_call_ready(TRUE);
        }
    }
}

static void * tencent_server_process(void *data)
{
    int ret = SUCCESS;
    g_tencent.running = FALSE;
    g_tencent.restarting = FALSE;
    g_tencent.toreconnect = FALSE;
    g_tencent.eth0linked = net_link_status("eth0");

    //循环等待网络连通
    while (tencent_isalive() == FALSE && !g_tencent.exiting) {
        DBG("networking fail\n");
        sleep (1);
    }

    do {
        if (g_tencent.exiting) {
            break;
        }

        cb_tencent_sync_utc_time(NULL);
        ret = tencent_get_privctrl_info();
        if (ret != SUCCESS) {
            ERR("tencent_get_privctrl_info fail\n");
        }

        //三元组等信息设置
        ret = tencent_set_device_triple();
        break_if_fail(ret == SUCCESS, FAILURE);

        //事件回调绑定,事件上报
        tencent_register_user_event();

        //系统资源的初始化。包括连接参数设置、上下线回调注册,物模型初始化等。
        ret = tencent_sys_init();
        break_if_fail(ret == SUCCESS, FAILURE);
        g_tencent.txserver_init |= E_TX_SYS_INIT;

        ret = tencent_model_init();
        break_if_fail(ret == SUCCESS, FAILURE);
        g_tencent.txserver_init |= E_TX_DM_INIT;

        //直播、回放sch初始化
        ret = tencent_media_manage_init();
        break_if_fail(ret == SUCCESS, FAILURE);

        //audio video talk init
        ret = tencent_media_init();
        break_if_fail(ret == SUCCESS, FAILURE);
        g_tencent.txserver_init |= E_TX_AVT_INIT;

        //语音对讲
        ret = tencent_talk_init();
        break_if_fail(ret == SUCCESS, FAILURE);

        if (g_tencent.sch == NULL) {
            g_tencent.sch = js_create_scheduler((char*)"tencent_server");
            break_if_fail(g_tencent.sch != NULL, FAILURE);
        }

        if (g_tencent.sch_restart == NULL) {
            g_tencent.sch_restart = js_create_scheduler((char*)"tencent_restart");
            break_if_fail(g_tencent.sch_restart != NULL, FAILURE);
        }

        js_create_timer_r(g_tencent.sch, 100, 1000, tencent_loop, NULL, &g_tencent.hdl);
        break_if_fail(g_tencent.hdl != NULL, FAILURE);

        g_tencent.running = TRUE;
        DBG("init_tencent success\n");
    }while(0);

    g_tencent.init_prog = E_TX_INIT_PROG_DONE;

    return NULL;
}

static void loop_tencent_uninit_block_watch(void *ctx)
{
    if (E_TX_INIT_PROG_NONE != g_tencent.init_prog) {
        g_tencent.block_cnt++;
    } else {
        g_tencent.block_cnt = 0;
    }

    //3 分钟还未反初始化完毕
    if (g_tencent.block_cnt > 60 * 3) {
        LOG("tencent uninit stuck, reboot master\n");
        DELAY_REBOOT_LINUX();
    }
}

static int tencent_uninit_block_watch(void)
{
    int ret = 0;

    do {
        if (NULL == g_tencent.sch_block) {
            g_tencent.sch_block = js_create_scheduler((char *)"sch_block_watch");
            if (NULL == g_tencent.sch_block) {
                ERR("create sch block watch failed\n");
                ret = FAILURE;
                break;
            }
        }

        js_create_timer_r(g_tencent.sch_block, 1000, 1000,
                          loop_tencent_uninit_block_watch, NULL,
                          &g_tencent.hdl_block);
        if (NULL == g_tencent.hdl_block) {
            ERR("create hdl block watch failed\n");
            ret = FAILURE;
            break;
        }
    } while (0);

    return ret;
}

static struct elf_info {
    void *start;
    size_t len;
} elfs[2] = {{0}};

void mfree_xp2p_code(void)
{
    munlock(elfs[0].start, elfs[0].len);
    munlock(elfs[1].start, elfs[1].len);

    SYSLOG("success prefetch_over len: %uKB\n", (elfs[0].len+elfs[1].len)>>10);
    return;
}

void mlock_xp2p_code(void)
{
    struct timespec clock = {0};
    ms_clock_reset(&clock);
    
    uint32_t start = 0;
    uint32_t end   = 0;
    LoadFile2("/ipc/app/xp2p_addr", "%x %x", &start, &end);
    // 确保 start <= end（按地址排序）
    if (start == 0 || end == 0) {
        DBG("start: %u end: %u\n", start, end);
        return;
    }
    
    if (start > end) {
        uint32_t tmp = start;
        start = end;
        end = tmp;
    }
    // 保守估计：覆盖 [start , end ] + 64KB                                            
    size_t page_size = getpagesize();

    // xp2p 强制触发缺页 512KB 400KB ：逐页读一个字节                                  
    void *prefetch_start = (void*)(((uintptr_t)start & ~(page_size - 1)) - 512 * 1024);
    size_t len = ((uintptr_t)end - (uintptr_t)prefetch_start) + 400 * 1024;

    for (size_t off = 0; off < len; off += page_size) {
        volatile char c = *((volatile char*)prefetch_start + off);
        (void)c;
    }
    mlock(prefetch_start, len);
    elfs[0].start = prefetch_start;
    elfs[0].len = len;

    // tencent 强制触发缺页：逐页读一个字节
    prefetch_start = (void*)((uintptr_t)tencent_push_live_video & ~(page_size - 1));
    len = 64 * 1024;
    for (size_t off = 0; off < len; off += page_size) {
        volatile char c = *((volatile char*)prefetch_start + off);
        (void)c;
    }
    mlock(prefetch_start, len);
    elfs[1].start = prefetch_start;
    elfs[1].len = len;
    SYSLOG("xp2p spend %lldms\n\n\n", ms_since_previous(&clock));
    SYSLOG("success prefetch_start 0x%x 0x%x %p\n", start, end, prefetch_start);
    return;
}

int init_tencent(void)
{
    DBG("init_tencent\n");
    g_tencent.init_prog = E_TX_INIT_PROG_START;
    if (0 == pthread_namecreate(__func__, tencent_server_process, NULL)) {
        ERR("create tencent server thread fail\n");
        g_tencent.init_prog = E_TX_INIT_PROG_DONE;
        return -1;
    }
    return 0;
}

int uninit_tencent(void)
{
    g_tencent.exiting = TRUE; // 退出等待网络连通的循环
    if (FALSE == g_tencent.running) {
        ERR("tencent server not running\n");
        return FAILURE;
    }
    do {
        usleep(10 * 1000);
    } while(E_TX_INIT_PROG_START == g_tencent.init_prog);

    DBG("uninit_tencent\n");
    g_tencent.dev.online = OFFLINE;

    tencent_unregister_user_event();

    set_video_call_ready(FALSE);

    js_delete_timer_r(&g_tencent.hdl);
    js_delete_timer_r(&g_tencent.hdl_cs);
    js_delete_timer_r(&g_tencent.hdl_ota);

    tencent_uninit_block_watch();

    if (g_tencent.txserver_init & (E_TX_OTA_INIT)) {
        iv_ota_exit();
        DBG("iv_ota_exit completed\n");
    }

    cs_uninit();

    if (g_tencent.txserver_init & (E_TX_AVT_INIT)) {
        iv_avt_exit();
        DBG("iv_avt_exit completed\n");
    }

    tencent_talk_uninit();
    iv_avt_voip_exit_v2();
    tencent_media_manage_uninit();

    if (g_tencent.txserver_init & (E_TX_DM_INIT)) {
        tencent_model_uninit();
        DBG("tencent_model_uninit completed\n");
    }

    if (g_tencent.txserver_init & (E_TX_SYS_INIT)) {
        iv_sys_exit();
        DBG("iv_sys_exit completed\n");
    }

    g_tencent.txserver_init = 0;

    js_delete_scheduler(g_tencent.sch);
    g_tencent.sch = NULL;

    //放最后执行
    js_delete_timer_r(&g_tencent.hdl_block);
    g_tencent.block_cnt = 0;  // 清掉计数

    js_delete_scheduler(g_tencent.sch_block);
    g_tencent.sch_block = NULL;

    g_tencent.cs_init = FALSE;
    g_tencent.ota_init = FALSE;
    g_tencent.twecall_init = FALSE;
    g_tencent.exiting = FALSE;
    g_tencent.init_prog = E_TX_INIT_PROG_NONE;
    DBG("uninit_tencent success\n");

    return SUCCESS;
}

static void uninit_tencent_async_cb(void* userdata)
{
    uninit_tencent();
}

void uninit_tencent_async(void)
{
    js_run_function(g_tencent.sch_restart, uninit_tencent_async_cb, NULL, 1);
}

void tencent_reconnect_cb(int id, void *p_src, int size, void *ctx)
{
    if (g_tencent.toreconnect == TRUE) {
        DBG("HULIANG############\n");
        return;
    }
    if (JEvent_AlarmCabDis == id || JEvent_AlarmCableNormal == id) {
        int linked = (JEvent_AlarmCableNormal == id)?TRUE:FALSE;
        if (g_tencent.eth0linked == linked) {
            return;
        }

        g_tencent.toreconnect = TRUE;
        g_tencent.eth0linked = linked;
    } else if (JEvent_Tencent_Offline == id || JEvent_TencentReset == id) {
        g_tencent.toreconnect = TRUE;
    }

    DBG("tencent server will to reconnect\n");
}
#endif //PLATFORM_TENCENT
