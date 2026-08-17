//sample_comm_vi.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <unistd.h>
#include <sys/prctl.h>

#include "ot_buffer.h"
#include "securec.h"

#include "ss_mpi_sys.h"
#include "ss_mpi_vi.h"
#include "encode_vi.h"
#include "ss_mpi_vb.h"
#include "ss_mpi_isp.h"

#include "ss_mpi_sys_mem.h"

#include "debug.h"
#include "encode_vi_isp.h"
#include "encode_common.h"
#include "encode_scene_isp.h"

#define PATH_MAX 256

static td_void encode_scene_get_vb_calc_cfg(get_frame_vb_cfg *get_frame_vb_cfg, ot_vb_calc_cfg *calc_cfg)
{
    ot_pic_buf_attr buf_attr;

    buf_attr.width         = get_frame_vb_cfg->size.width;
    buf_attr.height        = get_frame_vb_cfg->size.height;
    buf_attr.align         = OT_DEFAULT_ALIGN;
    buf_attr.bit_width     =
        (get_frame_vb_cfg->dynamic_range == OT_DYNAMIC_RANGE_SDR8) ? OT_DATA_BIT_WIDTH_8 : OT_DATA_BIT_WIDTH_10;
    buf_attr.pixel_format  = get_frame_vb_cfg->pixel_format;
    buf_attr.compress_mode = get_frame_vb_cfg->compress_mode;
    buf_attr.video_format  = get_frame_vb_cfg->video_format;

    ot_common_get_pic_buf_cfg(&buf_attr, calc_cfg);
}

static td_s32 encode_scene_malloc_frame_blk(ot_vb_pool pool_id,
                                              get_frame_vb_cfg *get_frame_vb_cfg, ot_vb_calc_cfg *calc_cfg,
                                              user_frame_info *user_frame_info)
{
    ot_vb_blk vb_blk;
    td_phys_addr_t phys_addr;
    td_void *virt_addr = TD_NULL;
    ot_video_frame_info *frame_info = TD_NULL;

    vb_blk = ss_mpi_vb_get_blk(pool_id, calc_cfg->vb_size, TD_NULL);
    if (vb_blk == OT_VB_INVALID_HANDLE) {
        ERR("ss_mpi_vb_get_blk err, size:%u\n", calc_cfg->vb_size);
        return TD_FAILURE;
    }

    phys_addr = ss_mpi_vb_handle_to_phys_addr(vb_blk);
    virt_addr = (td_u8 *)ss_mpi_sys_mmap(phys_addr, calc_cfg->vb_size);
    if (virt_addr == TD_NULL) {
        ERR("ss_mpi_sys_mmap err!\n");
        ss_mpi_vb_release_blk(vb_blk);
        return TD_FAILURE;
    }

    user_frame_info->vb_blk   = vb_blk;
    user_frame_info->blk_size = calc_cfg->vb_size;

    frame_info = &user_frame_info->frame_info;

    frame_info->pool_id                   = pool_id;
    frame_info->mod_id                    = OT_ID_VI;
    frame_info->video_frame.phys_addr[0]  = phys_addr;
    frame_info->video_frame.phys_addr[1]  = frame_info->video_frame.phys_addr[0] + calc_cfg->main_y_size;
    frame_info->video_frame.virt_addr[0]  = virt_addr;
    frame_info->video_frame.virt_addr[1]  = frame_info->video_frame.virt_addr[0] + calc_cfg->main_y_size;
    frame_info->video_frame.stride[0]     = calc_cfg->main_stride;
    frame_info->video_frame.stride[1]     = calc_cfg->main_stride;
    frame_info->video_frame.width         = get_frame_vb_cfg->size.width;
    frame_info->video_frame.height        = get_frame_vb_cfg->size.height;
    frame_info->video_frame.pixel_format  = get_frame_vb_cfg->pixel_format;
    frame_info->video_frame.video_format  = get_frame_vb_cfg->video_format;
    frame_info->video_frame.compress_mode = get_frame_vb_cfg->compress_mode;
    frame_info->video_frame.dynamic_range = get_frame_vb_cfg->dynamic_range;
    frame_info->video_frame.field         = OT_VIDEO_FIELD_FRAME;
    frame_info->video_frame.color_gamut   = OT_COLOR_GAMUT_BT601;

    return TD_SUCCESS;
}

