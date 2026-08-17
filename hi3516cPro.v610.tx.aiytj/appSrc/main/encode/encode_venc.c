#include "ot_math.h"
#include "ss_mpi_venc.h"

#include "debug.h"
#include "confapi.h"
#include "encode_common.h"
#include "encode_bind.h"
#include "encode_venc.h"
#include "encode_vi_isp.h"
#include "shm_buf.h"

td_s32 encode_venc_rc_params_init(ot_venc_chn venc_chn)
{
    td_s32 ret = 0;
    ot_venc_rc_param rc_param = {0};
    ot_venc_chn_attr chn_attr = {0};

    Appvecfg appvecfg = {0};
    conf_get_appve_cfg(&appvecfg);

    do {
        ret = ss_mpi_venc_get_chn_attr(venc_chn, &chn_attr);
        ENCODE_RET_BREAK(ret, "ss_mpi_venc_get_chn_attr failed\n");

        ret = ss_mpi_venc_get_rc_param(venc_chn, &rc_param);
        ENCODE_RET_BREAK(ret, "ss_mpi_venc_get_rc_param failed\n");

        //close_reencode  && set QP params
        if (chn_attr.rc_attr.rc_mode == OT_VENC_RC_MODE_H264_CBR) {
            rc_param.h264_cbr_param.max_reencode_times = 2;
            rc_param.h264_cbr_param.max_qp = appvecfg.imaxqp[venc_chn];
            rc_param.h264_cbr_param.min_qp = appvecfg.iminqp[venc_chn];
            rc_param.h264_cbr_param.max_i_qp = appvecfg.imaxqp[venc_chn];
            rc_param.h264_cbr_param.min_i_qp = appvecfg.iminqp[venc_chn];
        } else if (chn_attr.rc_attr.rc_mode == OT_VENC_RC_MODE_H264_VBR) {
            rc_param.h264_vbr_param.max_reencode_times = 0;
            rc_param.h264_vbr_param.max_qp = appvecfg.imaxqp[venc_chn];
            rc_param.h264_vbr_param.min_qp = appvecfg.iminqp[venc_chn];
            rc_param.h264_vbr_param.max_i_qp = appvecfg.imaxqp[venc_chn];
            rc_param.h264_vbr_param.min_i_qp = appvecfg.iminqp[venc_chn];
        } else if (chn_attr.rc_attr.rc_mode == OT_VENC_RC_MODE_H264_AVBR) {
            rc_param.h264_avbr_param.max_reencode_times = 0;
            rc_param.h264_avbr_param.max_qp = appvecfg.imaxqp[venc_chn];
            rc_param.h264_avbr_param.min_qp = appvecfg.iminqp[venc_chn];
            rc_param.h264_avbr_param.max_i_qp = appvecfg.imaxqp[venc_chn];
            rc_param.h264_avbr_param.min_i_qp = appvecfg.iminqp[venc_chn];
        } else if (chn_attr.rc_attr.rc_mode == OT_VENC_RC_MODE_H264_CVBR) {
            rc_param.h264_cvbr_param.max_reencode_times = 0;
            rc_param.h264_cvbr_param.max_qp = appvecfg.imaxqp[venc_chn];
            rc_param.h264_cvbr_param.min_qp = appvecfg.iminqp[venc_chn];
            rc_param.h264_cvbr_param.max_i_qp = appvecfg.imaxqp[venc_chn];
            rc_param.h264_cvbr_param.min_i_qp = appvecfg.iminqp[venc_chn];
        } else if (chn_attr.rc_attr.rc_mode == OT_VENC_RC_MODE_H265_CBR) {
            rc_param.h265_cbr_param.max_reencode_times = 2;
            rc_param.h265_cbr_param.max_qp = appvecfg.imaxqp[venc_chn];
            rc_param.h265_cbr_param.min_qp = appvecfg.iminqp[venc_chn];
            rc_param.h265_cbr_param.max_i_qp = appvecfg.imaxqp[venc_chn];
            rc_param.h265_cbr_param.min_i_qp = appvecfg.iminqp[venc_chn];
        } else if (chn_attr.rc_attr.rc_mode == OT_VENC_RC_MODE_H265_VBR) {
            rc_param.h265_vbr_param.max_reencode_times = 0;
            rc_param.h265_vbr_param.max_qp = appvecfg.imaxqp[venc_chn];
            rc_param.h265_vbr_param.min_qp = appvecfg.iminqp[venc_chn];
            rc_param.h265_vbr_param.max_i_qp = appvecfg.imaxqp[venc_chn];
            rc_param.h265_vbr_param.min_i_qp = appvecfg.iminqp[venc_chn];
        } else if (chn_attr.rc_attr.rc_mode == OT_VENC_RC_MODE_H265_AVBR) {
            rc_param.h265_avbr_param.max_reencode_times = 0;
            rc_param.h265_avbr_param.max_qp = appvecfg.imaxqp[venc_chn];
            rc_param.h265_avbr_param.min_qp = appvecfg.iminqp[venc_chn];
            rc_param.h265_avbr_param.max_i_qp = appvecfg.imaxqp[venc_chn];
            rc_param.h265_avbr_param.min_i_qp = appvecfg.iminqp[venc_chn];
        } else if (chn_attr.rc_attr.rc_mode == OT_VENC_RC_MODE_H265_CVBR) {
            rc_param.h265_cvbr_param.max_reencode_times = 0;
            rc_param.h265_cvbr_param.max_qp = appvecfg.imaxqp[venc_chn];
            rc_param.h265_cvbr_param.min_qp = appvecfg.iminqp[venc_chn];
            rc_param.h265_cvbr_param.max_i_qp = appvecfg.imaxqp[venc_chn];
            rc_param.h265_cvbr_param.min_i_qp = appvecfg.iminqp[venc_chn];
        } else {
            //[default qp: main 10~51 sub 24~51]
            ERR("chn_attr.rc_attr.rc_mode %d\n", chn_attr.rc_attr.rc_mode);
            break;
        }

        ret = ss_mpi_venc_set_rc_param(venc_chn, &rc_param);
        ENCODE_RET_BREAK(ret, "ss_mpi_venc_set_rc_param failed\n");

    } while(0);

    return ret;
}

