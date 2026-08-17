#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "securec.h"

#include "ot_common_aidetect.h"
#include "ot_common_region.h"
#include "ot_common_isp.h"
#include "ot_common_smartae.h"
#include "ss_mpi_vpss.h"
#include "ss_mpi_sys.h"
#include "ss_mpi_sys_mem.h"
#include "ss_mpi_aidetect.h"
#include "ss_mpi_smartae.h"
#include "ss_mpi_ae.h"
#include "ss_mpi_isp.h"
#include "scene_setparam_inner.h"
#include "ss_mpi_venc.h"

#include "g_stat.h"
#include "g_log.h"
#include "debug.h"
#include "utils.h"
#include "jconfstruct.h"
#include "jconfig.h"
#include "conf_list.h"
#include "alarmapi.h"
#include "fifo_queue.h"
#include "encode_typedef.h"
#include "encode_common.h"
#include "encode_ivp_aidetect.h"
#include "encode_osd.h"
#include "encode_isp.h"
#include "encode_region.h"
#include "encode_video.h"
#include "encode_vg.h"
#include "confapi.h"
#include "ptz_ctrl.h"
#include "ptz_follow.h"
#include "lamp_main.h"
#include "encode_main.h"
#include "record_alarm_param.h"
#include "tencent_cloud_storage.h"
#include "encode_noise.h"

#define NO_DETECT_MAX_FRAME (50)
#define NO_DETECT_FRAME (5)
#define AIDETECT_MAX_CLASS_NUM (8)
#define AIDETECT_MAX_OBJECT_CNT (20)
#define EVEN_PIXEL (2)
#define AIDETECT_MAX_RECT_CNT (30)

#define IVP_AIDETECT_MODEL_PATH "/algo/det_hvf_hor_ll_lite.bin"

#define IVP_AIDET_SKIP_AREA 1
#define IVP_AIDET_NO_SKIP_AREA 0
#define DET_ENABLE           (1)
#define DET_DISABLE          (0)

typedef struct {
    JSScheduler *sch;
    int         chn;
    char        path_model[64];
    JSTCFunc    cb_det;
} sIvsInfo;

typedef struct {
    int               *enable;
    int               *screen_enable;
    hd_area_pos       *area;
    int               *thresh;
    int               chn;
    ot_aidetect_class class;
} sObjInfo;

typedef struct {
    ot_video_frame_info *p_frm;
    int                 chn;
} sMultiThdDetInfo;

typedef struct {
    char alm_str[32];
    int alm_type;
} sAlmRepInfo;

typedef struct {
    ot_aidetect_input_model input_model;
    ot_aidetect_chn_attr    chn_attr;
} aidetect_info_t;

typedef struct {
    ot_aidetect_class class_type;
    int screenenable;
    int follow_type;
    td_u32 object_num;
    ot_aidetect_object objects[CNT_RECTS_MAX]; /* RW; object addr,memory must be setted by user */
} follow_object;

typedef struct {
    td_u32 rect_cnt;
    ot_rect corner_rect[AIDETECT_MAX_RECT_CNT];
} aidetect_rect;

typedef struct {
    ot_aidetect_result_array result[E_IVS_CHN_CNT];
    int flag_faceae;
    int flag_faceae_forced;
} aidetect_run_t;

static void cb_detect_async(void *usr_data);

static aidetect_info_t g_aidetect_info[E_IVS_CHN_CNT] = {0};
static sIvsInfo g_run_ivs[E_IVS_CHN_CNT] = {0};
static sObjInfo g_run_obj[E_IDX_OBJ_CNT] = {0};
static aidetect_run_t g_run_aidet = {0};

//要求增删改查和 eIdxObj 的下标保持一致
static const sAlmRepInfo g_alm_info[] = {
    {"face detect",  JALARM_TYPE_FACE},
    {"human detect", JALARM_TYPE_HUMAN_DETECT},
    {"car detect",   JALARM_TYPE_CAR},
    {"pet detect",   JALARM_TYPE_PET},
};

//要求增删改查和 eIdxObj 的下标保持一致
static sObjInfo *fet_obj_info(sObjInfo p_obj[E_IDX_OBJ_CNT])
{
    sObjInfo obj_info[] = {
        {&g_ivx_cfg->facetime, &g_ivx_cfg->hdinfo.screenenable,  &g_ivx_cfg->facearea, &g_ivx_cfg->hdinfo.thresh,  E_IVS_CHN_0, OT_AIDETECT_CLASS_FACE},
        {&g_ivx_cfg->hdtime,   &g_ivx_cfg->hdinfo.screenenable,  &g_ivx_cfg->hdarea,   &g_ivx_cfg->hdinfo.thresh,  E_IVS_CHN_0, OT_AIDETECT_CLASS_HUMAN},
        {&g_ivx_cfg->cartime,  &g_ivx_cfg->carinfo.screenenable, &g_ivx_cfg->cararea,  &g_ivx_cfg->carinfo.thresh, E_IVS_CHN_0, OT_AIDETECT_CLASS_VEHICLE},
        {&g_ivx_cfg->pettime,  &g_ivx_cfg->petinfo.screenenable, &g_ivx_cfg->petarea,  &g_ivx_cfg->petinfo.thresh, E_IVS_CHN_1, OT_AIDETECT_CLASS_PET}
    };

    memcpy(p_obj, obj_info, sizeof(obj_info));

    return p_obj;
}

static sIvsInfo *fet_ivs_info(sIvsInfo p_info[E_IVS_CHN_CNT])
{
    sIvsInfo ivs_info[] = {
        {NULL,                  E_IVS_CHN_0, "/algo/det_hvf_hor_ll_lite.bin", NULL},            //通道 0，多目标检测算法，直接用 ivx base sch
        {&(g_ivx_run->sch_pet), E_IVS_CHN_1, "/algo/det_pet_hor.bin",         cb_detect_async}  //通道 1，宠物检测算法，用新建的 sch_pet
    };

    memcpy(p_info, ivs_info, sizeof(ivs_info));

    return p_info;
}

int get_ae_force()
{
    if(g_run_aidet.flag_faceae_forced) {
        g_run_aidet.flag_faceae_forced = FALSE;
        return TRUE;
    }

    return g_run_aidet.flag_faceae_forced;
}

int get_facial_convergence_status()
{
    return g_run_aidet.flag_faceae;
}

static int encode_aidet_get_object(ot_rect *stRect, hd_area_pos *Alarm_Area)
{
    int Effective = 0;

    td_s32 stRect_x0,stRect_y0,Alarm_x2,Alarm_y2,stRect_x2,stRect_y2;

    Alarm_x2  = Alarm_Area->end_x_pos;
    Alarm_y2  = Alarm_Area->end_y_pos;
    stRect_x2 = stRect->x + stRect->width;
    stRect_y2 = stRect->y + stRect->height;
    stRect_x0 = (stRect->x+ stRect->x + stRect->width)/2;
    stRect_y0 = (stRect->y + stRect->y + stRect->height)/2;

    if(stRect->x >=Alarm_x2 || stRect_x2 <= Alarm_Area->start_x_pos || stRect->y >= Alarm_y2 || stRect_y2 <= Alarm_Area->start_y_pos) {
        Effective = 0;
    } else if(stRect->x >= Alarm_Area->start_x_pos && stRect_x2 <= Alarm_x2 && stRect->y >= Alarm_Area->start_y_pos && stRect_y2<=Alarm_y2) {
        Effective = 1;
    } else {
        if(stRect_x0>=Alarm_Area->start_x_pos && stRect_x0<=Alarm_x2 && stRect_y0>=Alarm_Area->start_y_pos && stRect_y0<=Alarm_y2) {
            Effective = 1;
        } else {
            Effective = 0;
        }
    }

    return Effective;
}

int encode_ivp_aidetect_init_result(ot_aidetect_result_array *result, int ivs_chn)
{
    int ret = 0, idx = 0;
    ot_aidetect_object_of_one_class *p_class = NULL;

    result->class_num = 0;

    //所有要检测的都动态开辟一个内存，方便其它线程将结果同步到本 result
    //class_num 只取本线程负责检测的
    for (idx = 0; idx < ARRAY_SIZE(g_run_obj); idx++) {
        if (g_run_obj[idx].chn == ivs_chn) {
            result->class_num++;
        }

        p_class = &result->object_class[idx];

        if (NULL == result->object_class[idx].objects) {
            p_class->class_type = g_run_obj[idx].class;
            p_class->object_capacity = CNT_RECTS_MAX;

            p_class->objects = (ot_aidetect_object *)calloc(1, sizeof(ot_aidetect_object) * p_class->object_capacity);
            if (NULL == p_class->objects) {
                ERR("failed to calloc %d size objects\n", sizeof(ot_aidetect_object) * p_class->object_capacity);
                ret = FAILURE;
                break;
            }
        } else {
            memset_s(p_class->objects, sizeof(ot_aidetect_object) * p_class->object_capacity, 0,
                     sizeof(ot_aidetect_object) * p_class->object_capacity);
            p_class->object_num = 0;
        }
    }

    return ret;
}

static td_void encode_ivp_aidetect_deinit_result(ot_aidetect_result_array *result)
{
    if (result == TD_NULL) {
        return;
    }

    for (td_u32 i = 0; i < result->class_num; ++i) {
        if (result->object_class[i].objects != TD_NULL) {
            free(result->object_class[i].objects);
            result->object_class[i].objects = TD_NULL;
        }
    }
}

