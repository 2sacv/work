#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

#include "zbar.h"
#include <assert.h>
#include "debug.h"
#include "net_check.h"
#include "net_qrcode.h"
#include "encode_audio_queue.h"
#include "cJSON.h"
#include "sim4g.h"
#include "airlink.h"
#include "confapi.h"
#include "pthread_manage.h"
#include "js_scheduler.h"
#include "utils.h"
#include "system_ctrl.h"
#include "cmdstat.h"

#include "ss_mpi_sys_mem.h"
#include "ss_mpi_vpss.h"

#ifdef PLATFORM_TENCENT
#include "tencent_server.h"
#include "tencent_http_service.h"
#endif

#include "system_sch.h"
#include "jconfig.h"
#include "shm_buf.h"
#include "g711.h"
#include "ggwave_decode.h"
#include "encode_audio_output.h"
#include "qrEncode.h"
#include "conf_nand.h"
#include "factory_db.h"
#include "encode_common.h"

#define QRCODE_MS_STEP  300
#define GGWAVE_BUF_SIZE 640*1024
#define QRCODE_TIME_OUT 30*60*1000
#define OT_IVP_MILLIC_SEC 100

enum {
    CMD_QRCODE_DEVCONF = 1 << 0,
};

struct qrcode_cfg {
    DevConfS devconf;
};

struct qrcode_run {
    int tick_6s;
    int qrcode_flag;

    JSScheduler sch;
    JSTCHandle  hdl_loop;
};

static struct cmdstat cmdstat_qrcode = {0};
static struct qrcode_cfg cfg = {0};
static struct qrcode_cfg raw = {0};
static struct qrcode_run run = {0};
struct qrcode_cfg *g_cfg_qrcode = &cfg;
struct qrcode_cfg *g_raw_qrcode = &raw;
struct qrcode_run *g_run_qrcode = &run;

#define fourcc(a, b, c, d)                      \
    ((uint32_t)(a) | ((uint32_t)(b) << 8) |     \
     ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))

static int qrcode_parse_from_buffer(void *y8data, int w, int h, char* result, int len);

// 生成二维码
int build_qrcode(void)
{
    int times = 3;
    static char s_builded = 0;

    if (s_builded || is_okey(QRCODE_BMP_PATH)) {
        return 0;
    }

    do {
        int ret = qrencode_tutkid((char *)get_cpuid(), (char *)QRCODE_BMP_PATH);
        if (ret == 0) {
            s_builded = 1;
            break;
        }

        SYSLOG("create qrencode fail, cpuid:%s\n", get_cpuid());
        times--;
        usleep(10*1000);
    } while (times > 0);

    UtilSystemCmd2("chmod 777 %s", QRCODE_BMP_PATH);

    return 0;
}

int get_nv12_frame(char *result, int len)
{
    if (result == NULL) {
        return -1;
    }

    int ret = 0;
    td_void *vir_addr = NULL;
    int vpss_grp = 0;
    td_s32 vpss_chn = OT_VPSS_CHN1;
    ot_video_frame_info frame = {0};
    td_bool isgetframe = TD_FALSE;

    do {
        ret = ss_mpi_vpss_get_chn_frame(vpss_grp, vpss_chn, &frame, OT_IVP_MILLIC_SEC);
        if (TD_SUCCESS != ret){
            //ERR("ss_mpi_vpss_get_chn_frame failed 0x%x\n",ret);
            break;
        }

        isgetframe = TD_TRUE;
        vir_addr = ss_mpi_sys_mmap(frame.video_frame.phys_addr[0], frame.video_frame.width*frame.video_frame.height);
        ENCODE_NULL_BREAK(vir_addr);
        frame.video_frame.virt_addr[0] = vir_addr;

        ret = qrcode_parse_from_buffer((void *)vir_addr, frame.video_frame.width, frame.video_frame.height, result, len);

        if (NULL != vir_addr){
            ss_mpi_sys_munmap(vir_addr, frame.video_frame.width*frame.video_frame.height);
            vir_addr = NULL;
        }
    } while(0);

    if(isgetframe) {
        ret = ss_mpi_vpss_release_chn_frame(vpss_grp, vpss_chn, &frame);
        ENCODE_RET_CHECK(ret, "ss_mpi_vpss_release_chn_frame failed\n");
    }

    return ret;
}

