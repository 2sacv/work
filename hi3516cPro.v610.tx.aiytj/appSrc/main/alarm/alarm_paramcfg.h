/**
 * Copyright (C) by Jabsco Company
 * 
 * @File Name    : alarm_paramcfg.h
 * @Created Time : 2014-05-05
 * @Version      : 1.0
 * @Author       : cheby
 * @Description  : 
 */

#ifndef __ALARM_PARAMCFG_H__
#define __ALARM_PARAMCFG_H__

#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "jconfstruct.h"
#include "jconfig.h"
#include "alarm_service.h"

typedef struct {
	int    interval;
	int    alarmcenter;
	int    email;
	int    alarmout1;
	int    alarmout2;
	int    sound;
	int    record;
	int    ftpup;
	int    snapshot;
	int    preset;  // 0 disable, otherwise preset number
} alarm_link_t;

void get_alarm_motionlink(alarm_link_t *data, MotionDetectLinkS lk_md);

void get_alarm_humandetectlink(alarm_link_t *data, HumanDetectLinkS lk_hd);

void get_alarm_vglinelink(alarm_link_t *data, VglineLinkS lk_vgline);

void get_alarm_vgrectlink(alarm_link_t *data, VgrectLinkS         lk_vgrect);

void get_alarm_ipbrokenlink(alarm_link_t *data, IpLinkS lk_ipbroken);

void get_alarm_ipconflictlink(alarm_link_t *data, IpLinkS lk_ipcflict);

void get_caralarmlink_param(alarm_link_t *data, CarDetectLinkS lk_car);

void get_petalarmlink_param(alarm_link_t *data, PetDetectLinkS lk_pet);

void get_cryalarmlink_param(alarm_link_t *data, CryDetectLinkS lk_cry);

void get_vmaskalarmlink_param(alarm_link_t *data, VMaskAlarmLinkS lk_vmask);

void get_cfg_param(JEventType type, struct alarm_map *alm_map);

#ifdef __cplusplus
}
#endif 
#endif


