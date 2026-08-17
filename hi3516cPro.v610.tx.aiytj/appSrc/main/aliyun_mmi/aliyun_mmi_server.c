/*
 *       Filename:  aliyun_mmi_server.c
 *    Description:  阿里云百炼MMI系统级初始化
 *                  License全托管模式完整流程:
 *                    时间同步 → 三重密钥 → SDK init → storage配置
 *                    → 设备注册(gen_register_str→HTTP透传→analyze_rsp)
 *                    → 令牌获取(gen_get_token_str→HTTP透传→analyze_rsp)
 *                    → WSS建连 → WS收发循环 → 对话就绪
 *        Version:  1.0
 *        Created:  07/10/2026 10:20:22 AM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  xiangyp (),
 *   Organization:
 */

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "c_mmi.h"
#include "g_log.h"
#include "utils.h"
#include "cmdstat.h"
#include "jconfig.h"
#include "conf_list.h"
#include "c_mmi_cmd.h"
#include "net_check.h"
#include "factory_db.h"
#include "time_config.h"
#include "jconfstruct.h"
#include "aliyun_mmi_cb.h"
#include "aliyun_mmi_cfg.h"
#include "aliyun_mmi_http.h"
#include "aliyun_mmi_talk.h"
#include "c_mmi_cmd_volume.h"
#include "aliyun_mmi_dialog.h"
#include "aliyun_mmi_server.h"

/* ================================================================
 *  内部常量
 * ================================================================ */
#define MS_INTV_EVT               (20)
#define MS_INTV_TXAUD             (30)
#define MS_INTV_RXAUD             (30)
#define S_MMI_HOST_TIMEOUT        (1)
#define S_MMI_REGISTER_MAX_RETRY  (3)
#define MMI_REG_BUF_SIZE          (2048)
#define MMI_TOKEN_BUF_SIZE        (4096)
#define MMI_RSP_BUF_SIZE          (4096)

#define MS_INTV_EVT_HB            (1000)

/* ================================================================
 *  内部类型定义
 * ================================================================ */
typedef enum {
    CMD_MMI_TIMECHG     = 1 << 0,
} eMmiCmd;

typedef struct {
    sMmiTripleKey  triple_key;
    TzoneS         tz;
} sMmiCfg;

typedef struct {
    char     name[32];
    int      id;
    cbMmiEvt cb;
} sMmiEvent;

/* ================================================================
 *  全局状态
 * ================================================================ */

static sMmiCfg  g_cfg_mmi = {0};
static sMmiCfg  g_raw_mmi = {0};
sMmiRun  g_run_mmi = {0};

/* ================================================================
 *  内部辅助: 网络可达性
 * ================================================================ */
static int is_mmi_host_reachable(void)
{
    return is_alive_tcp(MMI_WSS_HOST, MMI_WSS_PORT, S_MMI_HOST_TIMEOUT);
}

/* ================================================================
 *  时间同步回调
 * ================================================================ */
static void cb_sync_tzcfg(int id, void *p_src, int size, void *ctx)
{
    CPY2CMDCFG(CMD_MMI_TIMECHG, &g_raw_mmi.tz, p_src, size);
}

static void cb_sync_time(int id, void *p_src, int size, void *ctx)
{
    DBG("---- time sync ----\n");
    g_run_mmi.tz_synced = TRUE;
}

static void cb_mmi_diff_cfg2cmd(void *ctx)
{
    struct cmdstat *p_cmd = (struct cmdstat *)ctx;

    if (p_cmd->cmd_stage) {
        if (p_cmd->cmd_stage & CMD_MMI_TIMECHG) {
            DBG("---- time zone sync ----\n");
            memcpy(&g_cfg_mmi.tz, &g_raw_mmi.tz, sizeof(g_cfg_mmi.tz));
            g_run_mmi.tz_synced = TRUE;
        }
    }
}

int aliyun_mmi_tz_synced(void)
{
    return g_run_mmi.tz_synced;
}

/* ================================================================
 *  MMI 事件分发表
 * ================================================================ */
