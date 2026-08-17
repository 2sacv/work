#include "ot_defines.h"
#include "ot_buffer.h"
#include "ss_mpi_sys.h"
#include "ss_mpi_vpss.h"
#include "securec.h"

#include "debug.h"
#include "g_log.h"
#include "jconfstruct.h"
#include "encodeapi.h"

#include "encode_common.h"
#include "encode_vi.h"
#include "encode_vpss.h"

#define VPSS_DEFAULT_WIDTH  1920
#define VPSS_DEFAULT_HEIGHT 1080

td_void encode_vpss_get_default_grp_attr(ot_vpss_grp_attr *grp_attr)
{
    grp_attr->ie_en                     = TD_FALSE;
    grp_attr->dci_en                    = TD_FALSE;
    grp_attr->buf_share_en              = TD_FALSE;
    grp_attr->mcf_en                    = TD_FALSE;
    grp_attr->max_width                 = VPSS_DEFAULT_WIDTH;
    grp_attr->max_height                = VPSS_DEFAULT_HEIGHT;
    grp_attr->max_dei_width             = 0;
    grp_attr->max_dei_height            = 0;
    grp_attr->dynamic_range             = OT_DYNAMIC_RANGE_SDR8;
    grp_attr->pixel_format              = OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    grp_attr->dei_mode                  = OT_VPSS_DEI_MODE_OFF;
    grp_attr->buf_share_chn             = OT_VPSS_CHN0;
    grp_attr->frame_rate.src_frame_rate = -1;
    grp_attr->frame_rate.dst_frame_rate = -1;
}

td_void encode_vpss_get_default_3dnr_attr(ot_3dnr_attr *nr_attr)
{
    nr_attr->enable         = TD_FALSE;
    nr_attr->nr_type        = OT_NR_TYPE_VIDEO_NORM;
    nr_attr->compress_mode  = OT_COMPRESS_MODE_FRAME;
    nr_attr->nr_motion_mode = OT_NR_MOTION_MODE_NORM;
}

td_void encode_vpss_get_default_chn_attr(ot_vpss_chn_attr *chn_attr)
{
    chn_attr->mirror_en                 = TD_FALSE;
    chn_attr->flip_en                   = TD_FALSE;
    chn_attr->border_en                 = TD_FALSE;
    chn_attr->width                     = VPSS_DEFAULT_WIDTH;
    chn_attr->height                    = VPSS_DEFAULT_HEIGHT;
    chn_attr->depth                     = 0;
    chn_attr->chn_mode                  = OT_VPSS_CHN_MODE_USER;
    chn_attr->video_format              = OT_VIDEO_FORMAT_LINEAR;
    chn_attr->dynamic_range             = OT_DYNAMIC_RANGE_SDR8;
    chn_attr->pixel_format              = OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    chn_attr->compress_mode             = OT_COMPRESS_MODE_NONE;
    chn_attr->aspect_ratio.mode         = OT_ASPECT_RATIO_NONE;
    chn_attr->frame_rate.src_frame_rate = -1;
    chn_attr->frame_rate.dst_frame_rate = -1;
}

