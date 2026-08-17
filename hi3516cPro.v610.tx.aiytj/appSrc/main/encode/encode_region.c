#include <stdio.h>
#include <string.h>

#include "ot_type.h"
#include "ot_math.h"
#include "ot_defines.h"
#include "ot_common.h"
#include "securec.h"
#include "ot_common_region.h"
#include "ot_common_aidetect.h"
#include "ss_mpi_region.h"

#include "debug.h"
#include "g_log.h"
#include "confapi.h"
#include "conf_list.h"
#include "jconfstruct.h"
#include "encode_common.h"
#include "encode_osd.h"
#include "encode_region.h"
#include "encode_line.h"

#define MAX_CHN_MASK_NUM       4
#define Vgline_Osd_Max_Block_NUM           	1
#define Vgrect_Osd_Max_Block_NUM        	4

typedef struct {
    td_u32         enable;
    ot_rgn_handle  handle;
    ot_rgn_type    type;
    ot_mpp_chn     mppchn;
} rgn_handle_t;

typedef struct {
    VideoEnc0          venc;     // 编码参数
    rgn_handle_t       mask[4];  // 画遮挡
    rgn_handle_t       rect;     // 画矩形
    rgn_handle_t       line;     // 画线
    rgn_handle_t       aidet[AIDET_OSD_MAX_NUM];   // 画矩形
} rgn_info_t;

static rgn_info_t g_region[E_OSD_GROUP_MAX] = {0};


int encode_region_get_venc(int group, VideoEnc0 *venc)
{
    memcpy(&g_region[group].venc, venc, sizeof(VideoEnc0));
    return 0;
}

int encode_region_get_min_width(ot_point points[], int n)
{
    int min_width = points[0].x;

    for (int i = 1; i < n; ++i) {
        if (points[i].x < min_width) {
            min_width = points[i].x;
        }
    }

    return min_width;
}

int encode_region_get_max_width(ot_point points[], int n)
{
    int max_width = points[0].x;

    for (int i = 1; i < n; ++i) {
        if (points[i].x > max_width) {
            max_width = points[i].x;
        }
    }

    return max_width;
}

int encode_region_get_min_height(ot_point points[], int n)
{
    int min_height = points[0].y;

    for (int i = 1; i < n; ++i) {
        if (points[i].y < min_height) {
            min_height = points[i].y;
        }
    }

    return  min_height;
}

int encode_region_get_max_height(ot_point points[], int n)
{
    int max_height = points[0].y;

    for (int i = 1; i < n; ++i) {
        if (points[i].y > max_height) {
            max_height = points[i].y;
        }
    }

    return  max_height;
}

td_s32 encode_region_drawline_get_canvas(ot_rgn_handle handle, osd_line_t *line, td_u32 line_num)
{
    td_s32 ret = 0, i = 0, mem_len = 0;
    ot_rgn_canvas_info canvas_info = {0};
    encode_drawline_t drawline_param = {0};

    do {
        ret = ss_mpi_rgn_get_canvas_info(handle, &canvas_info);
        ENCODE_RET_BREAK(ret, "ss_mpi_rgn_get_canvas_info failed\n");

        mem_len = canvas_info.stride * canvas_info.size.height;
        if (OT_PIXEL_FORMAT_ARGB_CLUT2_2X2 == canvas_info.pixel_format || OT_PIXEL_FORMAT_ARGB_CLUT4_2X2 == canvas_info.pixel_format) {
            mem_len /= 2; /* 2: 2x2: height need ÷2 */
        } else if (OT_PIXEL_FORMAT_ARGB_CLUT2_4X4 == canvas_info.pixel_format || OT_PIXEL_FORMAT_ARGB_CLUT4_4X4 == canvas_info.pixel_format) {
            mem_len /= 4; /* 4: 4x4: height need ÷4 */
        }

        (td_void)memset_s(canvas_info.virt_addr, mem_len, 0, mem_len);
        drawline_param.pixel_format = canvas_info.pixel_format;
        drawline_param.data = canvas_info.virt_addr;
        (td_void)memcpy_s(&drawline_param.size, sizeof(ot_size), &canvas_info.size, sizeof(ot_size));

        drawline_param.stride = canvas_info.stride;
        drawline_param.is_set_bmp = TD_FALSE;
        for (i = 0; i < line_num; ++i) {
            drawline_param.line = &line[i];
            ret = encode_comm_osd_drawline(&drawline_param);
            if (ret != TD_SUCCESS) {
                ERR("encode_comm_osd_drawline failed! point1(%d, %d) point2(%d, %d)\n",
                    line[i].point1.x, line[i].point1.y, line[i].point2.x, line[i].point2.y);
                break;
            }
        }

        ret = ss_mpi_rgn_update_canvas(handle);
        ENCODE_RET_BREAK(ret, "ss_mpi_rgn_get_canvas_info failed\n");
    }while(0);

    return ret;
}