static sMmiEvent g_mmi_evt_maps[] = {
    {"USR_CONFIG",       C_MMI_EVENT_USER_CONFIG,      cb_recv_usr_config      },
    {"DATA_INIT",        C_MMI_EVENT_DATA_INIT,        cb_recv_data_init       },
    {"SPEECH_READY",     C_MMI_EVENT_SPEECH_READY,     cb_recv_speech_ready    },
    {"SPEECH_PREPARE",   C_MMI_EVENT_SPEECH_PREPARE,   cb_recv_speech_prepare  },
    {"SPEECH_START",     C_MMI_EVENT_SPEECH_START,     cb_recv_speech_start    },
    {"SPEECH_INTERRUPT", C_MMI_EVENT_SPEECH_INTERRUPT, cb_recv_speech_interrupt},
    {"DATA_DEINIT",      C_MMI_EVENT_DATA_DEINIT,      cb_recv_data_deinit     },
    {"ASR_START",        C_MMI_EVENT_ASR_START,        cb_recv_asr_start       },
    {"ASR_INCOMPLETE",   C_MMI_EVENT_ASR_INCOMPLETE,   cb_recv_asr_incomplete  },
    {"ASR_COMPLETE",     C_MMI_EVENT_ASR_COMPLETE,     cb_recv_asr_complete    },
    {"ASR_END",          C_MMI_EVENT_ASR_END,          cb_recv_asr_end         },
    {"LLM_INCOMPLETE",   C_MMI_EVENT_LLM_INCOMPLETE,   cb_recv_llm_incomplete  },
    {"LLM_COMPLETE",     C_MMI_EVENT_LLM_COMPLETE,     cb_recv_llm_complete    },
    {"TTS_START",        C_MMI_EVENT_TTS_START,        cb_recv_tts_start       },
    {"TTS_END",          C_MMI_EVENT_TTS_END,          cb_recv_tts_end         },
    {"HEARTBEAT",        C_MMI_EVENT_HEARTBEAT,        cb_recv_heartbeat       },
    {"ERROR",            C_MMI_EVENT_ERROR,            cb_recv_error           },
};

static sMmiEvent g_mmi_volume_maps[] = {
    {"VOLUME_UNKNOWN",  C_MMI_CMD_VOLUME_UNKNOWN,  cb_recv_volume_unkown},
    {"VOLUME_INCREASE", C_MMI_CMD_VOLUME_INCREASE, cb_recv_volume_increase},
    {"VOLUME_DECREASE", C_MMI_CMD_VOLUME_DECREASE, cb_recv_volume_decrease},
    {"VOLUME_SET",      C_MMI_CMD_VOLUME_SET,      cb_recv_volume_set},
    {"VOLUME_MUTE",     C_MMI_CMD_VOLUME_MUTE,     cb_recv_volume_mute},
    {"VOLUME_UNMUTE",   C_MMI_CMD_VOLUME_UNMUTE,   cb_recv_volume_unmute},
};

static int32_t cb_mmi_event(uint32_t evt_id, void *param)
{
    int32_t ret = SUCCESS;
    goto_exit_if_fail(evt_id >= 0 && evt_id < ARRAY_SIZE(g_mmi_evt_maps), exit,
                      ret = FAILURE, "invalid evt_id %d\n", evt_id);

    ret = g_mmi_evt_maps[evt_id].cb(param);
    if (SUCCESS == ret) {
        DBG("succ to handle event %s\n", g_mmi_evt_maps[evt_id].name);
    } else {
        ERR("failed to handle event %s\n", g_mmi_evt_maps[evt_id].name);
    }

exit:
    return ret;
}

/* ================================================================
 *  License 预付费模式: SDK内部存储 + 自注册
 * ================================================================ */

