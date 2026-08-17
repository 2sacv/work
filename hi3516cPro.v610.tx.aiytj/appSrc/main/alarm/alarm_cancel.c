/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    :
 * Created Time : 2015-02-06
 * Version      : 1.0
 * Author       : cheby
 * Description  :
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "alarm_event.h"
#include "debug.h"
#include "alarm_service.h"
#include "jevent.h"
#include "js_scheduler.h"
#include "system_sch.h"
#include "confapi.h"
#include "alarm_cancel.h"

typedef struct {
    double          time;
	int             chn;
	int             msgid;
    int             tick;
    int             enable;
} cancel_handle_t;

#define ALARM_EV_BOTTON JALARM_TYPE_END
static cancel_handle_t hdl_cancel[JALARM_TYPE_END] = {0};
static JSScheduler sch_cancel = NULL;
static JSTCHandle hdl_almcancle = NULL;

#define ALARM_CANCEL_TIMER     1
#define ALARM_CANCEL_INTERVAL  5  // ALARM_CANCEL_INTERVAL * ALARM_CANCEL_TIMER = 5s

int init_handle_alarm_cancel(void* data) 
{
    if (sch_cancel) {
        return -1;
    }

    memset(&hdl_cancel, 0, sizeof(cancel_handle_t)*ALARM_EV_BOTTON);
    sch_cancel = data;
    
    return 0;
}

void uninit_handle_alarm_cancel(void)
{
	js_delete_timer_r(&hdl_almcancle);
}

void set_alarm_cancel(void *data)
{
    if (hdl_cancel[JALARM_TYPE_MD].enable && (hdl_cancel[JALARM_TYPE_MD].tick++) >= 5) {
        hdl_cancel[JALARM_TYPE_MD].enable = 0;
        send_event(JEvent_StopAlarmMD);
    }

    if (hdl_cancel[JALARM_TYPE_VGLINE].enable && (hdl_cancel[JALARM_TYPE_VGLINE].tick++) >= 5) {
        hdl_cancel[JALARM_TYPE_VGLINE].enable = 0;
    }

    if (hdl_cancel[JALARM_TYPE_VGRECT].enable && (hdl_cancel[JALARM_TYPE_VGRECT].tick++) >= 5) {
        hdl_cancel[JALARM_TYPE_VGRECT].enable = 0;
    }

    if (hdl_cancel[JALARM_TYPE_HUMAN_DETECT].enable && (hdl_cancel[JALARM_TYPE_HUMAN_DETECT].tick++) >= 5) {
        hdl_cancel[JALARM_TYPE_HUMAN_DETECT].enable = 0;
    }

    if (hdl_cancel[JALARM_TYPE_CAR].enable && (hdl_cancel[JALARM_TYPE_CAR].tick++) >= 5) {
        hdl_cancel[JALARM_TYPE_CAR].enable = 0;
    }
}

int handle_alarm_cancel(int id)
{
    if (id != JALARM_TYPE_MD && id != JALARM_TYPE_VGLINE && id != JALARM_TYPE_VGRECT 
        && id != JALARM_TYPE_HUMAN_DETECT && id != JALARM_TYPE_CAR) {
        return 0;
    }

    hdl_cancel[id].chn = 0;
    hdl_cancel[id].msgid = id;
    hdl_cancel[id].enable = 1;
    hdl_cancel[id].tick = 0;
    
    if (NULL == hdl_almcancle) {
    	js_create_timer_r(sch_cancel, ALARM_CANCEL_TIMER*1000, ALARM_CANCEL_TIMER*1000, set_alarm_cancel, NULL, &hdl_almcancle);
    }

	return 0;
}