static int encode_venc_set_capability(ot_venc_chn chn)
{
    int ret = 0;
    int max_width = 0, max_height = 0;
    ot_venc_chn_capability chn_cap = {0};

    do {
        if (chn >= CH_FS_H26X_END) {
            break;
        }

        ret = ss_mpi_venc_get_chn_capability(chn, &chn_cap);
        ENCODE_RET_BREAK(ret, "ss_mpi_venc_get_chn_capability(%d) failed\n", chn);

        chn_cap.enable = TD_TRUE;
        encode_idx_to_resolution(encode_max_idx(chn), &max_width, &max_height);
        chn_cap.attr_capability.rcn_ref_share_buf_en = TD_TRUE;
        chn_cap.attr_capability.payload_support = OT_VENC_PAYLOAD_SUPPORT_H264 |
            OT_VENC_PAYLOAD_SUPPORT_H265 | OT_VENC_PAYLOAD_SUPPORT_JPEG;
        chn_cap.attr_capability.max_pic_width = max_width;
        chn_cap.attr_capability.max_pic_height = max_height;
        if (CH_FS_MAIN0 == chn) {
            chn_cap.attr_capability.buf_size = OT_ALIGN_UP(1 * 1024 * 1024, 64);
        } else {
            chn_cap.attr_capability.buf_size = OT_ALIGN_UP(max_width * max_height / 2, 64);
        }
        chn_cap.attr_capability.frame_buf_ratio = 80;

        chn_cap.rc_capability.rc_mode_support = OT_VENC_RC_SUPPORT_ABR |
            OT_VENC_RC_SUPPORT_AVBR | OT_VENC_RC_SUPPORT_CBR | OT_VENC_RC_SUPPORT_CVBR;
        chn_cap.rc_capability.min_gop = 10;
        chn_cap.rc_capability.max_gop = 300;
        chn_cap.rc_capability.max_stats_time = 60;
        chn_cap.rc_capability.max_src_frame_rate = ENCODE_SENSOR0_FRAME_RATE;
        chn_cap.rc_capability.max_short_term_stats_time = 60;
        chn_cap.rc_capability.max_long_term_stats_time = 60;

        chn_cap.gop_capability.gop_mode_support = OT_VENC_GOP_SUPPORT_NORMAL_P;
        //chn_cap.gop_capability.crr_recode_strategy_support = OT_VENC_RC_SUPPORT_ABR |
        //    OT_VENC_RC_SUPPORT_AVBR | OT_VENC_RC_SUPPORT_CBR | OT_VENC_RC_SUPPORT_CVBR;   // 不支持设置
        chn_cap.gop_capability.min_bg_interval = 10;
        chn_cap.gop_capability.max_bg_interval = 300;
        chn_cap.gop_capability.skip_ref_support = TD_FALSE;
        chn_cap.gop_capability.max_frame_num = 1;
 
        chn_cap.feature_capability.user_data_support = TD_FALSE;
        chn_cap.feature_capability.watermark_support = TD_FALSE;
        chn_cap.feature_capability.deblur_support = TD_FALSE;
        chn_cap.feature_capability.quality_balance_support = TD_FALSE;
        chn_cap.feature_capability.md_support = TD_FALSE;
        chn_cap.feature_capability.jpeg_roi_support = TD_FALSE;
        //chn_cap.feature_capability.bitrate_strategy_support = OT_VENC_CRR_RECODE_DISABLE; // 不支持设置
        chn_cap.feature_capability.svc_support = OT_VENC_SVC_SUPPORT_NONE;
        chn_cap.feature_capability.svc_v2_max_ref_num = 2;

        DBG("payload_support:%u, max_pic_width:%u, max_pic_height:%u, buf_size:%u\n", 
            chn_cap.attr_capability.payload_support, chn_cap.attr_capability.max_pic_width,
            chn_cap.attr_capability.max_pic_height, chn_cap.attr_capability.buf_size);

        ret = ss_mpi_venc_set_chn_capability(chn, &chn_cap);
        ENCODE_RET_BREAK(ret, "ss_mpi_venc_set_chn_capability(%d) failed\n", chn);
    } while(0);

    return ret;
}

