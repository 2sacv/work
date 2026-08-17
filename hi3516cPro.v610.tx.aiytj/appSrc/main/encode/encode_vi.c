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

#include "debug.h"
#include "securec.h"
#include "ss_mpi_sys.h"
#include "ss_mpi_vi.h"
#include "encode_vi.h"
#include "encode_vi_isp.h"
#include "encode_common.h"

#define MIPI_NUM        3
#define WIDTH_3840      3840
#define HEIGHT_2160     2160
#define WIDTH_2560      2560
#define HEIGHT_1440     1440
#define WIDTH_1920      1920
#define HEIGHT_1080     1080
#define OB_HEIGHT_START 0
#define MIPI_DEV_NAME "/dev/ot_mipi_rx"

static ext_data_type_t g_mipi_ext_data_type_default_attr = {
    .devno = 0,
    .num = MIPI_NUM,
    .ext_data_bit_width = {12, 12, 12},
    .ext_data_type = {0x2c, 0x2c, 0x2c}
};

static combo_dev_attr_t g_mipi_2lane_chn0_sensor_sc4336p_10bit_4m_nowdr_attr = {
    .devno = 0,
    .input_mode = INPUT_MODE_MIPI,
    .data_rate = MIPI_DATA_RATE_X1,
    .img_rect = {0, 0, WIDTH_2560, HEIGHT_1440},
    .mipi_attr = {
        DATA_TYPE_RAW_12BIT,
        OT_MIPI_WDR_MODE_NONE,
        {0, 2, -1, -1}
    }
};

static combo_dev_attr_t g_mipi_2lane_chn0_sensor_sc4336p_10bit_4m_nowdr_dev0_attr = {
    .devno = 0,
    .input_mode = INPUT_MODE_MIPI,
    .data_rate = MIPI_DATA_RATE_X1,
    .img_rect = {0, 0, WIDTH_2560, HEIGHT_1440},
    .mipi_attr = {
        DATA_TYPE_RAW_12BIT,
        OT_MIPI_WDR_MODE_NONE,
        {0, 2, -1, -1}
    }
};

static combo_dev_attr_t g_mipi_2lane_chn0_sensor_sc4336p_10bit_4m_nowdr_dev1_attr = {
    .devno = 1,
    .input_mode = INPUT_MODE_MIPI,
    .data_rate = MIPI_DATA_RATE_X1,
    .img_rect = {0, 0, WIDTH_2560, HEIGHT_1440},
    .mipi_attr = {
        DATA_TYPE_RAW_12BIT,
        OT_MIPI_WDR_MODE_NONE,
        {1, 3, -1, -1}
    }
};

static combo_dev_attr_t g_mipi_2lane_chn0_sensor_sc465sl_12bit_4m_nowdr_dev0_attr = {
    .devno = 0,
    .input_mode = INPUT_MODE_MIPI,
    .data_rate = MIPI_DATA_RATE_X1,
    .img_rect = {0, 0, WIDTH_2560, HEIGHT_1440},
    .mipi_attr = {
        DATA_TYPE_RAW_12BIT,
        OT_MIPI_WDR_MODE_NONE,
        {0, 2, -1, -1}
    }
};

static combo_dev_attr_t g_mipi_4lane_chn0_sensor_sc465sl_12bit_4m_nowdr_dev1_attr = {
    .devno = 0,
    .input_mode = INPUT_MODE_MIPI,
    .data_rate = MIPI_DATA_RATE_X1,
    .img_rect = {0, 0, WIDTH_2560, HEIGHT_1440},
    .mipi_attr = {
        DATA_TYPE_RAW_12BIT,
        OT_MIPI_WDR_MODE_NONE,
        {0, 1, 2, 3}
    }
};

static combo_dev_attr_t g_mipi_2lane_chn0_sensor_sc235_10bit_2m_nowdr_dev0_attr = {
    .devno = 0,
    .input_mode = INPUT_MODE_MIPI,
    .data_rate = MIPI_DATA_RATE_X1,
    .img_rect = {0, 0, WIDTH_1920, HEIGHT_1080},
    .mipi_attr = {
        DATA_TYPE_RAW_10BIT,
        OT_MIPI_WDR_MODE_NONE,
        {0, 2, -1, -1}
    }
};

static combo_dev_attr_t g_mipi_2lane_chn0_sensor_sc235_10bit_2m_nowdr_dev1_attr = {
    .devno = 1,
    .input_mode = INPUT_MODE_MIPI,
    .data_rate = MIPI_DATA_RATE_X1,
    .img_rect = {0, 0, WIDTH_1920, HEIGHT_1080},
    .mipi_attr = {
        DATA_TYPE_RAW_10BIT,
        OT_MIPI_WDR_MODE_NONE,
        {0, 2, -1, -1}
    }
};

static ot_vi_dev_attr g_mipi_raw_dev_attr = {
    .intf_mode = OT_VI_INTF_MODE_MIPI,

    /* Invalid argument */
    .work_mode = OT_VI_WORK_MODE_MULTIPLEX_1,

    /* mask component */
    .component_mask = {0xfff00000, 0x00000000},

    .scan_mode = OT_VI_SCAN_PROGRESSIVE,

    /* Invalid argument */
    .ad_chn_id = {-1, -1, -1, -1},

    /* data seq */
    .data_seq = OT_VI_DATA_SEQ_YVYU,

    /* sync param */
    .sync_cfg = {
        .vsync           = OT_VI_VSYNC_FIELD,
        .vsync_neg       = OT_VI_VSYNC_NEG_HIGH,
        .hsync           = OT_VI_HSYNC_VALID_SIG,
        .hsync_neg       = OT_VI_HSYNC_NEG_HIGH,
        .vsync_valid     = OT_VI_VSYNC_VALID_SIG,
        .vsync_valid_neg = OT_VI_VSYNC_VALID_NEG_HIGH,
        .timing_blank    = {
            /* hsync_hfb      hsync_act     hsync_hhb */
            0,                0,            0,
            /* vsync0_vhb     vsync0_act    vsync0_hhb */
            0,                0,            0,
            /* vsync1_vhb     vsync1_act    vsync1_hhb */
            0,                0,            0
        }
    },

    /* data type */
    .data_type = OT_VI_DATA_TYPE_RAW,

    /* data reverse */
    .data_reverse = TD_FALSE,

    /* input size */
    .in_size = {WIDTH_3840, HEIGHT_2160},

    /* data rate */
    .data_rate = OT_DATA_RATE_X1,
};

