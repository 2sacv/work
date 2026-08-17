#ifndef __ENCODE_VI_ISP_H__
#define __ENCODE_VI_ISP_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "ot_type.h"
#include "ot_common_isp.h"
#include "ot_sns_ctrl.h"

#include "encode_vi.h"

#define ENCODE_SENSOR0_FRAME_RATE (15)

int encode_vi_isp_start(const vi_cfg_t *vi_cfg);
void encode_vi_isp_stop(const vi_cfg_t *vi_cfg);
int encode_vi_isp_get_pub_attr_by_sns(sns_type_t sns_type, ot_isp_pub_attr *pub_attr);
ot_isp_sns_obj *encode_vi_isp_get_sns_obj(sns_type_t sns_type);


#ifdef __cplusplus
}
#endif
#endif