#if 0
static td_s32 encode_ivp_set_svc_param(ot_venc_chn chn)
{
    td_s32 ret = 0;
    ot_venc_svc_param_ex svc_param = {0};

    do {
        ret = ss_mpi_venc_get_svc_param_ex(chn, &svc_param);
        ENCODE_RET_BREAK(ret, "ss_mpi_venc_get_svc_param_ex failed\n");

        svc_param.svc_version = OT_VENC_SVC_V2;
        svc_param.svc_param_v2.max_ref_num = 2;      /* 2: algn */
        svc_param.svc_param_v2.refresh_interval = 2; /* 2: algn */
        for (td_u32 m = 0; m < 16; m++) {            /* 16: algn */
            svc_param.svc_param_v2.qp_delta[m] = 0;
        }
        svc_param.svc_param_v2.qp_delta[0] = 0;
        svc_param.svc_param_v2.qp_delta[1] = 0;
        svc_param.svc_param_v2.qp_delta[15] = 3;  /* 15 3: algn */
        ret = ss_mpi_venc_set_svc_param_ex(chn, &svc_param);
        ENCODE_RET_BREAK(ret, "ss_mpi_venc_get_svc_param_ex failed\n");
     } while(0);

    return ret;
}
#endif

static td_void encode_smart_get_smart_info(ot_aidetect_result_array *result, ot_smartae_roi_info *smart_info)
{
    td_u32 i = 0, j = 0, smart_info_index = 0;

    smart_info->class_num = 0;
    smart_info_index = 0;

    for (i = 0; i < result->class_num; i++) {
        if (result->object_class[i].object_num > 0 && result->object_class[i].class_type == OT_AIDETECT_CLASS_HUMAN) {
            smart_info->obj_class[smart_info_index].type = OT_SMARTAE_OBJ_PEOPLE;
            for (j = 0; j < result->object_class[i].object_num; j++) {
                if(smart_info->obj_class[smart_info_index].rect_num >= OT_SMARTAE_MAX_RECT_NUM) {
                    //DBG("rect_num :%d \r\n",smart_info->obj_class[smart_info_index].rect_num);
                    break;
                }

                if(OT_AIDETECT_TRACK_STATUS_NEW != result->object_class[i].objects[j].track_status && OT_AIDETECT_TRACK_STATUS_UPDATE != result->object_class[i].objects[j].track_status) {
                    //DBG("track_status :%d \r\n",result->object_class[i].objects[j].track_status);
                    continue;
                }

                if(0 == result->object_class[i].objects[j].detect_rect.width || 0 == result->object_class[i].objects[j].detect_rect.height) {
                    //DBG("width :%d height:%d \r\n",result->object_class[i].objects[j].detect_rect.width,result->object_class[i].objects[j].detect_rect.height);
                    continue;
                }

                smart_info->obj_class[smart_info_index].objs[j].score = 1.0;
                smart_info->obj_class[smart_info_index].objs[j].rect.x = ENC_GET2MULTIPLE(result->object_class[i].objects[j].detect_rect.x);
                smart_info->obj_class[smart_info_index].objs[j].rect.y = ENC_GET2MULTIPLE(result->object_class[i].objects[j].detect_rect.y);
                smart_info->obj_class[smart_info_index].objs[j].rect.width  = ENC_GET2MULTIPLE(result->object_class[i].objects[j].detect_rect.width);
                smart_info->obj_class[smart_info_index].objs[j].rect.height = ENC_GET2MULTIPLE(result->object_class[i].objects[j].detect_rect.height);

                if(smart_info->obj_class[smart_info_index].objs[j].rect.x + smart_info->obj_class[smart_info_index].objs[j].rect.width > RAW_W) {
                    smart_info->obj_class[smart_info_index].objs[j].rect.width = RAW_W - smart_info->obj_class[smart_info_index].objs[j].rect.x;
                }

                if(smart_info->obj_class[smart_info_index].objs[j].rect.y + smart_info->obj_class[smart_info_index].objs[j].rect.height > RAW_H) {
                    smart_info->obj_class[smart_info_index].objs[j].rect.height = RAW_H - smart_info->obj_class[smart_info_index].objs[j].rect.y;
                }

                smart_info->obj_class[smart_info_index].rect_num++;
            }
            smart_info->class_num++;
            smart_info_index++;
        }
    }
}


static int encode_ivp_aidet_smart_ae_init(void)
{
    int ret = 0;
    ot_vi_pipe vi_pipe = 0;
    ot_isp_smart_exposure_attr smart_exp_attr = {0};
    do {
        ret = ss_mpi_isp_get_smart_exposure_attr(vi_pipe, &smart_exp_attr);
        ENCODE_RET_BREAK(ret, "ss_mpi_isp_get_smart_exposure_attr failed 0x%x \n",ret);
        smart_exp_attr.enable = TD_TRUE;
        smart_exp_attr.ir_mode = TD_FALSE;
        smart_exp_attr.smart_exp_type = OT_OP_MODE_AUTO;
        smart_exp_attr.luma_target = 10;       //[90,130]
        smart_exp_attr.exp_coef_max = 4096;  //[0x0,0x1000;0x1000]
        smart_exp_attr.exp_coef_min = 256;   //[0x0,0x100;0x100]
        smart_exp_attr.smart_interval = 1;     //[0x1,0xFF;1]
        smart_exp_attr.smart_speed = 60;     //[0x0,0xFF;0x40]
        smart_exp_attr.smart_delay_num = 60; //[0x0,0x400;0xB4]
        ret = ss_mpi_isp_set_smart_exposure_attr(vi_pipe, &smart_exp_attr);
        ENCODE_RET_BREAK(ret, "ss_mpi_isp_set_smart_exposure_attr failed 0x%x \n",ret);
    }while(0);
    return ret;
}

static int encode_ivp_aidet_smart_ae_uninit(void)
{
    int ret = 0;
    ot_vi_pipe vi_pipe = 0;
    ot_isp_smart_exposure_attr smart_exp_attr = {0};
    do {
        ret = ss_mpi_isp_get_smart_exposure_attr(vi_pipe, &smart_exp_attr);
        ENCODE_RET_BREAK(ret, "ss_mpi_isp_get_smart_exposure_attr failed 0x%x \n",ret);
        smart_exp_attr.enable = TD_FALSE;
        ret = ss_mpi_isp_set_smart_exposure_attr(vi_pipe, &smart_exp_attr);
        ENCODE_RET_BREAK(ret, "ss_mpi_isp_set_smart_exposure_attr failed 0x%x \n",ret);
    }while(0);
    return ret;
}

/* 当前档位记忆(people 与 non-people 分开, 防止场景切换误跳) */
static eSmartAeStatus g_cur_status_people = E_SMARTAE_STATUS1;
static eSmartAeStatus g_cur_status_nonp  = E_SMARTAE_STATUS1;

/**
 * 根据 iso + area 计算新档位(双向 10 % 缓冲)
 * iso 越大越暗, 越大需要越亮
 * area 越大越近, 越大需要越暗
 */
static eSmartAeStatus calc_smartae_status(eSmartAeStatus curr_status,
                                         ot_isp_exp_info      exp_info,
                                         facial_convergence_t face_cfg,
                                         int                  area)
{
    const int32_t smart_ae_up = 110; /* 升档阈值 = 原阈值 * 110 % */
    const int32_t smart_ae_dn =  90; /* 降档阈值 = 原阈值 *  90 % */

    /* 档位越高数值越高 */
 //   const int32_t iso1_up = face_cfg.iso1 * smart_ae_up / 100;
 //   const int32_t iso1_dn = face_cfg.iso1 * smart_ae_dn / 100;
 //   const int32_t iso2_up = face_cfg.iso2 * smart_ae_up / 100;
 //   const int32_t iso2_dn = face_cfg.iso2 * smart_ae_dn / 100;
 //   const int32_t iso3_up = face_cfg.iso3 * smart_ae_up / 100;
 //   const int32_t iso3_dn = face_cfg.iso3 * smart_ae_dn / 100;

    /* 档位越高数值越低 */
    const int32_t area1_up = face_cfg.area1 * smart_ae_up / 100;
    const int32_t area1_dn = face_cfg.area1 * smart_ae_dn / 100;
    const int32_t area2_up = face_cfg.area2 * smart_ae_up / 100;
    const int32_t area2_dn = face_cfg.area2 * smart_ae_dn / 100;
    const int32_t area3_up = face_cfg.area3 * smart_ae_up / 100;
    const int32_t area3_dn = face_cfg.area3 * smart_ae_dn / 100;

    eSmartAeStatus new_status = curr_status;

    /* 同时满足 ISO 和面积才动作，避免单一抖动 */
    switch (curr_status) {
    case E_SMARTAE_STATUS4:
        if (area > area1_up) {
            new_status = E_SMARTAE_STATUS1;
        }
        break;
    case E_SMARTAE_STATUS1:
        if (area > area2_up) {
            new_status = E_SMARTAE_STATUS2;
        } else if (area <= area1_dn) {
            new_status = E_SMARTAE_STATUS4;
        }
        break;
    case E_SMARTAE_STATUS2:
        if (area > area3_up) {
            new_status = E_SMARTAE_STATUS3;
        } else if (area <= area2_dn) {
            new_status = E_SMARTAE_STATUS1;
        }
        break;
    case E_SMARTAE_STATUS3:
        if (area <= area3_dn) {
            new_status = E_SMARTAE_STATUS2;
        }
        break;
    default:
        new_status = E_SMARTAE_STATUS4;
        break;
    }

    dbg_face("curr_status: %d, new_status: %d\n", curr_status, new_status);
    return new_status;
}

