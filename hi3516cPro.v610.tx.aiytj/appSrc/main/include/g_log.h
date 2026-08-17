/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : g_log.h
 * @Created Time : 2023-03-24
 * @Version      : 1.0
 * @Author       : hul
 * @Description  : 动态打印日志
 */
#ifndef _G_LOG_H
#define _G_LOG_H
#ifdef __cplusplus
extern "C" {
#endif

#include "g_sys.h"

typedef enum {
    LVL_DBG   = 1 << 0,
    LVL_WAR   = 1 << 1,
    LVL_ERR   = 1 << 2,
    LVL_PANIC = 1 << 3,
    LVL_LOOP  = 1 << 4,
    LVL_PRINT = 1 << 5,
} eLogLvl;

/* switchs of system */

#define LOG_LVL_DBG(enable, fmt, args...)    if (enable) DBG(fmt, ##args)
#define LOG_LVL_WAR(enable, fmt, args...)    if (enable) WAR(fmt, ##args)
#define LOG_LVL_ERR(enable, fmt, args...)    if (enable) ERR(fmt, ##args)
#define LOG_LVL_PANIC(enable, fmt, args...)  if (enable) SYSLOG(fmt, ##args)
#define LOG_LVL_LOOP(enable, fmt, args...)   if (enable) DBG(fmt, ##args)
#define LOG_LVL_PRINT(enable, fmt, args...)  if (enable) printf(fmt, ##args)

#define pri_lvl(lvl, mod, fmt, args...)  LOG_##lvl(get_g_log(mod) & lvl, fmt, ##args)

#define pri_venc(lvl, fmt, args...)      pri_lvl(lvl, venc, fmt, ##args)
#define pri_audio(lvl, fmt, args...)     pri_lvl(lvl, audio, fmt, ##args)
#define pri_record(lvl, fmt, args...)    pri_lvl(lvl, record, fmt, ##args)
#define pri_jcp(lvl, fmt, args...)       pri_lvl(lvl, jcp, fmt, ##args)
#define pri_4g(lvl, fmt, args...)        pri_lvl(lvl, sim4g, fmt, ##args)
#define pri_wifi(lvl, fmt, args...)      pri_lvl(lvl, wifi, fmt, ##args)
#define pri_rtsp(lvl, fmt, args...)      pri_lvl(lvl, rtsp, fmt, ##args)
#define pri_search(lvl, fmt, args...)    pri_lvl(lvl, search, fmt, ##args)
#define pri_factory(lvl, fmt, args...)   pri_lvl(lvl, factory, fmt, ##args)
#define pri_upgrade(lvl, fmt, args...)   pri_lvl(lvl, upgrade, fmt, ##args)
#define pri_http(lvl, fmt, args...)      pri_lvl(lvl, http, fmt, ##args)
#define pri_onvif(lvl, fmt, args...)     pri_lvl(lvl, onvif, fmt, ##args)
#define pri_tencent(lvl, fmt, args...)   pri_lvl(lvl, tencent, fmt, ##args)
#define pri_lamp(lvl, fmt, args...)      pri_lvl(lvl, lamp, fmt, ##args)
#define pri_alarm(lvl, fmt, args...)     pri_lvl(lvl, alarm, fmt, ##args)
#define pri_md(lvl, fmt, args...)        pri_lvl(lvl, md, fmt, ##args)
#define pri_hd(lvl, fmt, args...)        pri_lvl(lvl, hd, fmt, ##args)
#define pri_mask(lvl, fmt, args...)      pri_lvl(lvl, mask, fmt, ##args)
#define pri_osd(lvl, fmt, args...)       pri_lvl(lvl, osd, fmt, ##args)
#define pri_ptz(lvl, fmt, args...)       pri_lvl(lvl, ptz, fmt, ##args)
#define pri_gb28181(lvl, fmt, args...)   pri_lvl(lvl, gb28181, fmt, ##args)
#define pri_netcheck(lvl, fmt, args...)  pri_lvl(lvl, netcheck, fmt, ##args)
#define pri_gpio(lvl, fmt, args...)      pri_lvl(lvl, gpio, fmt, ##args)
#define pri_isp(lvl, fmt, args...)       pri_lvl(lvl, isp, fmt, ##args)
#define pri_pwm(lvl, fmt, args...)       pri_lvl(lvl, pwm, fmt, ##args)
#define pri_face(lvl, fmt, args...)      pri_lvl(lvl, face, fmt, ##args)
#define pri_cry(lvl, fmt, args...)       pri_lvl(lvl, cry, fmt, ##args)
#define pri_vidcall(lvl, fmt, args...)   pri_lvl(lvl, vidcall, fmt, ##args)
#define pri_od(lvl, fmt, args...)        pri_lvl(lvl, od, fmt, ##args)
#define pri_vidmask(lvl, fmt, args...)   pri_lvl(lvl, vidmask, fmt, ##args)
#define pri_ivx(lvl, fmt, args...)       pri_lvl(lvl, ivx, fmt, ##args)
#define pri_mmi(lvl, fmt, args...)       pri_lvl(lvl, mmi, fmt, ##args)
#define pri_asr(lvl, fmt, args...)       pri_lvl(lvl, asr, fmt, ##args)

#define dbg_venc        if(get_g_log(venc)      & LVL_DBG) DBG
#define dbg_audio       if(get_g_log(audio)     & LVL_DBG) DBG
#define dbg_record      if(get_g_log(record)    & LVL_DBG) DBG
#define dbg_jcp         if(get_g_log(jcp)       & LVL_DBG) DBG
#define dbg_4g          if(get_g_log(sim4g )    & LVL_DBG) DBG
#define dbg_wifi        if(get_g_log(wifi)      & LVL_DBG) DBG
#define dbg_rtsp        if(get_g_log(rtsp)      & LVL_DBG) DBG
#define dbg_search      if(get_g_log(search)    & LVL_DBG) DBG
#define dbg_factory     if(get_g_log(factory)   & LVL_DBG) DBG
#define dbg_upgrade     if(get_g_log(upgrade)   & LVL_DBG) DBG
#define dbg_http        if(get_g_log(http)      & LVL_DBG) DBG
#define dbg_onvif       if(get_g_log(onvif )    & LVL_DBG) DBG
#define dbg_tencent     if(get_g_log(tencent)   & LVL_DBG) DBG
#define dbg_lamp        if(get_g_log(lamp)      & LVL_DBG) DBG
#define dbg_alarm       if(get_g_log(alarm)     & LVL_DBG) DBG
#define dbg_md          if(get_g_log(md)        & LVL_DBG) DBG
#define dbg_hd          if(get_g_log(hd)        & LVL_DBG) DBG
#define dbg_mask        if(get_g_log(mask)      & LVL_DBG) DBG
#define dbg_osd         if(get_g_log(osd)       & LVL_DBG) DBG
#define dbg_ptz         if(get_g_log(ptz)       & LVL_DBG) DBG
#define dbg_gb28181     if(get_g_log(gb28181)   & LVL_DBG) DBG
#define dbg_netcheck    if(get_g_log(netcheck)  & LVL_DBG) DBG
#define dbg_gpio        if(get_g_log(gpio)      & LVL_DBG) DBG
#define dbg_isp         if(get_g_log(isp)       & LVL_DBG) DBG
#define dbg_pwm         if(get_g_log(pwm)       & LVL_DBG) DBG
#define dbg_face        if(get_g_log(face)      & LVL_DBG) DBG
#define dbg_cry         if(get_g_log(cry)       & LVL_DBG) DBG
#define dbg_vidcall     if(get_g_log(vidcall)   & LVL_DBG) DBG
#define dbg_od          if(get_g_log(od)        & LVL_DBG) DBG
#define dbg_vidmask     if(get_g_log(vidmask)   & LVL_DBG) DBG
#define dbg_ivx         if(get_g_log(ivx)       & LVL_DBG) DBG
#define dbg_mmi         if(get_g_log(mmi)       & LVL_DBG) DBG
#define dbg_asr         if(get_g_log(asr)       & LVL_DBG) DBG

void init_g_log(void);

/* toggle 3 1001 */
void toggle_redirect(int tofile);


#ifdef __cplusplus
}
#endif
#endif