td_void encode_scene_free_frame_blk(user_frame_info *user_frame_info)
{
    td_s32 ret;
    ot_vb_blk vb_blk = user_frame_info->vb_blk;
    td_u32 blk_size = user_frame_info->blk_size;
    td_void *virt_addr = user_frame_info->frame_info.video_frame.virt_addr[0];

    do{
        ret = ss_mpi_sys_munmap(virt_addr, blk_size);
        ENCODE_RET_CHECK(ret, "ss_mpi_sys_munmap failure!\n");

        ret = ss_mpi_vb_release_blk(vb_blk);
        ENCODE_RET_CHECK(ret, "ss_mpi_vb_release_blk block 0x%x failure\n", vb_blk);
    }while(0);

    user_frame_info->vb_blk = OT_VB_INVALID_HANDLE;
}

td_s32 encode_scene_get_frame_blk(get_frame_vb_cfg *get_frame_vb_cfg,
                                    user_frame_info user_frame_info[], td_s32 frame_cnt)
{
    td_s32 ret;
    td_s32 i;
    ot_vb_pool pool_id;
    ot_vb_calc_cfg calc_cfg = {0};
    ot_vb_pool_cfg vb_pool_cfg = {0};

    encode_scene_get_vb_calc_cfg(get_frame_vb_cfg, &calc_cfg);

    vb_pool_cfg.blk_size   = calc_cfg.vb_size;
    vb_pool_cfg.blk_cnt    = frame_cnt;
    vb_pool_cfg.remap_mode = OT_VB_REMAP_MODE_NONE;
    pool_id = ss_mpi_vb_create_pool(&vb_pool_cfg);
    if (pool_id == OT_VB_INVALID_POOL_ID) {
        ERR("ss_mpi_vb_create_pool failed!\n");
        return TD_FAILURE;
    }

    for (i = 0; i < frame_cnt; i++) {
        ret = encode_scene_malloc_frame_blk(pool_id, get_frame_vb_cfg, &calc_cfg, &user_frame_info[i]);
        if (ret != TD_SUCCESS) {
            goto exit;
        }
    }

    return TD_SUCCESS;

exit:
    for (i = i - 1; i >= 0; i--) {
        encode_scene_free_frame_blk(&user_frame_info[i]);
    }
    ss_mpi_vb_destroy_pool(pool_id);
    return TD_FAILURE;
}

static td_void encode_scene_release_frame_blk(user_frame_info user_frame_info[], td_s32 frame_cnt)
{
    td_s32 i = 0;
    ot_vb_pool pool_id = 0;

    for (i = 0; i < frame_cnt; i++) {
        encode_scene_free_frame_blk(&user_frame_info[i]);
    }

    pool_id = user_frame_info[0].frame_info.pool_id;
    ss_mpi_vb_destroy_pool(pool_id);
}

static td_s32 encode_scene_get_fpn_frame_info(ot_vi_pipe vi_pipe,
                                                ot_pixel_format pixel_format, ot_compress_mode compress_mode,
                                                user_frame_info *user_frame_info, td_s32 blk_cnt)
{
    td_s32 ret;
    ot_vi_pipe_attr pipe_attr;
    get_frame_vb_cfg vb_cfg;

    ret = ss_mpi_vi_get_pipe_attr(vi_pipe, &pipe_attr);
    if (ret != TD_SUCCESS) {
        ERR("vi get pipe attr failed!\n");
        return ret;
    }

    vb_cfg.size.width    = pipe_attr.size.width;
    vb_cfg.size.height   = pipe_attr.size.height;
    vb_cfg.pixel_format  = pixel_format;
    vb_cfg.video_format  = OT_VIDEO_FORMAT_LINEAR;
    vb_cfg.compress_mode = compress_mode;
    vb_cfg.dynamic_range = OT_DYNAMIC_RANGE_SDR8;

    ret = encode_scene_get_frame_blk(&vb_cfg, user_frame_info, blk_cnt);
    if (ret != TD_SUCCESS) {
        ERR("get fpn frame vb failed!\n");
        return ret;
    }

    return TD_SUCCESS;
}

