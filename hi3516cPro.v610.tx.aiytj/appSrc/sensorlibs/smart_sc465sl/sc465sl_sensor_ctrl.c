/*
  Copyright (c), 2001-2024, Shenshu Tech. Co., Ltd.
 */

#include "sensor_common.h"
#include "sc465sl_cfg.h"
#include "sc465sl_cmos.h"

static void sc465sl_default_reg_init(cis_info *cis)
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

static td_s32 sc465sl_reg_init(cis_info *cis, cis_reg_cfg *cfg, td_u32 len)
{
    td_u32 i;

    sns_check_return(cis_write_reg(&cis->i2c, 0x0103, 0x01));
    cis_delay_ms(1); /* 1ms */

    for (i = 0; i < len; i++) {
        sns_check_return(cis_write_reg(&cis->i2c, cfg->addr, cfg->data));
        cfg++;
    }

    sc465sl_default_reg_init(cis);

    sns_check_return(cis_write_reg(&cis->i2c, 0x0100, 0x01));

    return TD_SUCCESS;
}

td_s32 sc465sl_linear_4m30_12bit_init(cis_info *cis)
{
    td_s32 ret;
    td_u32 len;
    cis_reg_cfg *cfg = sc465sl_linear_4m30_12bit;

    sns_check_pointer_return(cis);

    len = (td_u32)(sizeof(sc465sl_linear_4m30_12bit) / sizeof(sc465sl_linear_4m30_12bit[0]));
    ret = sc465sl_reg_init(cis, cfg, len);
    if (ret != TD_SUCCESS) {
        isp_err_trace("sc465sl_reg_init failed!\n");
        return ret;
    }

    printf("===================================================================================\n");
    printf("vi_pipe:%d,== Cleaned_0x4c_SC465SL_raw_MIPI_27Minput_4Lane_12bit_576Mbps_2560x1440_30fps Init OK! ==\n", cis->pipe);
    printf("===================================================================================\n");

    return TD_SUCCESS;
}

td_s32 sc465sl_vc_wdr_2t1_4m30_10bit_init(cis_info *cis)
{
    td_s32 ret;
    td_u32 len;
    cis_reg_cfg *cfg = sc465sl_wdr_2t1_4m30_10bit;

    sns_check_pointer_return(cis);

    len = (td_u32)(sizeof(sc465sl_wdr_2t1_4m30_10bit) / sizeof(sc465sl_wdr_2t1_4m30_10bit[0]));
    ret = sc465sl_reg_init(cis, cfg, len);
    if (ret != TD_SUCCESS) {
        isp_err_trace("sc465sl_reg_init failed!\n");
        return ret;
    }

    printf("===========================================================================================\n");
    printf("vi_pipe:%d,== SC465SL_MIPI_27MInput_4lane_630Mbps_10bit_WDR2T1_30fps_2560x1440 Init OK! ==\n", cis->pipe);
    printf("============================================================================================\n");

    return TD_SUCCESS;
}

td_s32 sc465sl_get_standby_cfg(ot_isp_sns_regs_info *standby_cfg)
{
    td_u32 i;
    standby_cfg->reg_num = (td_u32)(sizeof(sc465sl_standby_cfg) / sizeof(sc465sl_standby_cfg[0]));

    for (i = 0; i < standby_cfg->reg_num; i++) {
        standby_cfg->i2c_data[i].dev_addr = SC465SL_I2C_ADDR;
        standby_cfg->i2c_data[i].addr_byte_num = SC465SL_ADDR_BYTE;
        standby_cfg->i2c_data[i].data_byte_num = SC465SL_DATA_BYTE;
        standby_cfg->i2c_data[i].reg_addr = sc465sl_standby_cfg[i].addr;
        standby_cfg->i2c_data[i].data = sc465sl_standby_cfg[i].data;
    }

    return TD_SUCCESS;
}