#ifdef MMI_PREPAID_MODE
static int register_license_prepaid(sMmiTripleKey *p_key)
{
    int ret = SUCCESS;

    if (!c_license_device_is_registered()) {
        DBG("reset mmi storage\n");
        ret = c_mmi_storage_reset();
        goto_if_fatal_err(0 == ret, exit, ret = FAILURE,
                          "failed to reset storage\n");

        DBG("set app_id license\n");
        ret = c_license_set_app_id_str(p_key->app_id);
        goto_if_fatal_err(0 == ret, exit, ret = FAILURE,
                          "failed to set app_id license\n");

        DBG("set mmi app_id\n");
        ret = c_mmi_storage_set_app_id_str(p_key->app_id);
        goto_if_fatal_err(0 == ret, exit, ret = FAILURE,
                          "failed to set app_id\n");

        DBG("set mmi app_secret\n");
        ret = c_license_set_app_secret_str(p_key->app_secret);
        goto_if_fatal_err(0 == ret, exit, ret = FAILURE,
                          "failed to set app_secret\n");

        DBG("set mmi ws_id\n");
        ret = c_license_set_ws_id(p_key->workspace_id);
        goto_if_fatal_err(0 == ret, exit, ret = FAILURE,
                          "failed to set ws_id\n");

        DBG("storage mmi ws_id\n");
        ret = c_mmi_storage_set_ws_id(p_key->workspace_id);
        goto_if_fatal_err(0 == ret, exit, ret = FAILURE,
                          "failed to storage ws_id\n");

        DBG("set mmi device_name\n");
        ret = c_mmi_set_device_name(p_key->device_name);
        goto_if_fatal_err(0 == ret, exit, ret = FAILURE,
                          "failed to set device_name\n");

        DBG("storage mmi device_name\n");
        ret = c_mmi_storage_set_device_name(p_key->device_name);
        goto_if_fatal_err(0 == ret, exit, ret = FAILURE,
                          "failed to storage device_name\n");

        DBG("save mmi storage\n");
        ret = c_mmi_storage_save();
        goto_if_fatal_err(0 == ret, exit, ret = FAILURE,
                          "failed to save storage\n");
    }

#ifdef MMI_HOSTING_FULL
    DBG("set mmi api_key\n");
    ret = c_mmi_storage_set_api_key(p_key->api_key);
    goto_if_fatal_err(0 == ret, exit, ret = FAILURE,
                      "failed to set api_key\n");
#endif

exit:
    return ret;
}

#else
/* ================================================================
 *  License 后付费模式
 * ================================================================ */

static int register_license_postpaid(sMmiTripleKey *p_key)
{
    int ret = SUCCESS;

    ret = c_mmi_storage_set_app_id_str(p_key->app_id);
    goto_if_fatal_err(0 == ret, exit, ret = FAILURE,
                      "failed to set app_id\n");

    ret = c_mmi_storage_set_ws_id(p_key->workspace_id);
    goto_if_fatal_err(0 == ret, exit, ret = FAILURE,
                      "failed to set workspace_id\n");

    ret = c_mmi_storage_set_api_key(p_key->api_key);
    goto_if_fatal_err(0 == ret, exit, ret = FAILURE,
                      "failed to set api_key\n");

    ret = c_mmi_set_device_name(p_key->device_name);
    goto_if_fatal_err(0 == ret, exit, ret = FAILURE,
                      "failed to set device_name\n");

    ret = c_mmi_storage_save();
    goto_if_fatal_err(0 == ret, exit, ret = FAILURE,
                      "failed to save storage\n");

    ret = c_mmi_init(p_key->workspace_id, p_key->app_id, p_key->api_key);
    goto_if_fatal_err(0 == ret, exit, ret = FAILURE,
                      "failed to init mmi sdk\n");

exit:
    return ret;
}
#endif

/* ================================================================
 *  MMI SDK 初始化（预付费/后付费分发）
 * ================================================================ */

static int init_mmi_sdk(sMmiTripleKey *p_key)
{
    int ret = SUCCESS;

    pri_mmi(LVL_DBG, "init mmi sdk\n");
    ret = c_mmi_sdk_init();
    goto_if_fatal_err(0 == ret, exit, ret = FAILURE,
                      "failed to init mmi sdk\n");

#ifdef MMI_PREPAID_MODE
    ret = register_license_prepaid(p_key);
#else
    ret = register_license_postpaid(p_key);
#endif
    goto_if_fatal_err(0 == ret, exit, ret = FAILURE,
                      "failed to register mmi license\n");

    mmi_user_config_t mmi_cfg = C_MMI_CONFIG_CUSTOM();

    mmi_cfg.evt_cb = cb_mmi_event;
    pri_mmi(LVL_DBG, "c_mmi_config voice=%s ds_rate=%d\n",
            mmi_cfg.voice_id, mmi_cfg.ds_sample_rate);

    ret = c_mmi_config(&mmi_cfg);
    goto_if_fatal_err(0 == ret, exit, ret = FAILURE,
                      "failed to config mmi\n");

    ret = c_mmi_set_voice_id(mmi_cfg.voice_id);
    goto_if_fatal_err(0 == ret, exit, ret = FAILURE,
                      "failed to set voice_id\n");

    pri_mmi(LVL_DBG, "succ to init mmi sdk\n");

exit:
    return ret;
}