td_s32 encode_vpss_get_wrap_cfg(vpss_cfg_t *vpss_attr, ot_vi_vpss_mode_type mode, td_u32 full_lines_std)
{
    int ret = 0;
    ot_vpss_venc_wrap_param wrap_param = {0};
    td_u32 buf_line = 0;
    td_u32 wrap_buf_size = 0;
    ot_pic_buf_attr buf_attr = {0};

    do {
        vpss_attr->wrap_attr[0].enable = TD_FALSE;
        wrap_param.frame_rate = 30; /* frame_rate 30 */

        if (vpss_attr->chn_attr[0].frame_rate.src_frame_rate > 0 && vpss_attr->chn_attr[0].frame_rate.dst_frame_rate > 0) {
            /* frame_rate 30 */
            wrap_param.frame_rate = vpss_attr->chn_attr[0].frame_rate.dst_frame_rate * 30 / vpss_attr->chn_attr[0].frame_rate.src_frame_rate;
        }

        if (mode == OT_VI_ONLINE_VPSS_ONLINE) {
            wrap_param.all_online = TD_TRUE;
        }

        wrap_param.full_lines_std = full_lines_std;
        wrap_param.large_stream_size.width = vpss_attr->grp_attr.max_width;
        wrap_param.large_stream_size.height = vpss_attr->grp_attr.max_height;
        wrap_param.small_stream_size.width = vpss_attr->chn_attr[1].width;
        wrap_param.small_stream_size.height = vpss_attr->chn_attr[1].height;
        ret = ss_mpi_sys_get_vpss_venc_wrap_buf_line(&wrap_param, &buf_line);
        ENCODE_RET_BREAK(ret, "ss_mpi_sys_get_vpss_venc_wrap_buf_line failed\n");

        buf_attr.width = vpss_attr->grp_attr.max_width;
        buf_attr.height = vpss_attr->grp_attr.max_height;
        buf_attr.bit_width = OT_DATA_BIT_WIDTH_8;
        buf_attr.pixel_format = vpss_attr->chn_attr[0].pixel_format;
        buf_attr.compress_mode = vpss_attr->chn_attr[0].compress_mode;
        buf_attr.align = OT_DEFAULT_ALIGN;
        buf_attr.video_format = vpss_attr->chn_attr[0].video_format;
        wrap_buf_size = ot_comm_get_vpss_venc_wrap_buf_size(&buf_attr, buf_line);

        /* out */
        vpss_attr->wrap_attr[0].enable = TD_TRUE;
        vpss_attr->wrap_attr[0].buf_line = buf_line;
        vpss_attr->wrap_attr[0].buf_size = wrap_buf_size;
        DBG("wrap online is %u, buf line is %u, buf size is %u\n", wrap_param.all_online, buf_line, wrap_buf_size);
    } while(0);

    return ret;
}

void encode_vpss_get_cfg(sns_type_t sns_type, vpss_cfg_t *vpss_cfg, VideoEncS *enc_param)
{
    ot_vpss_chn chn = 0;
    int max_width = 0;
    int max_height = 0;
    int width = 0;
    int height = 0;

    VideoIdxE idx = encode_max_idx(0);
    encode_idx_to_resolution(idx, &max_width, &max_height);

    encode_vpss_get_default_grp_attr(&vpss_cfg->grp_attr);
    encode_vpss_get_default_3dnr_attr(&vpss_cfg->nr_attr);

    vpss_cfg->vpss_grp = 0;

    for (chn = 0; chn < VPSS_MAX_CHN_NUM; chn++) {
        vpss_cfg->chn_en[chn] = TD_TRUE;
        vpss_cfg->wrap_attr[chn].enable = TD_FALSE;

        encode_vpss_get_default_chn_attr(&vpss_cfg->chn_attr[chn]);
        vpss_cfg->chn_attr[chn].compress_mode = OT_COMPRESS_MODE_NONE;
        encode_vencsize_to_resolution(enc_param->enc[chn].vencsize, &width, &height);
        DBG("vpss init chn:%d width:%d height:%d \n",chn,width,height);
        if(CH_FS_IVE == chn || CH_FS_SUB0 == chn) {
            vpss_cfg->chn_attr[chn].depth = 1;
            vpss_cfg->chn_attr[chn].width  = RAW_W;
            vpss_cfg->chn_attr[chn].height = RAW_H;
            max_width = MAX2(max_width, RAW_W);
            max_height = MAX2(max_height, RAW_H);
        } else {
            vpss_cfg->chn_attr[chn].width  = width;
            vpss_cfg->chn_attr[chn].height = height;
            max_width = MAX2(max_width, width);
            max_height = MAX2(max_height, height);
        }
    }

    vpss_cfg->wrap_attr[0].enable = TD_TRUE;
    vpss_cfg->grp_attr.max_width  = max_width;
    vpss_cfg->grp_attr.max_height = max_height;
}