/**
 * 给 smart_ae 相关属性赋值
 */
static void apply_smartae_status(td_bool                   people_roi_en,
                                eSmartAeStatus             status,
                                ot_isp_exp_info            exp_info,
                                facial_convergence_t       face_cfg,
                                ot_isp_smart_exposure_attr *smart,
                                ot_isp_exposure_attr       *exp_attr)
{
    switch (status) {
    case E_SMARTAE_STATUS1: /* 一档 */
        g_run_aidet.flag_faceae = TRUE;
        /* left leftValue right rightValue : 200, 1600, 2000, 1024 */
        smart->exp_coef_max = scene_interpulate(exp_info.iso,  200, 1600, 2000, 1024);
        /* left leftValue right rightValue : 200, 512, 2000, 400 */
        smart->exp_coef_min = scene_interpulate(exp_info.iso,  200, 512,  2000, 400);
        exp_attr->auto_attr.compensation   = face_cfg.compensation1;
        exp_attr->auto_attr.max_hist_offset = 4;
        if (!people_roi_en) {
            /* left leftValue right rightValue : 200, 1600, 2000, 1024 */
            smart->exp_coef_max = scene_interpulate(exp_info.iso, 200, 1600, 2000, 1024);
            smart->exp_coef_min = 1024; /* MIN expCoef 1024 */
        }

        break;
    case E_SMARTAE_STATUS2: /* 二档 */
        g_run_aidet.flag_faceae = TRUE;
        /* left leftValue right rightValue : 10000, 2048, 90000, 1600 */
        smart->exp_coef_max = scene_interpulate(exp_info.exposure, 10000, 2048, 90000, 1600);
        /* left leftValue right rightValue : 10000, 650, 90000, 512 */
        smart->exp_coef_min = scene_interpulate(exp_info.exposure, 10000, 650,  90000, 512);
        exp_attr->auto_attr.compensation   = face_cfg.compensation2;
        exp_attr->auto_attr.max_hist_offset = 4;
        if (!people_roi_en) {
            /* left leftValue right rightValue : 10000, 2048, 90000, 1600 */
            smart->exp_coef_max = scene_interpulate(exp_info.exposure, 10000, 2048, 90000, 1600);
            smart->exp_coef_min = 1024; /* MIN expCoef 1024 */
        }

        break;

    case E_SMARTAE_STATUS3: /* 三档 */
        g_run_aidet.flag_faceae = TRUE;
        /* left leftValue right rightValue : 10000, 2048, 90000, 1600 */
        smart->exp_coef_max = scene_interpulate(exp_info.exposure, 10000, 2048, 90000, 1600);
        /* left leftValue right rightValue : 10000, 650, 90000, 512 */
        smart->exp_coef_min = scene_interpulate(exp_info.exposure, 10000, 650,  90000, 512);
        exp_attr->auto_attr.compensation   = face_cfg.compensation3;
        exp_attr->auto_attr.max_hist_offset = 4;
        if (!people_roi_en) {
            /* left leftValue right rightValue : 10000, 2048, 90000, 1600 */
            smart->exp_coef_max = scene_interpulate(exp_info.exposure, 10000, 2048, 90000, 1600);
            smart->exp_coef_min = 1024; /* MIN expCoef 1024 */
        }

        break;
    case E_SMARTAE_STATUS4: /* 四档 */
    default:
        g_run_aidet.flag_faceae = FALSE;
        if (people_roi_en) {
            /* left leftValue right rightValue : 2000, 1600, 10000, 2048 */
            smart->exp_coef_max = scene_interpulate(exp_info.exposure, 2000, 1600, 10000, 2048);
            /* left leftValue right rightValue : 2000, 750, 10000, 650 */
            smart->exp_coef_min = scene_interpulate(exp_info.exposure, 2000, 750,  10000, 650);
            exp_attr->auto_attr.compensation   = 0;
        } else {
            /* left leftValue right rightValue : 2000, 2300, 10000, 2048 */
            smart->exp_coef_max = scene_interpulate(exp_info.exposure, 2000, 2300, 10000, 2048);
            smart->exp_coef_min = 1024; /* MIN expCoef 1024 */
        }

        break;
    }

    return;
}

static int encode_ivp_aidet_smart_ae_autoset(struct ivx_cfg* ivx_cfg, int area)
{
    ot_isp_exp_info exp_info = {0};
    ot_isp_smart_info smart_info = {0};
    ot_isp_smart_exposure_attr smart_exposure_attr = {0};
    ot_isp_exposure_attr exposure_attr = {0};
    td_s32 ret = 0;
    int vi_pipe = 0;

    do {
        ret = ss_mpi_isp_get_exposure_attr(vi_pipe, &exposure_attr);
        ENCODE_RET_BREAK(ret, "ss_mpi_isp_get_exposure_attr failed 0x%x \n",ret);
        dbg_hd("get compensation:%d max_hist_offset:%d\r\n",
                exposure_attr.auto_attr.compensation,exposure_attr.auto_attr.max_hist_offset);

        ret = ss_mpi_isp_query_exposure_info(vi_pipe, &exp_info);
        ENCODE_RET_BREAK(ret, "ss_mpi_isp_query_exposure_info failed 0x%x \n",ret);

        ret = ss_mpi_isp_get_smart_info(vi_pipe, &smart_info);
        ENCODE_RET_BREAK(ret, "ss_mpi_isp_get_smart_info failed 0x%x \n",ret);

        ret = ss_mpi_isp_get_smart_exposure_attr(vi_pipe, &smart_exposure_attr);
        ENCODE_RET_BREAK(ret, "ss_mpi_isp_get_smart_exposure_attr failed 0x%x \n",ret);

        dbg_face("people_roi.enable:%d exp_info.iso:%d exp_info.exposure:%d\r\n",
            smart_info.people_roi[0].enable,exp_info.iso,exp_info.exposure);

        td_bool people_roi_en = TD_FALSE;
        facial_convergence_t face_cfg = {0};
        eSmartAeStatus new_status = E_SMARTAE_STATUS4;
        eSmartAeStatus *curr_status = NULL;
        if (smart_info.people_roi[0].enable) {
            curr_status = &g_cur_status_people;
            people_roi_en = TD_TRUE;
        } else {
            curr_status = &g_cur_status_nonp;
            people_roi_en = TD_FALSE;
        }

        if (get_isp_mode() == ISP_COLOR_NIGHT) {
            face_cfg = ivx_cfg->faceinfo.wh;
        } else {
            face_cfg = ivx_cfg->faceinfo.ir;
        }

        new_status = calc_smartae_status(*curr_status, exp_info, face_cfg, area);
        if (new_status != *curr_status) {
            dbg_face("people_roi_en: %d, status %d -> %d\n", people_roi_en, *curr_status, new_status);
            *curr_status = new_status;
        }

        apply_smartae_status(people_roi_en, new_status, exp_info,
                            face_cfg, &smart_exposure_attr, &exposure_attr);

        dbg_hd("set compensation:%d max_hist_offset:%d\r\n",
                exposure_attr.auto_attr.compensation,exposure_attr.auto_attr.max_hist_offset);
        ret = ss_mpi_isp_set_exposure_attr(vi_pipe, &exposure_attr);
        ENCODE_RET_BREAK(ret, "ss_mpi_isp_set_exposure_attr failed 0x%x \n",ret);

        dbg_hd("exp_coef_max:0x%x exp_coef_min:0x%x \r\n",smart_exposure_attr.exp_coef_max,smart_exposure_attr.exp_coef_min);
        ret = ss_mpi_isp_set_smart_exposure_attr(vi_pipe, &smart_exposure_attr);
        ENCODE_RET_BREAK(ret, "ss_mpi_isp_get_smart_exposure_attr failed 0x%x \n",ret);
    }while(0);

    return ret;
}

static int encode_facial_convergence(int enable, struct ivx_cfg* ivx_cfg, osd_aidet_t *aidet_rect)
{
    td_s32 vi_pipe  = 0;
    int ret = 0;
    ot_isp_pub_attr pub_attr = {0};
    static char tick = -1;
    int area_max = 0;
    int width = 0, height = 0;

    // 无需人脸收光
    if(!enable && !g_run_aidet.flag_faceae) {
        return 0;
    }

    if (E_STATUS_DAY == get_day_curr()){
        dbg_hd("Light is OFF, no need to enhance! \n");
        enable = 0;
    }

    ret = ss_mpi_isp_get_pub_attr(vi_pipe, &pub_attr);
    if (TD_SUCCESS != ret){
        ERR("ss_mpi_isp_get_pub_attr failed 0x%x\n",ret);
        return ret;
    }

    if(!enable) {
        if(g_run_aidet.flag_faceae){ //是否处于人脸收光
            if(tick >= 0 && tick++ > pub_attr.frame_rate * 2) { //连续无人帧超过2s强制切回自动ae
                //强制切ae
                tick = -1;
                g_run_aidet.flag_faceae = FALSE;
                g_run_aidet.flag_faceae_forced = TRUE;
            }
        }
    } else {
        tick = 0;
        for(int i = 0; i < aidet_rect->object_num;i++) {
            int area = aidet_rect->aidet_info[i].detect_rect.width * aidet_rect->aidet_info[i].detect_rect.height;
            if(area > area_max) {
                area_max = area;
                width  = aidet_rect->aidet_info[i].detect_rect.width;
                height = aidet_rect->aidet_info[i].detect_rect.height;
            }
        }
        dbg_face("currect human facial convergence width:%d, hetght:%d, area:%d\n", width, height, area_max);
        ret = encode_ivp_aidet_smart_ae_autoset(ivx_cfg, area_max);
        ENCODE_RET_CHECK(ret, "encode_ivp_aidet_smart_ae_autoset failed 0x%x\n",ret);
    }
    return ret;
}