td_s32 encode_venc_h265_chn_param_init(ot_venc_chn venc_chn, VideoEncS *venc_param, ot_venc_chn_attr *chn_attr)
{
    unsigned int width = 0;
    unsigned int height = 0;
    unsigned int max_width = 0;
    unsigned int max_height = 0;

    encode_idx_to_resolution(encode_max_idx(venc_chn), (int *)&max_width, (int *)&max_height);
    encode_vencsize_to_resolution(venc_param->enc[venc_chn].vencsize, (int *)&width, (int *)&height);
    chn_attr->gop_attr.gop_mode = OT_VENC_GOP_MODE_NORMAL_P;
    chn_attr->gop_attr.normal_p.ip_qp_delta = 2;

    chn_attr->venc_attr.type = OT_PT_H265;
    chn_attr->venc_attr.profile = 0;
    chn_attr->venc_attr.is_by_frame = TD_TRUE;
    chn_attr->venc_attr.max_pic_width = max_width;
    chn_attr->venc_attr.max_pic_height = max_height;
    chn_attr->venc_attr.pic_width = width;
    chn_attr->venc_attr.pic_height = height;
    if (CH_FS_MAIN0 == venc_chn) {
        chn_attr->venc_attr.buf_size = OT_ALIGN_UP(1 * 1024 * 1024, 64);
    } else {
        chn_attr->venc_attr.buf_size = OT_ALIGN_UP(max_width * max_height / 2, 64);
    }

    chn_attr->venc_attr.h265_attr.frame_buf_ratio = 80;
    chn_attr->venc_attr.h265_attr.rcn_ref_share_buf_en = TD_TRUE;
    if(0 == venc_param->enc[venc_chn].fixbps) {
        chn_attr->rc_attr.rc_mode = OT_VENC_RC_MODE_H265_AVBR;
        chn_attr->rc_attr.h265_avbr.stats_time = venc_param->enc[venc_chn].gop/venc_param->enc[venc_chn].fps;
        chn_attr->rc_attr.h265_avbr.max_bit_rate =  venc_param->enc[venc_chn].bps;
        chn_attr->rc_attr.h265_avbr.src_frame_rate = ENCODE_SENSOR0_FRAME_RATE;
        chn_attr->rc_attr.h265_avbr.dst_frame_rate = venc_param->enc[venc_chn].fps;
        chn_attr->rc_attr.h265_avbr.gop = venc_param->enc[venc_chn].gop;
    } else if (1 == venc_param->enc[venc_chn].fixbps) {
        chn_attr->rc_attr.rc_mode = OT_VENC_RC_MODE_H265_CBR;
        chn_attr->rc_attr.h265_cbr.stats_time = venc_param->enc[venc_chn].gop/venc_param->enc[venc_chn].fps;
        chn_attr->rc_attr.h265_cbr.src_frame_rate = ENCODE_SENSOR0_FRAME_RATE;
        chn_attr->rc_attr.h265_cbr.dst_frame_rate = venc_param->enc[venc_chn].fps;
        chn_attr->rc_attr.h265_cbr.gop = venc_param->enc[venc_chn].gop;
        chn_attr->rc_attr.h265_cbr.bit_rate = venc_param->enc[venc_chn].bps;
    } else if (2 == venc_param->enc[venc_chn].fixbps) {
        chn_attr->rc_attr.rc_mode = OT_VENC_RC_MODE_H265_AVBR;
        chn_attr->rc_attr.h265_avbr.stats_time = venc_param->enc[venc_chn].gop/venc_param->enc[venc_chn].fps;
        chn_attr->rc_attr.h265_avbr.max_bit_rate =  venc_param->enc[venc_chn].bps;
        chn_attr->rc_attr.h265_avbr.src_frame_rate = ENCODE_SENSOR0_FRAME_RATE;
        chn_attr->rc_attr.h265_avbr.dst_frame_rate = venc_param->enc[venc_chn].fps;
        chn_attr->rc_attr.h265_avbr.gop = venc_param->enc[venc_chn].gop;
    } else if (3 == venc_param->enc[venc_chn].fixbps) {
        chn_attr->rc_attr.rc_mode = OT_VENC_RC_MODE_H265_CVBR;
        chn_attr->rc_attr.h265_cvbr.stats_time = venc_param->enc[venc_chn].gop/venc_param->enc[venc_chn].fps;
        chn_attr->rc_attr.h265_cvbr.short_term_stats_time = venc_param->enc[venc_chn].gop/venc_param->enc[venc_chn].fps;
        chn_attr->rc_attr.h265_cvbr.long_term_stats_time = venc_param->enc[venc_chn].gop/venc_param->enc[venc_chn].fps;
        chn_attr->rc_attr.h265_cvbr.max_bit_rate = venc_param->enc[venc_chn].bps;
        chn_attr->rc_attr.h265_cvbr.long_term_max_bit_rate = venc_param->enc[venc_chn].bps;
        chn_attr->rc_attr.h265_cvbr.long_term_min_bit_rate = venc_param->enc[venc_chn].bps/2;
        chn_attr->rc_attr.h265_cvbr.src_frame_rate = ENCODE_SENSOR0_FRAME_RATE;
        chn_attr->rc_attr.h265_cvbr.dst_frame_rate = venc_param->enc[venc_chn].fps;
        chn_attr->rc_attr.h265_cvbr.gop = venc_param->enc[venc_chn].gop;
    } else if(4 == venc_param->enc[venc_chn].fixbps){
        chn_attr->rc_attr.rc_mode = OT_VENC_RC_MODE_H265_ABR;
        chn_attr->rc_attr.h265_abr.stats_time = venc_param->enc[venc_chn].gop/venc_param->enc[venc_chn].fps;
        chn_attr->rc_attr.h265_abr.src_frame_rate = ENCODE_SENSOR0_FRAME_RATE;
        chn_attr->rc_attr.h265_abr.dst_frame_rate = venc_param->enc[venc_chn].fps;
        chn_attr->rc_attr.h265_abr.gop = venc_param->enc[venc_chn].gop;
        chn_attr->rc_attr.h265_abr.bit_rate = venc_param->enc[venc_chn].bps;
        chn_attr->rc_attr.h265_abr.vbv_buf_delay = 20;
    }

    return 0;
}

