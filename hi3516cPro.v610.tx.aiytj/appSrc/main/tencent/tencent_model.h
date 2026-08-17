#ifdef PLATFORM_TENCENT

#ifndef TENCENT_MODEL_H_
#define TENCENT_MODEL_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "iv_def.h"
#include "iv_av.h"
#include "iv_cm.h"
#include "iv_config.h"
#include "qcloud_iot_export.h"

//物模型下发BUF的大小
#define MAX_MODEL_BUFSIZE (4096)
enum {
    EVENT_MD            = 1,
    EVENT_VGRECT        = 2,
    EVENT_HD            = 3,
    EVENT_VGLINE        = 4,
    EVENT_CAR           = 5,
    EVENT_LOW_BATTERY   = 6,
    EVENT_VIDEOMASK     = 7,
    EVENT_FLOW          = 8,
    EVENT_THROW         = 9,
    EVENT_ELEC_CAR      = 10,
    EVENT_PIR           = 11,
    EVENT_VIDEOCALL     = 100,// 一键呼叫
};

void tencent_report_event(int eventType, int channel, char *payload);
void tencent_report_attr(int status);
int tencent_model_init(void);
void tencent_model_uninit(void);

#ifdef __cplusplus
}
#endif

#endif
#endif //PLATFORM_TENCENT

