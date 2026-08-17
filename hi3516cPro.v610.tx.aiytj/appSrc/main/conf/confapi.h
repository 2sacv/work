/**
 * Copyright (C) by Jabsco Company
 * 
 * @File Name    : confapi.h
 * @Created Time : 2013-10-28
 * @Version      : 1.0
 * @Author       : zengy
 * @Description  : 
 */

#ifndef _CONFAPI_H_
#define _CONFAPI_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "jconfstruct.h"
#include "encode_audio_input.h"

int conf_get_gpiocfg(gpio_t *data);
int conf_set_gpiocfg(gpio_t data);
int conf_get_capability(SysCustomS *data);

int conf_get_updatecfg(UpdateS *update);
int conf_set_updatecfg(UpdateS update);
int conf_clr_updatecfg();

int conf_get_vglinecfg(VglineS *data);
int conf_set_vglinecfg(VglineS data);
int conf_get_vgrectcfg(VgrectS *data);
int conf_set_vgrectcfg(VgrectS data);

int conf_get_OnvifInfocfg(OnvifInfoCfg *data);
int conf_set_OnvifInfocfg(OnvifInfoCfg data);

int conf_get_update_type();
int conf_set_update_type(int type);
int conf_get_update_progressbar();
int conf_set_update_progressbar(int progressbar);

int conf_get_ethcfg(NetEthS *eth);

int conf_set_ethcfg(NetEthS eth);

int conf_get_bootargs(BOOTARGS_CFG_S *bootargs);

int conf_get_sysinfocfg(SysInfoS *info);

int conf_set_sysinfocfg(SysInfoS info);

int conf_get_ptzserialcfg(PtzSerialS *ptzs);

int conf_set_ptzserialcfg(PtzSerialS ptzs);

int conf_get_ntpcfg(SysNtpS *ntp);

int conf_set_ntpcfg(SysNtpS ntp);

int conf_get_timezonecfg(TzoneS *time);

int conf_set_timezonecfg(TzoneS time);

int conf_get_videocfg(VideoEncS *encode);

int conf_set_videocfg(VideoEncS encode);

int conf_get_realvideocfg(VideoEncS *encode);

int conf_get_osdexpandcfg(OsdExpandS *cus);

int conf_set_osdexpandcfg(OsdExpandS cus);

int conf_get_audiocfg(AudioCfgS *audio);

int conf_set_audiocfg(AudioCfgS audio);

int conf_get_audiotestcfg(AudioTestCfgS* data);

int conf_set_audiotestcfg(AudioTestCfgS data);

int conf_get_httpportcfg(int *port);

int conf_set_httpportcfg(int port);

int conf_get_ftpportcfg(int *port);

int conf_set_ftpportcfg(int port);

int conf_get_rtspportcfg(int *port);

int conf_set_rtspportcfg(int port);

int conf_get_speekportcfg(int *port);

int conf_set_speekportcfg(int port);

int conf_get_updateportcfg(int *port);

int conf_set_updateportcfg(int port);

int conf_get_emailcfg(EmailS *mail);

int conf_set_emailcfg(EmailS mail);

int conf_get_wificfg(NetWifiS *wifi);

int conf_set_wificfg(NetWifiS wifi);

int conf_get_motiondetectlinkcfg(MotionDetectLinkS *mdlink);

int conf_get_vglinelinkcfg(VglineLinkS* data);

int conf_set_vglinelinkcfg(VglineLinkS data);

int conf_get_vgrectlinkcfg(VgrectLinkS* data);

int conf_set_vgrectlinkcfg(VgrectLinkS data);

int conf_get_humandetectlinkcfg(HumanDetectLinkS* data);

int conf_set_humandetectlinkcfg(HumanDetectLinkS data);

int conf_get_cardetectlinkcfg(CarDetectLinkS* data);

int conf_set_cardetectlinkcfg(CarDetectLinkS data);

int conf_get_petdetectlinkcfg(PetDetectLinkS* datal);

int conf_set_petdetectlinkcfg(PetDetectLinkS data);

int conf_get_crydetectlinkcfg(CryDetectLinkS* data);

int conf_set_crydetectlinkcfg(CryDetectLinkS data);
int conf_set_motiondetectlinkcfg(MotionDetectLinkS mdlink);

int conf_get_irctrlcfg(IrCtrlS *ir);

int conf_set_irctrlcfg(IrCtrlS ir);

int conf_get_lightcfg(LightCfg *data);

int conf_set_lightcfg(LightCfg data);

int conf_get_upnpcfg(NetUpnpS *upnp);

int conf_set_upnpcfg(NetUpnpS upnp);

int conf_get_osdstylecfg(OsdStyleS *osdSty);

int conf_set_osdstylecfg(OsdStyleS osdSty);

int conf_get_recordcfg(RecordCtrlS *rctl);

int conf_set_recordcfg(RecordCtrlS rctl);

int conf_get_autorebootcfg(AutoRebootS *ar);

int conf_set_autorebootcfg(AutoRebootS ar);

int conf_get_bootargcfg(SysBootArgS *bootArg);

int conf_set_bootargcfg(SysBootArgS bootArg);

int conf_get_equipcfg(SysCustomS *sc);

int conf_set_equipcfg(SysCustomS sc);

int conf_get_usercfg(SysUserS *suser);

int conf_add_usercfg(char *user, char *passwd, int group);

int conf_set_user_passwd(char *user, char *passwd, int group); //group<0表示不修改group属???
int conf_del_usercfg(char *user);

int conf_get_auth_realm(AuthRealmS *ars);

int conf_get_webshowcfg(ShowWebS *web);

int conf_get_video3acfg(Video3aS *v3a);

