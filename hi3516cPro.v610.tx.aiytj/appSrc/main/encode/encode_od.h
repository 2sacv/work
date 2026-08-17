/******************************************************************************
    Copyright (C), 2008-2028, JABSCO ELECTRONIC Tech. Co., Ltd
 
    File Name    : encode_od.h
    Version      : 1.0
    Author       : JABSCO Video Server Software Group
    Created      : 2017-01-20
    Description  : 2017-04-13 by guoxg
    History      : 
******************************************************************************/

#ifndef __ENCODE_OD_H__
#define __ENCODE_OD_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "jconfstruct.h"
#include "encode_common.h"
#include "encode_Pre_od.h"

typedef enum {
    CMD_IVX_VMA_INFO   = 1 << 0,   // 遮挡报警
} eOdCmd;

void encode_od_sync_vmaskalmcfg(void);
int encode_od_init(void);
void encode_od_uninit(void);
void encode_od_freeze(void);

#ifdef __cplusplus
}
#endif

#endif//__ENCODE_OD_H__