/* ================================================================
 *  HTTP 设备注册 + 令牌获取（全托管模式，透传SDK加密字符串）
 *
 *  流程:
 *    1. c_license_gen_register_str() → 生成加密注册字符串
 *    2. aliyun_mmi_register_raw()    → HTTP POST 到云端
 *    3. c_license_analyze_register_rsp() → SDK 解析响应, save
 *    4. c_license_gen_get_token_str() → 生成令牌请求字符串
 *    5. aliyun_mmi_get_token_raw()   → HTTP POST 到云端
 *    6. c_license_analyze_get_token_rsp() → SDK 解析令牌响应
 * ================================================================ */

static int do_http_register_and_token(sMmiTripleKey *p_key)
{
    char reg_buf[MMI_REG_BUF_SIZE]   = {0};
    char rsp_buf[MMI_RSP_BUF_SIZE]   = {0};
    char token_buf[MMI_TOKEN_BUF_SIZE] = {0};
    char time_str[32]                = {0};
    int  ret     = SUCCESS;
    int  try_cnt = 0;

    /* ---------- 1. 设备注册（已注册则跳过）---------- */
    if (c_license_device_is_registered()) {
        DBG("device already registered, skip http register\n");
        ret = SUCCESS;
    } else {
        for (try_cnt = 0; try_cnt < S_MMI_REGISTER_MAX_RETRY; try_cnt++) {
            DBG("http register try %d/%d\n",
                try_cnt + 1, S_MMI_REGISTER_MAX_RETRY);

            snprintf(time_str, sizeof(time_str), "%lld",
                     (long long)util_now_ms());

            ret = c_license_gen_register_str(reg_buf, sizeof(reg_buf), time_str);
            goto_if_fatal_err(0 == ret, exit, ret = FAILURE,
                              "failed to gen_register_str\n");

            ret = aliyun_mmi_register_raw(reg_buf, rsp_buf, sizeof(rsp_buf));
            if (SUCCESS != ret) {
                ERR("failed to register mmi http, ret=%d\n", ret);
                sleep(2);
                continue;
            }

            /* 全托管 API 返回信封 {"code":...,"data":{...}}，剥离出 data */
            char *data = aliyun_mmi_extract_data_json(rsp_buf);
            goto_if_fatal_err(NULL != data, exit, ret = FAILURE,
                              "failed to extract register json data\n");

            ret = c_license_analyze_register_rsp(data);
            free(data);

            /* 设备已激活 → 跳过注册 */
            if (ret != 0 &&
                E_ALIYUN_ERR_DEV_ACTIVATED == aliyun_mmi_get_json_code(rsp_buf)) {
                DBG("device already activated, skip to get_token\n");
                ret = SUCCESS;
            }

            if (0 == ret) {
                ret = c_mmi_storage_save();
                goto_if_fatal_err(0 == ret, exit, ret = FAILURE,
                                  "failed to save after register\n");
                break;
            }

            ERR("failed to analyze_register_rsp, ret=%d\n", ret);
            sleep(2);
        }
    }

    goto_if_fatal_err(SUCCESS == ret, exit, ret = FAILURE,
                      "failed to register mmi http after %d retries\n",
                      S_MMI_REGISTER_MAX_RETRY);

    DBG("succ to register mmi http\n");

    /* ---------- 2. 获取交互令牌 ---------- */
    memset(rsp_buf, 0, sizeof(rsp_buf));

    for (try_cnt = 0; try_cnt < S_MMI_REGISTER_MAX_RETRY; try_cnt++) {
        DBG("http get_token try %d/%d\n",
            try_cnt + 1, S_MMI_REGISTER_MAX_RETRY);

        snprintf(time_str, sizeof(time_str), "%lld",
                 (long long)util_now_ms());

#ifdef MMI_HOSTING_FULL
        ret = c_license_gen_get_token_str(token_buf, sizeof(token_buf),
                                          time_str, p_key->api_key);
#else
        ret = c_license_gen_get_token_str(token_buf, sizeof(token_buf),
                                          time_str, NULL);
#endif
        goto_if_fatal_err(0 == ret, exit, ret = FAILURE,
                          "failed to gen_get_token_str\n");

        ret = aliyun_mmi_get_token_raw(token_buf, rsp_buf, sizeof(rsp_buf));
        if (SUCCESS != ret) {
            ERR("failed to get_token mmi http, ret=%d\n", ret);
            sleep(2);
            continue;
        }

        /* 全托管 API 返回信封，剥离出 data 字段 */
        char *data = aliyun_mmi_extract_data_json(rsp_buf);
        goto_if_fatal_err(NULL != data, exit, ret = FAILURE,
                          "failed to extract token json data\n");

        DBG("data is %s\n", data);
        ret = c_license_analyze_get_token_rsp(data);
        free(data);

        if (0 == ret) {
            break;
        }

        ERR("failed to analyze_get_token_rsp, ret=%d\n", ret);
        sleep(2);
    }
    goto_if_fatal_err(0 == ret, exit, ret = FAILURE,
                      "failed to get_token mmi http after %d retries\n",
                      S_MMI_REGISTER_MAX_RETRY);

    DBG("succ to get_token mmi http\n");

exit:
    return ret;
}

