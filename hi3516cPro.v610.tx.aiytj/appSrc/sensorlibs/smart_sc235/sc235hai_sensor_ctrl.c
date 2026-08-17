/*
  Copyright (c), 2001-2024, Shenshu Tech. Co., Ltd.
 */

#include "sensor_common.h"
#include "sc235hai_cfg.h"
#include "sc235hai_cmos.h"

static void sc235hai_default_reg_init(cis_info *cis)
{
    td_u32 i;
    td_s32 ret = TD_SUCCESS;
    ot_isp_sns_state *past_sensor = TD_NULL;

    past_sensor = cis->sns_state;
    for (i = 0; i < past_sensor->regs_info[0].reg_num; i++) {
        ret += cis_write_reg(&cis->i2c,
            past_sensor->regs_info[0].i2c_data[i].reg_addr,
            past_sensor->regs_info[0].i2c_data[i].data);
    }

    if (ret != TD_SUCCESS) {
        isp_err_trace("write register failed!\n");
    }
    return;
}

static td_s32 sc235hai_reg_init(cis_info *cis, cis_reg_cfg *cfg, td_u32 len)
{
    td_u32 i;

    sns_check_return(cis_write_reg(&cis->i2c, 0x0103, 0x01));
    cis_delay_ms(1); /* 1ms */

    for (i = 0; i < len; i++) {
        sns_check_return(cis_write_reg(&cis->i2c, cfg->addr, cfg->data));
        cfg++;
    }

    sc235hai_default_reg_init(cis);

    sns_check_return(cis_write_reg(&cis->i2c, 0x0100, 0x01));

    return TD_SUCCESS;
}

td_s32 sc235hai_linear_2m30_10bit_init(cis_info *cis)
{
    td_s32 ret;
    td_u32 len;
    cis_reg_cfg *cfg = sc235hai_linear_2m30_10bit;

    sns_check_pointer_return(cis);

    len = (td_u32)(sizeof(sc235hai_linear_2m30_10bit) / sizeof(sc235hai_linear_2m30_10bit[0]));
    ret = sc235hai_reg_init(cis, cfg, len);
    if (ret != TD_SUCCESS) {
        isp_err_trace("sc235hai_reg_init failed!\n");
        return ret;
    }

    printf("===================================================================================\n");
    printf("vi_pipe:%d,== Cleaned_0x05_FT_SC235AI_MIPI_27Minput_2lane_371.25_10bit_1920x1080_30fps Init OK! ==\n", cis->pipe);
    printf("===================================================================================\n");

    return TD_SUCCESS;
}

td_s32 sc235hai_vc_wdr_2t1_2m30_10bit_init(cis_info *cis)
{
    td_s32 ret;
    td_u32 len;
    cis_reg_cfg *cfg = sc235hai_wdr_2t1_2m30_10bit;

    sns_check_pointer_return(cis);

    len = (td_u32)(sizeof(sc235hai_wdr_2t1_2m30_10bit) / sizeof(sc235hai_wdr_2t1_2m30_10bit[0]));
    ret = sc235hai_reg_init(cis, cfg, len);
    if (ret != TD_SUCCESS) {
        isp_err_trace("sc235hai_reg_init failed!\n");
        return ret;
    }

    printf("===========================================================================================\n");
    printf("vi_pipe:%d,== Cleaned_0x17_SC235AI_MIPI_27Minput_2lane_742.5Mbps_10bit_1920x1080_30fps_SHDR_VC Init OK! ==\n", cis->pipe);
    printf("============================================================================================\n");

    return TD_SUCCESS;
}

td_s32 sc235_get_standby_cfg(ot_isp_sns_regs_info *standby_cfg)
{
    td_u32 i;
    standby_cfg->reg_num = (td_u32)(sizeof(sc235_standby_cfg) / sizeof(sc235_standby_cfg[0]));

    for (i = 0; i < standby_cfg->reg_num; i++) {
        standby_cfg->i2c_data[i].dev_addr = SC235HAI_I2C_ADDR;
        standby_cfg->i2c_data[i].addr_byte_num = SC235HAI_ADDR_BYTE;
        standby_cfg->i2c_data[i].data_byte_num = SC235HAI_DATA_BYTE;
        standby_cfg->i2c_data[i].reg_addr = sc235_standby_cfg[i].addr;
        standby_cfg->i2c_data[i].data = sc235_standby_cfg[i].data;
    }

    return TD_SUCCESS;
}

