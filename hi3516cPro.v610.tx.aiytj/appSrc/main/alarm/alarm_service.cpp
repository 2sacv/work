/**
 * Copyright (C) by Jabsco Company
 * 
 * @File Name    : alarmService.cpp
 * @Created Time : 2021-04-26
 * @Version      : 1.0
 * @Author       : cheby
 * @Description  : 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/times.h>
#include <pthread.h>
#include <unistd.h>

#include "mxml.h"
#include "debug.h"
#include "utils.h"

#include "alarm_service.h"
#include "alarm_log.h"
#include "alarm_event.h"
#include "alarm_cancel.h"
#include "alarm_paramcfg.h"
#include "alarmapi.h"

#include "confapi.h"
#include "jevent.h"
#include "jconfig.h"
#include "js_scheduler.h"
#include "record_watch.h"
#include "recordapi.h"
#include "fifo_queue.h"
#include "system_sch.h"
#include "conf_list.h"
#include "g_log.h"
#include "lamp_main.h"

static alarm_map map = {0};
static JSScheduler sch_alarm = NULL;
static JSTCHandle hdl_alarm = NULL;
static queue_t *g_alm_queue = NULL;
static bool g_alarm_init = false;

#define ALARM_NOTIFY_INTERVAL   2
#define INTV_ALARM    (1*200)

EventCmdS eventCmd[] = {
    // {JEvent_VideoLossLinkCfgChg , CMD_LK_VLOSS    },
    {JEvent_MotionDetLinkCfgChg , CMD_LK_MD       },
    {JEvent_HumanDetLinkCfgChg  , CMD_LK_HD       },
    {JEvent_CarDetLinkCfgChg    , CMD_LK_CAR      },
    {JEvent_PetDetLinkCfgChg    , CMD_LK_PET      },
    {JEvent_CryDetLinkCfgChg    , CMD_LK_CRY      },
    {JEvent_VglineLinkCfgChg    , CMD_LK_VGLINE   },
    {JEvent_VgrectLinkCfgChg    , CMD_LK_VGRECT   },
    {JEvent_VMaskAlarmLinkCfg   , CMD_LK_VMASK    },
    {JEvent_IpConflictCfgChg    , CMD_CFG_IPCFLICT},
    {JEvent_IpBrokenCfgChg      , CMD_CFG_IPBROKEN},
    {JEvent_MotionDetectCfgChg  , CMD_CFG_MD      },
    {JEvent_HumanDetectCfgChg   , CMD_CFG_HD      },
    {JEvent_CarDetectCfgChg     , CMD_CFG_CAR     },
    {JEvent_PetDetectCfgChg     , CMD_CFG_PET     },
    {JEvent_CryDetectCfgChg     , CMD_CFG_CRY     },
    {JEvent_VglineCfgChg        , CMD_CFG_VGLINE  },
    {JEvent_VgrectCfgChg        , CMD_CFG_VGRECT  },
    // {JEvent_VideoLossCfgChg     , CMD_CFG_VLOSS   },
    {JEvent_VMaskAlarmCfg       , CMD_CFG_VMASK   },
    {JEvent_Followcfg           , CMD_CFG_FOLLOW  },
    {JEvent_End                 , 0               },
};

int get_alarm_link_cfg(JEventType type, void* link_cfg, int chn)
{
    alarm_map* alm_map = &map;
    alarm_link_t* alarm_link = (alarm_link_t*)link_cfg;

    switch (type) {
    case JEvent_AlarmMD: {
        get_alarm_motionlink(alarm_link, alm_map->cfg.lk_md);
        break;
    }
    case JEvent_Alarmhumadetect: {
        get_alarm_humandetectlink(alarm_link, alm_map->cfg.lk_hd);
        break;
    }
    case JEvent_AlarmCar: {
        get_caralarmlink_param(alarm_link, alm_map->cfg.lk_car);
        break;
    }
    case JEvent_AlarmPet: {
        get_petalarmlink_param(alarm_link, alm_map->cfg.lk_pet);
        break;
    }
    case JEvent_AlarmCry: {
        get_cryalarmlink_param(alarm_link, alm_map->cfg.lk_cry);
        break;
    }
    case JEvent_AlarmVgline: {
        get_alarm_vglinelink(alarm_link, alm_map->cfg.lk_vgline);
        break;
    }
    case JEvent_AlarmVgrect: {
        get_alarm_vgrectlink(alarm_link, alm_map->cfg.lk_vgrect);
        break;
    }
    // case JEvent_AlarmVL: {
    //     get_alarm_videolosslink(alarm_link, alm_map->cfg.lk_vloss);
    //     break;
    // }
    case JEvent_AlarmCabDis: {
        get_alarm_ipbrokenlink(alarm_link, alm_map->cfg.lk_ipbroken);
        break;
    }
    case JEvent_AlarmIpConflict: {
        get_alarm_ipconflictlink(alarm_link, alm_map->cfg.lk_ipcflict);
        break;
    }
    case JEvent_AlarmVMask: {
        get_vmaskalarmlink_param(alarm_link, alm_map->cfg.lk_vmask);
        break;
    }
    default:
        dbg_alarm("%s no find type: %d\n", __func__, type);
        return -1;
    }

    return 0;
}

int parse_alarm_handle(void *req, void *buf, int bufsize, int *retlen, void *arg)
{
    if (g_alarm_init == false || NULL == g_alm_queue) {
        return 0;
    }

    ALARM_INFOS *handle = NULL;

    handle = (ALARM_INFOS *)malloc(sizeof(ALARM_INFOS));
    if (NULL == handle) {
        ERR("malloc fail\n");
        return -1;
    }

    memcpy(handle, req, sizeof(ALARM_INFOS));
    DBG("add alarm play type :%d\n", handle->type);
    fifo_queue_push(g_alm_queue, (void*)handle);
    return 0;
}

static int do_link_process(ALARM_INFOS *alarminfo)
{
    dbg_alarm("%s: type:%d\n", __func__, alarminfo->type);

    handle_alarm_cancel(alarminfo->type);

    if (strlen(alarminfo->desc) == 0) {
        alarm_add_event_log(alarminfo->type, 0, NULL);
    } else {
        alarm_add_event_log(alarminfo->type, alarminfo->chn, alarminfo->desc);
    }

    if (JALARM_TYPE_VGLINE == alarminfo->type || JALARM_TYPE_VGRECT == alarminfo->type || 
        JALARM_TYPE_MD == alarminfo->type || JALARM_TYPE_HUMAN_DETECT == alarminfo->type ||
        JALARM_TYPE_CAR == alarminfo->type || JALARM_TYPE_PET == alarminfo->type ||
        JALARM_TYPE_CRY == alarminfo->type || JALARM_TYPE_MASK == alarminfo->type) {
        // 强制录像
        time_t curTime = time(NULL);
        if ((curTime + 8) % (60*60*24) < 8) { // 考虑以后改 6s gop 加上预录制，跨天 8s 内不进行报警录像
            SYSLOG("curTime:%lld\n", curTime);
        } else {
            record_request_rec(JREC_TYPE_ALARM);
        }
    }

    return 0;
}

static int alarm_link_process(ALARM_INFOS *alarminfo, alarm_map *alm_map)
{
    last_alarm_time_t last_event_time = {0};
    memcpy(&last_event_time, &alm_map->last_event_time, sizeof(last_alarm_time_t));

    JSEventType event_type = JEvent_Begin;
    alarm_link_t alarm_link = {0};

    time_t now = time(NULL);
    time_t event_time = 0;
    int chn = alarminfo->chn;
    dbg_alarm("alarminfo->type: %d\n", alarminfo->type);

    switch (alarminfo->type) {
    // case JALARM_TYPE_VL: {
    //     dbg_alarm("got JALARM_TYPE_VL\n");
    //     event_time = last_event_time.videolosslink_time;
    //     event_type = JEvent_AlarmVL;
    //     last_event_time.videolosslink_time = now;
    //     get_alarm_videolosslink(&alarm_link, alm_map->cfg.lk_vloss);
    //     break;
    // }
    case JALARM_TYPE_MD: {
        dbg_alarm("got JALARM_TYPE_MD\n");
        event_time = last_event_time.motionlink_time;
        event_type = JEvent_AlarmMD;
        last_event_time.motionlink_time = now;
        get_alarm_motionlink(&alarm_link, alm_map->cfg.lk_md);
        break;
    }
    case JALARM_TYPE_VGLINE: {
        dbg_alarm("got JALARM_TYPE_VGLINE\n");
        event_time = last_event_time.vglinelink_time;
        event_type = JEvent_AlarmVgline;
        last_event_time.vglinelink_time = now;
        get_alarm_vglinelink(&alarm_link, alm_map->cfg.lk_vgline);
        break;
    }
    case JALARM_TYPE_VGRECT: {
        dbg_alarm("got JALARM_TYPE_VGRECT\n");
        event_time = last_event_time.vgrectlink_time;
        event_type = JEvent_AlarmVgrect;
        last_event_time.vgrectlink_time = now;
        get_alarm_vgrectlink(&alarm_link, alm_map->cfg.lk_vgrect);
        break;
    }
    case JALARM_TYPE_MASK: {
        dbg_alarm("got JALARM_TYPE_MASK\n");
        event_time = last_event_time.maskalarm_time;
        event_type = JEvent_AlarmVMask;
        last_event_time.maskalarm_time = now;
        get_vmaskalarmlink_param(&alarm_link, alm_map->cfg.lk_vmask);
        break;
    }
    case JALARM_TYPE_HUMAN_DETECT: {
        dbg_alarm("got JALARM_TYPE_HUMAN_DETECT\n");
        event_time = last_event_time.humandetectlink_time;
        event_type = JEvent_Alarmhumadetect;
        last_event_time.humandetectlink_time = now;
        get_alarm_humandetectlink(&alarm_link, alm_map->cfg.lk_hd);
        break;
    }
    case JALARM_TYPE_CAR: {
        dbg_alarm("got JALARM_TYPE_CAR\n");
        event_time = last_event_time.carlink_time;
        event_type = JEvent_AlarmCar;
        last_event_time.carlink_time = now;
        get_caralarmlink_param(&alarm_link, alm_map->cfg.lk_car);
        break;
    }

    case JALARM_TYPE_SCENE_CHANGE: {
        DBG("got JALARM_TYPE_SCENE_CHANGE\n");
        event_time = last_event_time.scenelink_time;
        event_type = JEvent_SceneChange;
        last_event_time.scenelink_time = now;
        break;
    }
  
    case JALARM_TYPE_PET: {
        DBG("got JALARM_TYPE_PET\n");
        event_time = last_event_time.petlink_time;
        event_type = JEvent_AlarmPet;
        last_event_time.petlink_time = now;
        get_petalarmlink_param(&alarm_link, alm_map->cfg.lk_pet);
        break;
    }

    case JALARM_TYPE_CRY: {
        DBG("got JALARM_TYPE_CRY\n");
        event_time = last_event_time.crylink_time;
        event_type = JEvent_AlarmCry;
        last_event_time.crylink_time = now;
        get_cryalarmlink_param(&alarm_link, alm_map->cfg.lk_cry);
        break;
    }
  
    case JALARM_TYPE_IP_CONFLICT: {
        dbg_alarm("got JALARM_TYPE_IP_CONFLICT\n");
        event_time = last_event_time.ipconflictlink_time;
        event_type = JEvent_AlarmIpConflict;
        last_event_time.ipconflictlink_time = now;
        get_alarm_ipconflictlink(&alarm_link, alm_map->cfg.lk_ipcflict);
        break;
    }
    case JALARM_TYPE_CABLE_DISC: {
        dbg_alarm("got JALARM_TYPE_CABLE_DISC\n");
        event_time = last_event_time.cablediscon_time;
        event_type = JEvent_AlarmCabDis;
        last_event_time.cablediscon_time = now;
        get_alarm_ipbrokenlink(&alarm_link, alm_map->cfg.lk_ipbroken);
        break;
    }

    default:
        dbg_alarm("%s no find type : %d\n", __func__, alarminfo->type);
        return 0;
    }

    //网口断开事件不做校验，走实时逻辑
    if (alarminfo->type != JALARM_TYPE_CABLE_DISC) {
        if (event_time <= now && ((event_time + ALARM_NOTIFY_INTERVAL) >= now)) {
            dbg_alarm("last event time: [%llu], now time: [%llu], diff: %llu\n",
                      event_time, now, now - event_time);
            return 0;
        }
    }

    memcpy(&alm_map->last_event_time, &last_event_time, sizeof(last_alarm_time_t));

    dbg_alarm("send_event: %d, link_time: %lld\n", (int)event_type, event_time);
    send_event_chn(event_type, chn);

    do_link_process(alarminfo);
    return 0;
}

static int time_check_process(JALARM_TYPE type, int chn, alarm_map *alm_map)
{
    TimeCbS maps[] = {
        {JALARM_TYPE_MD          , alm_map->cfg.cfg_md.times,           &alm_map->cfg.cfg_md.enable},
        {JALARM_TYPE_HUMAN_DETECT, alm_map->cfg.cfg_hd.times,           &alm_map->cfg.cfg_hd.enable},
        {JALARM_TYPE_VGLINE      , alm_map->cfg.cfg_vgline.times,       &alm_map->cfg.cfg_vgline.enable},
        {JALARM_TYPE_VGRECT      , alm_map->cfg.cfg_vgrect.times,       &alm_map->cfg.cfg_vgrect.enable},
        // {JALARM_TYPE_VL          , alm_map->cfg.cfg_vloss.times,        &alm_map->cfg.cfg_vloss.enable},
        {JALARM_TYPE_MASK        , alm_map->cfg.cfg_vmask.times,        &alm_map->cfg.cfg_vmask.enable},
        {JALARM_TYPE_CAR         , alm_map->cfg.cfg_car.times,          &alm_map->cfg.cfg_car.enable},
        {JALARM_TYPE_PET         , alm_map->cfg.cfg_pet.times,          &alm_map->cfg.cfg_pet.enable},
        {JALARM_TYPE_CRY         , alm_map->cfg.cfg_cry.times,          &alm_map->cfg.cfg_cry.enable},
        {JALARM_TYPE_HUMAN_DETECT, alm_map->cfg.cfg_follow.times,       &alm_map->cfg.cfg_follow.enable},
    };

    int i = 0;
	int ab_count = 2;
	int time_count = 2;
    for (i = 0; i < ARRAY_SIZE(maps); i++) {
        if (maps[i].type == type) {
            if (JALARM_TYPE_HUMAN_DETECT == maps[i].type) {
                if (*(maps[i].enable) == 0) {
                    ab_count--;
                }

                if (TimeJudge(maps[i].times) == FALSE) {
                    time_count--;
                }

                if (!ab_count) {
                    DBG("enable is 0\n");
                    return -1;
                }

                if (!time_count) {
                    dbg_alarm("time is wrong\n");
                    return -1;
                }
            } else {
                if (*(maps[i].enable) == 0) {
                    DBG("enable is 0\n");
                    return -1;
                }

                if (TimeJudge(maps[i].times) == FALSE) {
                   dbg_alarm("time is wrong\n");
                   return -1;
                }
            }
        }
    }

    return 0;
}

static int read_alarm_link_cmd(ALARM_INFOS *alarminfo)
{
    int ret = -1;
    ALARM_INFOS *handle = NULL;
    if (NULL == g_alm_queue) {
        return -1;
    }
    
    handle = (ALARM_INFOS *)fifo_queue_pop_unblock(g_alm_queue);
    if (handle == NULL) {
        return -1;
    }    

    memcpy(alarminfo, handle, sizeof(ALARM_INFOS));
    ret = handle->type;
    DBG("pop alarm type: %d\n", ret);

    if (handle) {
        free(handle);
        handle = NULL;
    }

    return ret;
}

static void alarm_link_cb(void *data)
{
    struct alarm_map *alm_map = (alarm_map *)data;
    int i = 0;
    ALARM_INFOS alarminfo = {0};

    while (read_alarm_link_cmd(&alarminfo) >= 0) {
        if (alm_map->cfg_stat) {
            for (i = 0; eventCmd[i].alarm_type != JEvent_End; i++) {
                if (alm_map->cfg_stat & eventCmd[i].cmd) {
                    get_cfg_param(eventCmd[i].alarm_type, alm_map);
                    alm_map->cfg_stat &= (~eventCmd[i].cmd);
                }
            }
        }

        if (NEED_TIME_CHECK == alarminfo.filter) {
            if (time_check_process(alarminfo.type, alarminfo.chn, alm_map) < 0) {
                continue;
            }

            // 由于 alarm_report 已经是 4s 的间隔，这个地方不需要再进行过滤
            // if (alarminfo.type == JALARM_TYPE_MD || alarminfo.type == JALARM_TYPE_VGLINE || 
            //    alarminfo.type == JALARM_TYPE_VGRECT || alarminfo.type == JALARM_TYPE_HUMAN_DETECT ||
            //    alarminfo.type == JALARM_TYPE_CAR || alarminfo.type == JALARM_TYPE_MASK ||
            //    alarminfo.type == JALARM_TYPE_PET) {
            //     // 移动、区域、越线、人形、车型、遮挡算法开关灯可能会误报，开关的的 5S 内过滤掉
            //     if (mono_sec() - get_lamp_switch_time() <= 5) {
            //         DBG("Switching lights off, filter\n");
            //         continue;
            //     }
            // }
        }

        alarm_link_process(&alarminfo, alm_map);
    }
}

static void set_cfg_stat(int id, void *p_src, int size, void *data)
{
    int event_cmd = *(int *)data;
    DBG("event type = %d\n", event_cmd);

    map.cfg_stat |= event_cmd;
}

int init_alarm_server(void *data)
{
    int i = 0;
    
    if (init_alarm_log(data) < 0) {
        return -1;
    }

    sch_alarm = (JSScheduler)data;

    g_alm_queue = create_fifo_queue();

    for (i = 0; eventCmd[i].alarm_type != JEvent_End; i++) {
        get_cfg_param(eventCmd[i].alarm_type, &map);
        attach_config(eventCmd[i].alarm_type, set_cfg_stat, (void *)&eventCmd[i].cmd);
    }

    init_handle_alarm_cancel(sch_alarm);

    js_create_timer_r(sch_alarm, INTV_ALARM, INTV_ALARM, alarm_link_cb, &map, &hdl_alarm);
    
    g_alarm_init = true;
    return 0;
}

int uninit_alarm_server(void)
{
    g_alarm_init = false;

    uninit_handle_alarm_cancel();

    uninit_alarm_log();

    if (hdl_alarm) {
        js_delete_timer_r(&hdl_alarm);
    }

    for (int i = 0; eventCmd[i].alarm_type != JEvent_End; i++) {
        get_cfg_param(eventCmd[i].alarm_type, &map);
        detach_config(eventCmd[i].alarm_type, set_cfg_stat, (void *)&eventCmd[i].cmd);
    }

    if (g_alm_queue) {
        release_fifo_queue(g_alm_queue);
        g_alm_queue = NULL;
    }

    sch_alarm = NULL;

    return 0;
}