static void cb_detect_async(void *usr_data)
{
    sMultiThdDetInfo *p_info = (sMultiThdDetInfo *)usr_data;
    ot_aidetect_object_of_one_class *p_class = NULL;
    ot_aidetect_result_array *p_result = &g_run_aidet.result[p_info->chn];
    int ret = SUCCESS;
    size_t idx_obj = 0, idx_cls = 0;

    for (idx_obj = 0; idx_obj < ARRAY_SIZE(g_run_obj); idx_obj++) {
        if (g_run_obj[idx_obj].chn != p_info->chn) {
            continue;
        }

        p_class = &p_result->object_class[idx_cls];

        if (NULL == p_result->object_class[idx_cls].objects) {
            p_class->class_type = g_run_obj[idx_obj].class;
            p_class->object_capacity = CNT_RECTS_MAX;

            p_class->objects = (ot_aidetect_object *)calloc(1, sizeof(ot_aidetect_object) * p_class->object_capacity);
            goto_exit_if_fail(NULL != p_class->objects, exit, ret = FAILURE,
                              "failed to calloc class objects\n");

            p_result->class_num++;
        } else {
            memset_s(p_class->objects, sizeof(ot_aidetect_object) * p_class->object_capacity, 0,
                     sizeof(ot_aidetect_object) * p_class->object_capacity);
            p_class->object_num = 0;
        }

        idx_cls++;
    }

    ret = ss_mpi_aidetect_process(p_info->chn, &p_info->p_frm->video_frame, p_result);
    goto_exit_if_fail(SUCCESS == ret, exit, ret = FAILURE, "failed to detect objects\n");

    ot_aidetect_object_of_one_class *p_cls_all = NULL;

    //将检测到的结果拷贝到 E_IVS_CHN_0
    for (idx_obj = 0; idx_obj < ARRAY_SIZE(g_run_obj); idx_obj++) {
        for (idx_cls = 0; idx_cls < p_result->class_num; idx_cls++) {
            p_class = &p_result->object_class[idx_cls];
            if (g_run_obj[idx_obj].chn == p_info->chn &&
                g_run_obj[idx_obj].class == p_class->class_type) {
                p_cls_all = &g_run_aidet.result[E_IVS_CHN_0].object_class[g_run_obj[idx_obj].class];
                if (p_class->object_num > 0) {
                    pri_hd(LVL_LOOP, "ivs%d idx%d objects num: %d, copy to ivs0 class %d\n",
                           p_info->chn, idx_cls, p_class->object_num, g_run_obj[idx_obj].class);
                    memcpy(p_cls_all->objects, p_class->objects,
                           sizeof(ot_aidetect_object) * p_cls_all->object_capacity);
                    p_cls_all->object_num = p_class->object_num;
                }
            }
        }
    }

exit:

    //ivs0 不算 task，ivs 从 1 通道开始，一个线程提交一个 task，所以 task id 相应需 - 1
    sync_manager_submit_result(g_ivx_run->sync_mng, p_info->chn - 1, NULL, 0,
                               p_info->chn - 1, TRUE);

    return;
}

static int encode_ivp_aidetect_get_result(ot_aidetect_result_array *result)
{
    static sMultiThdDetInfo det_info[E_IVS_CHN_CNT] = {0};

    ot_video_frame_info frame = {0};
    struct timespec clk_intv = {0};
    int ret = -1, vpss_grp = 0, idx_ivs = 0;
    td_s32 vpss_chn = OT_VPSS_CHN2;
    td_bool isgetframe = TD_FALSE;
    td_bool isgainframe = TD_FALSE;
    td_void *vir_addr = NULL;
    sIvsInfo *p_ivs = NULL;
    sTaskResult **results = NULL;
    int cnt_completed = 0;
    ot_smartae_roi_info smart_info = {0};

    goto_exit_if_fail(NULL != result, exit, ret = FAILURE, "result param is null!\n");

    ret = ss_mpi_vpss_get_chn_frame(vpss_grp, vpss_chn, &frame, GET_STREAM_TIMEOUT);
    goto_exit_if_fail(TD_SUCCESS == ret, exit, ret = FAILURE, "failed to get chn frame\n");

    isgetframe = TRUE;

    vir_addr = ss_mpi_sys_mmap(frame.video_frame.phys_addr[0], frame.video_frame.width*frame.video_frame.height);
    goto_exit_if_fail(NULL != vir_addr, exit, ret = FAILURE, "failed to mmap frame\n");

    frame.video_frame.virt_addr[0] = vir_addr;

    /********************************异步送检区域************************************/
    //其它算法通道检测
    for (idx_ivs = 0; idx_ivs < E_IVS_CHN_CNT; idx_ivs++) {
        p_ivs = &g_run_ivs[idx_ivs];
        if (NULL != p_ivs->sch && NULL != *(p_ivs->sch) && NULL != p_ivs->cb_det) {
            det_info[idx_ivs].chn = p_ivs->chn;
            det_info[idx_ivs].p_frm = &frame;
            js_run_function(*(p_ivs->sch), p_ivs->cb_det, &det_info[idx_ivs], 0);
        }
    }
    /********************************异步送检区域************************************/

    ret = ss_mpi_aidetect_process(E_IVS_CHN_0, &frame.video_frame, result);
    goto_exit_if_fail(SUCCESS == ret, exit, ret = FAILURE, "failed to detect objects\n");

    //等待其它线程检测完成
    ms_clock_reset(&clk_intv);
    sync_manager_wait_all(g_ivx_run->sync_mng, &results, &cnt_completed);
    pri_hd(LVL_LOOP, "wait other ivs thread detect spend %lld ms\n", ms_since_previous(&clk_intv));
    //检测完成后清除 sync 状态
    sync_manager_reset(g_ivx_run->sync_mng);

    if(result->class_num != 0) { //检测到目标后开灯才需要人脸收光
        if (E_STATUS_DAY != get_day_curr()){
            isgainframe = TRUE;
        } else {
            dbg_hd("Light is OFF, no need to enhance! \n");
        }
    }

exit:
    if (NULL != vir_addr){
        ss_mpi_sys_munmap(vir_addr,frame.video_frame.width*frame.video_frame.height);
        vir_addr = NULL;
    }

    if (isgainframe){
        encode_smart_get_smart_info(result, &smart_info);
        if (smart_info.obj_class[0].rect_num != 0){
            ret = ss_mpi_smartae_update_roi_info(0, &frame, &smart_info);
            if (ret != S_OK) {
                for (int i = 0; i < smart_info.obj_class[0].rect_num; i++) {
                    ERR("obj_class[0].objs[%d].rect.x: %d, y: %d, w: %u, h: %u\n", i,
                        smart_info.obj_class[0].objs[i].rect.x, 
                        smart_info.obj_class[0].objs[i].rect.y, 
                        smart_info.obj_class[0].objs[i].rect.width,
                        smart_info.obj_class[0].objs[i].rect.height);
                }
            }
            ENCODE_RET_CHECK(ret, "ss_mpi_smartae_update_roi_info failed 0x%x\n",ret);
        }
    }

    if (isgetframe){
        ret = ss_mpi_vpss_release_chn_frame(vpss_grp, vpss_chn, &frame);
        ENCODE_RET_CHECK(ret, "ss_mpi_vpss_release_chn_frame failed 0x%x\n",ret);
    }

    return ret;
}

eIdxObj follow_get_center_object(follow_object p_obj[E_IDX_OBJ_CNT], ot_rect *followRect, int *track_id)
{
    int dx0, dy0, dx1, dy1, m0, m1;
    eIdxObj obj = E_IDX_OBJ_NONE;

    for (int idx_cls = E_IDX_OBJ_HUMAN; idx_cls <= E_IDX_OBJ_PET; idx_cls++) {
        //过滤车形，只跟踪人和宠物
        if (E_IDX_OBJ_CAR == idx_cls) {
            continue;
        }

        follow_object *p_result = &p_obj[idx_cls];
        for (int idx_obj = 0; idx_obj < p_result->object_num; ++idx_obj) {
            if (E_IDX_OBJ_NONE == obj) {
                // 第一个有效目标作为第一组数据
                memcpy(followRect, &p_result->objects[idx_obj].detect_rect, sizeof(ot_rect));
                obj = idx_cls;
                *track_id = p_result->objects[idx_obj].track_id;
                dx0 = abs((followRect->x + followRect->width/2) - RAW_W/2);
                dy0 = abs((followRect->y + followRect->height/2) - RAW_H/2);
            } else {
                // 从第二组数据开始要进行比较，取离中心点近的数据
                dx1 = abs((p_result->objects[idx_obj].detect_rect.x + p_result->objects[idx_obj].detect_rect.width/2) - RAW_W/2);
                dy1 = abs((p_result->objects[idx_obj].detect_rect.y + p_result->objects[idx_obj].detect_rect.height/2) - RAW_H/2);
                m0 = dx0 * dx0 + dy0 * dy0;
                m1 = dx1 * dx1 + dy1 * dy1;

                if(m1 <= m0) {
                    memcpy(followRect, &p_result->objects[idx_obj].detect_rect, sizeof(ot_rect));
                    *track_id = p_result->objects[idx_obj].track_id;
                    dx0 = dx1;
                    dy0 = dy1;
                }
            }
        }
    }

    return obj;
}