static ot_wdr_mode encode_vi_get_wdr_mode_by_sns_type(sns_type_t sns_type)
{
    switch (sns_type) {
        case SC4336P_MIPI_4M_30FPS_10BIT:
        case SC465SL_MIPI_4M_30FPS_12BIT:
        case SC235_MIPI_2M_15FPS_10BIT:
            return OT_WDR_MODE_NONE;

        default:
            return OT_WDR_MODE_NONE;
    }
}

static td_u32 encode_vi_get_pipe_num_by_sns_type(sns_type_t sns_type)
{
    switch (sns_type) {
        case SC4336P_MIPI_4M_30FPS_10BIT:
        case SC465SL_MIPI_4M_30FPS_12BIT:
        case SC235_MIPI_2M_15FPS_10BIT:
            return 1;

        default:
            return 1;
    }
}

td_u32 encode_vi_get_obheight_by_sns_type(sns_type_t sns_type)
{
    td_u32 ob_height = OB_HEIGHT_START;
    switch (sns_type) {
        case SC4336P_MIPI_4M_30FPS_10BIT:
        case SC465SL_MIPI_4M_30FPS_12BIT:
        case SC235_MIPI_2M_15FPS_10BIT:
            ob_height = OB_HEIGHT_START;
            break;
        default:
            break;
    }

    return ob_height;
}

/* calc online pipe sensor pixel_rate: lane_rate(Mbps) * 1000000 * lane_num / bit_num / wdr_num */
static td_u32 encode_vi_calc_pipe_pixel_rate(td_u32 lane_rate, td_u32 lane_num, td_u32 bit_num, td_u32 wdr_num)
{
    td_u32 lane_base_bps = 1000 * 1000; /* base: 1Mbps */
    td_u64 sensor_total_rate = lane_rate * lane_base_bps * lane_num / bit_num;
    return sensor_total_rate / wdr_num;
}

static td_u32 encode_vi_get_sensor_pixel_rate_by_type(sns_type_t sns_type)
{
    td_u32 pixel_rate = 0;
    switch (sns_type) {
        case SC4336P_MIPI_4M_30FPS_10BIT:
            pixel_rate = encode_vi_calc_pipe_pixel_rate(630, 2, 10, 1); /* 630Mbps * 2lane / 10bit */
            break;
        case SC465SL_MIPI_4M_30FPS_12BIT:
            pixel_rate = encode_vi_calc_pipe_pixel_rate(1080, 4, 12, 1); /* 792Mbps * 4lane / 12bit */
            break;
        case SC235_MIPI_2M_15FPS_10BIT:
            pixel_rate = encode_vi_calc_pipe_pixel_rate(630, 4, 10, 1); /* 792Mbps * 4lane / 12bit */
            break;
        default:
            break;
    }

    return pixel_rate;
}

void encode_vi_get_size_by_sns_type(sns_type_t sns_type, ot_size *size)
{
    switch (sns_type) {
        case SC4336P_MIPI_4M_30FPS_10BIT:
        case SC465SL_MIPI_4M_30FPS_12BIT:
            size->width  = WIDTH_2560;
            size->height = HEIGHT_1440;
            break;
        case SC235_MIPI_2M_15FPS_10BIT:
            size->width  = WIDTH_1920;
            size->height = HEIGHT_1080;
            break;
        default:
            size->width  = WIDTH_2560;
            size->height = HEIGHT_1440;
            break;
    }
}

static td_void encode_vi_get_mipi_attr(sns_type_t sns_type, combo_dev_attr_t *combo_attr)
{
    td_u32 ob_height = OB_HEIGHT_START;
    switch (sns_type) {
        case SC4336P_MIPI_4M_30FPS_10BIT:
            (td_void)memcpy_s(combo_attr, sizeof(combo_dev_attr_t),
                &g_mipi_2lane_chn0_sensor_sc4336p_10bit_4m_nowdr_attr, sizeof(combo_dev_attr_t));
            break;

        case SC465SL_MIPI_4M_30FPS_12BIT:
            (td_void)memcpy_s(combo_attr, sizeof(combo_dev_attr_t),
                &g_mipi_4lane_chn0_sensor_sc465sl_12bit_4m_nowdr_dev1_attr, sizeof(combo_dev_attr_t));
            break;

        case SC235_MIPI_2M_15FPS_10BIT:
            (td_void)memcpy_s(combo_attr, sizeof(combo_dev_attr_t),
                &g_mipi_2lane_chn0_sensor_sc235_10bit_2m_nowdr_dev0_attr, sizeof(combo_dev_attr_t));
            break;

        default:
            (td_void)memcpy_s(combo_attr, sizeof(combo_dev_attr_t),
                &g_mipi_2lane_chn0_sensor_sc4336p_10bit_4m_nowdr_attr, sizeof(combo_dev_attr_t));
            break;
    }
    combo_attr->img_rect.height = combo_attr->img_rect.height + ob_height;
}