int encode_osd_mask_draw(int group, rgn_handle_t *p_rgn, mask_info_t *p_mask, int show)
{
    ot_rgn_chn_attr chn_attr = {0};
    int ret = 0;
    float x_ratio = encode_osd_video_get_x_ratio(group);
    float y_ratio = encode_osd_video_get_y_ratio(group);
    ot_rect_cover *p_cover = &chn_attr.attr.cover_chn.cover.rect_attr;
    ot_rect *p_rect = &chn_attr.attr.cover_chn.cover.rect_attr.rect;

    ret = ss_mpi_rgn_get_chn_display_attr(p_rgn->handle, &p_rgn->mppchn, &chn_attr);
    ENCODE_RET_JUDGE(ret);

    chn_attr.is_show = show;
    p_cover->is_solid = TD_TRUE;
    chn_attr.attr.cover_chn.cover.color = p_mask->color;

    if (chn_attr.is_show) {
        p_rect->x = ENC_GET2MULTIPLE((int)(p_mask->x * x_ratio));
        p_rect->y = ENC_GET2MULTIPLE((int)(p_mask->y * y_ratio));
        p_rect->width = ENC_GET2MULTIPLE((int)(p_mask->width * x_ratio));
        p_rect->height = ENC_GET2MULTIPLE((int)(p_mask->height * y_ratio));

        dbg_osd("group%d mask, x:%d, y:%d, width:%d, height:%d\n",
                group, p_rect->x, p_rect->y, p_rect->width, p_rect->height);
    }

    ret = ss_mpi_rgn_set_chn_display_attr(p_rgn->handle, &p_rgn->mppchn, &chn_attr);
    ENCODE_RET_JUDGE(ret);

    return ret;
}

int encode_osd_cover_draw(int group, rgn_handle_t *region_handle, aidet_info_t *prect, int show)
{
    int ret = 0;
    ot_rgn_chn_attr chn_attr = {0};

    ret = ss_mpi_rgn_get_chn_display_attr(region_handle->handle, &region_handle->mppchn, &chn_attr);
    ENCODE_RET_JUDGE(ret);

    chn_attr.is_show = show;
    chn_attr.attr.cover_chn.cover.rect_attr.is_solid = TD_FALSE;

    if(OT_AIDETECT_CLASS_HUMAN == prect->aidetect_class) {
        chn_attr.attr.cover_chn.cover.color = OSD_COLOR_RED;
    } else if (OT_AIDETECT_CLASS_VEHICLE == prect->aidetect_class) {
        chn_attr.attr.cover_chn.cover.color = OSD_COLOR_GREEN;
    } else if (OT_AIDETECT_CLASS_PET == prect->aidetect_class) {
        chn_attr.attr.cover_chn.cover.color = OSD_COLOR_BLUE;
    }

    if(chn_attr.is_show) {
        chn_attr.attr.cover_chn.cover.rect_attr.rect.x = ENC_GET2MULTIPLE((int)(prect->detect_rect.x * encode_osd_draw_get_x_ratio(group)));
        chn_attr.attr.cover_chn.cover.rect_attr.rect.y = ENC_GET2MULTIPLE((int)(prect->detect_rect.y * encode_osd_draw_get_y_ratio(group)));
        chn_attr.attr.cover_chn.cover.rect_attr.rect.width = ENC_GET2MULTIPLE((int)(prect->detect_rect.width * encode_osd_draw_get_x_ratio(group)));
        chn_attr.attr.cover_chn.cover.rect_attr.rect.height = ENC_GET2MULTIPLE((int)(prect->detect_rect.height * encode_osd_draw_get_y_ratio(group)));
        dbg_hd("x:%d, y:%d, width:%d height:%d\n",
            chn_attr.attr.cover_chn.cover.rect_attr.rect.x,
            chn_attr.attr.cover_chn.cover.rect_attr.rect.y,
            chn_attr.attr.cover_chn.cover.rect_attr.rect.width,
            chn_attr.attr.cover_chn.cover.rect_attr.rect.height);
    }
    ret = ss_mpi_rgn_set_chn_display_attr(region_handle->handle, &region_handle->mppchn, &chn_attr);
    ENCODE_RET_JUDGE(ret);

    return ret;
}

