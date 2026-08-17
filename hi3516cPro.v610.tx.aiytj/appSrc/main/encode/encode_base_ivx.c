#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "jconfstruct.h"
#include "debug.h"
#include "js_scheduler.h"
#include "conf_list.h"
#include "jconfig.h"
#include "encode_base_ivx.h"
#include "cmdstat.h"
#include "encode_ive_motion.h"
#include "encode_common.h"
#include "encode_osd.h"
#include "encode_region.h"
#include "encode_ivp_aidetect.h"
#include "encode_od.h"
#include "encode_videomask.h"
#include "ot_ivs_md.h"
#include "lamp_smart_photo_sens.h"
#include "encode_noise.h"

#define INTV_MS_GET_DAYNGT        (1000)
#define INTV_MS_MDDETECT          (480)
#define INTV_MS_BASE_IVX          (80)

#define DETECTION_GRID_COLUMN     (22)
#define DETECTION_GRID_ROW        (18)

static struct ivx_cfg cfg = {{0}};
static struct ivx_cfg raw = {{0}};
static struct ivx_run run = {0};
struct ivx_cfg *g_ivx_cfg = &cfg;
struct ivx_cfg *g_ivx_raw = &raw;
struct ivx_run *g_ivx_run = &run;

static int encode_ivx_osd_handle(void);

int encode_get_follow_enable(void)
{
    return g_ivx_cfg->followinfo.enable;
}

static void cb_motion_detect_cfg(int id, void *p_src, int size, void *ctx)
{
    CPY2CMDCFG(CMD_IVX_MD_INFO, &g_ivx_raw->mdinfo, p_src, size);
}

static void cb_facial_convergence_cfg(int id, void *p_src, int size, void *ctx)
{
    CPY2CMDCFG(CMD_IVX_FACE_CONV, &g_ivx_raw->faceinfo, p_src, size);
}

static void cb_human_detect_cfg(int id, void *p_src, int size, void *ctx)
{
    CPY2CMDCFG(CMD_IVX_HD_INFO, &g_ivx_raw->hdinfo, p_src, size);
}

static void cb_car_detect_cfg(int id, void *p_src, int size, void *ctx)
{
    CPY2CMDCFG(CMD_IVX_CAR_INFO, &g_ivx_raw->carinfo, p_src, size);
}

static void cb_pet_detect_cfg(int id, void *p_src, int size, void *ctx)
{
    CPY2CMDCFG(CMD_IVX_PET_INFO, &g_ivx_raw->petinfo, p_src, size);
}

static void cb_follow_detect_cfg(int id, void *p_src, int size, void *ctx)
{
    CPY2CMDCFG(CMD_IVX_FW_INFO, &g_ivx_raw->followinfo, p_src, size);
}

static void cb_vgline_cfg(int id, void *p_src, int size, void *ctx)
{
    CPY2CMDCFG(CMD_IVX_LINE_INFO, &g_ivx_raw->lineinfo, p_src, size);
}

static void cb_vgrect_cfg(int id, void *p_src, int size, void *ctx)
{
    CPY2CMDCFG(CMD_IVX_RECT_INFO, &g_ivx_raw->rectinfo, p_src, size);
}

static void cb_video_maskcfg(int id, void *p_src, int size, void *ctx)
{
    CPY2CMDCFG(CMD_IVX_VD_INFO, &g_ivx_raw->vminfo, p_src, size);
}

