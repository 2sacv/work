#ifndef __ENCODE_OSD_H__
#define __ENCODE_OSD_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "cmdstat.h"
#include "encode_typedef.h"
#include "ot_defines.h"
#include "ot_common_aidetect.h"
#include "ot_common_video.h"
#include "encode_base_ivx.h"

#define AIDET_OSD_MAX_NUM      4

typedef struct {
    ot_aidetect_class aidetect_class;
    ot_rect detect_rect;
    int screen_enable;
} aidet_info_t;

typedef struct {
    int object_num;
    mask_info_t mask;
    aidet_info_t aidet_info[AIDET_OSD_MAX_NUM];
} osd_aidet_t;

typedef struct {
    int enable;
    char zoomstr[16];
} osd_zoom_t;

typedef enum {
    E_OSD_GROUP_MAIN = 0,
    E_OSD_GROUP_SUB  = 1,
    E_OSD_GROUP_MAX  = 2,
    E_OSD_GROUP_ALL  = 3
} OsdGroupE;

typedef enum {
    E_OSD_BEGIN = -1,
    E_OSD_NAME,          // 名字
    E_OSD_TIME,          // 时间
    E_OSD_STREAM,        // 码流信息
    E_OSD_ZOOM,          // 变倍
    E_OSD_TEXT      = 4, // osd 字幕(实际为 custom0)

    // 以下扩展字幕需单开 osd 区域时使用, 否则和 E_OSD_TEXT 做绑定, 具体视情况增删
    E_OSD_EXPAND1   = 5, // 扩展字幕 1 做预留
    E_OSD_EXPAND2   = 6, // 扩展字幕 2 做预留
    E_OSD_TEXT_MAX,

    E_OSD_MASK = 101,
    E_OSD_END,
} OsdTypeE;

typedef enum {
    E_OSD_EXPEND_PAN = 4,       //水平电机步长
    E_OSD_EXPEND_TILT = 5,      //水平电机步长
    E_OSD_EXPEND_SIM4G = 6,     // 运营商
    E_OSD_EXPEND_BATTERY = 7,   //电量字幕
    E_OSD_EXPEND_CHARGING = 8,  //充电状态字幕
    E_OSD_EXPEND_END,
} OsdExpandE;


typedef enum {
    E_RGN_MAIN_NAME_HANDLE = 0,
    E_RGN_MAIN_TIME_HANDLE,
    E_RGN_MAIN_TEXT_HANDLE,
    E_RGN_SUB_NAME_HANDLE,
    E_RGN_SUB_TIME_HANDLE,
    E_RGN_SUB_TEXT_HANDLE,

    E_RGN_MAIN_ZOOM_HANDLE  = 6,
    E_RGN_SUB_ZOOM_HANDLE   = 7,

    E_RGN_MAIN_STREAM_HANDLE = 8,
    E_RGN_SUB_STREAM_HANDLE  = 9,

    E_RGN_MAIN_EXPAND1_HANDLE = 10,
    E_RGN_SUB_EXPAND1_HANDLE  = 11,

    E_RGN_MAIN_EXPAND2_HANDLE = 12,
    E_RGN_SUB_EXPAND2_HANDLE  = 13,

    E_VGRECT_MAIN_HANDLE = 16,
    E_VGRECT_SUB_HANDLE,

    E_VGLINE_MAIN_HANDLE = 18,
    E_VGLINE_SUB_HANDLE,
    E_AIDET_MAIN_HANDLE = 20,   //cover 4
    E_AIDET_SUB_HANDLE = 24,    //cover 4
    E_AIDET_HANDLE_END = 32,
    E_RGN_HANDLE_END,
    E_RGN_HANDLE_MAX = OT_RGN_HANDLE_MAX - 1,
} OsdRgnHandleE;

#define NUM_GRP  2 //一共8个, 0~3为一组, 4~7为一组
#define NUM_MBR  4 //每组4个成员

enum {
    CMD_OSD_INFO    = 1 << 0,
    CMD_OSD_STYLE   = 1 << 1,
    CMD_OSD_EXPAND  = 1 << 2,
    CMD_TIME_OSD    = 1 << 3,
    CMD_VIDEO_CHAGE = 1 << 4,
    CMD_OSD_EXPAND_FONT = 1 << 5,
};

float encode_osd_video_get_x_ratio(int group);
float encode_osd_video_get_y_ratio(int group);
float encode_osd_draw_get_x_ratio(int group);
float encode_osd_draw_get_y_ratio(int group);

int encode_osd_init(void);
int encode_osd_uninit(void);
int encode_osd_group_stop(int group);
int encode_osd_group_start(int group);
int encode_osd_add_stream(OsdGroupE group, DWORD dwAddFps, DWORD dwAddBits);
int encode_osd_zoom_change(int enable, char *zoomstr);

int encode_osd_vgline_blink(int blink_cnt);
int encode_osd_vgrect_blink(int blink_cnt);


#ifdef __cplusplus
}
#endif
#endif