int encode_osd_aidet_draw(osd_aidet_t *aidet_info)
{
    int ret = 0;
    int count = 0;
    int group = 0;

    rgn_handle_t* pAidet = NULL;
    int is_show = TD_FALSE;

    for (group = 0; group < E_OSD_GROUP_MAX; group++) {
        for (count = 0; count < AIDET_OSD_MAX_NUM; count++) {
            pAidet = &g_region[group].aidet[count];
            if (!pAidet->enable) {
                continue;
            }

            //隐私遮挡时，设备无需算法，所以将其中两个 handle 复用为实心色块，其余画线则隐藏
            if (!aidet_info->mask.enable || count > 0) {
                if (aidet_info->aidet_info[count].screen_enable &&
                    count < aidet_info->object_num && !aidet_info->mask.enable) {
                    is_show = TD_TRUE;
                } else {
                    is_show = TD_FALSE;
                }

                ret = encode_osd_cover_draw(group, pAidet,
                                            &aidet_info->aidet_info[count], is_show);
                ENCODE_RET_JUDGE(ret);
            } else {
                ret = encode_osd_mask_draw(group, pAidet, &aidet_info->mask,
                                           aidet_info->mask.enable);
                ENCODE_RET_JUDGE(ret);
            }
        }
    }

    return ret;
}

int encode_osd_cover_init(int group, int handle, int num)
{
    int ret = 0;
    ot_rgn_attr rgn_attr = {0};
    ot_rgn_chn_attr chn_attr = {0};

    rgn_handle_t* pAidet = &g_region[group].aidet[num];

    if(pAidet->enable) {
        return ret;
    }

    memset(&rgn_attr, 0, sizeof(ot_rgn_attr));
    memset(&chn_attr, 0, sizeof(ot_rgn_chn_attr));

    rgn_attr.type = OT_RGN_COVER;

    pAidet->handle = handle;
    pAidet->type = OT_RGN_COVER;
    pAidet->mppchn.mod_id = OT_ID_VPSS;
    pAidet->mppchn.dev_id = 0;
    pAidet->mppchn.chn_id = group;
    pAidet->enable = TD_TRUE;

    chn_attr.is_show = TD_FALSE;
    chn_attr.type = OT_RGN_COVER;
    chn_attr.attr.cover_chn.coord = OT_COORD_ABS;
    chn_attr.attr.cover_chn.layer = group;

    chn_attr.attr.cover_chn.cover.color = OSD_COLOR_RED;
    chn_attr.attr.cover_chn.cover.type = OT_COVER_RECT;
    chn_attr.attr.cover_chn.cover.rect_attr.is_solid = TD_FALSE;

    if(E_OSD_GROUP_MAIN == group) {
        chn_attr.attr.cover_chn.cover.rect_attr.thick = 2;
    } else {
        chn_attr.attr.cover_chn.cover.rect_attr.thick = 2;
    }

    // 创建必须赋值不然创建失败
    chn_attr.attr.cover_chn.cover.rect_attr.rect.x = 20;
    chn_attr.attr.cover_chn.cover.rect_attr.rect.y = 20;
    chn_attr.attr.cover_chn.cover.rect_attr.rect.width = 20;
    chn_attr.attr.cover_chn.cover.rect_attr.rect.height = 20;

    ret = ss_mpi_rgn_create(pAidet->handle, &rgn_attr);
    ENCODE_RET_JUDGE(ret);

    ret = ss_mpi_rgn_attach_to_chn(pAidet->handle, &pAidet->mppchn, &chn_attr);
    ENCODE_RET_JUDGE(ret);

    return ret;
}

int encode_osd_aidet_group_init(int group)
{
    int ret = S_OK;
    int i = 0;
    int handle = E_AIDET_MAIN_HANDLE;
    for(i = 0; i < AIDET_OSD_MAX_NUM; i++) {
        handle = E_AIDET_MAIN_HANDLE + i + AIDET_OSD_MAX_NUM*group ;
        ret = encode_osd_cover_init(group, handle, i);
        ENCODE_RET_JUDGE(ret);
    }
    return ret;
}

int encode_osd_aidet_group_uninit(int group)
{
    int i = 0;
    int ret = S_OK;
    rgn_handle_t* pAidet = NULL;

    for(i = 0; i < AIDET_OSD_MAX_NUM; i++) {
        pAidet = &g_region[group].aidet[i];

        if(!pAidet->enable) {
            continue;
        }

        ret = ss_mpi_rgn_detach_from_chn(pAidet->handle, &pAidet->mppchn);
        ENCODE_RET_JUDGE(ret);

        ret = ss_mpi_rgn_destroy(pAidet->handle);
        ENCODE_RET_JUDGE(ret);

        pAidet->enable = TD_FALSE;
    }

    return ret;
}