td_s32 encode_venc_h264_chn_param_init(ot_venc_chn venc_chn, VideoEncS *venc_param, ot_venc_chn_attr *chn_attr)
{
    unsigned int width = 0;
    unsigned int height = 0;
    unsigned int max_width = 0;
    unsigned int max_height = 0;

    encode_idx_to_resolution(encode_max_idx(venc_chn), (int *)&max_width, (int *)&max_height);
    encode_vencsize_to_resolution(venc_param->enc[venc_chn].vencsize, (int *)&width, (int *)&height);
    chn_attr->gop_attr.gop_mode = OT_VENC_GOP_MODE_NORMAL_P;
    chn_attr->gop_attr.normal_p.ip_qp_delta = 2;

    chn_attr->venc_attr.type = OT_PT_H264;
    chn_attr->venc_attr.profile = 0;
    chn_attr->venc_attr.is_by_frame = TD_TRUE;
    chn_attr->venc_attr.max_pic_width = max_width;
    chn_attr->venc_attr.max_pic_height = max_height;
    chn_attr->venc_attr.pic_width = width;
    chn_attr->venc_attr.pic_height = height;
    if (CH_FS_MAIN0 == venc_chn) {
        chn_attr->venc_attr.buf_size = OT_ALIGN_UP(1 * 1024 * 1024, 64);
    } else {
        chn_attr->venc_attr.buf_size = OT_ALIGN_UP(max_width * max_height / 2, 64);
    }

    chn_attr->venc_attr.h264_attr.frame_buf_ratio = 80;
    chn_attr->venc_attr.h264_attr.rcn_ref_share_buf_en = TD_TRUE;
    if(0 == venc_param->enc[venc_chn].fixbps) {
        chn_attr->rc_attr.rc_mode = OT_VENC_RC_MODE_H264_AVBR;
        chn_attr->rc_attr.h264_avbr.stats_time = venc_param->enc[venc_chn].gop/venc_param->enc[venc_chn].fps;
        chn_attr->rc_attr.h264_avbr.src_frame_rate = ENCODE_SENSOR0_FRAME_RATE;
        chn_attr->rc_attr.h264_avbr.dst_frame_rate = venc_param->enc[venc_chn].fps;
        chn_attr->rc_attr.h264_avbr.gop = venc_param->enc[venc_chn].gop;
        chn_attr->rc_attr.h264_avbr.max_bit_rate = venc_param->enc[venc_chn].bps;
    } else if(1 == venc_param->enc[venc_chn].fixbps){
        chn_attr->rc_attr.rc_mode = OT_VENC_RC_MODE_H264_CBR;
        chn_attr->rc_attr.h264_cbr.stats_time = venc_param->enc[venc_chn].gop/venc_param->enc[venc_chn].fps;
        chn_attr->rc_attr.h264_cbr.src_frame_rate = ENCODE_SENSOR0_FRAME_RATE;
        chn_attr->rc_attr.h264_cbr.dst_frame_rate = venc_param->enc[venc_chn].fps;
        chn_attr->rc_attr.h264_cbr.gop = venc_param->enc[venc_chn].gop;
        chn_attr->rc_attr.h264_cbr.bit_rate = venc_param->enc[venc_chn].bps;
    } else if(2 == venc_param->enc[venc_chn].fixbps){
        chn_attr->rc_attr.rc_mode = OT_VENC_RC_MODE_H264_AVBR;
        chn_attr->rc_attr.h264_avbr.stats_time = venc_param->enc[venc_chn].gop/venc_param->enc[venc_chn].fps;
        chn_attr->rc_attr.h264_avbr.src_frame_rate = ENCODE_SENSOR0_FRAME_RATE;
        chn_attr->rc_attr.h264_avbr.dst_frame_rate = venc_param->enc[venc_chn].fps;
        chn_attr->rc_attr.h264_avbr.gop = venc_param->enc[venc_chn].gop;
        chn_attr->rc_attr.h264_avbr.max_bit_rate = venc_param->enc[venc_chn].bps;
    } else if(3 == venc_param->enc[venc_chn].fixbps){
        chn_attr->rc_attr.rc_mode = OT_VENC_RC_MODE_H264_CVBR;
        chn_attr->rc_attr.h264_cvbr.stats_time = venc_param->enc[venc_chn].gop/venc_param->enc[venc_chn].fps;
        chn_attr->rc_attr.h264_cvbr.src_frame_rate = ENCODE_SENSOR0_FRAME_RATE;
        chn_attr->rc_attr.h264_cvbr.dst_frame_rate = venc_param->enc[venc_chn].fps;
        chn_attr->rc_attr.h264_cvbr.gop = venc_param->enc[venc_chn].gop;
        chn_attr->rc_attr.h264_cvbr.max_bit_rate = venc_param->enc[venc_chn].bps;
    } else if(4 == venc_param->enc[venc_chn].fixbps){
        chn_attr->rc_attr.rc_mode = OT_VENC_RC_MODE_H264_ABR;
        chn_attr->rc_attr.h264_abr.stats_time = venc_param->enc[venc_chn].gop/venc_param->enc[venc_chn].fps;
        chn_attr->rc_attr.h264_abr.src_frame_rate = ENCODE_SENSOR0_FRAME_RATE;
        chn_attr->rc_attr.h264_abr.dst_frame_rate = venc_param->enc[venc_chn].fps;
        chn_attr->rc_attr.h264_abr.gop = venc_param->enc[venc_chn].gop;
        chn_attr->rc_attr.h264_abr.bit_rate = venc_param->enc[venc_chn].bps;
        chn_attr->rc_attr.h264_abr.vbv_buf_delay = 20;
    }

    return 0;
}

