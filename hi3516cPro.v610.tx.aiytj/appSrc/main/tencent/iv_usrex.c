#ifdef PLATFORM_TENCENT 
#include "iv_usrex.h"
#include "iv_def.h"
/*-----------------data config start  -------------------*/ 

#define TOTAL_WR_PROPERTY_COUNT 1

static sDataPoint    sg_WR_DataTemplate[TOTAL_WR_PROPERTY_COUNT];

#define TOTAL_RO_PROPERTY_COUNT 7

static sDataPoint    sg_RO_DataTemplate[TOTAL_RO_PROPERTY_COUNT];

ivm_extend_param_t    g_ivm_objs;

static void _ivm_init_data_template(void)
{
    sg_RO_DataTemplate[0].data_property.data = &g_ivm_objs.ProReadonly.m_cid;
    sg_RO_DataTemplate[0].data_property.key  = "cid";
    sg_RO_DataTemplate[0].data_property.type = TYPE_TEMPLATE_INT;
    sg_RO_DataTemplate[0].state = eNOCHANGE;

    sg_RO_DataTemplate[1].data_property.data = &g_ivm_objs.ProReadonly.m_lac;
    sg_RO_DataTemplate[1].data_property.key  = "lac";
    sg_RO_DataTemplate[1].data_property.type = TYPE_TEMPLATE_INT;
    sg_RO_DataTemplate[1].state = eNOCHANGE;

    sg_RO_DataTemplate[2].data_property.data = &g_ivm_objs.ProReadonly.m_mnc;
    sg_RO_DataTemplate[2].data_property.key  = "mnc";
    sg_RO_DataTemplate[2].data_property.type = TYPE_TEMPLATE_INT;
    sg_RO_DataTemplate[2].state = eNOCHANGE;

    sg_RO_DataTemplate[3].data_property.data = &g_ivm_objs.ProReadonly.m_mcc;
    sg_RO_DataTemplate[3].data_property.key  = "mcc";
    sg_RO_DataTemplate[3].data_property.type = TYPE_TEMPLATE_INT;
    sg_RO_DataTemplate[3].state = eNOCHANGE;

    sg_RO_DataTemplate[4].data_property.data = &g_ivm_objs.ProReadonly.m_networkType;
    sg_RO_DataTemplate[4].data_property.key  = "networkType";
    sg_RO_DataTemplate[4].data_property.type = TYPE_TEMPLATE_ENUM;
    sg_RO_DataTemplate[4].state = eNOCHANGE;

    sg_RO_DataTemplate[5].data_property.data = &g_ivm_objs.ProReadonly.m_location_4g;
    sg_RO_DataTemplate[5].data_property.key  = "location_4g";
    sg_RO_DataTemplate[5].data_property.type = TYPE_TEMPLATE_INT;
    sg_RO_DataTemplate[5].state = eNOCHANGE;

    sg_RO_DataTemplate[6].data_property.data = &g_ivm_objs.ProReadonly.m_CallStatus;
    sg_RO_DataTemplate[6].data_property.key  = "CallStatus";
    sg_RO_DataTemplate[6].data_property.type = TYPE_TEMPLATE_INT;
    sg_RO_DataTemplate[6].state = eNOCHANGE;

    sg_WR_DataTemplate[0].data_property.data = &g_ivm_objs.ProWritable.m_record_enable;
    sg_WR_DataTemplate[0].data_property.key  = "record_enable";
    sg_WR_DataTemplate[0].data_property.type = TYPE_TEMPLATE_BOOL;
    sg_WR_DataTemplate[0].state = eNOCHANGE;

};

#define TOTAL_ACTION_COUNTS     (1)

static ivm_DataServicesByJCP_t sg_DataServicesByJCP;
static DeviceProperty g_actionInput_DataServicesByJCP[] = {
   {.key = "jcpParams", .data_buff_len = sizeof(sg_DataServicesByJCP.action_in.m_jcpParams) - 1, .data = sg_DataServicesByJCP.action_in.m_jcpParams, .type = TYPE_TEMPLATE_STRING},
};
static DeviceProperty g_actionOutput_DataServicesByJCP[] = {
   {.key = "result", .data_buff_len = sizeof(sg_DataServicesByJCP.action_out.m_result) - 1, .data = sg_DataServicesByJCP.action_out.m_result, .type = TYPE_TEMPLATE_STRING},
};

static DeviceAction g_actions[]={
    {
     .pActionId = "DataServicesByJCP",
     .timestamp = 0,
     .input_num = 1,
     .output_num = 1,
     .pInput = g_actionInput_DataServicesByJCP,
     .pOutput = g_actionOutput_DataServicesByJCP,
    },
};

#define EVENT_COUNTS     (1)

static DeviceProperty g_propertyEvent_uploadDeviceEvent[] = {
   {.key = "eventType", .data = &g_ivm_objs.Event.m_uploadDeviceEvent.m_eventType, .type = TYPE_TEMPLATE_ENUM},
   {.key = "payload", .data = g_ivm_objs.Event.m_uploadDeviceEvent.m_payload, .type = TYPE_TEMPLATE_STRING},
   {.key = "channel", .data = &g_ivm_objs.Event.m_uploadDeviceEvent.m_channel, .type = TYPE_TEMPLATE_INT},

};

static sEvent g_events[]={
    {
     .event_name = "uploadDeviceEvent",
     .type = "alert",
     .timestamp = 0,
     .eventDataNum = 3,
     .pEventData = g_propertyEvent_uploadDeviceEvent,
    },
};

extern int ivm_init_Action(void *obj, int num, char *name, ivm_callback_Action cb);
extern int ivm_init_ProWritable(void *obj, int num, char *name, ivm_callback_ProWritable cb, char close_sync);
extern int ivm_init_ProReadonly(void *obj, int num, char *name);
extern int ivm_init_Event(void *obj, int num, char *name);

#define ivm_doi_init_ProReadonly(data) ivm_init_ProReadonly(sg_RO_DataTemplate, TOTAL_RO_PROPERTY_COUNT, #data)
#define ivm_doi_init_ProWritable(data, flag) ivm_init_ProWritable(sg_WR_DataTemplate, TOTAL_WR_PROPERTY_COUNT, #data, (ivm_callback_ProWritable)iv_usrcb_ProWritable_##data, flag)
#define ivm_doi_init_Action(data)  ivm_init_Action(g_actions, TOTAL_ACTION_COUNTS, #data, (ivm_callback_Action)iv_usrcb_Action_##data)
#define ivm_doi_init_Event(data) ivm_init_Event(g_events, EVENT_COUNTS, #data)


int ivm_env_init(void)
{
    memset((void *) &g_ivm_objs, 0, sizeof(ivm_extend_param_t));
    _ivm_init_data_template();

    ivm_doi_init_ProReadonly(cid);
    ivm_doi_init_ProReadonly(lac);
    ivm_doi_init_ProReadonly(mnc);
    ivm_doi_init_ProReadonly(mcc);
    ivm_doi_init_ProReadonly(networkType);
    ivm_doi_init_ProReadonly(location_4g);
    ivm_doi_init_ProReadonly(CallStatus);
    ivm_doi_init_ProWritable(record_enable, 0);

    ivm_doi_init_Action(DataServicesByJCP);

    ivm_doi_init_Event(uploadDeviceEvent);

    return 0;
}
#endif //PLATFORM_TENCENT

