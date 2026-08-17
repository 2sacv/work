/* 
 *       Filename:  encode_cry.c
 *    Description:  
 *        Version:  1.0
 *        Created:  02/04/2026 08:38:58 AM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  xiangyp (), 
 *   Organization:  
 */
#include "g711.h"
#include "debug.h"
#include "g_log.h"
#include "utils.h"
#include "cmdstat.h"
#include "confapi.h"
#include "securec.h"
#include "shm_buf.h"
#include "alarmapi.h"
#include "encode_cry.h"
#include "encode_main.h"
#include "jconfstruct.h"
#include "js_scheduler.h"
#include "ss_audio_bcd.h"
#include "encode_common.h"
#include "ot_common_aio.h"
#include "encode_audio_input.h"
#include "encode_audio_output.h"

#define PATH_CRY    "/algo/cry_det.mgk"

#define MS_INTV_CRY_LOOP   (10)
#define CNT_REFRESH        (1000 / MS_INTV_CRY_LOOP)
#define LVLS_CRYDET_THR    (3)

#define THRESH_CRYDET_HIGH (40)
#define THRESH_CRYDET_MID  (50)
#define THRESH_CRYDET_LOW  (60)

#define ENABLE_CRYDET      (TD_TRUE)
#define DISBLE_CRYDET      (TD_FALSE)

typedef ot_bcd_handle      sIvsIface;

typedef enum {
    E_THRESH_LOW    = 33,
    E_THRESH_MID    = 66,
    E_THRESH_HIGH   = 100,
} eCryDetThresh;

typedef enum {
    CMD_CRY_CFG      = 1 << 0,
    CMD_CRY_CFG_SYNC = 1 << 1,
} eCryCmd;

typedef struct {
    ot_aio_attr    attr_audin;
    int            cry_time;
    int            audio_serial;
    int            bytes_once_pushed;
    float          confidence_det;
    sIvsIface      p_iface;
    JSScheduler    sch;
    JSScheduler    hdl_det;
    shm_buf_t      shmbuf_aud;
    struct cmdstat *ctx;
} sCryDetRun;

typedef struct {
    CryDetectionS cry;
} sCryDetCfg;

typedef struct {
    char framedata[AUDIO_IN_NUM_PERFRM];
    int frame_size;
    eShmMediaType mediatype;
} sCopyFrm;

static sCryDetRun g_run_cry = {0};
static sCryDetCfg g_raw_cry = {0};
static sCryDetCfg g_cfg_cry = {0};

static int init_crydet_ivs(int enable);
static int uninit_crydet_ivs(void);

static int sync_crydet_switch(void)
{
    static int enable_prev = -1;
    int ret = SUCCESS;

    if (enable_prev != g_cfg_cry.cry.enable) {
        ret = uninit_crydet_ivs();
        goto_exit_if_fail(SUCCESS == ret, exit, ret = FAILURE,
                          "failed to uninit ivs crydet\n");

        ret = init_crydet_ivs(g_cfg_cry.cry.enable);
        goto_exit_if_fail(SUCCESS == ret, exit, ret = FAILURE,
                          "failed to init ivs crydet\n");

        enable_prev = g_cfg_cry.cry.enable;
    }

exit:

    return ret;
}

static void sync_confidence_thresh(void)
{
    if (g_cfg_cry.cry.thresh > E_THRESH_MID) {
        g_run_cry.confidence_det = THRESH_CRYDET_HIGH;
    } else if (g_cfg_cry.cry.thresh > E_THRESH_LOW) {
        g_run_cry.confidence_det = THRESH_CRYDET_MID;
    } else {
        g_run_cry.confidence_det = THRESH_CRYDET_LOW;
    }

    COLOR_G("cry detection thresh is %.2f\n", g_run_cry.confidence_det);
}

static int cb_sync_crycfg(void *usr_data)
{
    sync_confidence_thresh();

    g_run_cry.cry_time = TimeJudge(g_cfg_cry.cry.times);

    COLOR_G("sync crydet cfg, cry_time: %d\n", g_run_cry.cry_time);

    return sync_crydet_switch();
}

