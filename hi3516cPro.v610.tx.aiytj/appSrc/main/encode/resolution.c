/* 
 *       Filename:  resolution.c
 *    Description:  分辨率相关从 encode_common.c 抽离
 *        Created:  2024年11月28日 11时43分21秒
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "resolution.h"
#include "utils.h"
#include "jconfstruct.h"
#include "confapi.h"

/* 2024-11-18 李棋回复:
 * 创建编码通道的限制: 宽不能低于256，高不能低于128
 * 因为 180P 较少用，我们重新限制最小分辨率为 QVGA
 */
struct resolution g_resolution[] = {
    {176 , 144 , VideoIdxE_QCIF   , VencSizeE_QCIF   }, //
    {320 , 240 , VideoIdxE_QVGA   , VencSizeE_QVGA   }, //
    {352 , 288 , VideoIdxE_CIF    , VencSizeE_CIF    }, //
    {640 , 360 , VideoIdxE_360P   , VencSizeE_360P   }, //
    {640 , 480 , VideoIdxE_VGA    , VencSizeE_VGA    }, //
    {704 , 576 , VideoIdxE_D1     , VencSizeE_D1     }, // NTSC 704, 480
    {1280, 720 , VideoIdxE_720P   , VencSizeE_720P   }, //
    // {1280, 960 , VideoIdxE_960P   , VencSizeE_960P   }, //
    {1920, 1080, VideoIdxE_1080P  , VencSizeE_1080P  }, //
    {2304, 1296, VideoIdxE_3M_16_9, VencSizeE_3M     }, //
    {2560, 1440, VideoIdxE_4M     , VencSizeE_4M     }, //
    {2880, 1620, VideoIdxE_5M     , VencSizeE_5M     }, //
    {3840, 2160, VideoIdxE_8M     , VencSizeE_8M     }, //
};

int array_sizeof_g_resolution()
{
    return ARRAY_SIZE(g_resolution);
}

VideoIdxE encode_vencsize_to_idx(VencSizeE vencsize)
{
    for (int i = 0; i < ARRAY_SIZE(g_resolution); i++) {
        if (vencsize == g_resolution[i].vencsize) {
            return g_resolution[i].idx;
        }
    }
    ERR("not fount vencsize: %d, use 1080P\n", vencsize);
    return VideoIdxE_1080P;
}

VencSizeE encode_idx_to_vencsize(VideoIdxE idx)
{
    for (int i = 0; i < ARRAY_SIZE(g_resolution); i++) {
        if (idx == g_resolution[i].idx) {
            return g_resolution[i].vencsize;
        }
    }
    ERR("not fount idx: %d, use 1080P\n", idx);
    return VencSizeE_1080P;
}

VideoIdxE encode_resolution_to_idx(int width, int height)
{
    for (int i = 0; i < ARRAY_SIZE(g_resolution); i++) {
        if (width == g_resolution[i].width && height == g_resolution[i].height) {
            return g_resolution[i].idx;
        }
    }

    ERR("not fount idx of: %d %d, use 1080P\n", width, height);
    return VideoIdxE_1080P;
}

int encode_vencsize_to_resolution(VencSizeE vencsize, int *p_width, int *p_height)
{
    *p_width = 1920;
    *p_height = 1080;

    for (int i = 0; i < ARRAY_SIZE(g_resolution); i++) {
        if (vencsize == g_resolution[i].vencsize) {
            *p_width = g_resolution[i].width;
            *p_height = g_resolution[i].height;
            return SUCCESS;
        }
    }
    ERR("not fount vencsize: %d, use 1080P\n", vencsize);
    return SUCCESS;
}

int encode_idx_to_resolution(VideoIdxE idx, int *p_width, int *p_height)
{
    *p_width = 1920;
    *p_height = 1080;

    for (int i = 0; i < ARRAY_SIZE(g_resolution); i++) {
        if (idx == g_resolution[i].idx) {
            *p_width = g_resolution[i].width;
            *p_height = g_resolution[i].height;
            return SUCCESS;
        }
    }
    ERR("not fount idx: %d, use 1080P\n", idx);
    return SUCCESS;
}

float encode_get_ve_x_ratio(VideoIdxE idx)
{
    float ratio = 1.0;
    int   width = 0;
    int   height = 0;

    encode_idx_to_resolution(idx, &width, &height);
    ratio = width * 1.0 / P1080_WIDTH;

    return ratio;
}

float encode_get_ve_y_ratio(VideoIdxE idx)
{
    float ratio = 1.0;
    int   width = 0;
    int   height = 0;

    encode_idx_to_resolution(idx, &width, &height);
    ratio = height * 1.0 / P1080_HEIGHT;

    return ratio;
}

float encode_get_x_ratio(VideoIdxE idx, int x_w)
{
    int   width = 0;
    int   height = 0;
    float ratio = 1.0;

    encode_idx_to_resolution(idx, &width, &height);
    ratio = width * 1.0 / x_w;
    return ratio;
}

float encode_get_y_ratio(VideoIdxE idx, int y_h)
{
    int   width = 0;
    int   height = 0;
    float ratio = 1.0;

    encode_idx_to_resolution(idx, &width, &height);
    ratio = height * 1.0 / y_h;
    return ratio;
}

int get_maxheight(void)
{
    static int hei = 0;

    if (hei != 0) {
        return hei;
    }

    int nw = LoadFile2("/ipc/etc/maxheight", "%d", &hei);

    if (nw <= 0 || hei <= 1080) {
        hei = 1080;
    }

    SYSLOG("get_maxheight %d\n", hei);

    return hei;
}

/* 2024-11-18 李棋回复:
 * 创建编码通道的限制: 宽不能低于256，高不能低于128
 * 因为 180P 较少用，我们重新限制最小分辨率为 QVGA
 *
 * 下面4个函数，对不同的 sensor 可能要做修正
 *
 */
VideoIdxE encode_max_idx(int ch)
{
    if (ch == 0) {
        static VideoIdxE max_idx = -1;
        if (max_idx != -1) {
            return max_idx;
        }

        int maxheight = get_maxheight();

        for (int i = 0; i < ARRAY_SIZE(g_resolution); i++) {
            if (maxheight == g_resolution[i].height) {
                return (max_idx = g_resolution[i].idx);
            }
        }
        return (max_idx = VideoIdxE_1080P);
    } else {
        return VideoIdxE_360P;
    }
}

VideoIdxE encode_max_app_idx(int ch)
{
    if (ch == 0) {
        static VideoIdxE max_idx = -1;
        if (max_idx != -1) {
            return max_idx;
        }
        
        Appvecfg appdev = {0};
        conf_get_appve_cfg(&appdev);
        max_idx = encode_vencsize_to_idx(appdev.appxvsz);
        
        return max_idx;
    } else {
        return VideoIdxE_360P;
    }
}

VideoIdxE encode_max_web_idx(int ch)
{
    if (ch == 0) {
        static VideoIdxE max_idx = -1;
        if (max_idx != -1) {
            return max_idx;
        }

        Appvecfg appdev = {0};
        conf_get_appve_cfg(&appdev);
        max_idx = encode_vencsize_to_idx(appdev.webxvsz);
        return max_idx;
    } else {
        return VideoIdxE_360P;
    }
}

VideoIdxE encode_min_idx(int ch) 
{
    return (ch == 0) ? VideoIdxE_720P : VideoIdxE_360P;
}