/* in : @p_obj      人形返回结果
 * out: @followRect 人形坐标信息等
 *
 * ret: @obj        E_IDX_OBJ_NONE:未匹配到，其它:匹配到跟踪目标
 **/
eIdxObj encode_person_get_follow_object(follow_object p_obj[E_IDX_OBJ_CNT],
                                    ot_rect *followRect, int *trkid,
                                    FollowStatus_t follow_stat)
{
    static int track_id = -1;
    static int fail_cnt = 0;
    eIdxObj obj = E_IDX_OBJ_NONE;

    /*
        1. 未开始跟踪选择距离中心近的目标
        2. 已经开始跟踪的，选择跟踪 id 相同的目标，如果一定帧数未发现目标，选择距离中心近的目标
        云台移动一定幅度，跟踪 ID 就会变更，失败帧数不宜设置过高，过高容易丢失跟踪目标，推荐设置值为一秒内人形检测的帧数
    */
    if (follow_stat == FOLLOW_RUN && track_id != -1) {
        // 已经开始跟踪的，按照跟踪 ID 匹配目标
        for (int idx_cls = E_IDX_OBJ_HUMAN; idx_cls <= E_IDX_OBJ_PET; idx_cls++) {
            //过滤车形，只跟踪人和宠物
            if (E_IDX_OBJ_CAR == idx_cls) {
                continue;
            }

            follow_object *p_result = &p_obj[idx_cls];
            for (int idx_obj = 0; idx_obj < p_result->object_num; ++idx_obj) {
                if (p_result->objects[idx_obj].track_id == track_id) { // id 匹配
                    memcpy(followRect, &p_result->objects[idx_obj].detect_rect, sizeof(ot_rect));
                    obj = idx_cls;
                    break;
                }
            }
        }

        if ((E_IDX_OBJ_NONE == obj && ++fail_cnt > 7) || track_id == -1) {
            /*没有有效目标，失败一定次数后选择靠中心目标*/
            obj = follow_get_center_object(p_obj, followRect, &track_id);
        }
    } else {
        // 还未开始跟踪，选择靠近中心的目标
        obj = follow_get_center_object(p_obj, followRect, &track_id);
    }

    if (E_IDX_OBJ_NONE != obj) {
        fail_cnt = 0;
    }

    *trkid = track_id;

    return obj;
}

int encode_person_follow_handing(struct ivx_cfg* ivx_info, follow_object p_obj[E_IDX_OBJ_CNT])
{
    static struct timespec clk_prev[E_IDX_OBJ_CNT] = {0};
    static int tik = 0;
    static int clear_cnt = 0;

    osd_aidet_t aidet_rect = {0};
    ot_rect followRect = {0};
    int x = 0, y = 0, w = 0, h = 0, trkid = -1, obj_num = 0;
    int draw_flag = 0, is_follow = 0, idx_cls = 0;
    int follow_time = FALSE, screenenable = FALSE;
    eIdxObj follow_obj = E_IDX_OBJ_NONE;
    FollowStatus_t follow_stat = FOLLOW_IDEL;

    ++tik;

    // ptz 没有初始化不走跟踪, 已知 ptz 未初始化计算坐标会出现除 0 段错误
    if (!is_ptz_init()) {
        return draw_flag;
    }

    for (int idx_cls = E_IDX_OBJ_HUMAN; idx_cls <= E_IDX_OBJ_PET; idx_cls++) {
        screenenable |= p_obj[idx_cls].screenenable;
        follow_time |= p_obj[idx_cls].follow_type;
    }

    //人宠算在一起，人或宠跟踪使能开启其一，都算作要跟踪
    if (follow_time) {
        follow_stat = get_follow_status();
    } else {
        follow_stat = get_person_center_status();
    }

    do {
        follow_obj = encode_person_get_follow_object(p_obj, &followRect, &trkid, follow_stat);
        if (E_IDX_OBJ_NONE != follow_obj) {
            w = (int)(followRect.width);
            h = (int)(followRect.height);
            x = (int)(followRect.x + followRect.width/2);
            y = (int)(followRect.y + followRect.height/2);

            w = (w * BASE_WIDTH * 1.0) / RAW_W; // 这里不能用 *= ，h *= BASE_HEIGHT / RAW_H; , 这样的表达式会丢失精度
            x = (x * BASE_WIDTH * 1.0) / RAW_W;
            h = (h * BASE_HEIGHT * 1.0) / RAW_H;
            y = (y * BASE_HEIGHT * 1.0) / RAW_H;
            //电机跟踪居中
            if (p_obj[follow_obj].follow_type) {
                // 将变倍后的坐标还原为变倍之前的坐标
                x = x_zoom2origin(x);
                y = y_zoom2origin(y);
                w = w_zoom2origin(w);
                h = h_zoom2origin(h);
                is_follow = ptz_follow_handing(tik, trkid, x, y, w, h);
            } else {
                //电子放大居中
                is_follow = ptz_person_center_handing(x, y, w, h, trkid);
            }

            //目标画框
            for (int idx_cls = E_IDX_OBJ_HUMAN; idx_cls <= E_IDX_OBJ_PET; idx_cls++) {
                //跟踪时不画车形框
                if (E_IDX_OBJ_CAR == idx_cls) {
                    continue;
                }

                //aidet_rect 专用于画框，不画的不算入内
                if (!p_obj[idx_cls].screenenable) {
                    pri_hd(LVL_LOOP, "class idx %d screen not enable\n", idx_cls);
                    continue;
                }

                for (int idx_obj = 0; idx_obj < p_obj[idx_cls].object_num; idx_obj++) {
                    if (obj_num < AIDET_OSD_MAX_NUM) {
                        aidet_rect.aidet_info[obj_num].screen_enable = TRUE;
                        aidet_rect.aidet_info[obj_num].aidetect_class = p_obj[idx_cls].class_type;
                        memcpy(&aidet_rect.aidet_info[obj_num].detect_rect,
                               &p_obj[idx_cls].objects[obj_num].detect_rect,sizeof(ot_rect));
                        obj_num++;
                    } else {
                        WAR("obj num bigger than %d osd, give up drawing cls %d obj %d\n",
                            AIDET_OSD_MAX_NUM, idx_cls, idx_obj);
                        break;
                    }
                }
            }

            aidet_rect.object_num = obj_num;

            encode_facial_convergence(ivx_info->hdinfo.faceae, ivx_info, &aidet_rect);

            if (screenenable) {
                if(is_follow == SUCCESS) {
                    draw_flag = TRUE;
                    encode_osd_aidet_draw(&aidet_rect);
                } else { //手动控制云台转动时能检测到人形需清除框
                    memset(&aidet_rect, 0, sizeof(osd_aidet_t));
                    encode_osd_aidet_draw(&aidet_rect);
                }
            }

            if (E_TIME_INTERVAL_OK == encode_time_interval(&clk_prev[E_IDX_OBJ_HUMAN], 3000)) {
                ENCODE_RET_JUDGE(alarm_report(g_alm_info[E_IDX_OBJ_HUMAN].alm_type, 0, NEED_TIME_CHECK, g_alm_info[E_IDX_OBJ_HUMAN].alm_str));
            }

            clear_cnt = 0;
        } else {
            encode_osd_aidet_draw(&aidet_rect);
            encode_facial_convergence(FALSE, ivx_info, &aidet_rect);
            if (++clear_cnt >= 15) {
                clear_item(); // 清跟踪 buf
            }
        }
    } while(0);

    for (idx_cls = E_IDX_OBJ_HUMAN + 1; idx_cls < E_IDX_OBJ_CNT; idx_cls++) {
        if (*(g_run_obj[idx_cls].enable) && p_obj[idx_cls].object_num > 0) {
            if(E_TIME_INTERVAL_OK == encode_time_interval(&clk_prev[idx_cls], 3000)) {
                ENCODE_RET_JUDGE(alarm_report(g_alm_info[idx_cls].alm_type, 0, NEED_TIME_CHECK, g_alm_info[idx_cls].alm_str));
            }
        }
    }

    return draw_flag;
}

static int encode_ivp_get_draw_result(osd_aidet_t* aidet_rect, ot_aidetect_object* objects, td_u32 object_count, td_u32 class_type, td_bool screen_enable)
{
    do {
        if (!screen_enable) {
            break;
        }

        for (td_u32 i = 0; i < object_count; i++) {
            if (aidet_rect->object_num >= CNT_RECTS_MAX) {
                break; // 防止越界
            }

            aidet_rect->aidet_info[aidet_rect->object_num].detect_rect.x = objects[i].detect_rect.x;
            aidet_rect->aidet_info[aidet_rect->object_num].detect_rect.y = objects[i].detect_rect.y;
            aidet_rect->aidet_info[aidet_rect->object_num].detect_rect.width = objects[i].detect_rect.width;
            aidet_rect->aidet_info[aidet_rect->object_num].detect_rect.height = objects[i].detect_rect.height;
            aidet_rect->aidet_info[aidet_rect->object_num].aidetect_class = class_type;
            aidet_rect->aidet_info[aidet_rect->object_num].screen_enable = TRUE;
            aidet_rect->object_num++;
        }
    } while(0);

    return aidet_rect->object_num;
}