int conf_set_video3acfg(Video3aS v3a);

int conf_stop_synccfg();        //when reboot or restart service ONLY

int conf_restore_default();

int conf_set_netportcfg(NetPortS netport);

int conf_get_netportcfg(NetPortS *netport);

int conf_get_ipconflictcfg(IpLinkS* link);

int conf_set_ipconflictcfg(IpLinkS link);

int conf_get_ipbrokencfg(IpLinkS* link);

int conf_set_ipbrokencfg(IpLinkS link);

int conf_get_osdinfocfg(OsdInfoS* info);

int conf_set_osdinfocfg(OsdInfoS info);

int conf_get_video_maxparam(VencMaxParamS *maxparam, VencSizeE size);

int conf_get_viinfocfg(ViInfoS* info);

int conf_set_viinfocfg(ViInfoS info);

int conf_user_basic_auth(char *user, char *passwd);  //明文passwd

int conf_user_digest_auth(char *user, char *ha2Hex, char *nonce, char *nc, 
            char *cNonce, char *qop, char *passwd); //MD5加密后passwd,只兼容MD5方式

int conf_set_roicfg(RoiAreaS area);

int conf_get_roicfg(RoiAreaS *area);

int conf_set_profilecfg(VeProfileS areas);

int conf_get_profilecfg(VeProfileS *ves);

int conf_get_denoisecfg(DnrCfgS *dnr);

int conf_set_denoisecfg(DnrCfgS dnr);

int conf_get_authmodecfg(AuthtypeE *mode);

int conf_set_authmodecfg(AuthtypeE mode);

int conf_get_motiondetectcfg(MotionDetectS *motion);

int conf_set_motiondetectcfg(MotionDetectS motion);

int conf_get_humandetectioncfg(HumanDetectionS* data);

int conf_set_humandetectioncfg(HumanDetectionS data);

int conf_get_cardetectioncfg(CarDetectionS* data);

int conf_set_cardetectioncfg(CarDetectionS data);

int conf_get_petdetectioncfg(PetDetectionS* data);

int conf_set_petdetectioncfg(PetDetectionS data);

int conf_get_crydetectioncfg(CryDetectionS* data);

int conf_set_crydetectioncfg(CryDetectionS data);

int conf_sysctrl_dev(int cmd);

int conf_get_guobiaocfg(GuoBiaoS *guobiao);

int conf_get_guobiaoaddr(GBAddrS *addr);

int conf_get_capability(SysCustomS *cus);

int conf_get_lenscsccfg(LensSCS *scs);

int conf_set_lenscsccfg(LensSCS scs);

int conf_get_whiteledcfg(WhiteLedS *data);

int conf_set_whiteledcfg(WhiteLedS data);

int conf_get_alarmaudiocfg(alarm_audio_t *data);

int conf_set_alarmaudiocfg(alarm_audio_t data);

int conf_set_pelcodcfg(PelcodCfg *data);

int conf_get_pelcodcfg(PelcodCfg *data);

int conf_get_presetnew_cfg(presetcfg *data);

int conf_set_presetnew_cfg(presetcfg data);

int conf_get_daynightcfg(DaynightCfgS *data);

int conf_set_daynightcfg(DaynightCfgS data);

int conf_get_motorcfg(motor_t *data);

int conf_set_motorcfg(motor_t data);

int conf_get_sim4g_cfg(Sim4gCfgS *data);

int conf_set_sim4g_cfg(Sim4gCfgS data);

int conf_get_follow_cfg(follow_info_t *data);

int conf_set_follow_cfg(follow_info_t data);

int conf_get_lightext_cfg(LightExtCfg *data);

int conf_set_lightext_cfg(LightExtCfg data);

int conf_get_audioalarm_cfg(AudioAlarmS *data);

int conf_set_audioalarm_cfg(AudioAlarmS data);

int conf_get_lightalarm_cfg(LightAlarmS *data);

int conf_set_lightalarm_cfg(LightAlarmS data);

int conf_get_IOalarm_cfg(IOAlarmS *data);

int conf_set_IOalarm_cfg(IOAlarmS data);

int conf_get_vmaskalarmcfg(VMaskAlarmS *data);

int conf_set_vmaskalarmcfg(VMaskAlarmS data);

int conf_get_vmaskalarmlinkcfg(VMaskAlarmLinkS *data);

int conf_set_vmaskalarmlinkcfg(VMaskAlarmLinkS data);

int conf_get_driveout_cfg(DriveOut *data);

int conf_set_driveout_cfg(DriveOut data);

int conf_get_videomaskcfg(VideoMaskS *data);

int conf_set_videomaskcfg(VideoMaskS data);

int conf_get_videomaskplan_cfg(videomask_plan_t *data);

int conf_set_videomaskplan_cfg(videomask_plan_t data);

int conf_get_appve_cfg(Appvecfg *data);

int conf_set_appve_cfg(Appvecfg data);

int conf_get_alarminfo_cfg(AlarmInfocfg *data);

int conf_set_alarminfo_cfg(AlarmInfocfg data);

int conf_get_devconf_cfg(DevConfS *data);

int conf_set_devconf_cfg(DevConfS data);

int conf_get_videocall_cfg(sVideoCallCfg *data);
int conf_set_videocall_cfg(sVideoCallCfg data);

int conf_get_aivqev2_cfg(sAiVqeV2Cfg *data);
int conf_set_aivqev2_cfg(sAiVqeV2Cfg data);

int conf_get_aispeex_cfg(sAiSpeexCfg *data);
int conf_set_aispeex_cfg(sAiSpeexCfg data);

#ifdef __cplusplus
}
#endif
#endif

