#ifndef __ENCODE_IVE_MOTION_H__
#define __ENCODE_IVE_MOTION_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "time.h"

int change_motion_time_interval(struct timespec time);
void encode_ive_md_process(MotionDetectS *mdinfo);
int encode_ive_md_set_param(MotionDetectS *mdinfo);
int encode_ive_md_init(MotionDetectS *mdinfo);
int encode_ive_md_uninit(void);

#ifdef __cplusplus
}
#endif
#endif