td_s32 encode_venc_jpeg_chn_param_init(ot_venc_chn venc_chn, VideoEncS *venc_param, ot_venc_chn_attr *chn_attr)
{
    unsigned int width = 640;
    unsigned int height = 360;

    chn_attr->gop_attr.gop_mode = OT_VENC_GOP_MODE_NORMAL_P;
    chn_attr->gop_attr.normal_p.ip_qp_delta = 2;

    chn_attr->venc_attr.type = OT_PT_JPEG;
    chn_attr->venc_attr.profile = 0;
    chn_attr->venc_attr.is_by_frame = TD_TRUE;
    chn_attr->venc_attr.max_pic_width = width;
    chn_attr->venc_attr.max_pic_height = height;
    chn_attr->venc_attr.pic_width = width;
    chn_attr->venc_attr.pic_height = height;
    chn_attr->venc_attr.buf_size = OT_ALIGN_UP(width * height * 3 / 2, 64);

    chn_attr->venc_attr.jpeg_attr.dcf_en = TD_FALSE;
    chn_attr->venc_attr.jpeg_attr.mpf_cfg.large_thumbnail_num = TD_FALSE;
    chn_attr->venc_attr.jpeg_attr.recv_mode = OT_VENC_PIC_RECV_SINGLE;

    return 0;
}