int encode_osd_vgrect_group_init(int group, VgrectS *vgRectcfg)
{
    int ret = 0;
    int thick = 0;
    int width = 0;
    int height = 0;

    ot_rgn_attr rgn_attr = {0};
    ot_rgn_chn_attr chn_attr = {0};

    osd_line_t line_info[Vgrect_Osd_Max_Block_NUM] = {0};
    ot_point point_info[Vgrect_Osd_Max_Block_NUM] = {0};

    rgn_handle_t* pRect = &g_region[group].rect;

    if(pRect->enable) {
        return ret;
    }

    if(E_OSD_GROUP_MAIN == group) {
        thick = 4;
    } else {
        thick = 2;
    }

    encode_vencsize_to_resolution(g_region[group].venc.vencsize, &width, &height);

    memset(&rgn_attr, 0, sizeof(ot_rgn_attr));
    memset(&chn_attr, 0, sizeof(ot_rgn_chn_attr));

    rgn_attr.type = OT_RGN_OVERLAY;

    pRect->handle = (int)(E_VGRECT_MAIN_HANDLE + group);
    pRect->type = OT_RGN_OVERLAY;
    pRect->mppchn.mod_id = OT_ID_VENC;
    pRect->mppchn.dev_id = 0;
    pRect->mppchn.chn_id = group;

    point_info[0].x = ENC_GET4MULTIPLE((int)(vgRectcfg->x0*encode_osd_video_get_x_ratio(group)));
    point_info[0].y = ENC_GET4MULTIPLE((int)(vgRectcfg->y0*encode_osd_video_get_y_ratio(group)));
    point_info[1].x = ENC_GET4MULTIPLE((int)(vgRectcfg->x1*encode_osd_video_get_x_ratio(group)));
    point_info[1].y = ENC_GET4MULTIPLE((int)(vgRectcfg->y1*encode_osd_video_get_y_ratio(group)));
    point_info[2].x = ENC_GET4MULTIPLE((int)(vgRectcfg->x2*encode_osd_video_get_x_ratio(group)));
    point_info[2].y = ENC_GET4MULTIPLE((int)(vgRectcfg->y2*encode_osd_video_get_y_ratio(group)));
    point_info[3].x = ENC_GET4MULTIPLE((int)(vgRectcfg->x3*encode_osd_video_get_x_ratio(group)));
    point_info[3].y = ENC_GET4MULTIPLE((int)(vgRectcfg->y3*encode_osd_video_get_y_ratio(group)));

    int max_width = encode_region_get_max_width(point_info, Vgrect_Osd_Max_Block_NUM);
    int min_width = encode_region_get_min_width(point_info, Vgrect_Osd_Max_Block_NUM);
    int max_height = encode_region_get_max_height(point_info, Vgrect_Osd_Max_Block_NUM);
    int min_height = encode_region_get_min_height(point_info, Vgrect_Osd_Max_Block_NUM);

    for(int i =0; i < Vgrect_Osd_Max_Block_NUM; i++) {
        point_info[i].x -= min_width;
        point_info[i].y -= min_height;
    }

    rgn_attr.type = OT_RGN_OVERLAY;
    rgn_attr.attr.overlay.canvas_num = 1;//设置成2刷新的时候概率闪一下花屏
    rgn_attr.attr.overlay.bg_color = 0;
    rgn_attr.attr.overlay.clut[0] = COLOR_ARGB_TRANSPARENT;
    rgn_attr.attr.overlay.clut[1] = COLOR_ARGB_WHITE;
    rgn_attr.attr.overlay.clut[2] = COLOR_ARGB_BLACK;
    rgn_attr.attr.overlay.clut[3] = COLOR_ARGB_RED;
    rgn_attr.attr.overlay.clut[4] = COLOR_ARGB_GREEN;

    // rgn_attr.attr.overlay.clut[0] = 0x00ffffff;
    // rgn_attr.attr.overlay.clut[1] = 0xFF00FF00;
    // rgn_attr.attr.overlay.clut[2] = 0xFFFF0000;
    // rgn_attr.attr.overlay.clut[3] = 0x00000000;

    rgn_attr.attr.overlay.pixel_format = OT_PIXEL_FORMAT_ARGB_CLUT4;
    rgn_attr.attr.overlay.size.width = ENC_GET4MULTIPLE(max_width - min_width);
    rgn_attr.attr.overlay.size.height = ENC_GET4MULTIPLE(max_height - min_height);

    if(rgn_attr.attr.overlay.size.width > (width - min_width - thick)) {
        rgn_attr.attr.overlay.size.width = ENC_GET4MULTIPLE((width - min_width - thick));
    }

    if(rgn_attr.attr.overlay.size.height > (height - min_height - thick)) {
        rgn_attr.attr.overlay.size.height = ENC_GET4MULTIPLE((height - min_height - thick));
    }

    for(int i = 0; i < Vgrect_Osd_Max_Block_NUM; i++) {
        if(point_info[i].x >= (rgn_attr.attr.overlay.size.width - thick * 3)) {
            point_info[i].x = rgn_attr.attr.overlay.size.width - thick * 3;
        }

        if(point_info[i].y >= (rgn_attr.attr.overlay.size.height - thick * 3)) {
            point_info[i].y = rgn_attr.attr.overlay.size.height - thick * 3;
        }
    }

    chn_attr.is_show = TD_TRUE;
    chn_attr.type = OT_RGN_OVERLAY;
    chn_attr.attr.overlay_chn.bg_alpha = 0;
    chn_attr.attr.overlay_chn.fg_alpha = 255;
    chn_attr.attr.overlay_chn.qp_info.enable = TD_FALSE;
    chn_attr.attr.overlay_chn.qp_info.is_abs_qp = TD_TRUE;
    chn_attr.attr.overlay_chn.qp_info.qp_val = 30;
    chn_attr.attr.overlay_chn.dst = OT_RGN_ATTACH_JPEG_MAIN;
    chn_attr.attr.overlay_chn.point.x = min_width;
    chn_attr.attr.overlay_chn.point.y = min_height;
    chn_attr.attr.overlay_chn.layer = group;

    for (int i = 0; i < Vgrect_Osd_Max_Block_NUM; ++i) {
        line_info[i].point1.x = ENC_GET4MULTIPLE(point_info[i].x);
        line_info[i].point1.y = ENC_GET4MULTIPLE(point_info[i].y);
        line_info[i].point2.x = ENC_GET4MULTIPLE(point_info[(i + 1) % Vgrect_Osd_Max_Block_NUM].x);
        line_info[i].point2.y = ENC_GET4MULTIPLE(point_info[(i + 1) % Vgrect_Osd_Max_Block_NUM].y);
        line_info[i].color = 4;

        if (vgRectcfg->blink) {
            line_info[i].is_display = TD_TRUE;
        } else {
            line_info[i].is_display = TD_FALSE;
        }

        line_info[i].thick = thick;
    }

    ret = ss_mpi_rgn_create(pRect->handle, &rgn_attr);
    ENCODE_RET_JUDGE(ret);

    ret = ss_mpi_rgn_attach_to_chn(pRect->handle, &pRect->mppchn, &chn_attr);
    ENCODE_RET_JUDGE(ret);

    ret = encode_region_drawline_get_canvas(pRect->handle, line_info, Vgrect_Osd_Max_Block_NUM);
    ENCODE_RET_JUDGE(ret);

    pRect->enable = TD_TRUE;

    return ret;
}

