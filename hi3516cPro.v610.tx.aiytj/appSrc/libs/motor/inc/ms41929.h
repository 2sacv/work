/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    : pwm.h
 * Created Time : 2017-12-28
 * Version      : 1.0
 * Author       : tianjun
 * Description  :
 */


#ifndef __MS41929_MSTAR_IPC_H__
#define __MS41929_MSTAR_IPC_H__


#ifdef __cplusplus
extern "C" {
#endif

int ms41929_get_focus_state(void);
int ms41929_get_zoom_state(void);
int ms41929_clear_focus_isr(void);
int ms41929_clear_zoom_isr(void);
int ms41929_vdfz(void); 
int ms41929_write_reg(unsigned char addr,unsigned short value);
int  ms41929_read_reg(unsigned char addr,unsigned short *value);
int m41909_reg_dump(void);
int ms41929_init(void);
int ms41929_uninit(void);

#ifdef __cplusplus
}
#endif

#endif //__MS41929_MSTAR_IPC_H__