static td_s32 encode_scene_get_raw_bit_width(ot_pixel_format pixel_format)
{
    switch (pixel_format) {
        case OT_PIXEL_FORMAT_RGB_BAYER_8BPP:
            return 8; /* 8:single pixel width */
        case OT_PIXEL_FORMAT_RGB_BAYER_10BPP:
            return 10; /* 10:single pixel width */
        case OT_PIXEL_FORMAT_RGB_BAYER_12BPP:
            return 12; /* 12:single pixel width */
        case OT_PIXEL_FORMAT_RGB_BAYER_14BPP:
            return 14; /* 14:single pixel width */
        case OT_PIXEL_FORMAT_RGB_BAYER_16BPP:
            return 16; /* 16:single pixel width */
        default:
            return 0;
    }
}

static td_void encode_scene_get_fpn_file_name(ot_video_frame *video_frame, td_char *file_name, td_u32 length)
{
    (td_void)snprintf_s(file_name, length, length - 1, "./FPN_frame_%ux%u_%dbit.raw",
                        video_frame->width, video_frame->height,
                        encode_scene_get_raw_bit_width(video_frame->pixel_format));
}

#if 0
static td_s32 encode_scene_get_fpn_file_name_iso(ot_video_frame *video_frame, const td_char *dir_name,
                                                   td_char *file_name, td_u32 length, td_u32 iso)
{
    td_s32 err;
    err = snprintf_s(file_name, length, length - 1, "./%s/FPN_frame_%ux%u_%dbit_iso%u.raw",
                     dir_name, video_frame->width, video_frame->height,
                     encode_scene_get_raw_bit_width(video_frame->pixel_format), iso);
    if (err < 0) {
        return TD_FAILURE;
    }
    return TD_SUCCESS;
}
#endif

td_void encode_scene_save_fpn_file(ot_isp_fpn_frame_info *fpn_frame_info, FILE *pfd)
{
    td_u8 *virt_addr;
    td_u32 fpn_height;
    td_s32 i;

    fpn_height = fpn_frame_info->fpn_frame.video_frame.height;
    virt_addr = (td_u8 *)fpn_frame_info->fpn_frame.video_frame.virt_addr[0];

    /* save Y
        * ---------------------------------------------------------------- */
    (td_void)fprintf(stderr,
                     "FPN: saving......Raw data......stide: %u, width: %u, "
                     "height: %u, iso: %u.\n",
                     fpn_frame_info->fpn_frame.video_frame.stride[0],
                     fpn_frame_info->fpn_frame.video_frame.width, fpn_height,
                     fpn_frame_info->iso);
    (td_void)fprintf(stderr, "phys addr: 0x%lx\n", (td_ulong)fpn_frame_info->fpn_frame.video_frame.phys_addr[0]);
    (td_void)fprintf(stderr, "please wait a moment to save FPN raw data.\n");
    (td_void)fflush(stderr);

    (td_void)fwrite(virt_addr, fpn_frame_info->frm_size, 1, pfd);
    sync();

    /* save offset */
    for (i = 0; i < OT_VI_MAX_SPLIT_NODE_NUM; i++) {
        (td_void)fwrite(&fpn_frame_info->offset[i], 4, 1, pfd); /* 4: 4byte */
        sync();
    }

    /* save compress mode */
    (td_void)fwrite(&fpn_frame_info->fpn_frame.video_frame.compress_mode, 4, 1, pfd); /* 4: 4byte */
    sync();

    /* save fpn frame size */
    (td_void)fwrite(&fpn_frame_info->frm_size, 4, 1, pfd); /* 4: 4byte */
    sync();

    /* save iso */
    (td_void)fwrite(&fpn_frame_info->iso, 4, 1, pfd); /* 4: 4byte */
    sync();
    (td_void)fflush(pfd);
}

