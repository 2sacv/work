#ifdef PLATFORM_TENCENT
#ifndef _TENCENT_EVENT_HANDLE_H_
#define _TENCENT_EVENT_HANDLE_H_

#include "iv_def.h"
#include "iv_av.h"
#include "iv_cm.h"
#include "iv_config.h"
#include "qcloud_iot_export.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char uint8_t;
typedef unsigned int uint32_t;

#define OTA_FIRMWARE_PATH    "/tmp/upgrade.tgz"

int tencent_p2p_is_running(void);
int tencent_p2p_is_timesync(void);

void tencent_talk_notify_process(iv_avt_event_e event, uint32_t visitor,
        uint32_t channel, iv_avt_video_res_type_e video_res_type);

int  tencent_talk_command_proc(iv_avt_command_type_e command, uint32_t visitor,
        uint32_t channel, iv_avt_video_res_type_e video_res_type, void *args);

int  tencent_register_user_event(void);
int  tencent_unregister_user_event(void);
void cb_tencent_sync_utc_time(void *data);

#ifdef __cplusplus
}
#endif

#endif //_TENCENT_EVENT_HANDLE_H_
#endif //PLATFORM_TENCENT

