#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#include "debug.h"
#include "jconfig.h"
#include "confapi.h"

#include "alarm_paramcfg.h"
#include "alarm_service.h"

void get_alarm_motionlink(alarm_link_t *data, MotionDetectLinkS lk_md)
{
    if (NULL == data) {
        return;
    }

    data->interval = lk_md.interval;
    data->alarmcenter = lk_md.alarmcenter;
    data->alarmout1 = lk_md.alarmout1;
    data->alarmout2 = lk_md.alarmout2;
    data->sound = lk_md.sound;
    data->snapshot = lk_md.snapshot;
    data->record = lk_md.record;
    data->ftpup = lk_md.ftpup;
    data->email = lk_md.email;
    data->preset = lk_md.preset;
}

void get_alarm_humandetectlink(alarm_link_t *data, HumanDetectLinkS lk_hd)
{
    if (NULL == data) {
        return;
    }

    data->interval = lk_hd.interval;
    data->alarmcenter = lk_hd.alarmcenter;
    data->alarmout1 = lk_hd.alarmout1;
    data->alarmout2 = lk_hd.alarmout2;
    data->sound = lk_hd.sound;
    data->snapshot = lk_hd.snapshot;
    data->record = lk_hd.record;
    data->ftpup = lk_hd.ftpup;
    data->email = lk_hd.email;
    data->preset = lk_hd.preset;
}

void get_alarm_vglinelink(alarm_link_t *data, VglineLinkS lk_vgline)
{
    if (NULL == data) {
        return;
    }

    data->interval = lk_vgline.interval;
    data->alarmcenter = lk_vgline.alarmcenter;
    data->alarmout1 = lk_vgline.alarmout1;
    data->alarmout2 = lk_vgline.alarmout2;
    data->sound = lk_vgline.sound;
    data->snapshot = lk_vgline.snapshot;
    data->record = lk_vgline.record;
    data->ftpup = lk_vgline.ftpup;
    data->email = lk_vgline.email;
    data->preset = lk_vgline.preset;
}

void get_alarm_vgrectlink(alarm_link_t *data, VgrectLinkS         lk_vgrect)
{
    if (NULL == data) {
        return;
    }

    data->interval = lk_vgrect.interval;
    data->alarmcenter = lk_vgrect.alarmcenter;
    data->alarmout1 = lk_vgrect.alarmout1;
    data->alarmout2 = lk_vgrect.alarmout2;
    data->sound = lk_vgrect.sound;
    data->snapshot = lk_vgrect.snapshot;
    data->record = lk_vgrect.record;
    data->ftpup = lk_vgrect.ftpup;
    data->email = lk_vgrect.email;
    data->preset = lk_vgrect.preset;
}

void get_alarm_ipbrokenlink(alarm_link_t *data, IpLinkS lk_ipbroken)
{
    if (NULL == data) {
        return;
    }

    data->interval = lk_ipbroken.interval;
    data->alarmout1 = lk_ipbroken.ao0en;
    data->alarmout2 = lk_ipbroken.ao1en;
    data->sound = lk_ipbroken.sounden;
    data->snapshot = lk_ipbroken.captureen;
    data->record = lk_ipbroken.recorden;
}

void get_alarm_ipconflictlink(alarm_link_t *data, IpLinkS lk_ipcflict)
{
    if (NULL == data) {
        return;
    }

    data->interval = lk_ipcflict.interval;
    data->alarmout1 = lk_ipcflict.ao0en;
    data->alarmout2 = lk_ipcflict.ao1en;
    data->sound = lk_ipcflict.sounden;
    data->snapshot = lk_ipcflict.captureen;
    data->record = lk_ipcflict.recorden;
}

void get_caralarmlink_param(alarm_link_t *data, CarDetectLinkS lk_car)
{
    if (NULL == data) {
        return;
    }

    data->interval = lk_car.interval;
    data->alarmout1 = lk_car.alarmout1;
    data->alarmout2 = lk_car.alarmout2;
    data->sound = lk_car.sound;
    data->record = lk_car.record;
    data->snapshot = lk_car.snapshot;
    data->email = lk_car.email;
    data->alarmcenter = lk_car.alarmcenter;
    data->ftpup = lk_car.ftpup;
    data->preset = lk_car.preset;
}

void get_petalarmlink_param(alarm_link_t *data, PetDetectLinkS lk_pet)
{
    if (NULL == data) {
        return;
    }

    data->interval = lk_pet.interval;
    data->alarmout1 = lk_pet.alarmout1;
    data->alarmout2 = lk_pet.alarmout2;
    data->sound = lk_pet.sound;
    data->record = lk_pet.record;
    data->snapshot = lk_pet.snapshot;
    data->email = lk_pet.email;
    data->alarmcenter = lk_pet.alarmcenter;
    data->ftpup = lk_pet.ftpup;
    data->preset = lk_pet.preset;
}