/* ================================================================
 *  对外状态查询
 * ================================================================ */

int ali_mmi_system_inited(void)
{
    return (g_run_mmi.init_step == E_STEP_INIT_OK);
}

void ali_mmi_system_start_dialog(void)
{
    int ret = SUCCESS;

    if (g_run_mmi.init_step != E_STEP_INIT_OK) {
        goto exit;
    }

    DBG("starting dialog\n");
    ret = mmi_dialog_init();
    if (SUCCESS == ret) {
        DBG("mmi dialog engine started\n");
    } else {
        ERR("failed to start mmi dialog engine, ret=%d\n", ret);
    }

exit:
    return;
}

/* ================================================================
 *  Sch1: WSS 发送线程 — send + heartbeat
 * ================================================================ */
static void cb_loop_wss_send(void *ctx)
{
    static int tick_hb = 0;

    if (g_run_mmi.init_step != E_STEP_INIT_OK) {
        goto exit;
    }

    /* 从 SDK 拉取待发帧并发送 */
    mmi_dialog_send_packed_data();

    /* 心跳 */
    if (tick_hb++ >= (MS_INTV_EVT_HB / MS_INTV_EVT)) {
        pri_mmi(LVL_DBG, "update mmi heartbeat\n");
        tick_hb = 0;
        c_mmi_heartbeat_update(util_now_ms());
    }

exit:
    return;
}

/* ================================================================
 *  Sch2: 录音泵线程 — 从共享内存取 PCM → SDK
 * ================================================================ */
static void cb_loop_pack_ai_frames(void *ctx)
{
    if (g_run_mmi.init_step != E_STEP_INIT_OK) {
        goto exit;
    }

    if (!is_mmi_data_inited()) {
        goto exit;
    }

    mmi_dialog_pack_audio_in_frames();

exit:
    return;
}

/* ================================================================
 *  Sch3: TTS 播放泵线程 — SDK 播放缓冲区 → speaker
 * ================================================================ */
static void cb_loop_recv_audio(void *ctx)
{
    if (g_run_mmi.init_step != E_STEP_INIT_OK) {
        goto exit;
    }

    mmi_dialog_play_speech();

exit:
    return;
}

static int cb_mmi_cmd_adjust_volume(uint32_t evt_id, char *req_id, void *param)
{
    int32_t ret = SUCCESS;
    goto_exit_if_fail(evt_id >= 0 && evt_id < ARRAY_SIZE(g_mmi_volume_maps), exit,
                      ret = FAILURE, "invalid evt_id %d\n", evt_id);

    ret = g_mmi_volume_maps[evt_id].cb(param);
    if (SUCCESS == ret) {
        DBG("succ to handle event %s\n", g_mmi_volume_maps[evt_id].name);
    } else {
        ERR("failed to handle event %s\n", g_mmi_volume_maps[evt_id].name);
    }

exit:
    return ret;
}