int encode_osd_vgrect_update(int group, VgrectS *vgRectcfg, int blink_cnt, int is_show)
{
    int ret = 0;
    int thick = 0;
    osd_line_t line_info[Vgrect_Osd_Max_Block_NUM] = {0};
    ot_point point_info[Vgrect_Osd_Max_Block_NUM] = {0};

    rgn_handle_t* pRect = &g_region[group].rect;

    if(E_OSD_GROUP_MAIN == group) {
        thick = 4;
    } else {
        thick = 2;
    }

    point_info[0].x = ENC_GET4MULTIPLE((int)(vgRectcfg->x0*encode_osd_video_get_x_ratio(group)));
    point_info[0].y = ENC_GET4MULTIPLE((int)(vgRectcfg->y0*encode_osd_video_get_y_ratio(group)));
    point_info[1].x = ENC_GET4MULTIPLE((int)(vgRectcfg->x1*encode_osd_video_get_x_ratio(group)));
    point_info[1].y = ENC_GET4MULTIPLE((int)(vgRectcfg->y1*encode_osd_video_get_y_ratio(group)));
    point_info[2].x = ENC_GET4MULTIPLE((int)(vgRectcfg->x2*encode_osd_video_get_x_ratio(group)));
    point_info[2].y = ENC_GET4MULTIPLE((int)(vgRectcfg->y2*encode_osd_video_get_y_ratio(group)));
    point_info[3].x = ENC_GET4MULTIPLE((int)(vgRectcfg->x3*encode_osd_video_get_x_ratio(group)));
    point_info[3].y = ENC_GET4MULTIPLE((int)(vgRectcfg->y3*encode_osd_video_get_y_ratio(group)));

    int min_width = encode_region_get_min_width(point_info, Vgrect_Osd_Max_Block_NUM);
    int min_height = encode_region_get_min_height(point_info, Vgrect_Osd_Max_Block_NUM);

    for(int i =0; i < Vgrect_Osd_Max_Block_NUM; i++) {
        point_info[i].x -= min_width;
        point_info[i].y -= min_height;
    }

    ot_rgn_attr rgn_attr = {0};
    ret = ss_mpi_rgn_get_attr(pRect->handle, &rgn_attr);
    if (ret != TD_SUCCESS) {
        ERR("Failed to get rgn attr\n");
        return ret;
    }

    unsigned int canvas_w = rgn_attr.attr.overlay.size.width;
    unsigned int canvas_h = rgn_attr.attr.overlay.size.height;

    for (int i = 0; i < Vgrect_Osd_Max_Block_NUM; i++) {
        if (point_info[i].x >= (int)(canvas_w - thick * 3)) {
            point_info[i].x = canvas_w - thick * 3;
        }
        if (point_info[i].y >= (int)(canvas_h - thick * 3)) {
            point_info[i].y = canvas_h - thick * 3;
        }
    }

    for (int i = 0; i < Vgrect_Osd_Max_Block_NUM; ++i) {
        line_info[i].point1.x = ENC_GET4MULTIPLE(point_info[i].x);
        line_info[i].point1.y = ENC_GET4MULTIPLE(point_info[i].y);
        line_info[i].point2.x = ENC_GET4MULTIPLE(point_info[(i + 1) % Vgrect_Osd_Max_Block_NUM].x);
        line_info[i].point2.y = ENC_GET4MULTIPLE(point_info[(i + 1) % Vgrect_Osd_Max_Block_NUM].y);
        if (blink_cnt > 0) {
            line_info[i].color = (blink_cnt % 2) == 0 ? 4 : 3;
            line_info[i].is_display = TD_TRUE;
        } else {
            if (vgRectcfg->blink) {
                line_info[i].is_display = is_show;
            } else {
                line_info[i].is_display = TD_FALSE;
            }
            line_info[i].color = 4;
        }
        line_info[i].thick = thick;
    }

    ret = encode_region_drawline_get_canvas(pRect->handle, line_info, Vgrect_Osd_Max_Block_NUM);
    ENCODE_RET_JUDGE(ret);

    return ret;
}