int qrcode_parse_from_buffer(void *y8data, int w, int h, char* result, int len)
{
    int ret = -1;
    zbar_image_scanner_t *scanner = NULL;
    if (y8data == NULL || result == NULL) {
        return -1;
    }

    scanner = zbar_image_scanner_create();
    zbar_image_scanner_set_config(scanner, ZBAR_QRCODE, ZBAR_CFG_ENABLE, 1);
    unsigned long srcfmt = fourcc('Y','8','0','0');
    zbar_image_t *image = zbar_image_create();
    zbar_image_set_userdata(image, NULL);
    zbar_image_set_format(image, srcfmt);
    zbar_image_set_size(image, w, h);
    zbar_image_set_data(image, y8data, w * h, (zbar_image_cleanup_handler_t *)zbar_image_get_userdata);

    int n = zbar_scan_image(scanner, image);
    if (-1 == n ) {
        goto exit;
    }

    const zbar_symbol_t *symbol = zbar_image_first_symbol(image);
    if (NULL == symbol) {
        goto exit;
    }

    for(; symbol; symbol = zbar_symbol_next(symbol)) {
        //zbar_symbol_type_t typ = zbar_symbol_get_type(symbol);
        const char *data = zbar_symbol_get_data(symbol);
        if(data != NULL) {
            if (len < zbar_symbol_get_data_length(symbol)) {
                ERR("buff is too small, qrcode len:%d, %s\n", zbar_symbol_get_data_length(symbol), data);
                continue;
            }

            strncpy(result, data, zbar_symbol_get_data_length(symbol));
            ret = 0;
            //DBG("result:%s\n", result);
        }
    }

exit:
    zbar_image_destroy(image);
    zbar_image_scanner_destroy(scanner);
    return ret;
}

/* 
 * format
 *  wifi:{"s":"TP-LINK_580B","p":"2124234"}
 *    => {"p":"12345678","s":"xuyx","t":"1bed643cb4"}
 *  4g  :{"p":"12345678","b":"C83A35230831","t":"911182"}
 *    => {"t":"aa85f8cc08"}
 * */
int parse_qrcode(char *data, NetWifiS *info)
{
    int ret = 0;
    SysInfoS sysinfo = {{0,},};
    if (NULL == data) {
        return -1;
    }

    if (strstr(data, "{") == NULL) {
        ERR("is not json\n");
        return -1;
    }

    DBG("data: %s\n", data);
    cJSON *cjson_root = cJSON_Parse(data);
    cJSON *random_data = cJSON_GetObjectItem(cjson_root, "t");
    if (random_data == NULL || strlen(random_data->valuestring) <= 6) {
        ERR("code json is NULL\n");
        ret = -1;
        goto json_out;
    }

    strcpy(info->token, random_data->valuestring);
    DBG("token:%s\n", info->token);

    conf_get_sysinfocfg(&sysinfo);
    cJSON *custom_appid = cJSON_GetObjectItem(cjson_root, "c");
    if (custom_appid != NULL) {
        if (atoi10(sysinfo.custom_appid)%100 != (atoi10(custom_appid->valuestring))) {
            SYSLOG("dev appid:%s appid:%s\n", sysinfo.custom_appid, custom_appid->valuestring);
            ret = -1;
            goto json_out;
        }
    } else {
        // 兼容大卫看家不下发c字段时当中性版本处理,设备中的appid不是30000时不让扫码
        if (atoi10(sysinfo.custom_appid)%100 != 0) {
            SYSLOG("dev appid:%s\n", sysinfo.custom_appid);
            ret = -1;
            goto json_out;
        }
    }

    if (get_g_sys(usb_wifi)) {
        info->mode = WifiModeE_AP_STATION;
        cJSON *ssid_data = cJSON_GetObjectItem(cjson_root, "s");
        if (ssid_data == NULL) {
            ERR("code json is NULL\n");
            ret = -1;
            goto json_out;
        }

        DBG("ssid:%s\n", ssid_data->valuestring);
        strcpy(info->ssid, ssid_data->valuestring);
        cJSON *pwd_data = cJSON_GetObjectItem(cjson_root, "p");
        if (pwd_data == NULL) {
            ERR("code json is NULL\n");
            ret = -1;
            goto json_out;
        }

        strcpy(info->weppasswd, pwd_data->valuestring);
        DBG("pwd:%s\n", pwd_data->valuestring);
    }

    set_g_stat(tencent, TENCENT_BAND);

json_out:
    cJSON_Delete(cjson_root);

    return ret;
}

