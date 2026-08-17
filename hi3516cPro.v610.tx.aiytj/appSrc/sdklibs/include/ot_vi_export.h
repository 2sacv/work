/*
  Copyright (c), 2001-2025, Shenshu Tech. Co., Ltd.
 */
#ifndef OT_VI_EXPORT_H
#define OT_VI_EXPORT_H

#include "ot_common.h"
#include "ot_common_vi.h"
#include "ot_type.h"


typedef struct {
    td_s32 (*vi_export_switch_gpio_high_level)(ot_vi_pipe vi_pipe);
    td_s32 (*vi_export_switch_gpio_low_level)(ot_vi_pipe vi_pipe);
} ot_vi_export_callback;

typedef td_s32 ot_vi_register_export_callback(ot_vi_pipe vi_pipe, ot_vi_export_callback *export_callback);

typedef struct {
    ot_vi_register_export_callback *register_export_callback;
} ot_vi_export_symbol;

ot_vi_export_symbol *ot_vi_get_export_symbol(td_void);

#endif /* OT_VI_EXPORT_H */