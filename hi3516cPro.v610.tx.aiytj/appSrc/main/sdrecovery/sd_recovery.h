/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    :
 * Created Time : 2024-01-25
 * Version      : 1.0
 * Author       : wuhy
 * Description  :
 */

#ifndef _SD_RECOVERY_H_
#define _SD_RECOVERY_H_

#include "js_scheduler.h"

#ifdef __cplusplus
extern "C" {
#endif
#define SD_CARD_CID  "/sys/block/mmcblk0/device/cid"

void init_sd_recovery(JSScheduler sch);
void uninit_sd_recovery();
void firmware_answer(int is_updating);

#ifdef __cplusplus
}
#endif
#endif

