/******************************************************************************
    Copyright (C), 2008-2028, JABSCO ELECTRONIC Tech. Co., Ltd

    File Name    : encode_od.c
    Version      : 1.0
    Author       : JABSCO Video Server Software Group
    Created      : 2017-04-13 by guoxg
    Description  :
    History      :
******************************************************************************/

#include "cmdstat.h"
#include "alarmapi.h"
#include "ptz_ctrl.h"
#include "conf_list.h"
#include "encode_od.h"
#include "ss_mpi_vpss.h"
#include "encode_video.h"
#include "encode_common.h"
#include "encode_Pre_od.h"
#include "ss_mpi_sys_mem.h"
#include "encode_base_ivx.h"
#include "encode_videomask.h"

#define MS_INTV_VMASK_ALARM     (200)
#define MS_VMASK_FREEZE         (4000)

typedef struct {
    PreODParamsOut_t param_OutPut;
    JSScheduler sch_vmask;
    JSTCHandle  hdl_loop;
    int         cnt_freeze;
    struct cmdstat *p_ctx;
} sOdRun;

typedef struct {
    VMaskAlarmS vmainfo;
    int vmatime;
} sOdCfg;

static sOdRun g_od_run = {0};
static sOdCfg g_od_raw = {0};
static sOdCfg g_od_cfg = {0};

static void cb_vmask_alarmcfg(int id, void *p_src, int size, void *ctx)
{
    CPY2CMDCFG(CMD_IVX_VMA_INFO, &g_od_raw.vmainfo, p_src, size);
}

void encode_od_sync_vmaskalmcfg(void)
{
    DBG("od enable: %d, thresh: %d\n",
        g_od_cfg.vmainfo.enable, g_od_cfg.vmainfo.thresh);

    encode_Pre_OD_init(&g_od_cfg.vmainfo);
}

int encode_od_process(void)
{
    static myIMPFrameInfo_t od_frame = {0};
    static struct timespec time_pre = {0};

    ot_video_frame_info frame = {0};
    ot_video_frame *p_frm = &frame.video_frame;
    int ret = S_OK, got_frame = FALSE;
    int vpss_grp = 0;
    td_s32 vpss_chn = OT_VPSS_CHN2;
    td_void *vir_addr = NULL;

    ret = ss_mpi_vpss_get_chn_frame(vpss_grp, vpss_chn, &frame, GET_STREAM_TIMEOUT);
    goto_exit_if_fail(TD_SUCCESS == ret, exit, ret = FAILURE,
                      "failed to get chn frame\n");

    got_frame = TRUE;

    vir_addr = ss_mpi_sys_mmap(p_frm->phys_addr[0],
                               p_frm->width * p_frm->height);
    goto_exit_if_fail(NULL != vir_addr, exit, ret = FAILURE, "failed to mmap frame\n");

    p_frm->virt_addr[0] = vir_addr;

    od_frame.Frame = &frame;
    ret = encode_Pre_OD_Run(&od_frame, &g_od_run.param_OutPut);

    if (TRUE == ret) {
        if (E_TIME_INTERVAL_OK == encode_time_interval(&time_pre, 3000)) {
            ENCODE_RET_JUDGE(alarm_report(JALARM_TYPE_MASK, 0, NEED_TIME_CHECK,
                             "mask alarm"));
        }

        encode_Pre_OD_Print_Message();
    }

    memcpy(&od_frame.copygYBuf[0],
           (void *)od_frame.Frame->video_frame.virt_addr[0], RAW_W * RAW_H);

exit:

    if (NULL != vir_addr){
        ss_mpi_sys_munmap(vir_addr, p_frm->width * p_frm->height);
        vir_addr = NULL;
    }

    if (got_frame){
        got_frame = FALSE;
        ret = ss_mpi_vpss_release_chn_frame(vpss_grp, vpss_chn, &frame);
        ENCODE_RET_CHECK(ret, "ss_mpi_vpss_release_chn_frame failed 0x%x\n",ret);
    }

    return ret;
}

static void diff_cfg2cmd(void *ctx)
{
    struct cmdstat *p_cmd = (struct cmdstat *)ctx;

    if (p_cmd->cmd_stage) {
        if (p_cmd->cmd_stage & CMD_IVX_VMA_INFO) {
            DBG("----- video mask alarm -----\n");
            memcpy(&g_od_cfg.vmainfo, &g_od_raw.vmainfo, sizeof(g_od_cfg.vmainfo));
        }
    }
}

static void loop_base_od(void *ctx)
{
    static int vma_time_prev = FALSE;

    int cmd = cmd_get_command((struct cmdstat *)ctx);

    if (cmd) {
        if (cmd & CMD_IVX_VMA_INFO) {
            encode_od_sync_vmaskalmcfg();
        }
    }

    if (g_od_cfg.vmainfo.enable) {
        g_od_cfg.vmatime = TimeJudge(g_od_cfg.vmainfo.times);
    } else {
        g_od_cfg.vmatime = FALSE;
    }

    if (videomask_enabled()) {
        g_od_cfg.vmatime = FALSE;
    }

    if (vma_time_prev != g_od_cfg.vmatime) {
        DBG("vma time: %d\n", g_od_cfg.vmatime);
        vma_time_prev = g_od_cfg.vmatime;
    }

    if (g_od_run.cnt_freeze > 0) {
        pri_od(LVL_DBG, "od is freezing, left %d\n", g_od_run.cnt_freeze);
        g_od_run.cnt_freeze--;
        goto exit;
    }

    if (g_od_cfg.vmatime) {
        if (ptz_is_run()) {
            pri_od(LVL_DBG, "od freezing cause motor is moving\n");
            encode_Pre_OD_Clear();
        } else {
            encode_od_process();
        }
    }

exit:

    return;
}

int encode_od_init(void)
{
    static struct cmdstat cmdstat_od;
    int ret = 0;

    cmdstat_od.diff_cfg2cmd = diff_cfg2cmd;
    g_od_run.p_ctx = &cmdstat_od;

    g_od_run.sch_vmask = js_create_scheduler("sch_vmask_alarm");
    goto_if_fatal_err(NULL != g_od_run.sch_vmask, exit, ret = FAILURE,
                      "failed to create sch_vmask\n");

    get_config(handleVMaskAlarmCfg, g_od_cfg.vmainfo);

    attach_config(JEvent_VMaskAlarmCfg, cb_vmask_alarmcfg, &cmdstat_od);

    encode_od_sync_vmaskalmcfg();

    js_create_timer_r(g_od_run.sch_vmask, 1000, MS_INTV_VMASK_ALARM, loop_base_od,
                      &cmdstat_od, &g_od_run.hdl_loop);
    goto_if_fatal_err(NULL != g_od_run.hdl_loop, exit, ret = FAILURE,
                      "failed to create sch vmask timer\n");

exit:

    return ret;
}

void encode_od_uninit(void)
{
    detach_config(JEvent_VMaskAlarmCfg, cb_vmask_alarmcfg, g_od_run.p_ctx);

    if (NULL != g_od_run.sch_vmask) {
        js_delete_scheduler(g_od_run.sch_vmask);
        g_od_run.sch_vmask = NULL;
    }
}

static void cb_od_freeze(void *usr_data)
{
    g_od_run.cnt_freeze = (MS_VMASK_FREEZE / MS_INTV_VMASK_ALARM);
    encode_Pre_OD_Clear();
}

void encode_od_freeze(void)
{
    if (NULL != g_od_run.sch_vmask) {
        js_run_function(g_od_run.sch_vmask, cb_od_freeze, NULL, 1);
    } else {
        cb_od_freeze(NULL);
    }
}
