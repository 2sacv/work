#ifndef __ENCODE_SCENE_ISP_H__
#define __ENCODE_SCENE_ISP_H__
#ifdef __cplusplus
extern "C" {
#endif

#define FPN_FILE_NAME_LENGTH 150

typedef struct {
    ot_size          size;
    ot_pixel_format  pixel_format;
    ot_video_format  video_format;
    ot_compress_mode compress_mode;
    ot_dynamic_range dynamic_range;
} get_frame_vb_cfg;

typedef struct {
    ot_vb_blk           vb_blk;
    td_u32              blk_size;
    ot_video_frame_info frame_info;
} user_frame_info;

typedef struct {
    ot_op_mode                op_mode;
    td_bool                   aibnr_mode;
    ot_isp_fpn_type           fpn_type;
    td_u32                    strength;
    ot_pixel_format           pixel_format;
    ot_compress_mode          compress_mode;
    user_frame_info           user_frame_info;
} fpn_correction_cfg;

typedef struct {
    td_u32           threshold;
    td_u32           frame_num;
    ot_isp_fpn_type  fpn_type;
    ot_pixel_format  pixel_format;
    ot_compress_mode compress_mode;
} fpn_calibration_cfg;

td_s32 encode_scene_disable_fpn_correction(ot_vi_pipe vi_pipe, fpn_correction_cfg *correction_cfg);
td_s32 encode_scene_enable_fpn_correction(ot_vi_pipe vi_pipe, fpn_correction_cfg *correction_cfg);


#ifdef __cplusplus
}
#endif
#endif