td_void encode_vpss_wrap_cfg_init(sns_type_t sns_type, sys_cfg_t *sys_cfg,vpss_cfg_t *vpss_cfg)
{
    td_u32 full_lines_std;
    if (sys_cfg->vpss_wrap_en) {
        if (sns_type == SC4336P_MIPI_4M_30FPS_10BIT || sns_type == OS04D10_MIPI_4M_30FPS_10BIT ||
            sns_type == GC4023_MIPI_4M_30FPS_10BIT || sns_type == SC431HAI_MIPI_4M_30FPS_10BIT ||
            sns_type == SC431HAI_MIPI_4M_30FPS_10BIT_WDR2TO1 || sns_type == SC4336P_MIPI_3M_30FPS_10BIT ) {
            full_lines_std = 1500; /* full_lines_std: 1500 */
        } else if (sns_type == SC450AI_MIPI_4M_30FPS_10BIT || sns_type == SC450AI_MIPI_4M_30FPS_10BIT_WDR2TO1) {
            full_lines_std = 1585; /* full_lines_std: 1585 */
        } else if (sns_type == SC500AI_MIPI_5M_30FPS_10BIT || sns_type == SC500AI_MIPI_5M_30FPS_10BIT_WDR2TO1) {
            full_lines_std = 1700; /* full_lines_std: 1700 */
        } else if (sns_type == SC465SL_MIPI_4M_30FPS_12BIT){
            full_lines_std = 3120; /* full_lines_std: hblank=2560;Vblank=560 */
        } else if (sns_type == SC235_MIPI_2M_15FPS_10BIT){
            full_lines_std = 1405; /* full_lines_std: hblank=280; Vblank=45  hblank+Vblank+height 1080 1171*/
        } else {
            sys_cfg->vpss_wrap_en = TD_FALSE;
            vpss_cfg->wrap_attr[0].enable = TD_FALSE;
            return;
        }

        (td_void)encode_vpss_get_wrap_cfg(vpss_cfg, sys_cfg->mode_type, full_lines_std);

        sys_cfg->vpss_wrap_size = vpss_cfg->wrap_attr[0].buf_size;
    }
}

static td_s32 encode_vpss_set_chn_wrap_attr(ot_vpss_grp grp, ot_vpss_chn chn,
    const ot_vpss_chn_buf_wrap_attr *attr)
{
    td_s32 ret = TD_SUCCESS;
    if (chn == OT_VPSS_CHN0 && attr->enable == TD_TRUE) {
        ret = ss_mpi_vpss_set_chn_buf_wrap(grp, chn, attr);
    }

    return ret;
}

static td_s32 encode_vpss_chn_start(ot_vpss_grp grp, const td_bool *chn_enable,
    const ot_vpss_chn_attr *chn_attr, const ot_vpss_chn_buf_wrap_attr *wrap_attr, td_u32 chn_array_size)
{
    td_s32 ret = 0;
    ot_vpss_chn vpss_chn = 0;

    for (int i = 0; i < (td_s32)chn_array_size; ++i) {
        if (chn_enable[i] == TD_TRUE) {
            vpss_chn = i;

            ret = ss_mpi_vpss_set_chn_attr(grp, vpss_chn, &chn_attr[vpss_chn]);
            ENCODE_RET_BREAK(ret, "ss_mpi_vpss_set_chn_attr failed\n");

            /* set chn0 wrap attr first, then enable chn */
            ret = encode_vpss_set_chn_wrap_attr(grp, vpss_chn, &wrap_attr[vpss_chn]);
            ENCODE_RET_BREAK(ret, "encode_vpss_set_chn_wrap_attr failed\n");

            ret = ss_mpi_vpss_enable_chn(grp, vpss_chn);
            ENCODE_RET_BREAK(ret, "ss_mpi_vpss_enable_chn failed\n");
        }
    }

    return ret;
}

td_s32 encode_vpss_grp_start(ot_vpss_grp grp, const ot_vpss_grp_attr *grp_attr,
    const vpss_chn_attr_t *vpss_chn_attr)
{
    td_s32 ret = 0;
    do {
        if (vpss_chn_attr->chn_array_size < VPSS_MAX_CHN_NUM) {
            ERR("array size(%u) of chn_enable and chn_attr need >= %u!\n",
            vpss_chn_attr->chn_array_size, VPSS_MAX_CHN_NUM);
            ret = -1;
            break;
        }

        ret = ss_mpi_vpss_create_grp(grp, grp_attr);
        ENCODE_RET_BREAK(ret, "ss_mpi_vpss_create_grp failed\n");

        ret = ss_mpi_vpss_start_grp(grp);
        ENCODE_RET_BREAK(ret, "ss_mpi_vpss_start_grp failed\n");

        ret = encode_vpss_chn_start(grp, vpss_chn_attr->chn_enable, &vpss_chn_attr->chn_attr[0],
                                    &vpss_chn_attr->wrap_attr[0], VPSS_MAX_CHN_NUM);
        ENCODE_RET_BREAK(ret, "encode_vpss_chn_start failed\n");
    }while(0);

    return ret;

}