static td_void encode_vi_get_mipi_attr_by_dev_id(sns_type_t sns_type, ot_vi_dev vi_dev,
    combo_dev_attr_t *combo_attr)
{
    td_u32 ob_height = OB_HEIGHT_START;
    switch (sns_type) {
        case SC4336P_MIPI_4M_30FPS_10BIT:
            if (vi_dev == 0) {
                (td_void)memcpy_s(combo_attr, sizeof(combo_dev_attr_t),
                    &g_mipi_2lane_chn0_sensor_sc4336p_10bit_4m_nowdr_dev0_attr, sizeof(combo_dev_attr_t));
            } else if (vi_dev == 1) { /* dev1 */
                (td_void)memcpy_s(combo_attr, sizeof(combo_dev_attr_t),
                    &g_mipi_2lane_chn0_sensor_sc4336p_10bit_4m_nowdr_dev1_attr, sizeof(combo_dev_attr_t));
            }
            break;

        case SC465SL_MIPI_4M_30FPS_12BIT:
            if (vi_dev == 0) {
                (td_void)memcpy_s(combo_attr, sizeof(combo_dev_attr_t),
                    &g_mipi_2lane_chn0_sensor_sc465sl_12bit_4m_nowdr_dev0_attr, sizeof(combo_dev_attr_t));
            } else if (vi_dev == 1) { /* dev1 */
                (td_void)memcpy_s(combo_attr, sizeof(combo_dev_attr_t),
                    &g_mipi_4lane_chn0_sensor_sc465sl_12bit_4m_nowdr_dev1_attr, sizeof(combo_dev_attr_t));
            }
            break;

        case SC235_MIPI_2M_15FPS_10BIT:
            if (vi_dev == 0) {
                (td_void)memcpy_s(combo_attr, sizeof(combo_dev_attr_t),
                    &g_mipi_2lane_chn0_sensor_sc235_10bit_2m_nowdr_dev0_attr, sizeof(combo_dev_attr_t));
            } else if (vi_dev == 1) { /* dev1 */
                (td_void)memcpy_s(combo_attr, sizeof(combo_dev_attr_t),
                    &g_mipi_2lane_chn0_sensor_sc235_10bit_2m_nowdr_dev1_attr, sizeof(combo_dev_attr_t));
            }
            break;

        default:
            (td_void)memcpy_s(combo_attr, sizeof(combo_dev_attr_t),
                &g_mipi_2lane_chn0_sensor_sc4336p_10bit_4m_nowdr_dev0_attr, sizeof(combo_dev_attr_t));
    }

    combo_attr->img_rect.height = combo_attr->img_rect.height + ob_height;
}

static td_void encode_vi_get_mipi_ext_data_attr(sns_type_t sns_type, ext_data_type_t *ext_data_attr)
{
    switch (sns_type) {
        case SC4336P_MIPI_4M_30FPS_10BIT:
        case SC465SL_MIPI_4M_30FPS_12BIT:
            (td_void)memcpy_s(ext_data_attr, sizeof(ext_data_type_t),
                &g_mipi_ext_data_type_default_attr, sizeof(ext_data_type_t));
            break;

        default:
            (td_void)memcpy_s(ext_data_attr, sizeof(ext_data_type_t),
                &g_mipi_ext_data_type_default_attr, sizeof(ext_data_type_t));
    }
}

static td_void encode_vi_get_dev_attr_by_intf_mode(ot_vi_intf_mode intf_mode, ot_vi_dev_attr *dev_attr)
{
    switch (intf_mode) {
        case OT_VI_INTF_MODE_MIPI:
            (td_void)memcpy_s(dev_attr, sizeof(ot_vi_dev_attr), &g_mipi_raw_dev_attr, sizeof(ot_vi_dev_attr));
            break;

        default:
            (td_void)memcpy_s(dev_attr, sizeof(ot_vi_dev_attr), &g_mipi_raw_dev_attr, sizeof(ot_vi_dev_attr));
            break;
    }
}

