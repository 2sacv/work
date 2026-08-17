/*
 *       Filename:  resolution.h
 *    Description:  分辨率相关从 encode_common.h 抽离
 *        Created:  2024年11月28日 11时43分21秒
 */

#ifndef _RESOLUTION_H
#define _RESOLUTION_H
#ifdef __cplusplus 
extern "C" {
#endif

#include "debug.h"
#include "ot_type.h"

#define         P1080_WIDTH             1920
#define         P1080_HEIGHT            1080

typedef enum {
    VencSizeE_BEGIN    = -1,
    VencSizeE_QCIF     = 0,  // QCIF
    VencSizeE_CIF      = 1,  // CIF
    VencSizeE_D1       = 2,  // D1
    VencSizeE_720P     = 3 , // 720P
    VencSizeE_UVGA     = 4,  // UVGA
    VencSizeE_1080P    = 5,  // 1080P
    VencSizeE_QVGA     = 6,  // QVGA
    VencSizeE_VGA      = 7,  // VGA
    VencSizeE_960P     = 8,  // 960P
    VencSizeE_3M       = 9,  // 
    VencSizeE_180P     = 10, // 180P
    VencSizeE_360P     = 11, // Q720P
    VencSizeE_4M       = 12,
    VencSizeE_5M       = 13,
    vencSizeE_4M_Dahua = 14,
    VencSizeE_2M_3M    = 14,
    VencSizeE_8M       = 15,
    VencSizeE_4M_1     = 16,
    VencSizeE_END
} VencSizeE;

typedef enum {
    VideoIdxE_BEGIN   = -1,
    VideoIdxE_QCIF    = 0,
    VideoIdxE_180P    = 1,
    VideoIdxE_QVGA    = 2,
    VideoIdxE_CIF     = 3,
    VideoIdxE_360P    = 4,
    VideoIdxE_VGA     = 5,
    VideoIdxE_D1      = 6,
    VideoIdxE_NTSC    = 7,
    VideoIdxE_720P    = 8,
    VideoIdxE_WXGA    = 9,
    VideoIdxE_960P    = 10,
    VideoIdxE_SXGA    = 11,
    VideoIdxE_UVGA    = 12,
    VideoIdxE_1080P   = 13,
    VideoIdxE_3M_16_9 = 14,
    VideoIdxE_3M_4_3  = 15,
    VideoIdxE_4M      = 16,
    VideoIdxE_5M      = 17,
    VideoIdxE_8M      = 18,
    VideoIdxE_4M_1    = 19,
    VideoIdxE_MAX,
} VideoIdxE;

struct resolution {
    int        width;           // 分辨率的宽
    int        height;          // 分辨率的高
    VideoIdxE  idx;             // 
    VencSizeE  vencsize;        // 
};

extern struct resolution g_resolution[];
int array_sizeof_g_resolution();

int get_maxheight(void);
VideoIdxE encode_max_idx(int ch);
VideoIdxE encode_max_app_idx(int ch);
VideoIdxE encode_max_web_idx(int ch);
VideoIdxE encode_min_idx(int ch);

/* 
 * Translate: 
 * idx
 * vencsize
 * resolution
 * x.y ratio
 **/
VideoIdxE encode_vencsize_to_idx(VencSizeE vencsize);
VencSizeE encode_idx_to_vencsize(VideoIdxE idx);
VideoIdxE encode_resolution_to_idx(int width, int height);

int       encode_vencsize_to_resolution(VencSizeE eVencSize, int *p_width, int *p_height);
int       encode_idx_to_resolution(VideoIdxE idx, int *p_width, int *p_height);
float     encode_get_ve_x_ratio(VideoIdxE idx);
float     encode_get_ve_y_ratio(VideoIdxE idx);
float     encode_get_x_ratio(VideoIdxE idx, int x_w);
float     encode_get_y_ratio(VideoIdxE idx, int y_h);

#ifdef __cplusplus
}
#endif
#endif