int encode_vpss_set_chn_param(ot_vpss_grp vpss_grp, ot_vpss_chn vpss_chn, td_u32 width, td_u32 height)
{
    int ret = 0;
    ot_vpss_chn_attr vpss_chn_attr = {0};

    do {
        ret = ss_mpi_vpss_disable_chn(vpss_grp, vpss_chn);
        ENCODE_RET_BREAK(ret, "ss_mpi_vpss_disable_chn failed\n");

        ret = ss_mpi_vpss_get_chn_attr(vpss_grp, vpss_chn, &vpss_chn_attr);
        ENCODE_RET_BREAK(ret, "ss_mpi_vpss_get_chn_attr failed\n");

        vpss_chn_attr.width = width;
        vpss_chn_attr.height = height;

        ret = ss_mpi_vpss_set_chn_attr(vpss_grp, vpss_chn, &vpss_chn_attr);
        ENCODE_RET_BREAK(ret, "ss_mpi_vpss_set_chn_attr failed\n");

        ret = ss_mpi_vpss_enable_chn(vpss_grp, vpss_chn);
        ENCODE_RET_BREAK(ret, "ss_mpi_vpss_enable_chn failed\n");
    } while(0);

    return ret;
}

int encode_vpss_init(ot_vpss_grp grp, vpss_cfg_t *vpss_cfg)
{
    int ret = 0;

    ot_frame_interrupt_attr frame_interrupt_attr = {0};
    vpss_chn_attr_t vpss_chn_attr = {0};

    do {
        frame_interrupt_attr.interrupt_type = OT_FRAME_INTERRUPT_EARLY_END;
        frame_interrupt_attr.early_line = vpss_cfg->grp_attr.max_height / 2; /* 2 half */
        ret = ss_mpi_vpss_set_grp_frame_interrupt_attr(0, &frame_interrupt_attr);
        ENCODE_RET_BREAK(ret, "ss_mpi_vpss_set_grp_frame_interrupt_attr failed\n");

        (td_void)memcpy_s(&vpss_chn_attr.chn_attr[0], sizeof(ot_vpss_chn_attr) * VPSS_MAX_CHN_NUM,
        vpss_cfg->chn_attr, sizeof(ot_vpss_chn_attr) * VPSS_MAX_CHN_NUM);
        (td_void)memcpy_s(vpss_chn_attr.chn_enable, sizeof(vpss_chn_attr.chn_enable),
        vpss_cfg->chn_en, sizeof(vpss_chn_attr.chn_enable));
        (td_void)memcpy_s(&vpss_chn_attr.wrap_attr[0], sizeof(ot_vpss_chn_buf_wrap_attr) * VPSS_MAX_CHN_NUM,
        vpss_cfg->wrap_attr, sizeof(ot_vpss_chn_buf_wrap_attr) * VPSS_MAX_CHN_NUM);
        vpss_chn_attr.chn_array_size = VPSS_MAX_CHN_NUM;

        ret = encode_vpss_grp_start(grp, &vpss_cfg->grp_attr, &vpss_chn_attr);
        ENCODE_RET_BREAK(ret, "encode_vpss_grp_start failed\n");

        //if (vpss_cfg->chn_en[1]) {
            //const ot_low_delay_info low_delay_info = { TD_TRUE, 200, TD_FALSE }; /* 200: lowdelay line */
            //ret = ss_mpi_vpss_set_chn_low_delay(grp, 1, &low_delay_info);
            //ENCODE_RET_BREAK(ret, "ss_mpi_vpss_set_chn_low_delay failed\n");
        //}
    }while(0);
    return ret;
}

int encode_vpss_uninit(ot_vpss_grp grp, td_u32 chn_array_size)
{
    int ret = 0;
    ot_vpss_chn vpss_chn = 0;

    do {
        if (chn_array_size < VPSS_MAX_CHN_NUM) {
            ERR("array size(%u) of chn_enable need > %u!\n", chn_array_size, VPSS_MAX_CHN_NUM);
            ret = TD_FAILURE;
            break;
        }

        for (vpss_chn = 0; vpss_chn < VPSS_MAX_CHN_NUM; ++vpss_chn) {
            ret = ss_mpi_vpss_disable_chn(grp, vpss_chn);
            ENCODE_RET_BREAK(ret, "ss_mpi_vpss_disable_chn failed\n");
        }

        ret = ss_mpi_vpss_stop_grp(grp);
        ENCODE_RET_BREAK(ret, "ss_mpi_vpss_stop_grp failed\n");

        ret = ss_mpi_vpss_destroy_grp(grp);
        ENCODE_RET_BREAK(ret, "ss_mpi_vpss_destroy_grp failed\n");
    } while(0);

    return ret;
}