td_void encode_vi_get_default_pipe_info(sns_type_t sns_type, ot_vi_bind_pipe *bind_pipe,
                                             vi_pipe_info_t pipe_info[])
{
    td_u32 i = 0;
    ot_size size = {0};

    encode_vi_get_size_by_sns_type(sns_type, &size);
    for (i = 0; i < bind_pipe->pipe_num; i++) {
        /* pipe attr */
        pipe_info[i].pipe_attr.pipe_bypass_mode               = OT_VI_PIPE_BYPASS_NONE;
        pipe_info[i].pipe_attr.isp_bypass                     = TD_FALSE;
        pipe_info[i].pipe_attr.size.width                     = size.width;
        pipe_info[i].pipe_attr.size.height                    = size.height;
        pipe_info[i].pipe_attr.pixel_format                   = OT_PIXEL_FORMAT_RGB_BAYER_12BPP; /* 12fps */
        pipe_info[i].pipe_attr.compress_mode                  = OT_COMPRESS_MODE_LINE;
        pipe_info[i].pipe_attr.frame_rate_ctrl.src_frame_rate = -1;
        pipe_info[i].pipe_attr.frame_rate_ctrl.dst_frame_rate = -1;

        pipe_info[i].pipe_need_start = TD_TRUE;

        pipe_info[i].isp_need_run = TD_TRUE;
        pipe_info[i].isp_quick_start = TD_FALSE;
        pipe_info[i].isp_info.frame_rate = 5;  //5/12 fps 根据AIISP算法库模型来填
        pipe_info[i].wrap_attr.buf_line = size.height*0.3+128;
        pipe_info[i].wrap_attr.enable = TD_TRUE;

        if (i == 0) {
            pipe_info[i].is_master_pipe = TD_TRUE;
        }

        /* pub attr */
        encode_vi_isp_get_pub_attr_by_sns(sns_type, &pipe_info[i].isp_info);

        pipe_info[i].nr_attr.enable = TD_TRUE;
        pipe_info[i].nr_attr.compress_mode = OT_COMPRESS_MODE_FRAME;
        pipe_info[i].nr_attr.nr_type = OT_NR_TYPE_VIDEO_NORM;
        pipe_info[i].nr_attr.nr_motion_mode = OT_NR_MOTION_MODE_NORM;
        pipe_info[i].pixel_rate = encode_vi_get_sensor_pixel_rate_by_type(sns_type);
        pipe_info[i].attach_pool = OT_VB_INVALID_POOL_ID;
        pipe_info[i].set_early_end_mode = TD_TRUE;
        pipe_info[i].isp_be_end_trigger = TD_TRUE;

        /* chn info */
        pipe_info[i].chn_num = 1;
        pipe_info[i].chn_info[0].vi_chn                                  = 0;
        pipe_info[i].chn_info[0].fmu_mode                                = OT_FMU_MODE_OFF;
        pipe_info[i].chn_info[0].chn_attr.size.width                     = size.width;
        pipe_info[i].chn_info[0].chn_attr.size.height                    = size.height;
        pipe_info[i].chn_info[0].chn_attr.pixel_format                   = OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
        pipe_info[i].chn_info[0].chn_attr.dynamic_range                  = OT_DYNAMIC_RANGE_SDR8;
        pipe_info[i].chn_info[0].chn_attr.video_format                   = OT_VIDEO_FORMAT_LINEAR;
        pipe_info[i].chn_info[0].chn_attr.compress_mode                  = OT_COMPRESS_MODE_NONE;
        pipe_info[i].chn_info[0].chn_attr.mirror_en                      = TD_FALSE;
        pipe_info[i].chn_info[0].chn_attr.flip_en                        = TD_FALSE;
        pipe_info[i].chn_info[0].chn_attr.depth                          = 0;
        pipe_info[i].chn_info[0].chn_attr.frame_rate_ctrl.src_frame_rate = -1;
        pipe_info[i].chn_info[0].chn_attr.frame_rate_ctrl.dst_frame_rate = -1;
    }
}

static td_void encode_vi_get_default_grp_info(sns_type_t sns_type, vi_grp_info_t *grp_info)
{
    td_u32 i;
    td_u32 pipe_num;
    ot_size size;

    encode_vi_get_size_by_sns_type(sns_type, &size);
    grp_info->grp_num = 1;
    grp_info->fusion_grp[0] = 0;

    grp_info->fusion_grp_attr[0].wdr_mode = encode_vi_get_wdr_mode_by_sns_type(sns_type);
    grp_info->fusion_grp_attr[0].cache_line = size.height;
    pipe_num = encode_vi_get_pipe_num_by_sns_type(sns_type);
    for (i = 0; i < pipe_num; i++) {
        grp_info->fusion_grp_attr[0].pipe_id[i] = i;
    }
}

static td_void encode_vi_get_default_bind_info(sns_type_t sns_type, ot_vi_bind_pipe *bind_pipe)
{
    td_u32 i = 0;

    bind_pipe->pipe_num = encode_vi_get_pipe_num_by_sns_type(sns_type);
    for (i = 0; i < bind_pipe->pipe_num; i++) {
        bind_pipe->pipe_id[i] = i;
    }
}

td_void encode_vi_get_default_dev_info(sns_type_t sns_type, vi_dev_info_t *dev_info)
{
    ot_size size;
    td_u32 ob_height;

    if (SC4336P_MIPI_4M_30FPS_10BIT == sns_type){
        dev_info->vi_dev = 0;
    }else{
        dev_info->vi_dev = 0;
    }

    encode_vi_get_dev_attr_by_intf_mode(OT_VI_INTF_MODE_MIPI, &dev_info->dev_attr);
    encode_vi_get_size_by_sns_type(sns_type, &size);

    ob_height = encode_vi_get_obheight_by_sns_type(sns_type);
    dev_info->dev_attr.in_size.width  = size.width;
    dev_info->dev_attr.in_size.height = size.height + ob_height;
    dev_info->bas_attr.enable = TD_FALSE;
}

/* used for two sensor: mipi lane 2 + 2 */
td_void encode_vi_get_mipi_info_by_dev_id(sns_type_t sns_type, ot_vi_dev vi_dev, mipi_info_t *mipi_info)
{
    mipi_info->mipi_dev = vi_dev;
    mipi_info->divide_mode = LANE_DIVIDE_MODE_1;
    encode_vi_get_mipi_attr_by_dev_id(sns_type, vi_dev, &mipi_info->combo_dev_attr);
    encode_vi_get_mipi_ext_data_attr(sns_type, &mipi_info->ext_data_type_attr);
    mipi_info->ext_data_type_attr.devno = vi_dev;
}

td_void encode_vi_get_default_mipi_info(sns_type_t sns_type, mipi_info_t *mipi_info)
{
    mipi_info->mipi_dev = 0;
    mipi_info->divide_mode = LANE_DIVIDE_MODE_0;
    encode_vi_get_mipi_attr(sns_type, &mipi_info->combo_dev_attr);
    encode_vi_get_mipi_ext_data_attr(sns_type, &mipi_info->ext_data_type_attr);
}