static int encode_ivp_facial_convergence(struct ivx_cfg* ivx_info, obj_filter_t *p_filt)
{
    td_u32 i = 0;
    osd_aidet_t aidet_rect = {0};

     for (i = 0; i < p_filt->update_num; i++) {
        if (aidet_rect.object_num >= CNT_RECTS_MAX) {
            break;
        }

        aidet_rect.aidet_info[aidet_rect.object_num].detect_rect.x = p_filt->obj_update[i].detect_rect.x;
        aidet_rect.aidet_info[aidet_rect.object_num].detect_rect.y = p_filt->obj_update[i].detect_rect.y;
        aidet_rect.aidet_info[aidet_rect.object_num].detect_rect.width = p_filt->obj_update[i].detect_rect.width;
        aidet_rect.aidet_info[aidet_rect.object_num].detect_rect.height = p_filt->obj_update[i].detect_rect.height;
        aidet_rect.aidet_info[aidet_rect.object_num].aidetect_class = OT_AIDETECT_CLASS_HUMAN;
        aidet_rect.object_num++;
    }

    for (i = 0; i < p_filt->new_num; i++) {
        if (aidet_rect.object_num >= CNT_RECTS_MAX) {
            break;
        }

        aidet_rect.aidet_info[aidet_rect.object_num].detect_rect.x = p_filt->obj_new[i].detect_rect.x;
        aidet_rect.aidet_info[aidet_rect.object_num].detect_rect.y = p_filt->obj_new[i].detect_rect.y;
        aidet_rect.aidet_info[aidet_rect.object_num].detect_rect.width = p_filt->obj_new[i].detect_rect.width;
        aidet_rect.aidet_info[aidet_rect.object_num].detect_rect.height = p_filt->obj_new[i].detect_rect.height;
        aidet_rect.aidet_info[aidet_rect.object_num].aidetect_class = OT_AIDETECT_CLASS_HUMAN;
        aidet_rect.object_num++;
    }

    encode_facial_convergence(ivx_info->hdinfo.faceae, ivx_info, &aidet_rect);

    return 0;
}

static int encode_ivp_handle_alarm_osd(struct ivx_cfg* ivx_info, obj_filter_t filter[E_IDX_OBJ_CNT])
{
    static struct timespec clk_prev[E_IDX_OBJ_CNT] = {0};
    static struct timespec vgline_time_pre = {0};
    static struct timespec vgrect_time_pre = {0};
    static struct timespec blink_time_pre = {0};
    static int line_blink_cnt = -1; // -1 表示没触发过报警闪烁 0 表示闪烁结束 >0 表示正在闪烁
    static int rect_blink_cnt = -1;

    osd_aidet_t aidet_rect = {0};
    int aidetect_count = 0, idx_cls = 0, idx_basic = 0, idx_obj = 0;
    obj_filter_t *p_filt = NULL;

    sObjects objs = {0};

    idx_basic = 0;

    for (idx_cls = E_IDX_OBJ_HUMAN; idx_cls < E_IDX_OBJ_CNT; idx_cls++) {
        p_filt = &filter[idx_cls];

        for (idx_obj = idx_basic; idx_obj < idx_basic + p_filt->update_num; idx_obj++) {
            if (idx_obj >= ARRAY_SIZE(objs.object_info)) {
                break;
            }

            objs.object_info[idx_obj].track_id = p_filt->obj_update[idx_obj].track_id;
            objs.object_info[idx_obj].start_x  = (int)(p_filt->obj_update[idx_obj].detect_rect.x * 1920 / 640);
            objs.object_info[idx_obj].start_y  = (int)(p_filt->obj_update[idx_obj].detect_rect.y * 1080 / 360);
            objs.object_info[idx_obj].end_x    = (int)((p_filt->obj_update[idx_obj].detect_rect.x + p_filt->obj_update[idx_obj].detect_rect.width) * 1920 / 640);
            objs.object_info[idx_obj].end_y    = (int)((p_filt->obj_update[idx_obj].detect_rect.y + p_filt->obj_update[idx_obj].detect_rect.height) * 1080 / 360);
        }

        idx_basic += p_filt->update_num;

        if (*(g_run_obj[idx_cls].enable) && (p_filt->update_num > 0 || p_filt->new_num > 0)) {
            if(E_TIME_INTERVAL_OK == encode_time_interval(&clk_prev[idx_cls], 3000)) {
                ENCODE_RET_JUDGE(alarm_report(g_alm_info[idx_cls].alm_type, 0, NEED_TIME_CHECK, g_alm_info[idx_cls].alm_str));
            }
        }
    }

    objs.objects_num = idx_basic;

    if (ivx_info->lineinfo.enable) {
        if (ivx_info->linetime && is_object_line_crossing(&objs)) {
            if (E_TIME_INTERVAL_OK == encode_time_interval(&vgline_time_pre, 3000)) {
                ENCODE_RET_JUDGE(alarm_report(JALARM_TYPE_VGLINE, 0, NEED_TIME_CHECK, "vgline detect"));
                line_blink_cnt = 7;
            }
        }

        if (line_blink_cnt >= 0 && encode_time_interval(&blink_time_pre, 500)) {
            encode_osd_vgline_blink(line_blink_cnt);
            line_blink_cnt--;
        }
    }

    if (ivx_info->rectinfo.enable) {
        if (ivx_info->recttime && is_object_rect_crossing(&objs)) {
            if (E_TIME_INTERVAL_OK == encode_time_interval(&vgrect_time_pre, 3000)) {
                ENCODE_RET_JUDGE(alarm_report(JALARM_TYPE_VGRECT, 0, NEED_TIME_CHECK, "vgrect detect"));
                rect_blink_cnt = 7;
            }
        }

        if (rect_blink_cnt >= 0 && encode_time_interval(&blink_time_pre, 500)) {
            encode_osd_vgrect_blink(rect_blink_cnt);
            rect_blink_cnt--;
        }
    }

    for (idx_cls = E_IDX_OBJ_HUMAN; idx_cls < E_IDX_OBJ_CNT; idx_cls++) {
        p_filt = &filter[idx_cls];

        if (*(g_run_obj[idx_cls].enable)) {
            aidetect_count = encode_ivp_get_draw_result(&aidet_rect, p_filt->obj_update, p_filt->update_num, g_run_obj[idx_cls].class, *(g_run_obj[idx_cls].screen_enable));
            if(aidetect_count >= CNT_RECTS_MAX) {
                break;
            }
        }
    }

    for (idx_cls = E_IDX_OBJ_HUMAN; idx_cls < E_IDX_OBJ_CNT; idx_cls++) {
        p_filt = &filter[idx_cls];

        if (*(g_run_obj[idx_cls].enable)) {
            aidetect_count = encode_ivp_get_draw_result(&aidet_rect, p_filt->obj_new, p_filt->new_num, g_run_obj[idx_cls].class, *(g_run_obj[idx_cls].screen_enable));
            if(aidetect_count >= CNT_RECTS_MAX) {
                break;
            }
        }
    }

    encode_osd_aidet_draw(&aidet_rect);

    p_filt = &filter[E_IDX_OBJ_HUMAN];
    if ((p_filt->new_num > 0 || p_filt->update_num > 0) && *(g_run_obj[E_IDX_OBJ_HUMAN].enable)) {
        encode_ivp_facial_convergence(ivx_info, p_filt);
    }

    return (aidetect_count > 0 ? 1 : 0);
}