td_s32 encode_venc_chn_param_init(ot_venc_chn venc_chn, VideoEncS *venc_param, ot_venc_chn_attr *chn_attr)
{
    int ret = 0;

    switch (venc_param->enc[venc_chn].codec) {
        case VENC_FORMAT_H265:
            ret = encode_venc_h265_chn_param_init(venc_chn, venc_param, chn_attr);
            break;

        case VENC_FORMAT_H264:
            ret = encode_venc_h264_chn_param_init(venc_chn, venc_param, chn_attr);
            break;

        case VENC_FORMAT_MJPEG:
            ret = encode_venc_jpeg_chn_param_init(venc_chn, venc_param, chn_attr);
            break;

        default:
            ERR("can't support this type (%d) in this version!\n", venc_param->enc[venc_chn].codec);
            return -1;
    }

    return ret;
}

int encode_venc_set_chn_param(int venc_chn, VideoEncS *venc_param)
{
    int ret = 0;
    ot_venc_chn_attr chn_attr = {0};

    do {
        ret = encode_venc_chn_param_init(venc_chn, venc_param, &chn_attr);
        ENCODE_RET_BREAK(ret, "encode_venc_chn_param_init failed\n");

        ret = ss_mpi_venc_set_chn_attr(venc_chn, &chn_attr);
        ENCODE_RET_BREAK(ret, "ss_mpi_venc_set_chn_attr failed\n");

        ret = encode_venc_rc_params_init(venc_chn);
        ENCODE_RET_BREAK(ret, "encode_venc_rc_params_init failed\n");
    }while(0);

    return TD_SUCCESS;
}

