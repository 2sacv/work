#include "ss_mpi_sys_bind.h"
#include "encode_bind.h"

td_s32 encode_vi_bind_vpss(ot_vi_pipe vi_pipe, ot_vi_chn vi_chn, ot_vpss_grp vpss_grp, ot_vpss_chn vpss_chn)
{
    ot_mpp_chn src_chn = {0};
    ot_mpp_chn dest_chn = {0};

    src_chn.mod_id = OT_ID_VI;
    src_chn.dev_id = vi_pipe;
    src_chn.chn_id = vi_chn;

    dest_chn.mod_id = OT_ID_VPSS;
    dest_chn.dev_id = vpss_grp;
    dest_chn.chn_id = vpss_chn;

    return ss_mpi_sys_bind(&src_chn, &dest_chn);
}

td_s32 encode_vi_unbind_vpss(ot_vi_pipe vi_pipe, ot_vi_chn vi_chn, ot_vpss_grp vpss_grp, ot_vpss_chn vpss_chn)
{
    ot_mpp_chn src_chn = {0};
    ot_mpp_chn dest_chn = {0};

    src_chn.mod_id = OT_ID_VI;
    src_chn.dev_id = vi_pipe;
    src_chn.chn_id = vi_chn;

    dest_chn.mod_id = OT_ID_VPSS;
    dest_chn.dev_id = vpss_grp;
    dest_chn.chn_id = vpss_chn;

    return ss_mpi_sys_unbind(&src_chn, &dest_chn);
}


td_s32 encode_vpss_bind_venc(ot_vpss_grp vpss_grp, ot_vpss_chn vpss_chn, ot_venc_chn venc_chn)
{
    ot_mpp_chn src_chn = {0};
    ot_mpp_chn dest_chn = {0};

    src_chn.mod_id = OT_ID_VPSS;
    src_chn.dev_id = vpss_grp;
    src_chn.chn_id = vpss_chn;

    dest_chn.mod_id = OT_ID_VENC;
    dest_chn.dev_id = 0;
    dest_chn.chn_id = venc_chn;

    return ss_mpi_sys_bind(&src_chn, &dest_chn);
}

td_s32 encode_vpss_unbind_venc(ot_vpss_grp vpss_grp, ot_vpss_chn vpss_chn, ot_venc_chn venc_chn)
{
    ot_mpp_chn src_chn = {0};
    ot_mpp_chn dest_chn = {0};

    src_chn.mod_id = OT_ID_VPSS;
    src_chn.dev_id = vpss_grp;
    src_chn.chn_id = vpss_chn;

    dest_chn.mod_id = OT_ID_VENC;
    dest_chn.dev_id = 0;
    dest_chn.chn_id = venc_chn;

    return ss_mpi_sys_unbind(&src_chn, &dest_chn);
}

td_s32 encode_ai_bind_aenc(ot_audio_dev ai_dev, ot_ai_chn ai_chn, ot_aenc_chn ae_chn)
{
    ot_mpp_chn src_chn = {0};
    ot_mpp_chn dest_chn = {0};

    src_chn.mod_id = OT_ID_AI;
    src_chn.dev_id = ai_dev;
    src_chn.chn_id = ai_chn;
    dest_chn.mod_id = OT_ID_AENC;
    dest_chn.dev_id = 0;
    dest_chn.chn_id = ae_chn;

    return ss_mpi_sys_bind(&src_chn, &dest_chn);
}

td_s32 encode_ai_unbind_aenc(ot_audio_dev ai_dev, ot_ai_chn ai_chn, ot_aenc_chn ae_chn)
{
    ot_mpp_chn src_chn = {0};
    ot_mpp_chn dest_chn = {0};

    src_chn.mod_id = OT_ID_AI;
    src_chn.dev_id = ai_dev;
    src_chn.chn_id = ai_chn;
    dest_chn.mod_id = OT_ID_AENC;
    dest_chn.dev_id = 0;
    dest_chn.chn_id = ae_chn;

    return ss_mpi_sys_unbind(&src_chn, &dest_chn);
}