static td_s32 encode_scene_fpn_multi_calibrate(ot_vi_pipe vi_pipe, user_frame_info *user_frame_info,
    ot_isp_fpn_calibrate_attr *calibrate_attr, td_s32 calib_cnt)
{
    td_s32 i, ret;

    for (i = 0; i < calib_cnt; i++) {
        /* point each fpn dark frame vb to calibrate_attr */
        (td_void)memcpy_s(&calibrate_attr->fpn_cali_frame.fpn_frame, sizeof(ot_video_frame_info),
                          &user_frame_info[i].frame_info, sizeof(ot_video_frame_info));

        ret = ss_mpi_isp_fpn_calibrate(vi_pipe, calibrate_attr);
        if (ret != TD_SUCCESS) {
            ERR("vi fpn calibrate failed!\n");
            return TD_FAILURE;
        }
        (td_void)memcpy_s(&user_frame_info[i].frame_info, sizeof(ot_video_frame_info),
                          &calibrate_attr->fpn_cali_frame.fpn_frame, sizeof(ot_video_frame_info));
    }

    return TD_SUCCESS;
}

static td_s32 encode_scene_fpn_calibrate_process(ot_vi_pipe vi_pipe, user_frame_info *user_frame_info,
    ot_isp_fpn_calibrate_attr *calibrate_attr, td_s32 calib_cnt)
{
    calibrate_attr->frame_num = calib_cnt;
    calibrate_attr->fpn_mode = OT_ISP_FPN_OUT_MODE_NORM;
    return  encode_scene_fpn_multi_calibrate(vi_pipe, user_frame_info, calibrate_attr, calib_cnt);
}

td_s32 encode_scene_fpn_calibrate(ot_vi_pipe vi_pipe, fpn_calibration_cfg *calibration_cfg)
{
    td_s32 ret, i;
    const ot_vi_chn vi_chn = 0;
    FILE *pfd = TD_NULL;
    user_frame_info user_frame_info = {0};
    ot_isp_fpn_calibrate_attr calibrate_attr;

    td_char fpn_file_name[FPN_FILE_NAME_LENGTH];

    printf("please turn off camera aperture to start calibrate!\nhit enter key ,start calibrate!\n");
    (td_void)getchar();

    ret = ss_mpi_vi_disable_chn(vi_pipe, vi_chn);
    if (ret != TD_SUCCESS) {
        return TD_FAILURE;
    }

    calibrate_attr.threshold = calibration_cfg->threshold;
    calibrate_attr.frame_num = calibration_cfg->frame_num;
    calibrate_attr.fpn_type  = calibration_cfg->fpn_type;
    ret = encode_scene_get_fpn_frame_info(vi_pipe, OT_PIXEL_FORMAT_RGB_BAYER_16BPP,
                                            calibration_cfg->compress_mode, &user_frame_info, 1);
    if (ret != TD_SUCCESS) {
        ss_mpi_vi_enable_chn(vi_pipe, vi_chn);
        return TD_FAILURE;
    }

    ret = encode_scene_fpn_calibrate_process(vi_pipe, &user_frame_info, &calibrate_attr, 1);
    if (ret != TD_SUCCESS) {
        ERR("vi fpn calibrate failed!\n");
        goto exit;
    } else {
        DBG("vi fpn calibrate done!\n");
    }

    printf("\nafter calibrate ");
    for (i = 0; i < OT_VI_MAX_SPLIT_NODE_NUM; i++) {
        printf("offset[%d] = 0x%x, ", i, calibrate_attr.fpn_cali_frame.offset[i]);
    }
    printf("frame_size = %u, iso = %u\n", calibrate_attr.fpn_cali_frame.frm_size, calibrate_attr.fpn_cali_frame.iso);

    encode_scene_get_fpn_file_name(&calibrate_attr.fpn_cali_frame.fpn_frame.video_frame,
                                     fpn_file_name, FPN_FILE_NAME_LENGTH);
    printf("save dark frame file: %s!\n", fpn_file_name);
    pfd = fopen(fpn_file_name, "wb");
    if (pfd == TD_NULL) {
        printf("open file %s err!\n", fpn_file_name);
        goto exit;
    }

    encode_scene_save_fpn_file(&calibrate_attr.fpn_cali_frame, pfd);

    (td_void)fclose(pfd);

exit:
    encode_scene_release_frame_blk(&user_frame_info, 1);
    ret = ss_mpi_vi_enable_chn(vi_pipe, vi_chn);
    return ret;
}