td_void encode_vi_get_default_sns_info(sns_type_t sns_type, sns_info_t *sns_info)
{
    sns_info->sns_type    = sns_type;
    sns_info->sns_clk_src = 0;
    sns_info->sns_rst_src = 0;
    sns_info->bus_id      = 0; /* asic i2c4 , FPGA default 0 */
    sns_info->sns_clk_rst_en = TD_TRUE;
}

void encode_vi_get_default_vi_cfg(sns_type_t sns_type, vi_cfg_t *vi_cfg)
{
    (td_void)memset_s(vi_cfg, sizeof(vi_cfg_t), 0, sizeof(vi_cfg_t));

    /* sensor info */
    encode_vi_get_default_sns_info(sns_type, &vi_cfg->sns_info);
    /* mipi info */
    if(SC235_MIPI_2M_15FPS_10BIT == sns_type) {
        encode_vi_get_mipi_info_by_dev_id(sns_type, 0, &vi_cfg->mipi_info);
    } else {
        encode_vi_get_default_mipi_info(sns_type, &vi_cfg->mipi_info);
    }
    /* dev info */
    encode_vi_get_default_dev_info(sns_type, &vi_cfg->dev_info);
    /* bind info */
    encode_vi_get_default_bind_info(sns_type, &vi_cfg->bind_pipe);
    /* grp info */
    encode_vi_get_default_grp_info(sns_type, &vi_cfg->grp_info);
    /* pipe info */
    encode_vi_get_default_pipe_info(sns_type, &vi_cfg->bind_pipe, vi_cfg->pipe_info);
}

int encode_vi_set_vi_vpss_mode(ot_vi_vpss_mode_type mode_type, ot_vi_aiisp_mode aiisp_mode)
{
    td_u32 i = 0;
    int ret = 0;
    ot_vi_vpss_mode_type other_pipe_mode_type = OT_VI_OFFLINE_VPSS_OFFLINE;
    ot_vi_vpss_mode vi_vpss_mode = {{0},};

    if (mode_type == OT_VI_OFFLINE_VPSS_ONLINE) {
        other_pipe_mode_type = OT_VI_OFFLINE_VPSS_ONLINE;
    } else {
        other_pipe_mode_type = OT_VI_OFFLINE_VPSS_OFFLINE;
    }

    vi_vpss_mode.mode[0] = mode_type;
    for (i = 1; i < OT_VI_MAX_PIPE_NUM; i++) {
        vi_vpss_mode.mode[i] = other_pipe_mode_type;
    }

    do {
        ret = ss_mpi_sys_set_vi_vpss_mode(&vi_vpss_mode);
        ENCODE_RET_BREAK(ret, "ss_mpi_sys_set_vi_vpss_mode failed\n");

        ret = ss_mpi_sys_set_vi_aiisp_mode(0, aiisp_mode); /* only pipe0 can set aiisp other mode */
        ENCODE_RET_BREAK(ret, "ss_mpi_sys_set_vi_aiisp_mode failed\n");
    } while(0);

    return ret;
}

td_s32 encode_vi_set_mipi_hs_mode(lane_divide_mode_t hs_mode)
{
    td_s32 fd;
    td_s32 ret;

    fd = open(MIPI_DEV_NAME, O_RDWR);
    if (fd < 0) {
        ERR("open %s failed!\n", MIPI_DEV_NAME);
        return TD_FAILURE;
    }

    ret = ioctl(fd, OT_MIPI_SET_HS_MODE, &hs_mode);

    close(fd);

    return ret;
}

td_s32 encode_vi_mipi_ctrl_cmd(td_u32 devno, td_u32 cmd)
{
    td_s32 ret;
    td_s32 fd;

    fd = open(MIPI_DEV_NAME, O_RDWR);
    if (fd < 0) {
        ERR("open %s failed!\n", MIPI_DEV_NAME);
        return TD_FAILURE;
    }

    ret = ioctl(fd, cmd, &devno);

    close(fd);

    return ret;
}

static td_s32 encode_vi_set_mipi_combo_attr(const combo_dev_attr_t *combo_dev_attr)
{
    td_s32 fd = -1;
    td_s32 ret = 0;

    fd = open(MIPI_DEV_NAME, O_RDWR);
    if (fd < 0) {
        ERR("open %s failed!\n", MIPI_DEV_NAME);
        return TD_FAILURE;
    }

    ret = ioctl(fd, OT_MIPI_SET_DEV_ATTR, combo_dev_attr);

    close(fd);

    return ret;
}

static td_s32 encode_vi_set_mipi_ext_data_type_attr(const ext_data_type_t *ext_data_type_attr)
{
    td_s32 fd;
    td_s32 ret;

    fd = open(MIPI_DEV_NAME, O_RDWR);
    if (fd < 0) {
        ERR("open %s failed!\n", MIPI_DEV_NAME);
        return TD_FAILURE;
    }

    ret = ioctl(fd, OT_MIPI_SET_EXT_DATA_TYPE, ext_data_type_attr);

    close(fd);

    return ret;
}

td_s32 encode_vi_start_sensor(const sns_info_t *sns_info, const mipi_info_t *mipi_info)
{
    td_s32 ret = 0;

    DBG("encode_vi_start_sensor:%d\n", sns_info->sns_type);

    do {
        ret = encode_vi_set_mipi_hs_mode(mipi_info->divide_mode);
        ENCODE_RET_BREAK(ret, "encode_vi_set_mipi_hs_mode failed\n");

        ret = encode_vi_mipi_ctrl_cmd(sns_info->sns_clk_src, OT_MIPI_ENABLE_SENSOR_CLOCK);
        ENCODE_RET_BREAK(ret, "devno %u enable sensor clock failed\n", sns_info->sns_clk_src);

        ret = encode_vi_mipi_ctrl_cmd(sns_info->sns_rst_src, OT_MIPI_RESET_SENSOR);
        ENCODE_RET_BREAK(ret, "devno %u reset sensor clock failed\n", sns_info->sns_rst_src);

        ret = encode_vi_mipi_ctrl_cmd(sns_info->sns_rst_src, OT_MIPI_UNRESET_SENSOR);
        ENCODE_RET_BREAK(ret, "devno %u unreset sensor clock failed\n", sns_info->sns_rst_src);
    } while(0);

    return ret;
}