static sCmdFunc g_cry_cmd_maps[] = {
    {CMD_CRY_CFG_SYNC, cb_sync_crycfg}
};

static void diff_cfg2cmd(void *ctx)
{
    struct cmdstat *p_cmd = (struct cmdstat *)ctx;

    if (p_cmd->cmd_stage <= 0) {
        goto exit;
    }

    if (p_cmd->cmd_stage & CMD_CRY_CFG) {
        DBG("----- crycfg -----\n");
        memcpy(&g_cfg_cry.cry, &g_raw_cry.cry, sizeof(g_cfg_cry.cry));

        cmd_set_command(p_cmd, CMD_CRY_CFG_SYNC);
    }

exit:

    return;
}

static void cb_crydet_cfgchg(int id, void *p_src, int size, void *ctx)
{
    DBG("crycfg changed\n");

    CPY2CMDCFG(CMD_CRY_CFG, &g_raw_cry.cry, p_src, size);
}

static int dec_and_det_crying_audio(sCopyFrm *p_frm_cp)
{
    static struct timespec clk_prev = {0};

    char aud_pcm_out[BYTES_AUDIO_PERFRM_MAX] = {0};
    char aud_pcm[BYTES_AUDIO_PERFRM_MAX] = {0};
    ot_bcd_process_data data_in = {0};
    ot_bcd_process_data data_out = {0};
    int ret = SUCCESS; 
    size_t idx = 0;
    short sample_pcm = 0;

    switch (p_frm_cp->mediatype) {
    case SHM_MEDIA_AUDIO_ALAW: {
        for (idx = 0; idx < p_frm_cp->frame_size; idx++) {
            sample_pcm = jco_alaw2linear(p_frm_cp->framedata[idx]);
            aud_pcm[idx * 2] = sample_pcm & 0xFF;
            aud_pcm[idx * 2 + 1] = (sample_pcm >> 8) & 0xFF;
        }
        break;
    }
    case SHM_MEDIA_AUDIO_ULAW: {
        for (idx = 0; idx < p_frm_cp->frame_size; idx++) {
            sample_pcm = jco_ulaw2linear(p_frm_cp->framedata[idx]);
            aud_pcm[idx * 2] = sample_pcm & 0xFF;
            aud_pcm[idx * 2 + 1] = (sample_pcm >> 8) & 0xFF;
        }
        break;
    }
    default: {
        ERR("unsupported media type %d\n", p_frm_cp->mediatype);
        goto exit;
    }
    }

    for (idx = 0; idx < p_frm_cp->frame_size * 2 / g_run_cry.bytes_once_pushed; idx++) {
        pri_cry(LVL_LOOP, "push idx %d, intv %lldms\n",
                idx, ms_since_previous2(&clk_prev));
        data_in.data       = (td_s16 *)&aud_pcm[idx * g_run_cry.bytes_once_pushed];
        data_in.data_size  = g_run_cry.bytes_once_pushed;
        data_out.data      = (td_s16 *)aud_pcm_out;
        data_out.data_size = g_run_cry.bytes_once_pushed;

        ret = ss_baby_crying_detection_process(g_run_cry.p_iface, &data_in, &data_out);
        goto_exit_if_fail(TD_SUCCESS == ret, exit, ret = FAILURE,
                          "failed to detect baby crying\n");

        ms_sleep(10);
    }

    ret = SUCCESS;

exit:

    return ret;
}