static int xslt_mb2area(char *mbdesc, hd_area_pos *area)
{
    if (g_ivx_cfg->lineinfo.enable || g_ivx_cfg->rectinfo.enable) {
        area->start_x_pos = 0;
        area->start_y_pos = 0;
        area->end_x_pos   = RAW_W;
        area->end_y_pos   = RAW_H;
        return 0;
    }

    int w = 0, h = 0;
    int mbdesc_x1 = 100, mbdesc_y1 = 100, mbdesc_x2 = -1, mbdesc_y2 = -1;

    for (h = 0; h < DETECTION_GRID_ROW; h++) {
        for (w = 0; w < DETECTION_GRID_COLUMN; w++) {
            if ('1' == mbdesc[h * (DETECTION_GRID_COLUMN + 1) + w]) {

                if (mbdesc_x1 > w) mbdesc_x1 = w;
                if (mbdesc_x2 < w) mbdesc_x2 = w;
                if (mbdesc_y1 > h) mbdesc_y1 = h;
                if (mbdesc_y2 < h) mbdesc_y2 = h;
            }
        }
    }

    if (100 == mbdesc_x1)
        mbdesc_x1 = 0;
    if (100 == mbdesc_y1)
        mbdesc_y1 = 0;

    area->start_x_pos = mbdesc_x1 * RAW_W / DETECTION_GRID_COLUMN;
    area->start_y_pos = mbdesc_y1 * RAW_H / DETECTION_GRID_ROW;
    area->end_x_pos   = (mbdesc_x2 + 1) * RAW_W / DETECTION_GRID_COLUMN;
    area->end_y_pos   = (mbdesc_y2 + 1) * RAW_H / DETECTION_GRID_ROW;

    DBG("start_x = %d start_y = %d end_x = %d end_y = %d\n",
        area->start_x_pos,
        area->start_y_pos,
        area->end_x_pos,
        area->end_y_pos);

    return 0;
}

static void encode_base_uninit_aidetect(void)
{
    //uninit osd
    encode_ivx_osd_handle();

    encode_ivp_aidetect_uninit();
    g_ivx_run->aidetect_init = 0;
}

static void encode_base_init_aidetect(void)
{
    //init osd
    encode_ivx_osd_handle();

    encode_ivp_aidetect_init(g_ivx_cfg, g_ivx_run);
    g_ivx_run->aidetect_init = 1;
}

static int encode_base_ive_md_handle(void)
{
    if (1 == g_ivx_run->md_init && 0 == g_ivx_cfg->mdinfo.enable) {
        DBG("----- motion uninit -----\n");
        encode_ive_md_uninit();
        g_ivx_run->md_init = 0;
    } else if(0 == g_ivx_run->md_init && 1 == g_ivx_cfg->mdinfo.enable) {
        if (1 == g_ivx_run->aidetect_init) {
            encode_base_uninit_aidetect();
        }

        DBG("----- motion init -----\n");
        encode_ive_md_init(&g_ivx_cfg->mdinfo);
        g_ivx_run->md_init = 1;
    } else {
        DBG("----- motion set param-----\n");
        encode_ive_md_set_param(&g_ivx_cfg->mdinfo);
    }

    return 0;
}

static int encode_ivx_osd_handle(void)
{
    if (g_ivx_cfg->hdinfo.enable
        || g_ivx_cfg->carinfo.enable
        || g_ivx_cfg->followinfo.enable
        || g_ivx_cfg->petinfo.enable) {
        encode_osd_aidet_init();
    } else {
        encode_osd_aidet_uninit();
    }

    if (!g_ivx_cfg->lineinfo.enable) {
        encode_osd_vgline_uninit();
    }

    if (!g_ivx_cfg->rectinfo.enable) {
        encode_osd_vgrect_uninit();
    }

    if (g_ivx_cfg->lineinfo.enable) {
        encode_osd_vgline_uninit();
        encode_osd_vgline_init();
    }

    if (g_ivx_cfg->rectinfo.enable) {
        encode_osd_vgrect_uninit();
        encode_osd_vgrect_init();
    }

    return 0;
}

