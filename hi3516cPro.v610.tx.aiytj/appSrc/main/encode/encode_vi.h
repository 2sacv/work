#ifndef __ENCODE_VI_H__
#define __ENCODE_VI_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "ot_type.h"

#include "ot_mipi_rx.h"
#include "ot_common_vi.h"
#include "ot_common_video.h"
#include "ot_common_isp.h"
#include "ot_common_sys.h"

typedef enum {
    SC4336P_MIPI_4M_30FPS_10BIT,
    OS04D10_MIPI_4M_30FPS_10BIT,
    GC4023_MIPI_4M_30FPS_10BIT,
    SC431HAI_MIPI_4M_30FPS_10BIT,
    SC431HAI_MIPI_4M_30FPS_10BIT_WDR2TO1,
    SC450AI_MIPI_4M_30FPS_10BIT,
    SC450AI_MIPI_4M_30FPS_10BIT_WDR2TO1,
    SC500AI_MIPI_5M_30FPS_10BIT,
    SC500AI_MIPI_5M_30FPS_10BIT_WDR2TO1,
    SC4336P_MIPI_3M_30FPS_10BIT,
    SC4336P_MIPI_2M_30FPS_10BIT,
    SC465SL_MIPI_4M_30FPS_12BIT,
    SC235_MIPI_2M_15FPS_10BIT,
    SNS_TYPE_BUTT,
} sns_type_t;

typedef struct {
    sns_type_t sns_type;
    td_u32          sns_clk_src;
    td_u32          sns_rst_src;
    td_u32          bus_id;
    td_bool         sns_clk_rst_en;
} sns_info_t;

typedef struct {
    td_s32             mipi_dev;
    lane_divide_mode_t divide_mode;
    combo_dev_attr_t   combo_dev_attr;
    ext_data_type_t    ext_data_type_attr;
} mipi_info_t;

typedef struct {
    ot_vi_dev      vi_dev;
    ot_vi_dev_attr dev_attr;
    ot_vi_bas_attr bas_attr;
} vi_dev_info_t;

typedef struct {
    td_u32                    grp_num;
    ot_vi_grp                 fusion_grp[OT_VI_MAX_WDR_FUSION_GRP_NUM];
    ot_vi_wdr_fusion_grp_attr fusion_grp_attr[OT_VI_MAX_WDR_FUSION_GRP_NUM];
} vi_grp_info_t;

typedef struct {
    ot_vi_chn      vi_chn;
    ot_vi_chn_attr chn_attr;
    ot_fmu_mode    fmu_mode;
} vi_chn_info_t;

typedef struct {
    ot_vi_pipe_attr    pipe_attr;
    td_bool            pipe_need_start;
	td_bool            isp_need_run;
    ot_isp_pub_attr    isp_info;
    td_bool            isp_be_end_trigger;
    td_bool            isp_quick_start;
    td_u32             chn_num;
    vi_chn_info_t      chn_info[OT_VI_MAX_PHYS_CHN_NUM];
    ot_3dnr_attr       nr_attr;
    td_bool            vc_change_en;
    td_u32             vc_number;
    td_bool            is_master_pipe;
    td_u32             bnr_bnf_num;
    ot_vi_pipe_buf_wrap_attr wrap_attr;
    td_u32             pixel_rate;
    td_bool            set_early_end_mode;
    ot_vb_pool         attach_pool;
    ot_low_delay_info low_delay_info;
    ot_frame_interrupt_attr frame_interrupt_attr;
} vi_pipe_info_t;

typedef struct {
    sns_info_t 	sns_info;
    mipi_info_t	mipi_info;
    vi_dev_info_t dev_info;
    ot_vi_bind_pipe bind_pipe;
    vi_grp_info_t grp_info;
    vi_pipe_info_t pipe_info[OT_VI_MAX_PHYS_PIPE_NUM];
} vi_cfg_t;

void encode_vi_get_default_vi_cfg(sns_type_t sns_type, vi_cfg_t *vi_cfg);

int encode_vi_set_vi_vpss_mode(ot_vi_vpss_mode_type mode_type, ot_vi_aiisp_mode aiisp_mode);

void encode_vi_get_size_by_sns_type(sns_type_t sns_type, ot_size *size);

int encode_vi_init(vi_cfg_t *vi_cfg);

void encode_vi_uninit(vi_cfg_t *vi_cfg);

#ifdef __cplusplus
}
#endif
#endif