int encode_osd_vgrect_group_uninit(int group)
{
    int ret = 0;
    rgn_handle_t* pRect = NULL;

    do {
        pRect = &g_region[group].rect;

        if(!pRect->enable) {
            break;
        }

        ret = ss_mpi_rgn_detach_from_chn(pRect->handle, &pRect->mppchn);
        ENCODE_RET_JUDGE(ret);

        ret = ss_mpi_rgn_destroy(pRect->handle);
        ENCODE_RET_JUDGE(ret);

        pRect->enable = TD_FALSE;

    } while(0);

    return ret;
}

int encode_osd_vgline_group_init(int group, VglineS *vgLinecfg)
{
    int ret = 0;
    int thick = 0;
    int width = 0;
    int height = 0;
    rgn_handle_t* pLine = NULL;
    ot_rgn_attr rgn_attr = {0};
    ot_rgn_chn_attr chn_attr = {0};
    osd_line_t line_info[Vgrect_Osd_Max_Block_NUM] = {0};
    ot_point point_info[Vgrect_Osd_Max_Block_NUM] = {0};

    pLine = &g_region[group].line;

    if(pLine->enable) {
        return ret;
    }

    if(E_OSD_GROUP_MAIN == group) {
        thick = 4;
    } else {
        thick = 2;
    }

    memset(&rgn_attr, 0, sizeof(ot_rgn_attr));
    memset(&chn_attr, 0, sizeof(ot_rgn_chn_attr));

    rgn_attr.type = OT_RGN_OVERLAY;

    pLine->handle = (int)(E_VGLINE_MAIN_HANDLE + group);
    pLine->type = OT_RGN_OVERLAY;
    pLine->mppchn.mod_id = OT_ID_VENC;
    pLine->mppchn.dev_id = 0;
    pLine->mppchn.chn_id = group;
    pLine->enable = TD_TRUE;

    encode_vencsize_to_resolution(g_region[group].venc.vencsize, &width, &height);

    point_info[0].x = ENC_GET4MULTIPLE((int)(vgLinecfg->x0 * encode_osd_video_get_x_ratio(group)));
    point_info[0].y = ENC_GET4MULTIPLE((int)(vgLinecfg->y0 * encode_osd_video_get_y_ratio(group)));
    point_info[1].x = ENC_GET4MULTIPLE((int)(vgLinecfg->x1 * encode_osd_video_get_x_ratio(group)));
    point_info[1].y = ENC_GET4MULTIPLE((int)(vgLinecfg->y1 * encode_osd_video_get_y_ratio(group)));

    int max_width = encode_region_get_max_width(point_info, 2);
    int min_width = encode_region_get_min_width(point_info, 2);
    int max_height = encode_region_get_max_height(point_info, 2);
    int min_height = encode_region_get_min_height(point_info, 2);

    for(int i = 0; i < 2; i++) {
        point_info[i].x -= min_width;
        point_info[i].y -= min_height;
    }

    rgn_attr.type = OT_RGN_OVERLAY;
    rgn_attr.attr.overlay.canvas_num = 1;// 设置为2会导致概率花屏
    rgn_attr.attr.overlay.bg_color = 0;
    rgn_attr.attr.overlay.clut[0] = COLOR_ARGB_TRANSPARENT;
    rgn_attr.attr.overlay.clut[1] = COLOR_ARGB_WHITE;
    rgn_attr.attr.overlay.clut[2] = COLOR_ARGB_BLACK;
    rgn_attr.attr.overlay.clut[3] = COLOR_ARGB_RED;
    rgn_attr.attr.overlay.clut[4] = COLOR_ARGB_GREEN;
    // rgn_attr.attr.overlay.clut[0] = 0x00ffffff;
    // rgn_attr.attr.overlay.clut[1] = 0xFF00FF00;
    // rgn_attr.attr.overlay.clut[2] = 0xFFFF0000;
    // rgn_attr.attr.overlay.clut[3] = 0x00000000;
    rgn_attr.attr.overlay.pixel_format = OT_PIXEL_FORMAT_ARGB_CLUT4;

    int min_required_w = thick * 4;
    if ((max_width - min_width) > min_required_w) {
        min_required_w = max_width - min_width;
    }
    rgn_attr.attr.overlay.size.width = ENC_GET4MULTIPLE(min_required_w);
    rgn_attr.attr.overlay.size.height = ENC_GET4MULTIPLE(max_height - min_height);

    rgn_attr.attr.overlay.size.width = rgn_attr.attr.overlay.size.width < 4 ? 4 : rgn_attr.attr.overlay.size.width;
    rgn_attr.attr.overlay.size.height = rgn_attr.attr.overlay.size.height < 4 ? 4 : rgn_attr.attr.overlay.size.height;

    if(rgn_attr.attr.overlay.size.width > (width - min_width - thick)) {
        rgn_attr.attr.overlay.size.width = ENC_GET4MULTIPLE(width - min_width - thick);
    }

    if(rgn_attr.attr.overlay.size.height > (height - min_height - thick)) {
        rgn_attr.attr.overlay.size.height = ENC_GET4MULTIPLE(height - min_height - thick);
    }

    for(int i = 0; i < 2; i++) {
        if(point_info[i].x >= (rgn_attr.attr.overlay.size.width - thick * 3)) {
            point_info[i].x = rgn_attr.attr.overlay.size.width - thick * 3;
        }

        if(point_info[i].y >= (rgn_attr.attr.overlay.size.height - thick * 3)) {
            point_info[i].y = rgn_attr.attr.overlay.size.height - thick * 3;
        }
    }

    chn_attr.is_show = TD_TRUE;
    chn_attr.type = OT_RGN_OVERLAY;
    chn_attr.attr.overlay_chn.bg_alpha = 0;
    chn_attr.attr.overlay_chn.fg_alpha = 255;
    chn_attr.attr.overlay_chn.qp_info.enable = TD_FALSE;
    chn_attr.attr.overlay_chn.qp_info.is_abs_qp = TD_TRUE;
    chn_attr.attr.overlay_chn.qp_info.qp_val = 30;
    chn_attr.attr.overlay_chn.dst = OT_RGN_ATTACH_JPEG_MAIN;
    chn_attr.attr.overlay_chn.point.x = min_width;
    chn_attr.attr.overlay_chn.point.y = min_height;
    chn_attr.attr.overlay_chn.layer = group;

    ret = ss_mpi_rgn_create(pLine->handle, &rgn_attr);
    ENCODE_RET_JUDGE(ret);

    ret = ss_mpi_rgn_attach_to_chn(pLine->handle, &pLine->mppchn, &chn_attr);
    ENCODE_RET_JUDGE(ret);

    line_info[0].point1.x = point_info[0].x;
    line_info[0].point1.y = point_info[0].y;
    line_info[0].point2.x = point_info[1].x;
    line_info[0].point2.y = point_info[1].y;
    line_info[0].thick = thick;
    line_info[0].color = 4;

    if (vgLinecfg->blink) {
        line_info[0].is_display = TD_TRUE;
    } else {
        line_info[0].is_display = TD_FALSE;
    }

    ret = encode_region_drawline_get_canvas(pLine->handle, line_info, Vgline_Osd_Max_Block_NUM);
    ENCODE_RET_JUDGE(ret);

    return ret;
}