static int encode_base_ivp_aidetect_handle(void)
{
    if(1 == g_ivx_run->aidetect_init
       && 0 == g_ivx_cfg->hdinfo.enable
       && 0 == g_ivx_cfg->carinfo.enable
       && 0 == g_ivx_cfg->followinfo.enable
       && 0 == g_ivx_cfg->lineinfo.enable
       && 0 == g_ivx_cfg->rectinfo.enable
       && 0 == g_ivx_cfg->petinfo.enable) {
        DBG("----- aidetect uninit -----\n");
        encode_base_uninit_aidetect();
    } else if(0 == g_ivx_run->aidetect_init &&
       (   1 == g_ivx_cfg->hdinfo.enable
        || 1 == g_ivx_cfg->carinfo.enable
        || 1 == g_ivx_cfg->followinfo.enable
        || 1 == g_ivx_cfg->lineinfo.enable
        || 1 == g_ivx_cfg->rectinfo.enable
        || 1 == g_ivx_cfg->petinfo.enable)){
        DBG("----- aidetect init -----\n");
        xslt_mb2area(g_ivx_cfg->hdinfo.mbdesc, &g_ivx_cfg->hdarea);
        xslt_mb2area(g_ivx_cfg->carinfo.mbdesc, &g_ivx_cfg->cararea);
        xslt_mb2area(g_ivx_cfg->petinfo.mbdesc, &g_ivx_cfg->petarea);

        if (g_ivx_run->md_init) {
            encode_ive_md_uninit();
            g_ivx_run->md_init = 0;
        }

        encode_base_init_aidetect();
    } else if(1 == g_ivx_run->aidetect_init) {
        DBG("----- aidetect set param-----\n");
        xslt_mb2area(g_ivx_cfg->hdinfo.mbdesc, &g_ivx_cfg->hdarea);
        xslt_mb2area(g_ivx_cfg->carinfo.mbdesc, &g_ivx_cfg->cararea);
        xslt_mb2area(g_ivx_cfg->petinfo.mbdesc, &g_ivx_cfg->petarea);
        encode_ivx_osd_handle();
        encode_ivp_aidetect_set_param(g_ivx_cfg, g_ivx_run);
    }

    return 0;
}

static void encode_base_sync_vmaskcfg(void)
{
    osd_aidet_t aidet_rect = {0};
    VideoMask0 *p_memb = &g_ivx_cfg->vminfo.mask[ID_VIDEO_MASK];

    if (p_memb->enable) {
        g_ivx_cfg->mask.enable = TRUE;
        g_ivx_cfg->mask.color = get_rgb_color(p_memb->color);
        g_ivx_cfg->mask.x = p_memb->x0;
        g_ivx_cfg->mask.y = p_memb->y0;
        g_ivx_cfg->mask.width = p_memb->x1 - p_memb->x0;
        g_ivx_cfg->mask.height = p_memb->y1 - p_memb->y0;
        g_ivx_cfg->vmtime = TRUE;

        memcpy(&aidet_rect.mask, &g_ivx_cfg->mask, sizeof(aidet_rect.mask));

        DBG("enable video mask, x: %d, y: %d, width: %d, height: %d\n",
            g_ivx_cfg->mask.x, g_ivx_cfg->mask.y, g_ivx_cfg->mask.width,
            g_ivx_cfg->mask.height);
    } else {
        g_ivx_cfg->mask.enable = FALSE;
        g_ivx_cfg->vmtime = FALSE;

        DBG("disable video mask\n");
    }

    encode_osd_aidet_draw(&aidet_rect);
}