static void cb_copy_crying_audio(void *userdata, tSBFrame *p_frm)
{
    sCopyFrm *p_frm_cp = (sCopyFrm *)userdata;

    if (g_run_cry.audio_serial > 0 &&
        p_frm->curframe_serial + 10 < g_run_cry.audio_serial) {
        pri_cry(LVL_DBG, "audio shmbuf maybe reset, mediatype: %d"
                ", cur serial: %d, aserial: %d\n", p_frm->mediatype,
                p_frm->curframe_serial, g_run_cry.audio_serial);
        goto exit;
    }

    switch (p_frm->error) {
    case SHM_ERR_SUCCESS: {
        if (g_run_cry.audio_serial <= 0) {
            g_run_cry.audio_serial = p_frm->frame_serial;
        }

        if (p_frm->frame_size <= sizeof(p_frm_cp->framedata)) {
            memcpy_s(p_frm_cp->framedata, sizeof(p_frm_cp->framedata), p_frm->framedata,
                     p_frm->frame_size);
            p_frm_cp->frame_size = p_frm->frame_size;
            p_frm_cp->mediatype = p_frm->mediatype;
        } else {
            ERR("copy buffer not bigger enough, need %d, real %d\n",
                p_frm->frame_size, sizeof(p_frm_cp->framedata));
        }

        break;
    }
    case SHM_ERR_NOT_READY: {
        pri_cry(LVL_WAR, "audio shmbuf not ready\n");
        break;
    }
    case SHM_ERR_OVER_WRITE: {
        pri_cry(LVL_DBG, "audio shmbuf serial %d over write\n", g_run_cry.audio_serial);
        g_run_cry.audio_serial = 0;
        break;
    }
    default: {
        DBG("failed to read audio shmbuf: %d\n", p_frm->error);
    }
    }

exit:

    return;
}

static void cb_loop_crydet_detect(void *ctx)
{
    static struct timespec clk_intv = {0};
    static size_t refresh_cnt = 0;

    static sCopyFrm frm_cp = {0};
    int cmd = cmd_get_command((struct cmdstat *)ctx);
    int ret = SUCCESS;
    size_t idx = 0;

    pri_cry(LVL_LOOP, "crydet is running, intv %lld ms\n",
            ms_since_previous2(&clk_intv));

    for (idx = 0; idx < ARRAY_SIZE(g_cry_cmd_maps); idx++) {
        if (cmd <= 0) {
            break;
        }

        if (cmd & g_cry_cmd_maps[idx].cmd) {
            ret = g_cry_cmd_maps[idx].cb_handle_cmd(ctx);
            if (SUCCESS != ret) {
                ERR("failed to sync cmd %d\n", cmd);
            }
        }
    }

    if (refresh_cnt++ >= CNT_REFRESH) {
        refresh_cnt = 0;

        if (g_cfg_cry.cry.enable) {
            g_run_cry.cry_time = TimeJudge(g_cfg_cry.cry.times);
        } else {
            g_run_cry.cry_time = FALSE;
        }

        pri_cry(LVL_DBG, "refresh cry time: %d\n", g_run_cry.cry_time);
    }

    if (!g_run_cry.cry_time) {
        pri_cry(LVL_LOOP, "crydet is not enable or in time\n");
        goto exit;
    }

    shm_buf_read_frame_ex(g_run_cry.shmbuf_aud, g_run_cry.audio_serial,
                          cb_copy_crying_audio, &frm_cp);

    if (frm_cp.frame_size > 0) {
        ret = dec_and_det_crying_audio(&frm_cp);
        if (SUCCESS == ret) {
            g_run_cry.audio_serial++;
        } else {
            ERR("failed to decode and detect crying audio\n");
        }

        frm_cp.frame_size = 0;
    }

exit:

    return;
}

static td_s32 cb_handle_detected_cry(td_void *usr_data)
{
    static struct timespec clk_cry_prev = {0};

    if (ms_clock_is_timeup(&clk_cry_prev, 2000)) {
        ENCODE_RET_JUDGE(alarm_report(JALARM_TYPE_CRY, 0, NEED_TIME_CHECK,
                                      "cry detect"));
    }

    return TD_SUCCESS;
}