int encode_osd_vgline_update(int group, VglineS *vgLinecfg, int blink_cnt, int is_show)
{
    int ret = 0;
    int thick = 0;
    rgn_handle_t* pLine = NULL;
    osd_line_t line_info[Vgrect_Osd_Max_Block_NUM] = {0};
    ot_point point_info[Vgrect_Osd_Max_Block_NUM] = {0};

    pLine = &g_region[group].line;

    if(E_OSD_GROUP_MAIN == group) {
        thick = 4;
    } else {
        thick = 2;
    }

    point_info[0].x = ENC_GET4MULTIPLE((int)(vgLinecfg->x0 * encode_osd_video_get_x_ratio(group)));
    point_info[0].y = ENC_GET4MULTIPLE((int)(vgLinecfg->y0 * encode_osd_video_get_y_ratio(group)));
    point_info[1].x = ENC_GET4MULTIPLE((int)(vgLinecfg->x1 * encode_osd_video_get_x_ratio(group)));
    point_info[1].y = ENC_GET4MULTIPLE((int)(vgLinecfg->y1 * encode_osd_video_get_y_ratio(group)));

    int min_width = encode_region_get_min_width(point_info, 2);
    int min_height = encode_region_get_min_height(point_info, 2);

    for(int i =0; i < 2; i++) {
        point_info[i].x -= min_width;
        point_info[i].y -= min_height;
    }

    ot_rgn_attr rgn_attr = {0};
    ret = ss_mpi_rgn_get_attr(pLine->handle, &rgn_attr);
    if (ret != TD_SUCCESS) {
        ERR("Failed to get rgn attr\n");
        return ret;
    }

    for(int i = 0; i < 2; i++) {
        if(point_info[i].x >= (rgn_attr.attr.overlay.size.width - thick * 3)) {
            point_info[i].x = rgn_attr.attr.overlay.size.width - thick * 3;
        }

        if(point_info[i].y >= (rgn_attr.attr.overlay.size.height - thick * 3)) {
            point_info[i].y = rgn_attr.attr.overlay.size.height - thick * 3;
        }
    }

    line_info[0].point1.x = point_info[0].x;
    line_info[0].point1.y = point_info[0].y;
    line_info[0].point2.x = point_info[1].x;
    line_info[0].point2.y = point_info[1].y;
    line_info[0].thick = thick;

    if (blink_cnt > 0) {
        line_info[0].color = (blink_cnt % 2) == 0 ? 4 : 3;
        line_info[0].is_display = TD_TRUE;
    } else {
        if (vgLinecfg->blink) {
            line_info[0].is_display = is_show;
        } else {
            line_info[0].is_display = TD_FALSE;
        }
        line_info[0].color = 4;
    }

    ret = encode_region_drawline_get_canvas(pLine->handle, line_info, Vgline_Osd_Max_Block_NUM);
    ENCODE_RET_JUDGE(ret);

    return ret;
}