static void loop_base_ivx(void *ctx)
{
    static int tick_md = 0;
    static int tick_dayngt = 0;

    //初始化动作放到loop，防止阻塞主线程，加快出图速度
    if(!g_ivx_run->ivx_init) {
        if (g_ivx_cfg->mdinfo.enable) {
            if (g_ivx_run->aidetect_init) {
                encode_base_uninit_aidetect();
            }

            encode_ive_md_init(&g_ivx_cfg->mdinfo);
            g_ivx_run->md_init = 1;
        } else if (g_ivx_cfg->hdinfo.enable
               || g_ivx_cfg->carinfo.enable
               || g_ivx_cfg->followinfo.enable
               || g_ivx_cfg->lineinfo.enable
               || g_ivx_cfg->rectinfo.enable
               || g_ivx_cfg->petinfo.enable){
            xslt_mb2area(g_ivx_cfg->hdinfo.mbdesc, &g_ivx_cfg->hdarea);
            xslt_mb2area(g_ivx_cfg->carinfo.mbdesc, &g_ivx_cfg->cararea);
            xslt_mb2area(g_ivx_cfg->petinfo.mbdesc, &g_ivx_cfg->petarea);
            if (g_ivx_run->md_init) {
                encode_ive_md_uninit();
                g_ivx_run->md_init = 0;
            }

            encode_base_init_aidetect();
        }
        g_ivx_run->ivx_init = 1;
    }

    int cmd = cmd_get_command((struct cmdstat *)ctx);
    if (cmd) {
        if (cmd & CMD_IVX_MD_INFO) {
            encode_base_ive_md_handle();
        }

        if ((cmd & CMD_IVX_HD_INFO)   ||
            (cmd & CMD_IVX_CAR_INFO)  ||
            (cmd & CMD_IVX_PET_INFO)  ||
            (cmd & CMD_IVX_FW_INFO)   ||
            (cmd & CMD_IVX_LINE_INFO) ||
            (cmd & CMD_IVX_RECT_INFO)){
            encode_base_ivp_aidetect_handle();
        }

        if (cmd & CMD_IVX_VD_INFO) {
            encode_base_sync_vmaskcfg();
        }

        if (cmd & CMD_IVX_RECT_BLINK) {
            encode_osd_vgrect_blink(0);
        }

        if (cmd & CMD_IVX_LINE_BLINK) {
            encode_osd_vgline_blink(0);
        }

    }

    if (tick_dayngt++ >= (INTV_MS_GET_DAYNGT / INTV_MS_BASE_IVX)) {
        g_ivx_cfg->is_day = is_photosens_day();
        pri_ivx(LVL_LOOP, "ps is_day: %d\n", g_ivx_cfg->is_day);
        tick_dayngt = 0;
    }

    if (1 == g_ivx_run->md_init) {
        if (tick_md++ >= (INTV_MS_MDDETECT / INTV_MS_BASE_IVX)) {
            encode_ive_md_process(&g_ivx_cfg->mdinfo);    //移动侦测
            tick_md = 0;
        }
    }

    if (!g_ivx_cfg->vmtime && 1 == g_ivx_run->aidetect_init) {
        encode_ivp_aidetect_process(g_ivx_cfg);       //AIDETECT框架，人形，车形等
    }
}