td_s32 encode_vi_start_mipi_rx(const sns_info_t *sns_info, const mipi_info_t *mipi_info)
{
    td_s32 ret = 0;

    do {
        if (sns_info->sns_clk_rst_en) {
            ret = encode_vi_start_sensor(sns_info, mipi_info);
            ENCODE_RET_BREAK(ret, "devno %d start sesor failed\n", mipi_info->mipi_dev);
        }

        ret = encode_vi_mipi_ctrl_cmd(mipi_info->mipi_dev, OT_MIPI_ENABLE_MIPI_CLOCK);
        ENCODE_RET_BREAK(ret, "devno %d enable mipi rx clock failed\n", mipi_info->mipi_dev);

        ret = encode_vi_mipi_ctrl_cmd(mipi_info->mipi_dev, OT_MIPI_RESET_MIPI);
        ENCODE_RET_BREAK(ret, "devno %d reset mipi rx clock failed\n", mipi_info->mipi_dev);

        ret = encode_vi_set_mipi_combo_attr(&mipi_info->combo_dev_attr);
        ENCODE_RET_BREAK(ret, "devno %d mipi rx set combo attr failed\n", mipi_info->mipi_dev);

        ret = encode_vi_set_mipi_ext_data_type_attr(&mipi_info->ext_data_type_attr);
        ENCODE_RET_BREAK(ret, "devno %d mipi rx set ext data attr failed\n", mipi_info->mipi_dev);

        ret = encode_vi_mipi_ctrl_cmd(mipi_info->mipi_dev, OT_MIPI_UNRESET_MIPI);
        ENCODE_RET_BREAK(ret, "devno %d unreset mipi rx failed\n", mipi_info->mipi_dev);
    }while(0);

    return ret;
}

static td_s32 encode_vi_start_dev(ot_vi_dev vi_dev, const ot_vi_dev_attr *dev_attr)
{
    td_s32 ret = 0;

    do {
        ret = ss_mpi_vi_set_dev_attr(vi_dev, dev_attr);
        ENCODE_RET_BREAK(ret, "ss_mpi_vi_set_dev_attr\n");

        ret = ss_mpi_vi_enable_dev(vi_dev);
        ENCODE_RET_BREAK(ret, "ss_mpi_vi_enable_devfailed\n");
    } while(0);

    return ret;
}

static td_s32 encode_vi_dev_bind_pipe(ot_vi_dev vi_dev, ot_vi_bind_pipe *bind_pipe)
{
    td_s32 ret = 0;

    for (int i = 0; i < bind_pipe->pipe_num; i++) {
        ret = ss_mpi_vi_bind(vi_dev, bind_pipe->pipe_id[i]);
        ENCODE_RET_BREAK(ret, "vi dev(%d) bind pipe(%d) failed with\n", vi_dev, bind_pipe->pipe_id[i]);
    }

    return ret;
}

static td_s32 encode_vi_set_grp_info(vi_grp_info_t *grp_info)
{
    td_s32 ret = 0;

    for (int i = 0; i < grp_info->grp_num; i++) {
        ret = ss_mpi_vi_set_wdr_fusion_grp_attr(grp_info->fusion_grp[i], &grp_info->fusion_grp_attr[i]);
        ENCODE_RET_BREAK(ret, "ss_mpi_vi_set_wdr_fusion_grp_attr failed\n");
    }

    return ret;
}

static td_s32 encode_vi_set_pipe_pixel_rate(ot_vi_pipe vi_pipe, td_u32 pixel_rate)
{
    ot_vi_vpss_mode vi_vpss_mode = {{0}};

    td_s32 ret = ss_mpi_sys_get_vi_vpss_mode(&vi_vpss_mode);
    if (ret != TD_SUCCESS) {
        ERR("pipe%d get vi_vpss_mode failed!", vi_pipe);
        return ret;
    }

    if (vi_vpss_mode.mode[vi_pipe] != OT_VI_ONLINE_VPSS_ONLINE &&
        vi_vpss_mode.mode[vi_pipe] != OT_VI_ONLINE_VPSS_OFFLINE) {
        return TD_SUCCESS; /* only vi online support set pixel_rate */
    }

    if (pixel_rate == 0) {
        ERR("pipe%d sns_type not adapt pixel_rate calc!", vi_pipe);
        return TD_SUCCESS; /* only vi online support set pixel_rate */
    }

    return ss_mpi_vi_set_pipe_online_clock(vi_pipe, pixel_rate);
}

