/** Copyright (C) by Jabsco Company
*
* @File Name	: jcoTOgb.h
* @Created Time : 2016-11-08
* @Version		: 1.0
* @Author		: 
* @Description	:
*/
#ifdef PLATFORM_GB
#ifndef __JCOTOGB_H__
#define __JCOTOGB_H__

#ifdef __cplusplus 
extern "C" {
#endif
#define tracepoint() printf("%s %d\n",__FILE__, __LINE__)
int init_guobiao_server();
int uninit_guobiao_server();

int ipc_gb_set_online(int status);
int ipc_gb_get_online();

int ipc_gb_stop_playback();

#ifdef __cplusplus
}

#endif 
#endif
#endif