static void diff_cfg2cmd(void *ctx)
{
    struct cmdstat *p_cmd = (struct cmdstat *)ctx;

    if (p_cmd->cmd_stage) {
        if (p_cmd->cmd_stage & CMD_IVX_MD_INFO) {
            DBG("----- motion -----\n");
            memcpy(&g_ivx_cfg->mdinfo, &g_ivx_raw->mdinfo, sizeof(g_ivx_cfg->mdinfo));
        }
        if (p_cmd->cmd_stage & CMD_IVX_HD_INFO) {
            DBG("----- human -----\n");
            memcpy(&g_ivx_cfg->hdinfo, &g_ivx_raw->hdinfo, sizeof(g_ivx_cfg->hdinfo));
        }
        if (p_cmd->cmd_stage & CMD_IVX_CAR_INFO) {
            DBG("----- car -----\n");
            memcpy(&g_ivx_cfg->carinfo, &g_ivx_raw->carinfo, sizeof(g_ivx_cfg->carinfo));
        }
        if (p_cmd->cmd_stage & CMD_IVX_PET_INFO) {
            DBG("----- pet -----\n");
            memcpy(&g_ivx_cfg->petinfo, &g_ivx_raw->petinfo, sizeof(g_ivx_cfg->petinfo));
        }
        if (p_cmd->cmd_stage & CMD_IVX_FW_INFO) {
            DBG("----- follow -----\n");
            memcpy(&g_ivx_cfg->followinfo, &g_ivx_raw->followinfo, sizeof(g_ivx_cfg->followinfo));
        }
        if (p_cmd->cmd_stage & CMD_IVX_FACE_CONV) {
            DBG("----- facial_convergence -----\n");
            memcpy(&g_ivx_cfg->faceinfo, &g_ivx_raw->faceinfo, sizeof(g_ivx_cfg->faceinfo));
        }
        if (p_cmd->cmd_stage & CMD_IVX_LINE_INFO) {
            DBG("----- line -----\n");
            memcpy(&g_ivx_cfg->lineinfo, &g_ivx_raw->lineinfo, sizeof(g_ivx_cfg->lineinfo));
        }
        if (p_cmd->cmd_stage & CMD_IVX_RECT_INFO) {
            if (g_ivx_cfg->rectinfo.blink != g_ivx_raw->rectinfo.blink) {
                cmd_set_command(p_cmd, CMD_IVX_RECT_BLINK);
            }
            DBG("----- rect -----\n");
            memcpy(&g_ivx_cfg->rectinfo, &g_ivx_raw->rectinfo, sizeof(g_ivx_cfg->rectinfo));
        }

        if (p_cmd->cmd_stage & CMD_IVX_VD_INFO) {
            DBG("----- video mask -----\n");
            memcpy(&g_ivx_cfg->vminfo, &g_ivx_raw->vminfo, sizeof(g_ivx_cfg->vminfo));
        }
    }
}

int encode_ivx_init()
{
    int ret = SUCCESS;
    static struct cmdstat cmdstat_ive;
    struct cmdstat *ctx = &cmdstat_ive;
    cmdstat_ive.diff_cfg2cmd = diff_cfg2cmd;

    g_ivx_run->sch = js_create_scheduler("sch_multiobj");
    goto_if_fatal_err(NULL != g_ivx_run->sch, exit, ret = FAILURE,
                      "failed to create sch_multiobj\n");

    g_ivx_run->sch_pet = js_create_scheduler("sch_pet");
    goto_if_fatal_err(NULL != g_ivx_run->sch_pet, exit, ret = FAILURE,
                      "failed to create sch_pet\n");

    g_ivx_run->sync_mng = sync_manager_create(CNT_IVS_TASKS, 0);
    goto_if_fatal_err(NULL != g_ivx_run->sync_mng, exit, ret = FAILURE,
                      "failed to create sync manager\n");

    for (size_t idx = 0; idx < CNT_IVS_PRODUCERS; idx++) {
        sync_manager_register_producer(g_ivx_run->sync_mng);
    }

    get_config(handleHumanDetectCfg     , g_ivx_cfg->hdinfo);
    get_config(handleCarDetectCfg       , g_ivx_cfg->carinfo);
    get_config(handlePetDetectCfg       , g_ivx_cfg->petinfo);
    get_config(handleFollowCfg          , g_ivx_cfg->followinfo);
    get_config(handleConvergenceCfg     , g_ivx_cfg->faceinfo);
    get_config(handleVglineCfg          , g_ivx_cfg->lineinfo);
    get_config(handleVgrectCfg          , g_ivx_cfg->rectinfo);
    get_config(handleVideoMaskCfg       , g_ivx_cfg->vminfo);
	get_config(handleMotionDetectCfg    , g_ivx_cfg->mdinfo);

    g_ivx_run->p_ctx = ctx;
    attach_config(JEvent_MotionDetectCfgChg , cb_motion_detect_cfg      , (void *)ctx);
    attach_config(JEvent_HumanDetectCfgChg  , cb_human_detect_cfg       , (void *)ctx);
    attach_config(JEvent_CarDetectCfgChg    , cb_car_detect_cfg         , (void *)ctx);
    attach_config(JEvent_PetDetectCfgChg    , cb_pet_detect_cfg         , (void *)ctx);
    attach_config(JEvent_Followcfg          , cb_follow_detect_cfg      , (void *)ctx);
    attach_config(JEvent_ConvergenceChg     , cb_facial_convergence_cfg , (void *)ctx);
    attach_config(JEvent_VglineCfgChg       , cb_vgline_cfg             , (void *)ctx);
    attach_config(JEvent_VgrectCfgChg       , cb_vgrect_cfg             , (void *)ctx);
    attach_config(JEvent_VideoMaskCfgChg    , cb_video_maskcfg          , (void *)ctx);

    init_encode_ivp_aidetect_cfg();

    encode_base_sync_vmaskcfg();

    encode_od_init();

    js_create_timer_r(g_ivx_run->sch, 1000, INTV_MS_BASE_IVX, loop_base_ivx,
                      ctx, &g_ivx_run->hdl_loop);
    goto_if_fatal_err(NULL != g_ivx_run->hdl_loop, exit, ret = FAILURE,
                      "failed to create sch multiobj timer\n");

exit:

    return ret;
}

