#ifndef __ENCODE_BASE_IVX_H__
#define __ENCODE_BASE_IVX_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "jconfstruct.h"
#include "sync_manager.h"
#include "js_scheduler.h"

#define MS_TIMEOUT_DETECTION (500)
#define CNT_IVS_TASKS        (E_IVS_CHN_CNT - 1)
#define CNT_IVS_PRODUCERS    (E_IVS_CHN_CNT - 1)

typedef struct {
    int enable;
    int color;
    int x;
    int y;
    int width;
    int height;
} mask_info_t;

enum {
    CMD_IVX_MD_INFO    = 1 << 0 ,   // 移动侦测
    CMD_IVX_VD_INFO    = 1 << 1 ,   // 视频遮挡
    CMD_IVX_MD_FREEZE  = 1 << 2 ,
    CMD_IVX_MD_LAMP    = 1 << 3 ,
    CMD_IVX_HD_INFO    = 1 << 4 ,   // 人形侦测
    CMD_IVX_IVS_FORZEN = 1 << 5 ,
    CMD_IVX_FW_INFO    = 1 << 6 ,   // 人形跟踪
    CMD_IVX_CAR_INFO   = 1 << 7 ,   // 车辆侦测
    CMD_IVX_PET_INFO   = 1 << 8 ,   // 宠物侦测
    CMD_IVX_FACE_CONV  = 1 << 9 ,   // 人脸收光
    CMD_IVX_LINE_INFO  = 1 << 10,   // 越界侦测
    CMD_IVX_RECT_INFO  = 1 << 11,   // 区域侦测
    CMD_IVX_LINE_BLINK = 1 << 12,   // 越界闪烁
    CMD_IVX_RECT_BLINK = 1 << 13,   // 区域闪烁
};

typedef enum {
    E_TASK_ID_PETDET    = 0,
    E_TASK_ID_VMASKDET  = 1,
} eIvsTaskId;

typedef enum {
    E_PRODUCER_ID_PETDET    = 0,
    E_PRODUCER_ID_VMASKDET  = 1,
} eIvsProducerId;

typedef enum {
    E_IVS_CHN_0     = 0,
    E_IVS_CHN_1     = 1,
    E_IVS_CHN_CNT   = 2,
} eIvsChn;

typedef enum {
    E_IDX_OBJ_NONE  = -1,
    E_IDX_OBJ_FACE  = 0,
    E_IDX_OBJ_HUMAN = 1,
    E_IDX_OBJ_CAR   = 2,
    E_IDX_OBJ_PET   = 3,
    E_IDX_OBJ_CNT   = 4,
} eIdxObj;

typedef struct {
    int start_x_pos;
    int start_y_pos;
    int end_x_pos;
    int end_y_pos;
}hd_area_pos;

struct ivx_run{
    int md_init;
    int aidetect_init;
    int vg_init;
    int ivx_init;
    sSyncManager *sync_mng;
    JSScheduler sch;
    JSScheduler sch_pet;
    JSTCHandle  hdl_loop;
    struct cmdstat *p_ctx;
};

struct ivx_cfg{
    MotionDetectS    mdinfo;
    HumanDetectionS  hdinfo;
    CarDetectionS    carinfo;
    PetDetectionS    petinfo;
    follow_info_t    followinfo;
    VglineS          lineinfo;
    VgrectS          rectinfo;
    VideoMaskS       vminfo;
    sFacialConvergence faceinfo;
    hd_area_pos facearea;   //人脸检测区域
    hd_area_pos hdarea;     //人形检测区域
    hd_area_pos cararea;    //车形检测区域
    hd_area_pos petarea;    //宠形检测区域
	hd_area_pos mdarea;     //移动检测区域
    mask_info_t mask;
    int facetime;
    int hdtime;
    int cartime;
    int pettime;
    int followtime[E_IDX_OBJ_CNT];
    int follow_has_enable;
    int linetime;
    int recttime;
    int vmtime;
    int is_day;
};

extern struct ivx_cfg *g_ivx_cfg;
extern struct ivx_cfg *g_ivx_raw;
extern struct ivx_run *g_ivx_run;

int encode_ivx_init();

int encode_ivx_uninit();

#ifdef __cplusplus
}
#endif
#endif