int encode_venc_chn_frame_strategy(int vechn)
{
    int ret = 0;
    ot_venc_frame_lost_strategy frame_lost_strategy = {0};

    frame_lost_strategy.enable = TD_TRUE;
    frame_lost_strategy.mode = OT_VENC_FRAME_LOST_NORMAL;
    frame_lost_strategy.frame_gap = 0;
    frame_lost_strategy.bit_rate_threshold = 1000*1024*8;
    ret = ss_mpi_venc_set_frame_lost_strategy(vechn, &frame_lost_strategy);
    ENCODE_RET_CHECK(ret, "ss_mpi_venc_set_frame_lost_strategy failed\n");

    ot_venc_super_frame_strategy super_frame_param = {0};
    super_frame_param.super_frame_mode = OT_VENC_SUPER_FRAME_DISCARD;
    super_frame_param.i_frame_bits_threshold = (MAX_FRAME_BYTES*8);
    super_frame_param.p_frame_bits_threshold = (MAX_FRAME_BYTES*8/5);
    super_frame_param.reencode_priority = OT_VENC_REENCODE_BIT_RATE_FIRST;
    super_frame_param.discard_param.qp_delta = 3;                           //超大帧丢帧后下一帧起始qp调节阈值
    super_frame_param.discard_param.max_lost_times = 10;                     //最大连续丢帧次数
    ret = ss_mpi_venc_set_super_frame_strategy(vechn, &super_frame_param);
    ENCODE_RET_CHECK(ret, "ss_mpi_venc_set_super_frame_strategy failed\n");

    return ret;
}

int encode_venc_frame_strategy(void)
{
    int ret = 0;
    int vechn  = 0;
    for (vechn = 0; vechn < CH_FS_H26X_END; vechn++) {
        ret = encode_venc_chn_frame_strategy(vechn);
        ENCODE_RET_CHECK(ret, "encode_venc_chn_frame_strategy failed\n");
    }

    return ret;
}

td_s32 encode_venc_chn_create(ot_venc_chn venc_chn, VideoEncS *venc_param)
{
    td_s32 ret = 0;
    ot_venc_chn_attr chn_attr = {0};
    static int initialized[CH_FS_END] = {FALSE};

    do {
        ret = encode_venc_chn_param_init(venc_chn, venc_param, &chn_attr);
        ENCODE_RET_BREAK(ret, "encode_venc_chn_param_init failed\n");

        if (!initialized[venc_chn]) {
            ret = ss_mpi_venc_create_chn(venc_chn, &chn_attr);
            ENCODE_RET_BREAK(ret, "ss_mpi_venc_create_chn(%d) failed\n", venc_chn);
            initialized[venc_chn] = TRUE;
        } else {
            ret = ss_mpi_venc_set_chn_attr(venc_chn, &chn_attr);
            ENCODE_RET_BREAK(ret, "ss_mpi_venc_set_chn_attr(%d) failed\n", venc_chn);
        }

        if (chn_attr.venc_attr.type == OT_PT_JPEG) {
            break;
        }

        ret = encode_venc_rc_params_init(venc_chn);
        ENCODE_RET_BREAK(ret, "encode_venc_close_reencode failed\n");
    } while(0);

    return TD_SUCCESS;
}