/* ================================================================
 *  初始化线程: 同步完成所有初始化阶段后自动销毁
 *
 *  阶段: 前置条件 → KEY_LOADED → SDK_CONFIGURED
 *        → TOKEN_GOT → WSS_CONNECTED → INITED
 *  完成后创建 scheduler + timer 启动主循环
 * ================================================================ */

static void *cb_thread_init_mmi(void *arg)
{
    struct cmdstat *p_cmd = (struct cmdstat *)arg;
    int ret = SUCCESS;

    /* ---- 等待前置条件 ---- */
    DBG("waiting for tz sync...\n");
    while (!g_run_mmi.tz_synced) {
        sleep(1);
    }

    DBG("waiting for host reachable...\n");
    while (!is_mmi_host_reachable()) {
        sleep(2);
    }

    /* ---- 阶段1: 加载三重密钥 ---- */
    if (!g_run_mmi.triple_key_loaded) {
        DBG("stage 1: sdk init\n");
        ret = aliyun_mmi_load_triple_key(&g_cfg_mmi.triple_key);
        goto_if_fatal_err(SUCCESS == ret, init_fail, ret = FAILURE,
                          "failed to load triple key\n");
        g_run_mmi.triple_key_loaded = TRUE;
    }

    /* ---- 阶段2: SDK 初始化 + storage 配置 ---- */
    DBG("stage 2: sdk init\n");
    ret = init_mmi_sdk(&g_cfg_mmi.triple_key);
    goto_if_fatal_err(SUCCESS == ret, init_fail, ret = FAILURE,
                      "failed to init mmi sdk\n");

    DBG("stage 3: register volume cmd\n");
    ret = c_mmi_cmd_volume_register(cb_mmi_cmd_adjust_volume);
    goto_if_fatal_err(SUCCESS == ret, init_fail, ret = FAILURE,
                      "failed to register volume cmd\n");

    /* ---- 阶段3: HTTP 设备注册 + 令牌获取 ---- */
#ifdef MMI_HOSTING_FULL
    DBG("stage 4: http register + token\n");
    ret = do_http_register_and_token(&g_cfg_mmi.triple_key);
    goto_if_fatal_err(SUCCESS == ret, init_fail, ret = FAILURE,
                      "failed to register mmi token\n");
#endif

    /* ---- 阶段4: 建立 WSS 连接 ---- */
    DBG("stage 5: wss connect\n");
    ret = mmi_dialog_init();
    goto_if_fatal_err(SUCCESS == ret, init_fail, ret = FAILURE,
                      "failed to init mmi dialog\n");

    util_set_log_level(UTIL_LOG_LV_INFO);

    /* ---- 阶段5: 创建调度器 + 定时器，启动主循环 ---- */
    DBG("stage 6: creating scheduler + 3 run loops\n");
    g_run_mmi.sch_evt = js_create_scheduler("sch_evt");
    goto_if_fatal_err(NULL != g_run_mmi.sch_evt, init_fail, ret = FAILURE,
                      "failed to create sch_evt\n");

    g_run_mmi.sch_pack_aud = js_create_scheduler("sch_pack_aud");
    goto_if_fatal_err(NULL != g_run_mmi.sch_pack_aud, init_fail, ret = FAILURE,
                      "failed to create sch_pack_aud\n");

    g_run_mmi.sch_rx_aud = js_create_scheduler("sch_rx_aud");
    goto_if_fatal_err(NULL != g_run_mmi.sch_rx_aud, init_fail, ret = FAILURE,
                      "failed to create sch_rx_aud\n");

    js_create_timer_r(g_run_mmi.sch_evt, MS_INTV_EVT, MS_INTV_EVT,
                      cb_loop_wss_send, p_cmd, &g_run_mmi.hdl_wss);
    goto_if_fatal_err(NULL != g_run_mmi.hdl_wss, init_fail, ret = FAILURE,
                      "failed to create evt timer\n");

    js_create_timer_r(g_run_mmi.sch_pack_aud, MS_INTV_TXAUD, MS_INTV_TXAUD,
                      cb_loop_pack_ai_frames, p_cmd, &g_run_mmi.hdl_pack_aud);
    goto_if_fatal_err(NULL != g_run_mmi.hdl_pack_aud, init_fail, ret = FAILURE,
                      "failed to create tx_aud timer\n");

    js_create_timer_r(g_run_mmi.sch_rx_aud, MS_INTV_RXAUD, MS_INTV_RXAUD,
                      cb_loop_recv_audio, p_cmd, &g_run_mmi.hdl_rx_aud);
    goto_if_fatal_err(NULL != g_run_mmi.hdl_rx_aud, init_fail, ret = FAILURE,
                      "failed to create rx_aud timer\n");

    g_run_mmi.init_step = E_STEP_INIT_OK;

    DBG("mmi full-managed engine started (3-loop thread init done)\n");

    return NULL;

init_fail:
    return NULL;
}

