#ifndef __ENCODE_BIND_H__
#define __ENCODE_BIND_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "ot_type.h"
#include "ot_common.h"

td_s32 encode_vi_bind_vpss(ot_vi_pipe vi_pipe, ot_vi_chn vi_chn, ot_vpss_grp vpss_grp, ot_vpss_chn vpss_chn);
td_s32 encode_vi_unbind_vpss(ot_vi_pipe vi_pipe, ot_vi_chn vi_chn, ot_vpss_grp vpss_grp, ot_vpss_chn vpss_chn);
td_s32 encode_vpss_bind_venc(ot_vpss_grp vpss_grp, ot_vpss_chn vpss_chn, ot_venc_chn venc_chn);
td_s32 encode_vpss_unbind_venc(ot_vpss_grp vpss_grp, ot_vpss_chn vpss_chn, ot_venc_chn venc_chn);
td_s32 encode_ai_bind_aenc(ot_audio_dev ai_dev, ot_ai_chn ai_chn, ot_aenc_chn ae_chn);
td_s32 encode_ai_unbind_aenc(ot_audio_dev ai_dev, ot_ai_chn ai_chn, ot_aenc_chn ae_chn);


#ifdef __cplusplus
}
#endif
#endif

