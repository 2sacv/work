/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : jconfig.h
 * @Created Time : 2013-10-15
 * @Version      : 1.0
 * @Author       : zengy
 * @Description  :
 */

#ifndef __JCONFIG_H__
#define __JCONFIG_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "mxml.h"
#include "js_event.h"


typedef enum {
    JEvent_Begin           = -1,
    JEvent_EthcfgChg        = 0,        //网络参数改变
    JEvent_EthMacCfgChg        ,
    JEvent_SysInfoCfgChg       ,
    JEvent_DnsCfgChg           ,        //dns配置参数改变
    JEvent_PtzSerialCfgChg     ,        //PTZ串口配置参数改变
    JEvent_NfsCfgChg        = 5,        // nfs 配置参数改变
    JEvent_SmbCfgChg           ,        // SMB 配置参数改变
    JEvent_NtpcfgChg           ,        // ntp 配置参数改变
    JEvent_TimeZoneCfgChg      ,        // 时区参数改变
    JEvent_VideoCfgChg         ,        //视频参数改变
    JEvent_OsdCfgChg = 10      ,        //osd基础配置信息
    JEvent_OsdExpandCfgChg     ,        //多行OSD信息
    JEvent_VideoMaskCfgChg     ,        //视频遮挡
    JEvent_AudioInCfgChg       ,        //音频输入配置
    JEvent_DdnsCfgChg          ,        //ddns配置
    JEvent_HttpPortCfgChg  = 15,
    JEvent_FtpPortCfgChg       ,
    JEvent_RtspPortCfgChg      ,
    JEvent_SpeekPortCfgChg     ,
    JEvent_UpdatePortCfgChg    ,
    JEvent_3GCfgChg        = 20,
    JEvent_MotionDetectCfgChg  ,        //移动侦测
    JEvent_AlarmInCfgChg       ,        //报警输入
    JEvent_AlarmOutCfgChg      ,        //报警输出
    JEvent_VideoLossCfgChg     ,        //视频丢失
    JEvent_EmailCfgChg     = 25,        //email配置
    JEvent_FtpCfgChg           ,        //ftp配置
    JEvent_WifiCfgChg          ,
    JEvent_WifiMacCfgChg       ,
    JEvent_AlarmExpandCfgChg   ,        //拓展报警配置
    JEvent_VideoLossLinkCfgChg = 30,    //视频丢失联动配置
    JEvent_MotionDetLinkCfgChg ,        //移动侦测联动配置
    JEvent_AlarmInLinkCfgChg   ,        //报警输入联动配置
    JEvent_AlarmExpLinkCfgChg  ,        //拓展报警联动配置
    JEvent_AudioOutCfgChg      ,        //音频输出配置 
    JEvent_IrCtrlCfgChg    = 35,
    JEvent_UpnpCfgChg          ,        //upnp配置
    JEvent_OsdStyleCfgChg      ,        //字体配置
    JEvent_RecordCfgChg        ,        //录像参数配置
    JEvent_AlarmCenterCfgChg   ,        //报警中心配置
    JEvent_MsnCfgChg       = 40,
    JEvent_MultiDevCfgChg      ,
    JEvent_AuthenCfgChg        ,
    JEvent_AutoRebootCfgChg    ,        //自动重启配置
    JEvent_BootargCfgChg       ,        //bootarg配置
    JEvent_EquipCfgChg     = 45,
    JEvent_UserCfgChg          ,        //用户管理配置
    JEvent_WebShowCfgChg      ,
    JEvent_Video3aCfgChg      ,                //白平衡配置
    JEvent_VideoMarginCfgChg   ,
    JEvent_AlarmAi0CfgChg  = 50,
    JEvent_AlarmAi1CfgChg      ,
    JEvent_AlarmAi2CfgChg      ,
    JEvent_AlarmAi3CfgChg      ,
    JEvent_AlarmMdCfgChg       ,
    JEvent_AlarmVlChg      = 55,
    JEvent_CaptureCfgChg       ,
    JEvent_IpConflictCfgChg    ,
    JEvent_IpBrokenCfgChg      ,
    JEvent_Ddns9299CfgChg      ,
    JEvent_DdnsDynCfgChg   = 60,
    JEvent_ViinfoCfgChg        ,        //图像参数配置
    JEvent_RoiCfgChg           ,        //roi参数配置
    JEvent_BlackMarginCfgChg   ,        //黑边配置
    JEvent_ProfileCfgChg       ,        //profile配置
    JEvent_DnrCfgChg           ,        //降噪配置
    JEvent_AuthModecfgChg      ,        //登录验证方式配置
    JEvent_DiskAlarmCfgChg = 95,        //磁盘满和错误报警配置
    JEvent_DiskErrorLinkCfg    ,        //磁盘错误联动配置
    JEvent_DiskFullLinkCfg     ,        //磁盘满联动配置
    JEvent_VMaskAlarmCfg       ,        //视频遮挡报警参数配置
    JEvent_VMaskAlarmLinkCfg   ,        //视频遮挡报警联动参数配置
    JEvent_HngsCfg        = 100,        //沪宁高速 参数配置
    JEvent_TimeChange          ,        //时间改变通知。
    JEvent_UpdateBegin         ,        //开始传输升级文件
    JEvent_NREECfg             ,        //机芯发送NR EE参数索引到编码进程
    Jevent_StopAlarmLink       ,        //停止球机的报警联动消息
    Jevent_LSCCfg         = 105,        //镜头阴影校正功能
    JEvent_AudioTestCfgChg     ,        //音频测试
    //Jevent_TrackCfg          ,        //跟踪参数配置
    JEvent_DayNightTypeChg     ,        //日夜切换
    JEvent_WhiteLedCfg         ,        //白光灯参数
    JEvent_DayLowLightDropFps  ,        //低照降帧
    JEvent_LightCfgChg    = 110,        //灯类型配置, (35,40)已经排满
    JEvent_LightExtCfgChg      ,        //新版补光设置
    JEvent_VglineCfgChg        ,        //vgline
    JEvent_VglineLinkCfgChg    ,        //vgline联动配置
    JEvent_VgrectCfgChg        ,        //vgrect
    JEvent_VgrectLinkCfgChg = 115,      //vgrect联动配置
	JEvent_HumanDetectCfgChg   ,        //人形侦测
	JEvent_HumanDetLinkCfgChg  ,        //人形侦测联动配置
	JEvent_CarDetectCfgChg     ,        //车形侦测
	JEvent_CarDetLinkCfgChg    ,        //车形侦测联动配置
	JEvent_PetDetectCfgChg = 120,       //宠物侦测
	JEvent_PetDetLinkCfgChg    ,        //宠形侦测联动配置
	JEvent_CryDetectCfgChg     ,      //哭声检测
	JEvent_CryDetLinkCfgChg    ,        //哭声检测联动配置
	JEvent_AlarmAudioCfgChg    ,        //alarm_audio 
	JEvent_RunXxxx             ,        //Run 表示是运行时消息事件，不与config关联
	JEvent_RunIspColor         ,        //效果切换
    JEvent_RunLuma             ,
    JEvent_RunFreezMD          ,        //MdStop
    JEvent_RunFreezLampCtrl    ,        //定住几秒：灯光切换+软光敏
    JEvent_RunButtCtrl        ,
    JEvent_Auth_Success       ,
    JEvent_PetLinkCfgChg      ,
    JEvent_ALGO_Forzen        ,
	JEvent_Snap_Jpeg           ,        //抓图
    JEvent_LightingCfgChg      ,
    JEvent_AlarmRMRcord        ,        //录像删除
    JEvent_DevVideoReport      ,        //分辨率切换
    JEvent_AppveCfgChg         ,        //QP切换
    JEvent_AlarmInfoCfgChg     ,

    JEvent_AlarmMD        = 500,        //移动侦测报警
    JEvent_AlarmVgline         ,        //Vgline
    JEvent_AlarmVgrect         ,        //Vgrect
    JEvent_AlarmVL             ,        //视频丢失报警
    JEvent_AlarmDiskFull       ,        //磁盘满报警
    JEvent_AlarmDiskErr   = 505,        //磁盘错误报警
    JEvent_AlarmCabDis         ,        //网口断开报警
    JEvent_AlarmIpConflict     ,        //ip冲突报警
    JEvent_AlarmIllegalVisit   ,        //非法访问报警
    JEvent_AlarmAI0            ,
    JEvent_AlarmAI1       = 510,
    JEvent_AlarmAI2            ,
    JEvent_AlarmAI3            ,
    JEvent_AlarmExpand0        ,
    JEvent_AlarmExpand1        ,
    JEvent_AlarmExpand2   = 515,
    JEvent_AlarmExpand3        ,
    JEvent_AlarmExpand4        ,
    JEvent_AlarmExpand5        ,
    JEvent_AlarmExpand6        ,
    JEvent_AlarmExpand7   = 520,
    JEvent_AlarmExpand8        ,
    JEvent_AlarmExpand9        ,
    JEvent_AlarmExpand10       ,
    JEvent_AlarmExpand11       ,
    JEvent_AlarmExpand12  = 525,
    JEvent_AlarmExpand13       ,
    JEvent_AlarmExpand14       ,
    JEvent_AlarmExpand15       ,
    JEvent_AlarmVMask          ,        //视频遮挡告警
    JEvent_AlarmCableNormal = 530,      //网口连接
    JEvent_Alarmhumadetect     ,        //人形侦测
    JEvent_AlarmFacesnap       ,        //人脸抓拍
    JEvent_AlarmFace           ,        //人脸侦测
    JEvent_AlarmCar            ,        //车形侦测
    JEvent_AlarmPet            ,        //宠形侦测
    JEvent_SceneChange         ,        //画面变化
    JEvent_AlarmCry            ,        //哭声侦测

    JEvent_GuoBiaoCfg    = 1000,        //国标配置信息
    Jevent_GuoBiaoAddrCfg      ,        //国标位置参数
    Jevent_TsLiveCfg           ,        //Tslive配置参数
    Jevent_HxhtCfg             ,        //互信互通参数变更
    Jevent_TutkCfg             ,        //TUTK参数配置
    JEvent_HanbangCfg    = 1005,        //汉邦
    JEvent_JstarCfg            ,        //J-star
    Jevent_DanaleCfg           ,        //Danale
    JEvent_XMCfg               ,        //XM
    JEvent_DHCfg               ,        //DH
    JEvent_HKCfg         = 1010,        //HK
	JEvent_DhcpNotify          ,
	JEvent_StopAlarmMD         ,
    JEvent_HanbangServiceCfg   ,        //汉邦主动连接配置
    JEvent_StopAlarmMask       ,        //遮挡报警停止
    JEvent_HTTPERR_408   = 1015,
    JEvent_OutdoorIspAdjust    ,        //君正室外isp动态调整，区分室内室外
    JEvent_MotorCfg            ,
    JEvent_PresetCfg           ,
    JEvent_PelcodCfg     = 1020,
    JEvent_Presetctrl          ,
    JEvent_Daynightcfg         ,
    JEvent_Sim4g               ,
    JEvent_Followcfg           ,
    JEvent_DevCfg              ,        //devcfg
    JEvent_StreamNotify        ,
    JEvent_LedTest             ,        //灯板测试
    JEvent_TimeOsdCfgChg      ,                //时间 OSD 格式
    JEvent_AudioAlarmCfg       ,        //声音报警
    JEvent_LightAlarmCfg      ,                // 灯光报警
    JEvent_OnvifInfoCfg        ,
    JEvent_MetaOverRtsp        ,
    JEvent_RtpOverMulticast    ,
	JEvent_IOAlarmCfg          ,        //IO报警
    JEvent_DriveOutCfg         ,        //驱赶设置
    JEvent_PrivCtrl            ,        //设备禁用
    JEvent_Test                ,
    JEvent_TencentReset       ,
    JEvent_Tencent_Offline    ,
    JEvent_Quality_Change     ,
    JEvent_Sim4gLocation      ,
    JEvent_ConvergenceChg     ,         /**< 人脸收光配置改变 */
    JEvent_DevVideoCodec      ,         //编码方式切换
    JEvent_VideoCallCfg       ,
    JEvent_VideoMaskPlanCfg   ,
    JEvent_AiVqeV2CfgChg      ,
    JEvent_AiSpeexCfgChg      ,
    JEvent_End                ,
} JEventType;

    /*
     * same as JApiMsgCb
     **/
    typedef int (*cmdCbFunc)(void*,   //  conf set buffer
                             void*,   //  conf get buffer
                             int,     //  conf get buffer size
                             int*,    //  conf get real size
                             void*);  //  client data

    typedef struct {
        int         id_param;
        cmdCbFunc   pCb;
    } JconfCmdCbS;

    int init_server_config();
    int uninit_server_config();
    int init_client_config_sync(void *data);
    int uninit_client_config_sync();

    JSEventManager *get_ev_mng_conf(void);

    void send_conf_data(JSEventType jevent, void *ptr, int size);
    void send_conf_nake(JSEventType jevent);

    int set_config_event_debug(int enable);
    int get_value_from_cjson(char *cjsondate, char*keystr, void *keyvalue,unsigned int strvalue_len);
    const char *get_fw_ver();

#define attach_config(id, cb_func, cb_data) do { \
        if (0 != js_event_attach_async(get_ev_mng_conf(), id, cb_func, cb_data)) { \
            ERR("attach failed in %s\n", __func__); \
        } \
    } while(0)

#define detach_config(id, cb_func, cb_data) do { \
        if (0 != js_event_detach(get_ev_mng_conf(), id, cb_func, cb_data)) { \
            ERR("detach failed in %s\n", __func__); \
        } \
    } while(0)

#ifdef __cplusplus
}
#endif
#endif

