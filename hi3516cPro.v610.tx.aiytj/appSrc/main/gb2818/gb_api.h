/** Copyright (C) by Jabsco Company
*
* @File Name    : gb_api.h
* @Created Time : 2024-04-29
* @Version      : 1.0
* @Author       :
* @Description  : 国标模块对外提供的所有接口都要在这里声明，外部调用国标函数的地方统一包含此文件
*/
#ifndef GB_API_H_
#define GB_API_H_

#ifdef __cplusplus
extern "C" {
#endif

int gb_init_server();
int gb_uninit_server();
int gb_get_online();
int gb_stop_playback();
const char *gb_get_version_str();

#ifdef __cplusplus
}
#endif

#endif