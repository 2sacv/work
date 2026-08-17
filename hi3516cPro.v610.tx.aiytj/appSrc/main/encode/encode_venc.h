#ifndef __ENCODE_VENC_H__
#define __ENCODE_VENC_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "ot_common.h"
#include "jconfstruct.h"

int encode_venc_set_chn_param(int venc_chn, VideoEncS *venc_param);

int encode_venc_chn_frame_strategy(int vechn);
int encode_venc_frame_strategy(void);

int encode_venc_chn_start(int venc_chn, VideoEncS *venc_param);
int encode_venc_chn_stop(int venc_chn);

int encode_venc_init(ot_vpss_grp vpss_grp, VideoEncS *venc_param);
int encode_venc_uninit(ot_vpss_grp vpss_grp);

#ifdef __cplusplus
}
#endif
#endif