static td_void encode_scene_read_fpn_file(ot_isp_fpn_frame_info *fpn_frame_info, FILE *pfd)
{
    ot_video_frame_info *frame_info;
    td_s32 i;

    frame_info = &fpn_frame_info->fpn_frame;
    (td_void)fread((td_u8 *)frame_info->video_frame.virt_addr[0], fpn_frame_info->frm_size, 1, pfd);

    for (i = 0; i < OT_VI_MAX_SPLIT_NODE_NUM; i++) {
        (td_void)fread((td_u8 *)&fpn_frame_info->offset[i], 4, 1, pfd); /* 4: 4byte */
    }

    (td_void)fread((td_u8 *)&frame_info->video_frame.compress_mode, 4, 1, pfd); /* 4: 4byte */
    (td_void)fread((td_u8 *)&fpn_frame_info->frm_size, 4, 1, pfd); /* 4: 4byte */
    (td_void)fread((td_u8 *)&fpn_frame_info->iso, 4, 1, pfd); /* 4: 4byte */
}

td_s32 encode_scene_enable_fpn_correction(ot_vi_pipe vi_pipe, fpn_correction_cfg *correction_cfg)
{
    td_s32 ret;
    td_u32 i;
    FILE *pfd = TD_NULL;
    ot_isp_fpn_attr correction_attr;
    user_frame_info *user_frame_info = &correction_cfg->user_frame_info;
    td_char fpn_file_name[FPN_FILE_NAME_LENGTH];

    ret = encode_scene_get_fpn_frame_info(vi_pipe, correction_cfg->pixel_format,
                                            correction_cfg->compress_mode, user_frame_info, 1);
    if (ret != TD_SUCCESS) {
        return TD_FAILURE;
    }
    (td_void)memcpy_s(&correction_attr.fpn_frm_info.fpn_frame, sizeof(ot_video_frame_info),
                      &user_frame_info->frame_info, sizeof(ot_video_frame_info));

    encode_scene_get_fpn_file_name(&correction_attr.fpn_frm_info.fpn_frame.video_frame,
                                     fpn_file_name, FPN_FILE_NAME_LENGTH);
    pfd = fopen(fpn_file_name, "rb");
    if (pfd == TD_NULL) {
        printf("open file %s err!\n", fpn_file_name);
        goto exit;
    }

    correction_attr.fpn_frm_info.frm_size = user_frame_info->blk_size;
    encode_scene_read_fpn_file(&correction_attr.fpn_frm_info, pfd);

    (td_void)fclose(pfd);

    for (i = 0; i < OT_VI_MAX_SPLIT_NODE_NUM; i++) {
        printf("offset[%u] = 0x%x; ", i, correction_attr.fpn_frm_info.offset[i]);
    }
    printf("\n");
    printf("frame_size = %u.\n", correction_attr.fpn_frm_info.frm_size);
    printf("iso = %u.\n", correction_attr.fpn_frm_info.iso);

    correction_attr.enable = TD_TRUE;
    correction_attr.aibnr_mode = TD_FALSE;
    correction_attr.op_type = correction_cfg->op_mode;
    correction_attr.fpn_type = correction_cfg->fpn_type;
    correction_attr.manual_attr.strength = correction_cfg->strength;
    ret = ss_mpi_isp_set_fpn_attr(vi_pipe, &correction_attr);
    if (ret != TD_SUCCESS) {
        ERR("set fpn attr failed!\n");
        goto exit;
    }

    return TD_SUCCESS;

exit:
    encode_scene_release_frame_blk(user_frame_info, 1);
    return ret;
}

