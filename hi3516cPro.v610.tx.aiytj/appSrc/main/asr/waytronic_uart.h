/*
 *       Filename:  waytronic_uart.h
 *    Description:  唯创知音的音频处理芯片串口协议交互模块
 *        Version:  1.0
 *        Created:  03/02/2026 04:14:11 PM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  xiangyp (), 
 *   Organization:  
 */

#ifndef _WAYTRONIC_UART_H
#define _WAYTRONIC_UART_H
#ifdef __cplusplus 
extern "C" {
#endif

#define PCT_VOLUME_MAX          (100)
#define PCT_VOLUME_MIN          (0)
#define PCT_VOLUME_ADD          (10)
#define PCT_VOLUME_DEC          (10)

int init_waytronic_uart(void);

int uninit_waytronic_uart(void);

void audiocfg_add_outvolume(int grade);

void audiocfg_set_outvolume(int grade);

#ifdef __cplusplus
}
#endif
#endif
