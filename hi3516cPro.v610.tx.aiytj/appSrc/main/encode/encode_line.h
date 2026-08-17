#ifndef __ENCODE_LINE_H__
#define __ENCODE_LINE_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "ot_type.h"
#include "ot_common_video.h"

#pragma pack(push, 1)
typedef struct {
    td_u32 thick;
    td_u32 color;
    td_bool is_display;
    ot_point point1;
    ot_point point2;
} osd_line_t;
#pragma pack(pop)

typedef struct {
    ot_pixel_format pixel_format;
    td_u8 *data;
    ot_size size;
    osd_line_t *line;
    td_u32 stride; /* get canvas interface need */
    td_bool is_set_bmp; /* use set_bmp or get_canvas interface */
} encode_drawline_t;

typedef struct {
    td_s32 start_x;
    td_s32 start_y;
    td_s32 end_x;
    td_s32 end_y;
} encode_line_t;

td_s32 encode_comm_osd_drawline(encode_drawline_t *drawline_param);

#ifdef __cplusplus
}
#endif
#endif