int encode_ivx_uninit()
{
    int ret = 0;
    DBG("encode_ivx_uninit begin\n");
 
    detach_config(JEvent_MotionDetectCfgChg , cb_motion_detect_cfg      , g_ivx_run->p_ctx);
    detach_config(JEvent_HumanDetectCfgChg  , cb_human_detect_cfg       , g_ivx_run->p_ctx);
    detach_config(JEvent_CarDetectCfgChg    , cb_car_detect_cfg         , g_ivx_run->p_ctx);
    detach_config(JEvent_PetDetectCfgChg    , cb_pet_detect_cfg         , g_ivx_run->p_ctx);
    detach_config(JEvent_Followcfg          , cb_follow_detect_cfg      , g_ivx_run->p_ctx);
    detach_config(JEvent_ConvergenceChg     , cb_facial_convergence_cfg , g_ivx_run->p_ctx);
    detach_config(JEvent_VglineCfgChg       , cb_vgline_cfg             , g_ivx_run->p_ctx);
    detach_config(JEvent_VgrectCfgChg       , cb_vgrect_cfg             , g_ivx_run->p_ctx);
    detach_config(JEvent_VideoMaskCfgChg    , cb_video_maskcfg          , g_ivx_run->p_ctx);

    do {
        encode_od_uninit();

        if (NULL != g_ivx_run->hdl_loop) {
            js_delete_timer_r(&g_ivx_run->hdl_loop);
        }

        if (NULL != g_ivx_run->sch) {
            js_delete_scheduler(g_ivx_run->sch);
            g_ivx_run->sch = NULL;
        }

        if (g_ivx_run->md_init) {
            DBG("--- motion uninit\n");
            encode_ive_md_uninit();
            g_ivx_run->md_init = 0;
        }

        if (NULL != g_ivx_run->sch_pet) {
            js_delete_scheduler(g_ivx_run->sch_pet);
            g_ivx_run->sch_pet = NULL;
        }

        for (size_t idx = 0; idx < CNT_IVS_PRODUCERS; idx++) {
            sync_manager_unregister_producer(g_ivx_run->sync_mng);
        }

        if (NULL != g_ivx_run->sync_mng) {
            sync_manager_destroy(g_ivx_run->sync_mng);
            g_ivx_run->sync_mng = NULL;
        }

        if (g_ivx_run->aidetect_init) {
            DBG("--- aidetect uninit\n");
            encode_base_uninit_aidetect();
        }

        g_ivx_run->ivx_init = 0;
        DBG("encode_ivx_uninit end\n");
    } while(0);

    DropCache(__func__);
    UtilSystemCmd("free");

    return ret;
}