int qrcode_addition(char *result)
{
    int ret = 0;
    NetWifiS info = {0};
    conf_get_wificfg(&info);

    ret = parse_qrcode(result, &info);
    if (ret < 0) {
        ERR("string err\n");
        return ret;
    }

    conf_set_wificfg(info);

    if (get_g_sys(usb_4g)) {
        //滴，正在添加设备
        encode_audio_queue_push_amr(AUDIO_DI_DI, FALSE);
        encode_audio_queue_push_amr(AUDIO_SIM4G_DI_DEV_BINDING, FALSE);
    }

#if defined(PLATFORM_TENCENT)
    ret = report_dev_bind(info.token);
#endif

    return ret;
}

void ggwave_push_audio(uint8_t *stream, int stream_len)
{
    int ret = 0;
    int ggwave_ok = 0;
    static int size = 0;
    char result[512] = {0};

    if (get_g_sys(factest) || !system_get_security()) {
        dbg_audio("it is factest or not exist dev_id\n");
        return;
    }

    if (g_run_qrcode->qrcode_flag || (net_link_status("eth0") == 1)) {
        dbg_audio("eth0 is running or dev binded, stop ggwave\n");
        return;
    }

    if (!platform_on_line() && get_g_sys(usb_4g)) {
        dbg_audio("usb_4g and tx is offline\n");
        return;
    }

    if (is_okey(SUPPLICANT_OK_CONF)) {
        dbg_audio("supplicantOK.conf is ok\n");
        return;
    }

    if (g_cfg_qrcode->devconf.devicebind && get_g_sys(usb_4g)) {
        dbg_audio("device is over binded\n");
        return;
    }

    ggwave_ok = size > 0 ? 1 : 0;
    size = (size + stream_len * 2) % (GGWAVE_BUF_SIZE);
    dbg_audio("size = %d, ggwave_init = %d\n", size, ggwave_ok);
    ret = ggwave_pcm2str((char *)stream, stream_len, result, ggwave_ok);
    if (ret != 0) {
        return;
    } else {
        if (/*!get_g_sys(eth) && */strlen(result) > 0 && (!g_run_qrcode->qrcode_flag)) {
            ret = qrcode_addition(result);
            if (ret == 0) {
                g_run_qrcode->qrcode_flag = 1;
                DBG("decode_result = %s\n", result);
            }
        }
    }

    return;
}

static void loop_qrcode(void *ctx)
{
    int ret = 0;
    char result[512] = {0};
    static int first_play = 0;  // 扫码语音播放
    static int second_play = 0;
    static struct timespec ts_play = {0};
    cmd_get_command((struct cmdstat *)ctx);

    if (g_run_qrcode->tick_6s > 0) {
        if (--g_run_qrcode->tick_6s == 0) {
            DBG("rest 6s done to re-qrcode\n");
        }
        return;
    }

    if (get_g_sys(upgrading)) {
        return;
    }

    if (g_run_qrcode->qrcode_flag) {
        ERR("device have binded\n");
        js_delete_timer_r(&g_run_qrcode->hdl_loop);
        return;
    }

    if (get_g_sys(usb_4g)) {
        if (!platform_on_line()) {
            dbg_audio("platform is offline\n");
            return ;
        }

        if (sim4g_video_workable() != TRUE) {
            dbg_audio("sim4g video not workable\n");
            return ;
        }

        if (!first_play) {
            first_play = 1;
            play_conditionally(AUDIO_SIM4G_SCAN_QRCODE);
            ms_clock_reset(&ts_play);
        }

        if (first_play && (!second_play)) {
            if (ms_clock_is_timeup(&ts_play, 7 * 1000)) {
                second_play = 1;
                play_conditionally(AUDIO_SIM4G_SCAN_QRCODE);
            }
        }
    }

    if (g_run_qrcode->qrcode_flag) {
        return ;
    }

    ret = get_nv12_frame(result, sizeof(result));
    if (strlen(result) != 0 && (!g_run_qrcode->qrcode_flag)) {
        ret = qrcode_addition(result);
        if (ret == 0) {
            g_run_qrcode->qrcode_flag = 1;
        }
    }
}