static int init_crydet_ivs(int enable)
{
    ot_bcd_config bcdcfg = {0};
    td_s32 ret = TD_SUCCESS;
    int dividend = 0;

    fet_ai_attr(&g_run_cry.attr_audin);

    switch (g_run_cry.attr_audin.bit_width) {
    case OT_AUDIO_BIT_WIDTH_8: {
        dividend = 1;
        break;
    }
    case OT_AUDIO_BIT_WIDTH_24: {
        dividend = 3;
        break;
    }
    case OT_AUDIO_BIT_WIDTH_16:
    default: {
        dividend = 2;
        break;
    }
    }

    g_run_cry.bytes_once_pushed = g_run_cry.attr_audin.sample_rate * dividend / 100;

    bcdcfg.usr_mode = TD_TRUE;
    bcdcfg.bypass = TD_FALSE;
    bcdcfg.alarm_sensitivity = g_run_cry.confidence_det;
    bcdcfg.alarm_period = 1000;
    bcdcfg.alarm_count_threshold = 1;
    bcdcfg.alarm_interval = 0;
    bcdcfg.detect_mode = OT_BCD_MODE_FAST;
    bcdcfg.callback = cb_handle_detected_cry;

    ret = ss_baby_crying_detection_init(&g_run_cry.p_iface,
                                        g_run_cry.attr_audin.sample_rate, &bcdcfg);
    goto_exit_if_fail(TD_SUCCESS == ret && NULL != g_run_cry.p_iface, exit,
                      ret = FAILURE, "failed to init baby crying detection\n");

    COLOR_G("bytes_once_pushed: %d, crying %s, sensitivity: %d\n",
            g_run_cry.bytes_once_pushed, enable ? "enable" : "disable",
            bcdcfg.alarm_sensitivity);

    ret = SUCCESS;

exit:

    return ret;
}

static int uninit_crydet_ivs(void)
{
    td_s32 ret = SUCCESS;

    if (NULL != g_run_cry.p_iface) {
        td_s32 ret = ss_baby_crying_detection_deinit(g_run_cry.p_iface);
        goto_exit_if_fail(TD_SUCCESS == ret, exit, ret = FAILURE,
                          "failed to deinit baby crying detection\n");

        g_run_cry.p_iface = NULL;
    }

    ret = SUCCESS;

exit:

    return ret;
}

int encode_crydet_start(void)
{
    static struct cmdstat cmdstat_cry = {0};
    int ret = SUCCESS;

    DBG("%s start\n", __func__);

    cmdstat_cry.diff_cfg2cmd = diff_cfg2cmd;
    g_run_cry.ctx = &cmdstat_cry;

    conf_get_crydetectioncfg(&g_cfg_cry.cry);

    ret = cb_sync_crycfg(NULL);
    goto_if_fatal_err(SUCCESS == ret, exit, ret = FAILURE,
                      "failed to sync cry config\n");

    g_run_cry.shmbuf_aud = get_shm_buf_pool(SHM_BUF_AUDIO);
    goto_if_fatal_err(NULL != g_run_cry.shmbuf_aud, exit, ret = FAILURE,
                      "failed to get shmbuf audio\n");

    if (NULL == g_run_cry.sch) {
        g_run_cry.sch = js_create_scheduler("sch_cry");
        goto_if_fatal_err(NULL != g_run_cry.sch, exit, ret = FAILURE,
                          "failed to create scheduler crydet\n");
    }

    if (NULL == g_run_cry.hdl_det) {
        js_create_timer_r(g_run_cry.sch, MS_INTV_CRY_LOOP, MS_INTV_CRY_LOOP,
                          cb_loop_crydet_detect, &cmdstat_cry, &g_run_cry.hdl_det);
        goto_if_fatal_err(NULL != g_run_cry.hdl_det, exit, ret = FAILURE,
                          "failed to create timer crydet detection\n");

        attach_config(JEvent_CryDetectCfgChg, cb_crydet_cfgchg, &cmdstat_cry);
    }

    ret = SUCCESS;

exit:

    if (SUCCESS != ret) {
        encode_crydet_stop();
    }

    DBG("%s end\n", __func__);

    return ret;
}

int encode_crydet_stop(void)
{
    DBG("%s\n", __func__);

    if (NULL != g_run_cry.hdl_det) {
        detach_config(JEvent_CryDetectCfgChg, cb_crydet_cfgchg, g_run_cry.ctx);

        js_delete_timer_r(&g_run_cry.hdl_det);
    }

    if (NULL != g_run_cry.sch) {
        js_delete_scheduler(g_run_cry.sch);
        g_run_cry.sch = NULL;
    }

    return uninit_crydet_ivs();
}
