#ifndef __ENCODE_VPSS_H__
#define __ENCODE_VPSS_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "ot_type.h"
#include "ot_common.h"
#include "ot_common_sys.h"
#include "ot_common_video.h"
#include "ot_common_vpss.h"
#include "jconfstruct.h"
#include "encode_vi.h"

#define VIE_MAX_ROUTE_NUM 4
#define VPSS_MAX_CHN_NUM  3
#define VPSS_DST_FRAME_RATE 5

typedef struct {
    td_s32               route_num;
    ot_vi_vpss_mode_type mode_type;
    ot_fmu_mode          vi_fmu[VIE_MAX_ROUTE_NUM];
    ot_fmu_mode          vpss_fmu[VIE_MAX_ROUTE_NUM];
    td_bool              vpss_wrap_en;
    td_u32               vpss_wrap_size;
} sys_cfg_t;

typedef struct {
    ot_vpss_grp                 vpss_grp;
    ot_vpss_grp_attr            grp_attr;
    ot_3dnr_attr                nr_attr;
    td_bool                     chn_en[VPSS_MAX_CHN_NUM];
    ot_vpss_chn_attr            chn_attr[VPSS_MAX_CHN_NUM];
    ot_vpss_chn_buf_wrap_attr   wrap_attr[VPSS_MAX_CHN_NUM];
} vpss_cfg_t;

typedef struct {
    ot_vpss_chn_attr chn_attr[VPSS_MAX_CHN_NUM];
    td_bool chn_enable[VPSS_MAX_CHN_NUM];
    ot_vpss_chn_buf_wrap_attr wrap_attr[VPSS_MAX_CHN_NUM];
    td_u32 chn_array_size;
} vpss_chn_attr_t;

void encode_vpss_get_cfg(sns_type_t sns_type, vpss_cfg_t *vpss_cfg, VideoEncS *enc_param);
td_void encode_vpss_wrap_cfg_init(sns_type_t sns_type, sys_cfg_t *sys_cfg,vpss_cfg_t *vpss_cfg);
int encode_vpss_set_chn_param(ot_vpss_grp vpss_grp, ot_vpss_chn vpss_chn, td_u32 width, td_u32 height);
int encode_vpss_init(ot_vpss_grp grp, vpss_cfg_t *vpss_cfg);
int encode_vpss_uninit(ot_vpss_grp grp, td_u32 chn_array_size);

#ifdef __cplusplus
}
#endif
#endif