int encode_ivp_check_alarm_switch(struct ivx_cfg* ivx_info)
{
    int follow_enable[E_IDX_OBJ_CNT] = {
        FALSE,
        ivx_info->followinfo.humanenable,
        ivx_info->followinfo.carenable,
        ivx_info->followinfo.petenable,
    };
    int ret = SUCCESS;
    int need_clean = FALSE;
    osd_aidet_t aidet_rect = {0};

    static int hd_tick = 0;
    static int hd_time_prev = FALSE;
    static int car_time_prev = FALSE;
    static int pet_time_prev = FALSE;
    static int follow_time_prev[E_IDX_OBJ_CNT] = {0};
    static int vgline_time_prev = FALSE;
    static int vgrect_time_prev = FALSE;
    hd_tick++;
    if (hd_tick > 25) {  // 2s 刷新一次
        hd_tick = 0;
        if (ivx_info->hdinfo.enable) {
            ivx_info->hdtime = TimeJudge(ivx_info->hdinfo.times);
        } else {
            ivx_info->hdtime = FALSE;
        }

        if (ivx_info->carinfo.enable) {
            ivx_info->cartime = TimeJudge(ivx_info->carinfo.times);
        } else {
            ivx_info->cartime = FALSE;
        }

        if (ivx_info->petinfo.enable) {
            ivx_info->pettime = TimeJudge(ivx_info->petinfo.times);
        } else {
            ivx_info->pettime = FALSE;
        }

        int followtime = TimeJudge(ivx_info->followinfo.times);
        int enable_cnt = 0;
        for (int idx = 0; idx < ARRAY_SIZE(ivx_info->followtime); idx++) {
            if (followtime) {
                ivx_info->followtime[idx] = follow_enable[idx];
                pri_hd(LVL_DBG, "%s idx class %d follow\n",
                       ivx_info->followtime[idx] ? "enable" : "disable", idx);
                enable_cnt += follow_enable[idx];
            } else {
                pri_hd(LVL_DBG, "follow not in time, all disable\n");
                memset(ivx_info->followtime, 0, sizeof(ivx_info->followtime));
                break;
            }
        }

        if (enable_cnt > 0) {
            ivx_info->follow_has_enable = TRUE;
        } else {
            ivx_info->follow_has_enable = FALSE;
        }

        if (ivx_info->lineinfo.enable) {
            ivx_info->linetime = TimeJudge(ivx_info->lineinfo.times);
        } else {
            ivx_info->linetime = FALSE;
        }

        if (ivx_info->rectinfo.enable) {
            ivx_info->recttime = TimeJudge(ivx_info->rectinfo.times);
        } else {
            ivx_info->recttime = FALSE;
        }

        if (hd_time_prev != ivx_info->hdtime) {
            hd_time_prev = ivx_info->hdtime;
            DBG("hd time: %d\n", ivx_info->hdtime);
            need_clean = TRUE;
        }

        if (car_time_prev != ivx_info->cartime) {
            DBG("car time: %d\n", ivx_info->cartime);
            car_time_prev = ivx_info->cartime;
            need_clean = TRUE;
        }

        if (pet_time_prev != ivx_info->pettime) {
            DBG("pet time: %d\n", ivx_info->pettime);
            pet_time_prev = ivx_info->pettime;
            need_clean = TRUE;
        }

        for (int idx_obj = 0; idx_obj < E_IDX_OBJ_CNT; idx_obj++) {
            if (follow_time_prev[idx_obj] != ivx_info->followtime[idx_obj]) {
                pri_hd(LVL_DBG, "follow time %d: %d\n", idx_obj, ivx_info->followtime[idx_obj]);
                follow_time_prev[idx_obj] = ivx_info->followtime[idx_obj];
                need_clean = TRUE;
            }
        }

        if (vgline_time_prev != ivx_info->linetime) {
            DBG("line time: %d\n", ivx_info->linetime);
            vgline_time_prev = ivx_info->linetime;
            need_clean = TRUE;
        }

        if (vgrect_time_prev != ivx_info->recttime) {
            DBG("rect time: %d\n", ivx_info->recttime);
            vgrect_time_prev = ivx_info->recttime;
            need_clean = TRUE;
        }
    }
    //DBG("ivx_info->hdtime:%d, ivx_info->cartime:%d, ivx_info->followtime:%d\n", ivx_info->hdtime, ivx_info->cartime, ivx_info->followtime);
    if (need_clean || (!ivx_info->hdtime && !ivx_info->cartime && !ivx_info->followtime && 
        !ivx_info->linetime && !ivx_info->recttime && !ivx_info->pettime)) {//低电量不报警
        encode_osd_aidet_draw(&aidet_rect);
        ret = FAILURE;
    }

    return ret;
}

int encode_ivp_get_result(int *dst_count, ot_aidetect_object *dst_object, ot_aidetect_object *src_object)
{
    do {
        if(*dst_count >= CNT_RECTS_MAX) {
            break;
        }

        memcpy(dst_object, src_object,sizeof(ot_aidetect_object));
        (*dst_count)++;
    } while(0);

    return 0;
}

int encode_ivp_car_person_filt_result(struct ivx_cfg* ivx_info, ot_aidetect_result_array *ai_result,
                                      obj_filter_t filter[E_IDX_OBJ_CNT])
{
    if(NULL == ivx_info || NULL == ai_result) {
        return -1;
    }

    int idx_cls = 0, idx_obj = 0;
    obj_filter_t *p_filt = NULL;

    for (idx_cls = 0; idx_cls < ARRAY_SIZE(g_run_obj); ++idx_cls) {
        p_filt = &filter[idx_cls];

        for (idx_obj = 0; idx_obj < ai_result->object_class[idx_cls].object_num; ++idx_obj) {
            ot_aidetect_object *p_obj = &ai_result->object_class[idx_cls].objects[idx_obj];

            if(0 == p_obj->detect_rect.width || 0 == p_obj->detect_rect.height) {
                continue;
            }

            if (encode_aidet_get_object(&p_obj->detect_rect, g_run_obj[idx_cls].area)) {
                if(OT_AIDETECT_TRACK_STATUS_UPDATE == p_obj->track_status) {
                    encode_ivp_get_result(&p_filt->update_num, &p_filt->obj_update[p_filt->update_num], p_obj);
                }

                //白天才将新的物体计入处理
                if(g_ivx_cfg->is_day && OT_AIDETECT_TRACK_STATUS_NEW == p_obj->track_status) {
                    encode_ivp_get_result(&p_filt->new_num, &p_filt->obj_new[p_filt->new_num], p_obj);
                }
            }
        }
    }

    return 0;
}

int encode_ivp_aidetect_process(struct ivx_cfg* ivx_info)
{
    int ret = 0;
    osd_aidet_t aidet_rect = {0};
    follow_object obj[E_IDX_OBJ_CNT] = {0};
    obj_filter_t filter[E_IDX_OBJ_CNT] = {0};
    int is_draw_osd = 0, idx_cls = 0, cnt_det = 0;
    static int osd_clear_cntdown = 0;   // osd 清除倒计时
    ot_aidetect_result_array *p_result = &g_run_aidet.result[E_IVS_CHN_0];

    do {
        ret = encode_ivp_check_alarm_switch(ivx_info);
        if(SUCCESS != ret) {
            break;
        }

        if (osd_clear_cntdown > 0) {
            osd_clear_cntdown--;
        }

        if (osd_clear_cntdown <= 0) {
            encode_osd_aidet_draw(&aidet_rect);
        }

        ret = encode_ivp_aidetect_init_result(p_result, E_IVS_CHN_0);
        ENCODE_RET_BREAK(ret, "encode_ivp_aidetect_init_result_param failed 0x%x\n",ret);

        ret = encode_ivp_aidetect_get_result(p_result);
        if (TD_SUCCESS != ret){
            //ERR("aidetect get p_result failed 0x%x\n",ret);
            break;
        }

        encode_ivp_object_filter(p_result, ivx_info);
        encode_ivp_car_person_filt_result(ivx_info, p_result, filter);

        if (0 == filter[E_IDX_OBJ_HUMAN].new_num &&
            0 == filter[E_IDX_OBJ_HUMAN].update_num) {
            encode_facial_convergence(FALSE, ivx_info, &aidet_rect);

            cnt_det = 0;
            for (idx_cls = E_IDX_OBJ_HUMAN + 1; idx_cls < E_IDX_OBJ_CNT; idx_cls++) {
                if (filter[idx_cls].new_num > 0 ||
                    filter[idx_cls].update_num > 0) {
                    cnt_det++;
                    break;
                }
            }

            if (0 == cnt_det) {
                encode_osd_aidet_draw(&aidet_rect);
            }
        }

        if (ivx_info->follow_has_enable ||
            (ivx_info->hdtime && ivx_info->hdinfo.person_center)) {
            for (idx_cls = E_IDX_OBJ_HUMAN; idx_cls < E_IDX_OBJ_CNT; idx_cls++) {
                obj_filter_t *p_filt = &filter[idx_cls];

                obj[idx_cls].follow_type = ivx_info->followtime[idx_cls];
                obj[idx_cls].class_type = g_run_obj[idx_cls].class;
                obj[idx_cls].screenenable = *(g_run_obj[idx_cls].screen_enable);

                if(p_filt->update_num > 0) {
                    memcpy(&obj[idx_cls].objects[obj[idx_cls].object_num],
                           &p_filt->obj_update[0],
                           sizeof(ot_aidetect_object) * p_filt->update_num);

                    obj[idx_cls].object_num += p_filt->update_num;
                } else if(p_filt->new_num > 0) {
                    memcpy(&obj[idx_cls].objects[obj[idx_cls].object_num],
                           &p_filt->obj_new[0],
                           sizeof(ot_aidetect_object)*p_filt->new_num);

                    obj[idx_cls].object_num += p_filt->new_num;
                }
            }

            is_draw_osd = encode_person_follow_handing(ivx_info, obj);
        } else {
            is_draw_osd = encode_ivp_handle_alarm_osd(ivx_info, filter);
        }
    } while(0);

    if (is_draw_osd) {
        osd_clear_cntdown = INTV_MS_CLR_OSD / IVX_LOOP_TIME;  // 100ms 清一次框
    }

    return 0;
}

static int encode_ivp_aidetect_param_init(struct ivx_cfg* ivx_info, aidetect_info_t *aidetect_info)
{
    aidetect_info->input_model.model_load_mode = OT_AIDETECT_MODEL_LOAD_FROM_PATH;
    aidetect_info->input_model.model = (td_void *)IVP_AIDETECT_MODEL_PATH;
    aidetect_info->input_model.size = (td_u32)strlen(IVP_AIDETECT_MODEL_PATH);

    (td_void)memset_s(&aidetect_info->chn_attr, sizeof(ot_aidetect_chn_attr), 0, sizeof(ot_aidetect_chn_attr));
    aidetect_info->chn_attr.track_class_num = 3;
    aidetect_info->chn_attr.track_class_attr[0].class_type = OT_AIDETECT_CLASS_FACE;
    aidetect_info->chn_attr.track_class_attr[0].track_en = TD_TRUE;

    aidetect_info->chn_attr.track_class_attr[1].class_type = OT_AIDETECT_CLASS_HUMAN;
    aidetect_info->chn_attr.track_class_attr[1].track_en = TD_TRUE;

    aidetect_info->chn_attr.track_class_attr[2].class_type = OT_AIDETECT_CLASS_VEHICLE;
    aidetect_info->chn_attr.track_class_attr[2].track_en = TD_TRUE;

    return 0;
}