void get_cryalarmlink_param(alarm_link_t *data, CryDetectLinkS lk_cry)
{
    if (NULL == data) {
        return;
    }

    data->interval = lk_cry.interval;
    data->alarmout1 = lk_cry.alarmout1;
    data->alarmout2 = lk_cry.alarmout2;
    data->sound = lk_cry.sound;
    data->record = lk_cry.record;
    data->snapshot = lk_cry.snapshot;
    data->email = lk_cry.email;
    data->alarmcenter = lk_cry.alarmcenter;
    data->ftpup = lk_cry.ftpup;
    data->preset = lk_cry.preset;
}

void get_vmaskalarmlink_param(alarm_link_t *data, VMaskAlarmLinkS lk_vmask)
{
	if (NULL == data) {
		return;
	}

	data->interval = lk_vmask.interval;
	data->alarmout1 = lk_vmask.alarmout1;
	data->alarmout2 = lk_vmask.alarmout2;
	data->sound = lk_vmask.sound;
	data->record = lk_vmask.record;
	data->snapshot = lk_vmask.capture;
	data->email = lk_vmask.email;
	data->alarmcenter = lk_vmask.alarmcenter;
	data->ftpup = lk_vmask.ftpup;
}

void get_cfg_param(JEventType type, alarm_map *alm_map)
{
    DBG("event type = %d\n", type);

    switch (type) {
    case JEvent_MotionDetLinkCfgChg:
        conf_get_motiondetectlinkcfg(&alm_map->cfg.lk_md);
        break;
    case JEvent_VglineLinkCfgChg: 
        conf_get_vglinelinkcfg(&alm_map->cfg.lk_vgline);
        break;
    case JEvent_VgrectLinkCfgChg: 
        conf_get_vgrectlinkcfg(&alm_map->cfg.lk_vgrect);
        break;
    case JEvent_HumanDetLinkCfgChg: 
        conf_get_humandetectlinkcfg(&alm_map->cfg.lk_hd);
        break;
    case JEvent_CarDetLinkCfgChg: 
        conf_get_cardetectlinkcfg(&alm_map->cfg.lk_car);
        break;
    case JEvent_PetDetLinkCfgChg: 
        conf_get_petdetectlinkcfg(&alm_map->cfg.lk_pet);
        break;
    case JEvent_CryDetLinkCfgChg: 
        conf_get_crydetectlinkcfg(&alm_map->cfg.lk_cry);
        break;
    case JEvent_VMaskAlarmLinkCfg: 
        conf_get_vmaskalarmlinkcfg(&alm_map->cfg.lk_vmask);
        break;
    case JEvent_IpConflictCfgChg: 
        conf_get_ipconflictcfg(&alm_map->cfg.lk_ipcflict);
        break;
    case JEvent_IpBrokenCfgChg: 
        conf_get_ipbrokencfg(&alm_map->cfg.lk_ipbroken);
        break;
    case JEvent_MotionDetectCfgChg: 
        conf_get_motiondetectcfg(&alm_map->cfg.cfg_md);
        break;
    case JEvent_HumanDetectCfgChg: 
        conf_get_humandetectioncfg(&alm_map->cfg.cfg_hd);
        break;
    case JEvent_CarDetectCfgChg: 
        conf_get_cardetectioncfg(&alm_map->cfg.cfg_car);
        break;
    case JEvent_PetDetectCfgChg: 
        conf_get_petdetectioncfg(&alm_map->cfg.cfg_pet);
        break;
    case JEvent_CryDetectCfgChg: 
        conf_get_crydetectioncfg(&alm_map->cfg.cfg_cry);
        break;
    case JEvent_VglineCfgChg: 
        conf_get_vglinecfg(&alm_map->cfg.cfg_vgline);
        break;
    case JEvent_VgrectCfgChg:   
        conf_get_vgrectcfg(&alm_map->cfg.cfg_vgrect);
        break;
    case JEvent_AudioAlarmCfg:
        conf_get_audioalarm_cfg(&alm_map->cfg.cfg_audio_alarm);
        break;
    case JEvent_LightAlarmCfg:
        conf_get_lightalarm_cfg(&alm_map->cfg.cfg_light_alarm);
        break;
    case JEvent_IOAlarmCfg: 
          // conf_get_IOalarm_cfg(&alm_map->cfg.cfg_io_alarm);
          break;
    case JEvent_VMaskAlarmCfg: 
          conf_get_vmaskalarmcfg(&alm_map->cfg.cfg_vmask);
          break;
    case JEvent_Followcfg:
        conf_get_follow_cfg(&alm_map->cfg.cfg_follow);
        break;
    default:
        DBG("not find JEventType: %d\n", type);
        break;
    }
}
