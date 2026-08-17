/*
 *       Filename:  lamp_sal.h
 *    Description:  
 *        Version:  1.0
 *        Created:  11/07/2025 11:39:13 AM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  wangr (), 
 *   Organization:  
 */
#ifndef __LAMP_SAL_H__
#define __LAMP_SAL_H__
#ifdef __cplusplus 
extern "C" {
#endif

#include "jconfstruct.h"

// hi3516cv608 画面越暗 ev 值越大
#define MAX_EXPOSURE (249485761)

int lamp_get_ev_lux(LightExtCfg *lamp);
uint32_t get_invert_exposure(void);
int lamp_set_day(int change_light);
int lamp_set_color(int change_light);
int lamp_set_night(void);

#ifdef __cplusplus
}
#endif
#endif // __LAMP_SAL_H__
