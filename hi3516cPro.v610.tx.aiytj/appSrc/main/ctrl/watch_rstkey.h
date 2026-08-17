/*
 *       Filename:  watch_rstkey.h
 *    Description:
 *        Version:  1.0
 *        Created:  01/08/2015 02:23:15 PM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  zhangjian (),
 *   Organization:
 */

#ifndef _WATCH_RSTKEY_H
#define _WATCH_RSTKEY_H
#ifdef __cplusplus
extern "C" {
#endif

int init_client_rst_key(void *data);
void uninit_client_rst_key(void);
int rst_get_stat(int *rststat);
void uninit_watch_free();

#ifdef __cplusplus
}
#endif
#endif
