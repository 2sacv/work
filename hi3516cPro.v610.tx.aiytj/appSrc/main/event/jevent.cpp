#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include "jevent.h"
#include "debug.h"

static JSEventManager *p_em_alarm = NULL;

JSEventManager *get_ev_mng_alarm(void)
{
    return p_em_alarm;
}

void send_event_data(JSEventType id, void *cb_data)
{
    js_event_send(p_em_alarm, id, cb_data, sizeof(int));
}

void send_event_chn(JSEventType id, int chn)
{
    js_event_send(p_em_alarm, id, &chn, sizeof(int));
}

void send_event(JSEventType id)
{
    js_event_send(p_em_alarm, id, NULL, 0);
}

int init_server_event()
{
    p_em_alarm = js_event_manager_new();
    return_val_if_fail(p_em_alarm != NULL, FAILURE);

    int id_events[] = {
		JEvent_AlarmMD            ,
	    JEvent_Alarmhumadetect    ,
        JEvent_AlarmFacesnap      ,
		JEvent_AlarmFace          ,
        JEvent_AlarmCar           ,
        JEvent_AlarmPet           ,
        JEvent_SceneChange        ,
        JEvent_AlarmCry           ,
        JEvent_AlarmVgline        ,
        JEvent_AlarmVgrect        ,
		JEvent_AlarmVL			  , 	
		JEvent_AlarmCabDis		  ,
		JEvent_AlarmIpConflict	  ,
		JEvent_AlarmIllegalVisit  , 
		JEvent_AlarmAI0 		  ,
		JEvent_AlarmAI1 		  ,
		JEvent_AlarmAI2 		  ,
		JEvent_AlarmAI3 		  ,
		JEvent_AlarmExpand0 	  ,
		JEvent_AlarmExpand1 	  ,
		JEvent_AlarmExpand2 	  ,
		JEvent_AlarmExpand3 	  ,
		JEvent_AlarmExpand4 	  ,
		JEvent_AlarmExpand5 	  ,
		JEvent_AlarmExpand6 	  ,
		JEvent_AlarmExpand7 	  ,
		JEvent_AlarmExpand8 	  ,
		JEvent_AlarmExpand9 	  ,
		JEvent_AlarmExpand10	  ,
		JEvent_AlarmExpand11	  ,
		JEvent_AlarmExpand12	  ,
		JEvent_AlarmExpand13	  ,
		JEvent_AlarmExpand14	  ,
		JEvent_AlarmExpand15	  ,
		JEvent_AlarmVMask		  ,
		JEvent_AlarmCableNormal	  ,
		JEvent_StopAlarmMD        ,
		JEvent_HTTPERR_408        ,
		JEvent_LedTest            ,
		JEvent_RunIspColor        ,
		JEvent_RunLuma            ,
		JEvent_RunFreezMD         ,
		JEvent_Auth_Success       ,
		JEvent_ALGO_Forzen        ,
		JEvent_RunFreezLampCtrl   ,
        JEvent_RunButtCtrl        ,
        JEvent_AlarmRMRcord       ,
        JEvent_DevVideoReport     ,
        JEvent_TencentReset       ,
        JEvent_Tencent_Offline    ,
        JEvent_Quality_Change     ,
        JEvent_Sim4gLocation      ,
        JEvent_DevVideoCodec      ,
    };

    int i = 0;
    for (i = 0; i < ARRAY_SIZE(id_events); i++) {
        js_event_register_type(p_em_alarm, id_events[i]);
    }

    return 0;
}