int encode_ivp_aidetect_set_param(struct ivx_cfg* ivx_info, struct ivx_run* ivx_run)
{
    int ret = 0;
    ot_aidetect_chn_param chn_param = {0};
    osd_aidet_t aidet_rect = {0};
    encode_osd_aidet_draw(&aidet_rect);

    do {
        ret = ss_mpi_aidetect_get_chn_param(E_IVS_CHN_0, &chn_param);
        ENCODE_RET_BREAK(ret, "ss_mpi_aidetect_get_chn_param failed\n");
        DBG("==================== Default Chn Param ====================\n");
        DBG("preemp_en: %d, priority: %d, priority_up_step_timeout: %d, priority_up_top_timeout: %d\n",
            chn_param.model_priority.preemp_en, chn_param.model_priority.priority,
            chn_param.model_priority.priority_up_step_timeout, chn_param.model_priority.priority_up_top_timeout);
        DBG("detect num: %d\n", chn_param.detect_threshold_num);
        for (td_u32 i = 0; i < chn_param.detect_threshold_num; ++i) {
            DBG("detect type:%d, threshold: %f, track_missing_frame_num: %d\n",
                chn_param.detect_threshold[i].class_type, chn_param.detect_threshold[i].detect_threshold,
                chn_param.detect_threshold[i].track_miss_frame_num);
        }

        for (td_u32 i = 0; i < chn_param.detect_threshold_num; ++i) {
            if (chn_param.detect_threshold[i].class_type == OT_AIDETECT_CLASS_FACE){
                chn_param.detect_threshold[i].detect_threshold = 1.0 -(float)(ivx_info->hdinfo.thresh)/100;
            }

            if (chn_param.detect_threshold[i].class_type == OT_AIDETECT_CLASS_HUMAN){
                chn_param.detect_threshold[i].detect_threshold = 1.0 -(float)(ivx_info->hdinfo.thresh)/100;
            }
            if (chn_param.detect_threshold[i].class_type == OT_AIDETECT_CLASS_VEHICLE){
                chn_param.detect_threshold[i].detect_threshold = 1.0 -(float)(ivx_info->carinfo.thresh)/100;
            }
            if (chn_param.detect_threshold[i].class_type == OT_AIDETECT_CLASS_PET){
                chn_param.detect_threshold[i].detect_threshold = 1.0 -(float)(ivx_info->petinfo.thresh)/100;
            }
        }
        ret = ss_mpi_aidetect_set_chn_param(E_IVS_CHN_0, &chn_param);
        ENCODE_RET_BREAK(ret, "ss_mpi_aidetect_set_chn_param failed\n");
    } while(0);

    return ret;
}

#if 0
int encode_ivp_aidetect_init(struct ivx_cfg* ivx_info, struct ivx_run* ivx_run)
{
    int ret = 0;

    do {
        ret = encode_ivp_aidetect_param_init(ivx_info, &g_aidetect_info);
        ENCODE_RET_BREAK(ret, "encode_ivp_aidetect_param_init failed\n");

        ret = ss_mpi_aidetect_create_chn(E_IVS_CHN_0, &g_aidetect_info.input_model, &g_aidetect_info.chn_attr);
        ENCODE_RET_BREAK(ret, "ss_mpi_aidetect_create_chn failed\n");

        ret = encode_ivp_aidetect_set_param(ivx_info, ivx_run);
        ENCODE_RET_BREAK(ret, "encode_ivp_aidetect_set_param failed\n");

        ret = encode_ivp_aidet_smart_ae_init();
        ENCODE_RET_BREAK(ret, "encode_ivp_aidet_set_smart_ae failed\n");
    } while(0);

    return ret;
}
#endif

int encode_ivp_aidetect_init(struct ivx_cfg* ivx_info, struct ivx_run* ivx_run)
{
    int ret = 0, idx_ivs = 0, idx_cls = 0;
    ot_aidetect_chn_attr *p_attr = NULL;

    for (idx_ivs = 0; idx_ivs < E_IVS_CHN_CNT; idx_ivs++) {
        ot_aidetect_chn_param chn_param = {0};

        g_aidetect_info[idx_ivs].input_model.model_load_mode = OT_AIDETECT_MODEL_LOAD_FROM_PATH;
        g_aidetect_info[idx_ivs].input_model.model = g_run_ivs[idx_ivs].path_model;
        g_aidetect_info[idx_ivs].input_model.size = strlen(g_run_ivs[idx_ivs].path_model);

        COLOR_G("ivs%d model: %s\n", idx_ivs, (char *)g_aidetect_info[idx_ivs].input_model.model);

        p_attr = &g_aidetect_info[idx_ivs].chn_attr;
        memset_s(p_attr, sizeof(ot_aidetect_chn_attr), 0, sizeof(ot_aidetect_chn_attr));

        for (idx_cls = 0; idx_cls < ARRAY_SIZE(g_run_obj); idx_cls++) {
            if (idx_ivs == g_run_obj[idx_cls].chn) {
                COLOR_G("ivs%d bind class type %d\n", idx_ivs, g_run_obj[idx_cls].class);
                p_attr->track_class_attr[p_attr->track_class_num].class_type = g_run_obj[idx_cls].class;
                p_attr->track_class_attr[p_attr->track_class_num].track_en = TD_TRUE;
                p_attr->track_class_num++;
            }
        }

        ret = ss_mpi_aidetect_create_chn(idx_ivs, &g_aidetect_info[idx_ivs].input_model, p_attr);
        ENCODE_RET_BREAK(ret, "ss_mpi_aidetect_create_chn failed\n");

        ret = ss_mpi_aidetect_get_chn_param(idx_ivs, &chn_param);
        ENCODE_RET_BREAK(ret, "ss_mpi_aidetect_get_chn_param failed\n");

        for (idx_cls = 0; idx_cls < ARRAY_SIZE(g_run_obj); idx_cls++) {
            if (idx_ivs == g_run_obj[idx_cls].chn) {
                chn_param.detect_threshold[idx_cls].detect_threshold = 1.0 - (float)(*(g_run_obj[idx_cls].thresh)) / 100;
            }
        }

        ret = ss_mpi_aidetect_set_chn_param(idx_ivs, &chn_param);
        ENCODE_RET_BREAK(ret, "ss_mpi_aidetect_set_chn_param failed\n");
    }

    do {
        ENCODE_RET_BREAK(ret, "aidetect create or set chn failed\n");

        ret = encode_ivp_aidet_smart_ae_init();
        ENCODE_RET_BREAK(ret, "encode_ivp_aidet_set_smart_ae failed\n");
    } while(0);

    return ret;
}

int encode_ivp_aidetect_uninit()
{
    int ret = 0, idx_ivs = 0;

    // 关闭算法检测时处于人脸收光状态，则需要强制切自动ae
    if(g_run_aidet.flag_faceae) {
        g_run_aidet.flag_faceae = FALSE;
        g_run_aidet.flag_faceae_forced = TRUE;
    }

    do {
        ret = encode_ivp_aidet_smart_ae_uninit();
        ENCODE_RET_BREAK(ret, "encode_ivp_aidet_set_smart_ae failed\n");

        for (idx_ivs = 0; idx_ivs < E_IVS_CHN_CNT; idx_ivs++) {
            ret = ss_mpi_aidetect_destroy_chn(idx_ivs);
            ENCODE_RET_BREAK(ret, "ss_mpi_aidetect_destroy_chn failed\n");
        }
    } while(0);

    return ret;
}

// 一个小时以上没有报警云存上传，就特地发送一个报警上传云存
void send_alarm_for_hour(void)
{
    char recalarm_flag[MINS_OF_1DAY + 1] = {0};
    int minutes_num = 0;
    time_t curTime = 0;
    struct tm curTm_s = {0};
    struct tm *curTm = &curTm_s;
    
    curTime = time(NULL);
    localtime_r(&curTime, &curTm_s);
    minutes_num = curTm->tm_hour * 60 + curTm->tm_min;
    if (minutes_num < 60) {
        ERR("--------minutes_num = %d\n", minutes_num);
        return;
    }

    get_curday_recalarm_flag(recalarm_flag, sizeof(recalarm_flag));

    int i = 0;
    for (i = minutes_num - 60; i < minutes_num; i++) {
        if (recalarm_flag[i] == 'A') {
            return;
        }
    }

    for (i = 0; i < MAX_SENSOR_NUM; i++) {
#ifdef PLATFORM_TENCENT
        if (event_cs_status(i)) {
            ENCODE_RET_JUDGE(alarm_report(JALARM_TYPE_SCENE_CHANGE, i, 
                              NO_NEED_TIME_CHECK, "senceChange"));
        }
#endif
    }

    return;
}

void init_encode_ivp_aidetect_cfg(void)
{
    fet_obj_info(g_run_obj);

    fet_ivs_info(g_run_ivs);
}