static td_s32 encode_vi_init_one_pipe(ot_vi_pipe vi_pipe, vi_pipe_info_t *pipe_info)
{
    td_s32 ret = 0;

    do {
        if (pipe_info->bnr_bnf_num != 0) {
            ret = ss_mpi_vi_set_pipe_bnr_buf_num(vi_pipe, pipe_info->bnr_bnf_num);
            ENCODE_RET_BREAK(ret, "vi_pipe(%d) ss_mpi_vi_set_pipe_bnr_buf_num failed\n", vi_pipe);
        }

        if (pipe_info->wrap_attr.enable) {
            ret = ss_mpi_vi_set_pipe_buf_wrap_attr(vi_pipe, &pipe_info->wrap_attr);
            ENCODE_RET_CHECK(ret, "vi_pipe(%d) ss_mpi_vi_set_pipe_buf_wrap_attr failed\n", vi_pipe);
        }

        ret = ss_mpi_vi_create_pipe(vi_pipe, &pipe_info->pipe_attr);
        ENCODE_RET_BREAK(ret, "vi_pipe(%d) ss_mpi_vi_create_pipe failed\n", vi_pipe);

        ret = encode_vi_set_pipe_pixel_rate(vi_pipe, pipe_info->pixel_rate);
        ENCODE_RET_BREAK(ret, "vi_pipe(%d) encode_vi_set_pipe_pixel_rate failed\n", vi_pipe);

        if (pipe_info->vc_change_en) {
            ret = ss_mpi_vi_set_pipe_vc_number(vi_pipe, pipe_info->vc_number);
            ENCODE_RET_BREAK(ret, "vi_pipe(%d) ss_mpi_vi_set_chn_attr failed\n", vi_pipe);
        }

        //if (pipe_info->set_early_end_mode == TD_TRUE) {
        //    ot_frame_interrupt_attr attr = {OT_FRAME_INTERRUPT_EARLY_END, pipe_info->pipe_attr.size.height - 100};
        //    ret = ss_mpi_vi_set_pipe_frame_interrupt_attr(vi_pipe, &attr);
        //    ENCODE_RET_BREAK(ret, "vi set pipe(%d) interrupt_attr failed\n", vi_pipe);
        //}
    }while(0);

    return 0;
}

static td_s32 encode_vi_start_chn(ot_vi_pipe vi_pipe, vi_pipe_info_t *pipe_info)
{
    td_s32 ret = 0;
    td_u32 chn_num = pipe_info->chn_num;
    const vi_chn_info_t *chn_info = pipe_info->chn_info;

    do {
        for (int i = 0; i < chn_num; i++) {
            ot_vi_chn vi_chn = chn_info[i].vi_chn;
            const ot_vi_chn_attr *chn_attr = &chn_info[i].chn_attr;

            ret = ss_mpi_vi_set_chn_attr(vi_pipe, vi_chn, chn_attr);
            ENCODE_RET_BREAK(ret, "vi_chn(%d) ss_mpi_vi_set_chn_attr failed\n", vi_chn);

            ret = ss_mpi_vi_enable_chn(vi_pipe, vi_chn);
            ENCODE_RET_BREAK(ret, "vi_chn(%d) ss_mpi_vi_enable_chn failed\n", vi_chn);
        }

        if (pipe_info->nr_attr.enable == TD_TRUE) {
            ret = ss_mpi_vi_set_pipe_3dnr_attr(vi_pipe, &pipe_info->nr_attr);
            ENCODE_RET_BREAK(ret, "vi pipe(%d) ss_mpi_vi_set_pipe_3dnr_attr failed\n", vi_pipe);
        }
    } while(0);

    return ret;
}

static td_s32 encode_vi_start_one_pipe(ot_vi_pipe vi_pipe, vi_pipe_info_t *pipe_info,
    td_bool is_master_pipe)
{
    td_s32 ret = 0;

    do {
        ret = encode_vi_init_one_pipe(vi_pipe, pipe_info);
        ENCODE_RET_BREAK(ret, "vi pipe(%d) encode_vi_init_one_pipe failed\n", vi_pipe);

        if (pipe_info->low_delay_info.enable == TD_TRUE) {
            ret = ss_mpi_vi_set_pipe_low_delay(vi_pipe, &pipe_info->low_delay_info);
            ENCODE_RET_BREAK(ret, "set pipe(%d) low delay failed.\n", vi_pipe);
        }

        if (pipe_info->frame_interrupt_attr.interrupt_type != OT_FRAME_INTERRUPT_START) {
            ret = ss_mpi_vi_set_pipe_frame_interrupt_attr(vi_pipe, &pipe_info->frame_interrupt_attr);
            ENCODE_RET_BREAK(ret, "set pipe(%d) frame interrupt attr failed.\n", vi_pipe);
        }

        if (pipe_info->pipe_need_start == TD_TRUE) {
        ret = ss_mpi_vi_start_pipe(vi_pipe);
        ENCODE_RET_BREAK(ret, "vi pipe(%d) ss_mpi_vi_start_pipe failed\n", vi_pipe);
    }

    if (is_master_pipe != TD_TRUE) {
        break;
    }

    ret = encode_vi_start_chn(vi_pipe, pipe_info);
        ENCODE_RET_BREAK(ret, "vi pipe(%d) encode_vi_start_chn failed\n", vi_pipe);
    }while(0);

    return ret;
}

static td_s32 encode_vi_start_pipe(ot_vi_bind_pipe *bind_pipe, vi_pipe_info_t pipe_info[])
{
    td_s32 i = 0;
    td_s32 ret = 0;

    for (i = 0; i < (td_s32)bind_pipe->pipe_num; i++) {
        ot_vi_pipe vi_pipe = bind_pipe->pipe_id[i];
        td_bool is_master_pipe = pipe_info[i].is_master_pipe;
        ret = encode_vi_start_one_pipe(vi_pipe, &pipe_info[i], is_master_pipe);
        ENCODE_RET_BREAK(ret, "vi pipe(%d) encode_vi_start_one_pipe failed\n", vi_pipe);
    }

    return ret;
}

