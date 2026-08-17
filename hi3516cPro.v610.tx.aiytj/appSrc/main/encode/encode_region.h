#ifndef __ENCODE_REGION_H__
#define __ENCODE_REGION_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "jconfstruct.h"

int encode_region_get_venc(int group, VideoEnc0 *venc);

int encode_osd_vgline_group_init(int group, VglineS *vgLinecfg);
int encode_osd_vgline_update(int group, VglineS *vgLinecfg, int blink_cnt, int is_show);
int encode_osd_vgline_group_uninit(int group);
int encode_osd_vgrect_group_init(int group, VgrectS *vgRectcfg);
int encode_osd_vgrect_update(int group, VgrectS *vgRectcfg, int blink_cnt, int is_show);
int encode_osd_vgrect_group_uninit(int group);
int encode_osd_aidet_group_init(int group);
int encode_osd_aidet_group_uninit(int group);

int encode_osd_vgline_init(void);
int encode_osd_vgline_uninit(void);
int encode_osd_vgrect_init(void);
int encode_osd_vgrect_uninit(void);
int encode_osd_aidet_init(void);
int encode_osd_aidet_uninit(void);

int encode_osd_aidet_draw(osd_aidet_t *aidet_info);

#ifdef __cplusplus
}
#endif
#endif

