/*
 *       Filename:  aliyun_mmi_cb.c
 *    Description:  MMI 回调集实现 + 状态管理
 *                  USER_CONFIG → 重置对话 | DATA_INIT → 触发WSS建连
 *                  SPEECH_START → 启录音 | ASR_END → 停录音
 *                  TTS_START → 启播放 | ERROR → 解析 err_info
 *        Version:  1.0
 *        Created:  07/10/2026 02:26:52 PM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  xiangyp (),
 *   Organization:
 */

#include "g_log.h"
#include "utils.h"
#include "c_mmi.h"
#include "aliyun_mmi_cb.h"
#include "waytronic_uart.h"
#include "aliyun_mmi_talk.h"
#include "c_mmi_cmd_volume.h"
#include "aliyun_mmi_dialog.h"

/* ================================================================
 *  内部状态标志
 * ================================================================ */

typedef struct {
    int data_inited;
    int ai_talking;
} sMmiCbRun;

static sMmiCbRun g_cb_run = {0};

/* ================================================================
 *  状态查询接口
 * ================================================================ */

int is_mmi_data_inited(void)
{
    return g_cb_run.data_inited;
}

int is_mmi_ai_talking(void)
{
    return g_cb_run.ai_talking;
}

void mmi_cb_reset_ai_talking(void)
{
    g_cb_run.ai_talking = FALSE;
}

/* ================================================================
 *  MMI SDK 回调实现
 * ================================================================ */

int cb_recv_usr_config(void *param)
{
    int ret = SUCCESS;

    ret = c_mmi_reset_dialog_id();
    if (SUCCESS != ret) {
        ERR("failed to reset_dialog_id\n");
    }

    return ret;
}

int cb_recv_data_init(void *param)
{
    int ret = SUCCESS;

    ret = mmi_dialog_init();
    if (SUCCESS == ret) {
        g_cb_run.data_inited = TRUE;
        DBG("wss connected\n");
    } else {
        ERR("failed to connect wss\n");
    }

    return ret;
}

int cb_recv_speech_ready(void *param)
{
    return SUCCESS;
}

int cb_recv_speech_prepare(void *param)
{
    return SUCCESS;
}

int cb_recv_speech_start(void *param)
{
    return SUCCESS;
}

int cb_recv_speech_interrupt(void *param)
{
    return SUCCESS;
}

int cb_recv_data_deinit(void *param)
{
    g_cb_run.ai_talking = FALSE;
    g_cb_run.data_inited = FALSE;

    return SUCCESS;
}

int cb_recv_asr_start(void *param)
{
    mmi_dialog_reset_player();

    g_cb_run.ai_talking = FALSE;

    return SUCCESS;
}

int cb_recv_asr_incomplete(void *param)
{
    if (NULL != param) {
        pri_mmi(LVL_LOOP, "ASR_INCOMPLETE text=[%s]\n", (char *)param);
    }

    return SUCCESS;
}

int cb_recv_asr_complete(void *param)
{
    if (NULL != param) {
        pri_mmi(LVL_DBG, "ASR_COMPLETE text=[%s]\n", (char *)param);
    }

    return SUCCESS;
}

int cb_recv_asr_end(void *param)
{
    return SUCCESS;
}

int cb_recv_llm_incomplete(void *param)
{
    if (NULL != param) {
        pri_mmi(LVL_LOOP, "LLM_INCOMPLETE text=[%s]\n", (char *)param);
    }

    return SUCCESS;
}

int cb_recv_llm_complete(void *param)
{
    if (NULL != param) {
        pri_mmi(LVL_DBG, "LLM_COMPLETE text=[%s]\n", (char *)param);
    }

    return SUCCESS;
}

int cb_recv_tts_start(void *param)
{
    int ret = SUCCESS;

    g_cb_run.ai_talking = TRUE;

    ret = aliyun_mmi_talk_init();
    if (SUCCESS != ret) {
        ERR("failed to init talk\n");
    }

    return ret;
}

int cb_recv_tts_end(void *param)
{
    g_cb_run.ai_talking = FALSE;

    return SUCCESS;
}

int cb_recv_heartbeat(void *param)
{
    return SUCCESS;
}

int cb_recv_error(void *param)
{
    c_mmi_err_info_t *err = (c_mmi_err_info_t *)param;

    if (err) {
        ERR("ERROR code=%d name=%s msg=%s\n",
               err->code,
               err->name  ? err->name  : "NULL",
               err->msg   ? err->msg   : "NULL");
        switch (err->code) {
        case -1:
        case 425:
        default: {
            ERR("error code reset status\n");
            mmi_dialog_reset_player();
            g_cb_run.ai_talking = FALSE;
        }
        }
    } else {
        ERR("ERROR (no detail)\n");
    }

    return SUCCESS;
}

int cb_recv_volume_unkown(void *param)
{
    ERR("recved unknown volume cmd\n");

    return SUCCESS;
}

int cb_recv_volume_increase(void *param)
{
    audiocfg_add_outvolume(PCT_VOLUME_ADD);

    return SUCCESS;
}

int cb_recv_volume_decrease(void *param)
{
    audiocfg_add_outvolume(-PCT_VOLUME_DEC);

    return SUCCESS;
}

int cb_recv_volume_set(void *param)
{
    c_mm_volume_param_t *p_param = (c_mm_volume_param_t *)param;

    DBG("recv outvolume %d\n", p_param->value);
    audiocfg_set_outvolume(p_param->value);

    return SUCCESS;
}

int cb_recv_volume_mute(void *param)
{
    audiocfg_add_outvolume(-PCT_VOLUME_MAX);

    return SUCCESS;
}

int cb_recv_volume_unmute(void *param)
{
    audiocfg_add_outvolume(PCT_VOLUME_MAX);

    return SUCCESS;
}
