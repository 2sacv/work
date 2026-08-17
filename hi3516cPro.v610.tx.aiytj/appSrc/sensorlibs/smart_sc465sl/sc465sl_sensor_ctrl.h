/*
  Copyright (c), 2001-2024, Shenshu Tech. Co., Ltd.
 */

#ifndef SC465SL_SENSOR_CTRL_H
#define SC465SL_SENSOR_CTRL_H

#include "sensor_common.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

td_s32 sc465sl_linear_4m30_12bit_init(cis_info *cis);
td_s32 sc465sl_vc_wdr_2t1_4m30_10bit_init(cis_info *cis);
td_s32 sc465sl_get_standby_cfg(ot_isp_sns_regs_info *standby_cfg);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
#endif