/* ================================================================
 *  公开 API: 系统初始化入口
 *
 *  网络就绪后调用，创建独立线程同步完成初始化，
 *  最后阶段自动启动 scheduler + timer 进入主循环。
 *  线程完成后自动销毁。
 * ================================================================ */
int init_aliyun_mmi(void)
{
    static struct cmdstat cmd_mmi = {0};
    int ret = SUCCESS;

    cmd_mmi.diff_cfg2cmd = cb_mmi_diff_cfg2cmd;
    g_run_mmi.ctx = &cmd_mmi;

    if (!is_okey(F_ALIYUN_MMI_ENABLE)) {
        DBG("mmi enable flag not exist, skip init\n");
        goto exit;
    }

    if (E_STEP_INIT_NONE != g_run_mmi.init_step) {
        DBG("mmi already inited or init in progress\n");
        goto exit;
    }

    g_run_mmi.init_step = E_STEP_INITIALIZING;

    attach_config(JEvent_TimeZoneCfgChg, cb_sync_tzcfg, &cmd_mmi);
    attach_config(JEvent_TimeChange, cb_sync_time, &cmd_mmi);

    ret = pthread_create(&g_run_mmi.init_thread, NULL,
                         cb_thread_init_mmi, &cmd_mmi);
    goto_if_fatal_err(0 == ret, exit, ret = FAILURE,
                      "failed to create init thread\n");
    pthread_detach(g_run_mmi.init_thread);

    DBG("mmi init thread spawned\n");

exit:
    return ret;
}

int uninit_aliyun_mmi(void)
{
    int ret = SUCCESS;
    int tz_synced = g_run_mmi.tz_synced;

    if (E_STEP_INIT_NONE == g_run_mmi.init_step ||
        E_STEP_DEINIT_START == g_run_mmi.init_step) {
        goto exit;
    }

    g_run_mmi.init_step = E_STEP_DEINIT_START;

    /* 1. 删除三个定时器 */
    js_delete_timer_r(&g_run_mmi.hdl_wss);
    js_delete_timer_r(&g_run_mmi.hdl_pack_aud);
    js_delete_timer_r(&g_run_mmi.hdl_rx_aud);

    /* 2. 安全删除调度器 */
    js_delete_scheduler(g_run_mmi.sch_pack_aud);
    g_run_mmi.sch_pack_aud = NULL;

    js_delete_scheduler(g_run_mmi.sch_evt);
    g_run_mmi.sch_evt = NULL;

    js_delete_scheduler(g_run_mmi.sch_rx_aud);
    g_run_mmi.sch_rx_aud = NULL;

    /* 3. detach 配置回调 */
    detach_config(JEvent_TimeZoneCfgChg, cb_sync_tzcfg, g_run_mmi.ctx);
    detach_config(JEvent_TimeChange, cb_sync_time, g_run_mmi.ctx);

    /* 5. 反初始化 MMI SDK */
    DBG("mmi deinit\n");
    c_mmi_deinit();

    int ms_wait = 0;
    while (is_mmi_data_inited()) {
        ms_sleep(20);
        ms_wait += 20;
        if (ms_wait >= 3000) {
            ERR("wait deinit event timeout\n");
            break;
        }
    }

    c_mmi_stop();

    /* 4. 断开 WSS + 停止 TTS */
    DBG("dialog deinit\n");
    mmi_dialog_deinit();

    /* 6. 重置全局状态 */
    memset(&g_run_mmi, 0, sizeof(g_run_mmi));
    g_run_mmi.tz_synced = tz_synced;

    DBG("mmi uninit done\n");
    g_run_mmi.init_step = E_STEP_INIT_NONE;

exit:
    return ret;
}
