/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : alarmapi.cpp
 * @Created Time : 2014-03-11
 * @Version      : 1.0
 * @Author       : zengy
 * @Description  :
 */

#include <stdio.h>
#include <time.h>
#include <sys/times.h>

#include "mxml.h"
#include "jconfstruct.h"
#include "debug.h"
#include "alarm_service.h"
#include "alarm_log.h"
#include "alarmapi.h"

typedef struct {
    JALARM_TYPE   type;
    const char*   cmd;
} Type2CmdMap;

Type2CmdMap tcMap[] = {
    {JALARM_TYPE_BEGIN         , NULL},
    {JALARM_TYPE_MD            , "motionDetect"},
    {JALARM_TYPE_VGLINE        , "vgline"      },
    {JALARM_TYPE_VGRECT        , "vgrect"      },
    {JALARM_TYPE_VL            , "videoLoss"   },

    {JALARM_TYPE_CABLE_DISC    , "cableDiscon" },
    {JALARM_TYPE_IP_CONFLICT   , "IPConflict"  },
    {JALARM_TYPE_ILLEGAL_ACCESS, "illegalVisit"},
        
    {JALARM_TYPE_EXP           , "expandAlarm" },
    {JALARM_TYPE_MASK          , "maskalarm"   },
    {JALARM_TYPE_HUMAN_DETECT  , "humanDetect" },

    {JALARM_TYPE_CAR           , "carDetect"  },
    {JALARM_TYPE_PET           , "petDetect"  },
    {JALARM_TYPE_CRY           , "cryDetect"  },
    {JALARM_TYPE_EBIKE         , "ebikeDetect" }, 
    {JALARM_TYPE_THROW         , "throwDetect" },
    
    {JALARM_TYPE_CIGARETTE     , "cigaretteDetect" },
    {JALARM_TYPE_FALL          , "fall"            },
    {JALARM_TYPE_BARCODE       , "barcode"         },
    {JALARM_TYPE_DISK_ERR      , "diskerror"       },
    {JALARM_TYPE_SCENE_CHANGE  , "senceChange"     },
    {JALARM_TYPE_END           , NULL              },
};

int alarm_report(JALARM_TYPE type, int channel, int filter, const char *desc)
{
    if (type <= JALARM_TYPE_BEGIN || type >= JALARM_TYPE_END) {
        ERR("Parameter unavailable....\n");
        return -1;
    }

    if ((channel < 0 || channel > (AIN_MAX_CHN - 1))) {
        ERR("Parameter 'channel' unavailable....\n");
        return -1;
    }

    if ((type == JALARM_TYPE_EXP) && (channel < 0 || channel > (AEXPAND_MAX_CHN - 1))) {
        ERR("Parameter 'channel' unavailable....\n");
        return -1;
    }

    if (filter != 0 && filter != 1) {
        ERR("filter need 1 | 0\n");
        return -1;
    }

    int i = -1;
    for (i = 0; tcMap[i].type != JALARM_TYPE_END; i++) {
        if (type == tcMap[i].type) {
            break;
        }
    }

    if (tcMap[i].type == JALARM_TYPE_END) {
        ERR("alarm type is not exist\n");
        return -1;
    }
    
    ALARM_INFOS alarminfo = {0};
    alarminfo.type = type;
    alarminfo.chn = channel;
    alarminfo.filter = filter;
    if (NULL != desc) {
        strcpy(alarminfo.desc, desc);
    }

	return parse_alarm_handle((void *)&alarminfo, NULL, 0, NULL, NULL);
}

int alarm_query_handle(void *req, void *buf, int bufsize, int *retlen, void *arg)
{
    if (NULL == req) {
        ERR("Paramter error!\n");
        return -1;
    }

    QueryInfoS *info = (QueryInfoS *) req;

    if (alarm_event_query(info, (char *)buf, bufsize, retlen) < 0) {
        return -1;
    }

    return 0;
}

int alarm_log_sync_handle(void *req, void *buf, int bufsize, int *retlen, void *arg)
{
    return alarm_log_sync();
}

int alarm_query(QueryInfoS *info, char *buf, int bufsize)
{
    if(info == NULL || NULL == buf || 0 == bufsize) {
        ERR("parameter error!\n");
        return -1;
    }

    struct tm tim;

    if (strptime(info->stime, "%Y-%m-%d %H:%M:%S", &tim) == NULL) {
        ERR("time format wrong\n");
        return -1;
    }

    if (strptime(info->etime, "%Y-%m-%d %H:%M:%S", &tim) == NULL) {
        ERR("time format wrong\n");
        return -1;
    }

    if (strcmp(info->stime, info->etime) > 0) {
        ERR("starttime > endtime\n");
        return -1;
    }

    if ((info->itemindex) < 0) {
        info->itemindex = 1;
    }

    if ((info->itemnum) < 0) {
        info->itemnum = 50;
    }

	int respLen = 0;
	
	return alarm_query_handle((void*)info, buf, bufsize, &respLen, NULL);  
}

int alarm_sync_log()
{
    return alarm_log_sync_handle(NULL, NULL, 0, NULL, NULL);
}