int encode_vi_init(vi_cfg_t *vi_cfg)
{
    int ret = 0;
    ot_vi_dev vi_dev = 0;

    do {
        ret = encode_vi_start_mipi_rx(&vi_cfg->sns_info, &vi_cfg->mipi_info);
        ENCODE_RET_BREAK(ret, "encode_vi_start_mipi_rx failed\n");

        vi_dev = vi_cfg->dev_info.vi_dev;
        ret = encode_vi_start_dev(vi_dev, &vi_cfg->dev_info.dev_attr);
        ENCODE_RET_BREAK(ret, "encode_vi_start_dev failed\n");

        ret = encode_vi_dev_bind_pipe(vi_dev, &vi_cfg->bind_pipe);
        ENCODE_RET_BREAK(ret, "encode_vi_dev_bind_pipe failed\n");

        ret = encode_vi_set_grp_info(&vi_cfg->grp_info);
        ENCODE_RET_BREAK(ret, "encode_vi_set_grp_info failed\n");

        ret = encode_vi_start_pipe(&vi_cfg->bind_pipe, vi_cfg->pipe_info);
        ENCODE_RET_BREAK(ret, "ss_mpi_vi_destroy_pipe failed\n");
    } while(0);

    return ret;
}

static td_void encode_vi_stop_mipi_rx(const sns_info_t *sns_info, const mipi_info_t *mipi_info)
{
    td_s32 ret = 0;

    ret = encode_vi_mipi_ctrl_cmd(mipi_info->mipi_dev, OT_MIPI_RESET_MIPI);
    ENCODE_RET_CHECK(ret, "devno %u reset mipi rx  failed!\n", mipi_info->mipi_dev);

    ret = encode_vi_mipi_ctrl_cmd(mipi_info->mipi_dev, OT_MIPI_DISABLE_MIPI_CLOCK);
    ENCODE_RET_CHECK(ret, "devno %u disable mipi rx clock failed!\n", mipi_info->mipi_dev);

    ret = encode_vi_mipi_ctrl_cmd(sns_info->sns_rst_src, OT_MIPI_RESET_SENSOR);
    ENCODE_RET_CHECK(ret, "devno %u reset senso failed!\n", sns_info->sns_rst_src);

    ret = encode_vi_mipi_ctrl_cmd(sns_info->sns_clk_src, OT_MIPI_DISABLE_SENSOR_CLOCK);
    ENCODE_RET_CHECK(ret, "devno %u disable sensor clock failed!\n", sns_info->sns_clk_src);
}

static td_void encode_vi_stop_dev(ot_vi_dev vi_dev)
{
    td_s32 ret = 0;

    ret = ss_mpi_vi_disable_dev(vi_dev);
    ENCODE_RET_CHECK(ret, "ss_mpi_vi_disable_dev %d failed\n", vi_dev);
}

static td_void encode_vi_dev_unbind_pipe(ot_vi_dev vi_dev, const ot_vi_bind_pipe *bind_pipe)
{
    td_s32 ret = 0;

    for (int i = 0; i < bind_pipe->pipe_num; i++) {
        ret = ss_mpi_vi_unbind(vi_dev, bind_pipe->pipe_id[i]);
        ENCODE_RET_CHECK(ret, "ss_mpi_vi_unbind %d failed\n", vi_dev);
    }
}

static td_s32 encode_vi_stop_chn(ot_vi_pipe vi_pipe, const vi_chn_info_t chn_info[], td_u32 chn_num)
{
    td_s32 ret = 0;

    for (int i = 0; i < chn_num; i++) {
        ot_vi_chn vi_chn = chn_info[i].vi_chn;

        ret = ss_mpi_vi_disable_chn(vi_pipe, vi_chn);
        ENCODE_RET_BREAK(ret, "ss_mpi_vi_destroy_pipe %d failed\n", vi_pipe);
    }

    return TD_SUCCESS;
}

static td_void encode_vi_stop_one_pipe(ot_vi_pipe vi_pipe, const vi_pipe_info_t *pipe_info,
    td_bool is_master_pipe)
{
    td_s32 ret = 0;

    if (is_master_pipe == TD_TRUE) {
        ret = encode_vi_stop_chn(vi_pipe, pipe_info->chn_info, pipe_info->chn_num);
        ENCODE_RET_CHECK(ret, "encode_vi_stop_chn %d failed\n", vi_pipe);
    }

    ret = ss_mpi_vi_stop_pipe(vi_pipe);
    ENCODE_RET_CHECK(ret, "ss_mpi_vi_stop_pipe %d failed\n", vi_pipe);

    ret = ss_mpi_vi_destroy_pipe(vi_pipe);
    ENCODE_RET_CHECK(ret, "ss_mpi_vi_destroy_pipe %d failed\n", vi_pipe);
}

static td_void encode_vi_stop_pipe(const ot_vi_bind_pipe *bind_pipe, const vi_pipe_info_t pipe_info[])
{
    for (int i = 0; i < bind_pipe->pipe_num; i++) {
        ot_vi_pipe vi_pipe = bind_pipe->pipe_id[i];
        td_bool is_master_pipe = pipe_info[i].is_master_pipe;
        encode_vi_stop_one_pipe(vi_pipe, &pipe_info[i], is_master_pipe);
    }
}

void encode_vi_uninit(vi_cfg_t *vi_cfg)
{
    ot_vi_dev vi_dev = vi_cfg->dev_info.vi_dev;

    encode_vi_stop_pipe(&vi_cfg->bind_pipe, vi_cfg->pipe_info);

    encode_vi_dev_unbind_pipe(vi_dev, &vi_cfg->bind_pipe);

    encode_vi_stop_dev(vi_dev);

    encode_vi_stop_mipi_rx(&vi_cfg->sns_info, &vi_cfg->mipi_info);
}