td_s32 encode_scene_enable_fpn_correction_for_thermo(ot_vi_pipe vi_pipe, fpn_correction_cfg *correction_cfg)
{
    td_s32 ret;
    FILE *pfd = TD_NULL;
    ot_isp_fpn_attr correction_attr;
    user_frame_info *user_frame_info = &correction_cfg->user_frame_info;
    td_char fpn_file_name[FPN_FILE_NAME_LENGTH];

    ret = encode_scene_get_fpn_frame_info(vi_pipe, correction_cfg->pixel_format,
                                            correction_cfg->compress_mode, user_frame_info, 1);
    if (ret != TD_SUCCESS) {
        return TD_FAILURE;
    }
    (td_void)memcpy_s(&correction_attr.fpn_frm_info.fpn_frame, sizeof(ot_video_frame_info),
                      &user_frame_info->frame_info, sizeof(ot_video_frame_info));

    encode_scene_get_fpn_file_name(&correction_attr.fpn_frm_info.fpn_frame.video_frame,
                                     fpn_file_name, FPN_FILE_NAME_LENGTH);
    pfd = fopen(fpn_file_name, "rb");
    if (pfd == TD_NULL) {
        printf("open file %s err!\n", fpn_file_name);
        goto exit;
    }

    correction_attr.fpn_frm_info.frm_size = user_frame_info->blk_size;
    encode_scene_read_fpn_file(&correction_attr.fpn_frm_info, pfd);

    (td_void)fclose(pfd);

    correction_attr.enable = TD_TRUE;
    correction_attr.aibnr_mode = TD_FALSE;
    correction_attr.op_type = correction_cfg->op_mode;
    correction_attr.fpn_type = correction_cfg->fpn_type;
    correction_attr.manual_attr.strength = correction_cfg->strength;
    ret = ss_mpi_isp_set_fpn_attr(vi_pipe, &correction_attr);
    if (ret != TD_SUCCESS) {
        ERR("set fpn attr failed!\n");
        goto exit;
    }

    return TD_SUCCESS;

exit:
    encode_scene_release_frame_blk(user_frame_info, 1);
    return ret;
}

td_s32 encode_scene_disable_fpn_correction(ot_vi_pipe vi_pipe, fpn_correction_cfg *correction_cfg)
{
    td_s32 ret;
    ot_isp_fpn_attr correction_attr;

    do {
        ret = ss_mpi_isp_get_fpn_attr(vi_pipe, &correction_attr);
        ENCODE_RET_BREAK(ret, "get fpn attr failed!\n");

        correction_attr.enable = TD_FALSE;
        ret = ss_mpi_isp_set_fpn_attr(vi_pipe, &correction_attr);
        ENCODE_RET_BREAK(ret, "set fpn attr failed!\n");
    }while(0);

    if (ret == TD_SUCCESS){
        encode_scene_release_frame_blk(&correction_cfg->user_frame_info, 1);
    }

    return TD_SUCCESS;
}

td_s32 encode_scene_disable_fpn_correction_for_thermo(ot_vi_pipe vi_pipe,
                                                        fpn_correction_cfg *correction_cfg)
{
    td_s32 ret;
    ot_isp_fpn_attr correction_attr;

    ret = ss_mpi_isp_get_fpn_attr(vi_pipe, &correction_attr);
    if (ret != TD_SUCCESS) {
        ERR("get fpn attr failed!\n");
        return TD_FAILURE;
    }

    if (correction_attr.enable == TD_FALSE) {
        return TD_SUCCESS;
    }

    correction_attr.enable = TD_FALSE;
    ret = ss_mpi_isp_set_fpn_attr(vi_pipe, &correction_attr);
    if (ret != TD_SUCCESS) {
        ERR("set fpn attr failed!\n");
        return TD_FAILURE;
    }

    encode_scene_release_frame_blk(&correction_cfg->user_frame_info, 1);

    return TD_SUCCESS;
}