int encode_osd_vgline_group_uninit(int group)
{
    int ret = 0;
    rgn_handle_t* pLine = NULL;

    do {
        pLine = &g_region[group].line;

        if(!pLine->enable) {
            break;
        }

        ret = ss_mpi_rgn_detach_from_chn(pLine->handle, &pLine->mppchn);
        ENCODE_RET_JUDGE(ret);

        ret = ss_mpi_rgn_destroy(pLine->handle);
        ENCODE_RET_JUDGE(ret);

        pLine->enable = TD_FALSE;

    } while(0);

    return ret;
}

int encode_osd_vgline_init(void)
{
    int ret = 0;

    VglineS vgLinecfg = {0};
    get_config(handleVglineCfg, vgLinecfg);

    for (int group = 0; group < E_OSD_GROUP_MAX; group++) {
        encode_osd_vgline_group_init(group, &vgLinecfg);
        ENCODE_RET_JUDGE(ret);
    }

    return ret;
}

int encode_osd_vgline_uninit(void)
{
    int ret = 0;

    for (int group = 0; group < E_OSD_GROUP_MAX; group++) {
        ret = encode_osd_vgline_group_uninit(group);
        ENCODE_RET_JUDGE(ret);
    }

    return ret;
}

int encode_osd_vgrect_init(void)
{
    int ret = 0;

    VgrectS vgRectcfg = {0};
    get_config(handleVgrectCfg, vgRectcfg);

    for (int group = 0; group < E_OSD_GROUP_MAX; group++) {
        ret = encode_osd_vgrect_group_init(group, &vgRectcfg);
        ENCODE_RET_JUDGE(ret);
    }

    return ret;
}

int encode_osd_vgrect_uninit(void)
{
    int ret = 0;

    for (int group = 0; group < E_OSD_GROUP_MAX; group++) {
        ret = encode_osd_vgrect_group_uninit(group);
        ENCODE_RET_JUDGE(ret);
    }

    return ret;
}

int encode_osd_aidet_init(void)
{
    int ret = S_OK;

    for (int group = 0; group < E_OSD_GROUP_MAX; group++) {
        ret = encode_osd_aidet_group_init(group);
        ENCODE_RET_JUDGE(ret);
    }
    return ret;
}

int encode_osd_aidet_uninit(void)
{
    int ret = S_OK;

    for (int group = 0; group < E_OSD_GROUP_MAX; group++) {
        ret = encode_osd_aidet_group_uninit(group);
        ENCODE_RET_JUDGE(ret);
    }

    return ret;
}