int encode_venc_chn_start(int venc_chn, VideoEncS *venc_param)
{
    int ret = 0;
    ot_venc_start_param start_param = {0};

    do {
        /* step 1: create encode chnl */
        ret = encode_venc_chn_create(venc_chn, venc_param);
        ENCODE_RET_BREAK(ret, "encode_venc_chn_create failed\n");

        /* step 2:  start recv venc pictures */
        start_param.recv_pic_num = -1;
        ret = ss_mpi_venc_start_chn(venc_chn, &start_param);
        ENCODE_RET_BREAK(ret, "ss_mpi_venc_start_chn failed\n");
    } while(0);

    return ret;
}

int encode_venc_mini_buf_en()
{
    int ret = 0;
    ot_venc_mod_type mod[] = {OT_VENC_MOD_H264,OT_VENC_MOD_H265,OT_VENC_MOD_JPEG};
    td_s32 i;
    ot_venc_mod_param mod_param = {0};

    for (i = 0; i < sizeof(mod)/sizeof(mod[0]); i++) {
        mod_param.mod_type = mod[i];

        ret = ss_mpi_venc_get_mod_param(&mod_param);
        if (ret != TD_SUCCESS) {
            ERR("ss_mpi_venc_get_mod_param %d failed with %#x!\n", mod_param.mod_type,ret);
            return ret;
        }

        //0：码流buffer根据分辨率分配 ; 1：码流buffer下限为32k，用户保证合理 ; 默认值：0
        switch (mod_param.mod_type) {
            case OT_VENC_MOD_H264:
                mod_param.h264_mod_param.mini_buf_mode = 1;
                break;
            case OT_VENC_MOD_H265:
                mod_param.h265_mod_param.mini_buf_mode = 1;
                break;
            case OT_VENC_MOD_JPEG:
                mod_param.jpeg_mod_param.mini_buf_mode = 1;
                break;
            case OT_VENC_MOD_SVAC3:
                mod_param.svac3_mod_param.mini_buf_mode = 1;
                break;
            default:
                return TD_FAILURE;
        }

        ret = ss_mpi_venc_set_mod_param(&mod_param);
        if (ret != TD_SUCCESS) {
            ERR("ss_mpi_venc_set_mod_param failed with %#x!\n", ret);
            return ret;
        }
    }

    return ret;
}

int encode_venc_init(ot_vpss_grp vpss_grp, VideoEncS *venc_param)
{
    int ret = 0;

    do {
        ret = encode_venc_mini_buf_en();
        ENCODE_RET_BREAK(ret, "encode_venc_mini_buf_en failed\n");

        for (int chn = 0; chn <= CH_FS_H26X_END; chn++) {
            ret = encode_venc_set_capability(chn);
            ENCODE_RET_BREAK(ret, "encode_venc_set_capability(%d) failed\n", chn);

            ret = encode_venc_chn_start(chn, venc_param);
            ENCODE_RET_BREAK(ret, "encode_venc_chn_start failed\n");
            if (chn < CH_FS_H26X_END) {
                ret = encode_vpss_bind_venc(vpss_grp, chn, chn);
                ENCODE_RET_BREAK(ret, "encode_vpss_bind_venc failed\n");
            }
        }
    } while(0);

    return ret;
}

int encode_venc_chn_stop(int venc_chn)
{
    int ret = 0;

    do {
        /* stop venc chn */
        ret = ss_mpi_venc_stop_chn(venc_chn);
        ENCODE_RET_BREAK(ret, "ss_mpi_venc_stop_chn failed\n");

        /* distroy venc channel */
        ret = ss_mpi_venc_destroy_chn(venc_chn);
        ENCODE_RET_BREAK(ret, "ss_mpi_venc_destroy_chn failed\n");
    } while(0);

    return ret;
}

int encode_venc_uninit(ot_vpss_grp vpss_grp)
{
    int ret = 0;

    do {
        for (ot_venc_chn chn = 0; chn <= CH_FS_H26X_END; chn++) {
            if(chn < CH_FS_H26X_END) {
                ret = encode_vpss_unbind_venc(vpss_grp, chn, chn);
                ENCODE_RET_BREAK(ret, "encode_vpss_unbind_venc failed\n");
            }

            ret = encode_venc_chn_stop(chn);
            ENCODE_RET_BREAK(ret, "encode_venc_chn_stop failed\n");
        }
    } while(0);

    return ret;
}
