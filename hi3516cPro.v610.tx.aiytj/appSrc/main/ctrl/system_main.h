/*
 *       Filename:  system_main.h
 *    Description:
 *        Version:  1.0
 *        Created:  04/15/2014 02:25:32 PM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  zhangjian (),
 *   Organization:
 */

#ifndef _SYSTEM_MAIN_H
#define _SYSTEM_MAIN_H
#ifdef __cplusplus
extern "C" {
#endif

/* macro */
#if defined(CUST_PASSENGER)
#define MAX_ALARM_IN    0
#define MAX_ALARM_OUT   1
#else
#define MAX_ALARM_IN    0
#define MAX_ALARM_OUT   0
#endif

/* declaration */
#include "jconfstruct.h"

int system_get_quit(void);
int system_upmedia_uninit(void);
int system_upgrade_uninit(void);

#ifdef __cplusplus
}
#endif
#endif
