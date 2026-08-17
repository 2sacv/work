/*
 *       Filename:  url.h
 *    Description:  统一资源
 *        Version:  1.0
 *        Created:  2025年03月14日 09时49分52秒
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  zhangjian (),
 *   Organization:
 */

#ifndef _URL_H
#define _URL_H
#ifdef __cplusplus
extern "C" {
#endif

#define URL_4G_AIRBURN  "produce.platform.cnjabsco.com"     // 烧录
#define METHOD_API_ACTION  "/api/action"

// 绑定，解绑，4g定位，异常上报(sd_stat,p2pid,custom_appid,)，TWecall
#define URL_PROD        "platform.cnjabsco.com"
#define METHOD_TOOL_DEV    "/api/tool/device/action"

#define URL_4G_REPORT   "flow.hcciot.com"                   // 4G上报
#define METHOD_API_DEV     "/api/device/action"

#ifdef __cplusplus
}
#endif
#endif