static void cb_devvecfg(int id, void *p_src, int size, void *ctx)
{
    g_run_qrcode->tick_6s = (60*1000/QRCODE_MS_STEP) ;
    DBG("get codec param change, skip 6s\n");
}

static void cb_devcfg(int id, void *p_src, int size, void *ctx)
{
    CPY2CMDCFG(CMD_QRCODE_DEVCONF, &g_raw_qrcode->devconf, p_src, size);
}

static void diff_cfg2cmd(void *ctx)
{
    struct cmdstat *p_cmd = (struct cmdstat *)ctx;
    if (p_cmd->cmd_stage) {
        if (p_cmd->cmd_stage & CMD_QRCODE_DEVCONF) {
            memcpy(&g_cfg_qrcode->devconf, &g_raw_qrcode->devconf, sizeof(DevConfS));
        }
    }
}

int start_qrcode_server()
{
    if (!get_g_sys(usb_wifi) && !get_g_sys(usb_4g)) {
        DBG("qrcode_server not detected \n");
        return 0;
    }

    struct cmdstat *ctx = &cmdstat_qrcode;
    cmdstat_qrcode.diff_cfg2cmd = diff_cfg2cmd;

    conf_get_devconf_cfg(&g_cfg_qrcode->devconf);

    /* 
     * 当消费类使用 NVR 入网，为防止被恶意添加，eth_runing 时不启用。
     **/
    if ((net_link_status("eth0") == 1)) {
        DBG("eth0 is run\n");
        return 0;
    }

    if (!system_get_security()) {
        DBG("qrcode no security\n");
        return 0;
    }

    if (is_okey(SUPPLICANT_OK_CONF)) {
        DBG("supplicantOK.conf is ok\n");
        return 0;
    }

    if (g_cfg_qrcode->devconf.devicebind && get_g_sys(usb_4g)) {
        DBG("device is over binded\n");
        return 0;
    }

    if (g_run_qrcode->sch == NULL) {
        g_run_qrcode->sch = js_create_scheduler((char *)"qrcode_server");
    }

    stop_qrcode_server();
    attach_config(JEvent_VideoCfgChg  , cb_devvecfg, NULL);
    attach_config(JEvent_ProfileCfgChg, cb_devvecfg, NULL);
    attach_config(JEvent_DevCfg       , cb_devcfg  , (void *)ctx);

    if (NULL == g_run_qrcode->hdl_loop) {
        js_create_timer_r(g_run_qrcode->sch, 1*1000, QRCODE_MS_STEP, loop_qrcode, ctx, &g_run_qrcode->hdl_loop);
    }

    DBG("start_qrcode_server:%p\n", g_run_qrcode->hdl_loop);
    return 0;
}

int stop_qrcode_server()
{
    DBG("stop_qrcode_server\n");
    if (g_run_qrcode->hdl_loop) {
        js_delete_timer_r(&g_run_qrcode->hdl_loop);
    } else {
        DBG("stop_qrcode_server succeed\n");
        return 0;
    }

    struct cmdstat *ctx = &cmdstat_qrcode;
    detach_config(JEvent_VideoCfgChg  , cb_devvecfg, NULL);
    detach_config(JEvent_ProfileCfgChg, cb_devvecfg, NULL);
    detach_config(JEvent_DevCfg       , cb_devcfg  , (void *)ctx);
    DBG("stop_qrcode_server success\n");

    return 0;
}

int uninit_qrcode_server()
{
    js_delete_timer_r(&g_run_qrcode->hdl_loop);
    js_delete_scheduler(g_run_qrcode->sch);
    g_run_qrcode->sch = NULL;

    return 0;
}
