/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : jconfstruct.h
 * @Created Time : 2013-10-28
 * @Version      : 1.0
 * @Author       : zengy
 * @Description  :
 */

#ifndef _JCONFSTRUCT_H_
#define _JCONFSTRUCT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "alarm_event.h"
#include "js_scheduler.h"
#include "resolution.h"

#define AIN_MAX_CHN          4
#define AEXPAND_MAX_CHN      16
#define MULTI_DEV_MAX_CHN    9
#define USER_MAX_NUM         8
#define AOUT_MAX_CHN         2
#define OSD_EXPAND_MAX_CHN   10
#define ENCODE_MAX_CHN       3
#define VIDEO_MASK_MAX_CHN   4
#define VIDEO_MAX_FPS        30
#define MAX_JCPBUF_SIZE      1024
#define MAX_ROI_AREA         8
#define MAX_PROFILE_NUM      10
#define MAX_DDNS_NUM         3

#define VIDEO_VGLINE_MAX_CHN   1
#define VIDEO_VGRECT_MAX_CHN   1


#define DomeParameter_len      128
#define DomeParameter_Dwordlen 16         //*4
#define DomeParameter_wordlen   4         //*2
#define DomeParameter_Bytelen   56        //*1

#define CameraParameter_len      128
#define CameraParameter_Dwordlen 16       //*4
#define CameraParameter_Wordlen  4        //*2
#define CameraParameter_Bytelen  56       //*1
#define MAX_PRESET_NUM       8

#define LANGUAGE_ENGLISH     2            //语言环境为English=2,中文是0|1

#define PRODUCT_KEY_MAXLEN          (20 + 1)
#define DEVICE_NAME_MAXLEN          (32 + 1)
#define DEVICE_SECRET_MAXLEN        (64 + 1)
#define PRODUCT_SECRET_MAXLEN       (64 + 1)

    //PK校验
#ifndef __PK__
#define __PK__ "not_define_pk"
#endif
#define XSTR(s) STR(s)
#define STR(s) #s
#define PK XSTR(__PK__)
#define P_TYPE_STR  XSTR(P_TYPE)

#define TENCENT_CONF_PATH "/opt/conf/tencent.conf"

    typedef struct {
        char product_key[PRODUCT_KEY_MAXLEN];          //产品key
        char product_secret[PRODUCT_SECRET_MAXLEN];    //产品秘钥
        char device_name[DEVICE_NAME_MAXLEN];          //设备名称
        char device_secret[DEVICE_SECRET_MAXLEN];      //设备秘钥
    } TripleInfoS;

    typedef enum {
        WIDTH_QCIF = 176,
        WIDTH_180P = 320,
        WIDTH_QVGA = 320,
        WIDTH_CIF  = 352,
        WIDTH_360P = 640,
        WIDTH_VGA  = 640,
        WIDTH_D1   = 720,
        WIDTH_720P = 1280,
        WIDTH_960P = 1280,
        WIDTH_UVGA = 1600,
        WIDTH_1080P= 1920,
        WIDTH_2M_3M= 2304,   // 16:9
        WIDTH_3M   = 2048,   // 4:3
        WIDTH_4M   = 2560,
        WIDTH_5M   = 2880,
        WIDTH_6M   = 3264,
        WIDTH_8M   = 3840,
    } WidthE;

    typedef enum {
        HEIGHT_QCIF = 144,
        HEIGHT_180P = 180,
        HEIGHT_QVGA = 240,
        HEIGHT_CIF  = 288,
        HEIGHT_360P = 360,
        HEIGHT_VGA  = 480,
        HEIGHT_D1   = 576,
        HEIGHT_720P = 720,
        HEIGHT_960P = 960,
        HEIGHT_UVGA = 1200,
        HEIGHT_1080P = 1080,
        HEIGHT_2M_3M= 1296, // 16:9
        HEIGHT_3M   = 1536, // 4:3
        HEIGHT_4M   = 1440,
        HEIGHT_5M   = 1620,
        HEIGHT_6M   = 1856,
        HEIGHT_8M   = 2160,
    } HeightE;

    typedef enum {
        MAXBPS_QCIF = 1024,
        MAXBPS_180P = 1024,
        MAXBPS_QVGA = 1024,
        MAXBPS_CIF  = 2048,
        MAXBPS_360P = 2048,
        MAXBPS_VGA  = 2048,
        MAXBPS_D1   = 4096,
        MAXBPS_720P = 8192,
        MAXBPS_960P = 8192,
        MAXBPS_UVGA = 8192,
        MAXBPS_1080P= 8192,
        MAXBPS_2M_3M= 8192,
        MAXBPS_3M   = 8192,
        MAXBPS_4M   = 8192,
        MAXBPS_5M   = 8192,
        MAXBPS_6M   = 8192,
        MAXBPS_8M   = 9216,
    } MaxBpsE;

    typedef enum {
        VENC_FORMAT_BEGIN = -1,
        VENC_FORMAT_H261  = 0,  // H261
        VENC_FORMAT_H263  = 1,  // H263
        VENC_FORMAT_H264  = 2,  // H264
        VENC_FORMAT_MPEG2 = 3,  // MPEG2
        VENC_FORMAT_MPEG4 = 4,  // MPEG4
        VENC_FORMAT_MJPEG = 5,  // MOTION_JPEG
        VENC_FORMAT_SVC   = 6,  // SVC
        VENC_FORMAT_H265  = 7,
        VENC_FORMAT_END
    } VENC_FORMAT_E;

    typedef enum {
        E_IRCUT_NORMAL = 0,                 // IRCUT 方向正常
        E_IRCUT_REVERSE,                    // IRCUT 方向反向
        E_IRCUT_DISABLE,                    // IRCUT 功能禁止
    } EIRCUTMode;

    typedef enum {
        AudioFormatE_BEGIN  = -1,
        AudioFormatE_PCM    = 0 ,   // Raw PCM
        AudioFormatE_G711A,         // G.711 A
        AudioFormatE_G711U,     // G.711 Mu
        AudioFormatE_ADPCM,     // ADPCM
        AudioFormatE_G726,      // G.726
        AudioFormatE_AMRNB,     // AMR encoder format narrow band
        AudioFormatE_AMRWB,     // AMR encoder formant wide band
        AudioFormatE_AAC,       // AAC encoder
        AudioFormatE_END
    } AudioFormatE;

    typedef enum {
        AmrBps_BEGIN    = -1,
        AmrBps_4_75_K   = 0 ,   //4.75K
        AmrBps_5_15_K,          //5.15K
        AmrBps_5_90_K,          //5.9K
        AmrBps_6_70_K,          //6.7K
        AmrBps_7_40_K,          //7.4K
        AmrBps_7_95_K,          //7.95K
        AmrBps_10_2_K,          //10.2K
        AmrBps_12_2_K,          //12.2K
        AmrBps_END
    } AudAmrBpsE;

    typedef enum {
        WifiModeE_BEGIN     = -1,
        WifiModeE_AP        = 0, // ap
        WifiModeE_AP_STATION= 1, //ap+station
        WifiModeE_END
    } WifiModeE;

    typedef enum {
        WifiApStrategyE_BEGIN   = -1,
        WifiApStrategyE_AUTO    = 0,
        WifiApStrategyE_MAC,
        WifiApStrategyE_CHN,
        WifiApStrategyE_END,
    } WifiApStrategyE;

    typedef enum {
        WifiAuthTypeE_BEGAIN    = -1,
        WifiAuthTypeE_OPEN      = 0,
        WifiAuthTypeE_SHARE,
        WifiAuthTypeE_WPA_PSK,
        WifiAuthTypeE_WPA,
        WifiAuthTypeE_WPA2_PSK,
        WifiAuthTypeE_WPA2,
        WifiAuthTypeE_END,
    } WifiAuthTypeE;

    typedef enum {
        WifiSecTypeE_BEGAIN = -1,
        WifiSecTypeE_NONE   = 0,
        WifiSecTypeE_WEP,
        WifiSecTypeE_TKIP,
        WifiSecTypeE_AES,
        WifiSecTypeE_END,
    } WifiSecTypeE;

    typedef enum {
        WifiWepkeyTypeE_BEGAIN  = -1,
        WifiWepkeyTypeE_5CHAR   = 0,
        WifiWepkeyTypeE_13CHAR,
        WifiWepkeyTypeE_40BITS,
        WifiWepkeyTypeE_104BITS,
        WifiWepkeyTypeE_END,
    } WifiWepkeyTypeE;

    typedef enum {
        HardStatusE_Begain = 0,
        HardStatusE_RwError,
        HardStatusE_SignalError,
        HardStatusE_NetError,
        HardStatusE_PppError,
        HardStatusE_OK,
        HardStatusE_End,
    } HardStatusE;

    typedef enum {
        SIMStatusE_Begain = 0,
        SIMStatusE_ON,
        SIMStatusE_OFF,
        SIMStatusE_End,
    } SIMStatusE;

    typedef enum {
        PppStatusE_Begain = 0,
        PppStatusE_OFF,
        PppStatusE_Error,
        PppStatusE_OK,
        PppStatusE_End,
    } PppStatusE;

    typedef enum {
        RECORD_STATUS_BEGIN = -1,
        RECORD_STATUS_STOP  = 0 ,   // 停止录像
        RECORD_STATUS_MANUAL,       // 正在手动录像
        RECORD_STATUS_ALARM,        // 正在报警录像
        RECORD_STATUS_MD,           // 正在移动录像
        RECORD_STATUS_SCHEDULE,     // 正在计划任务录像
        RECORD_STATUS_VOICE,        // 声音录像
        RECORD_STATUS_JPEG_SCHEDULE,// 正在定时抓拍
        RECORD_STATUS_JPEG_MANUAL,  // 正在手动抓图
        RECORD_STATUS_JPEG_ALARM,   // 正在报警输入报警抓图
        RECORD_STATUS_JPEG_MD,      // 正在移动侦测报警抓图
        RECORD_STATUS_EXIT,         // 正在退出录像，即停止当前任务并不再开始新的录像任务
        RECORD_STATUS_END
    } RecStatusE;

    typedef enum {
        RECORD_DISK_STRATEGY_BEGIN = -1,
        RECORD_DISK_STRATEGY_STOP  = 0 ,// 当磁盘满时停止录像
        RECORD_DISK_STRATEGY_DELETE,    // 当磁盘满时删除早期文件
        RECORD_DISK_STRATEGY_END
    } recDiskStrategyE;

    typedef struct {
        int            id;
        int            enable;
    } AlarmInChnS;

    typedef struct {
        int           id;
        int           enable;
        VENC_FORMAT_E codec;
        VencSizeE     vencsize;
        int           standard; //0,P制; 1, N制。（现都高清，只支持0）
        int           fps;
        int           bps;
        int           gop;
        int           fixfps;   // 质量优先，0  码率优先
        int           fixbps;   // 定码流，0 变码流
    } VideoEnc0;


    typedef struct {
        int    id;
        int    enable;
        int    x;
        int    y;
        char   content[128];
    } OsdExpand0;

    typedef struct {
        int    id;
        int    enable;
        int    color;
        int    x0;
        int    y0;
        int    x1;
        int    y1;
    } VideoMask0;

    typedef struct {
        int    id;
        int    group;
        char   username[32];
        char   cryptpasswd[36];
        char   digestpasswd[36];
        char   onvifpasswd[48];
    } SysUser0;

    enum UPDATE_TYPE {
        UPDATE_UNINIT,      // 0 initial, update __stopped__.
        UPDATE_LINUX,       // 1 firmware
        UPDATE_MCU,         // 2 mcu
        UPDATE_CUSTOM,      // 3 custom
    };

    typedef struct {
        int type;
        int progressbar;    // 0 initial 100 finish 101 error
    } UpdateS;

    typedef struct {
        char nic[32];
        char ip[32];
        char mask[32];
        char gw[32];
        int  mtu;
        int  dhcpen;
        int  reticle;       //非标准长网线
        int  ipadaen;       // IP自适应开关
        char nicEnd[0];
        char dns[32];
        char mac[64];
        int  ipcheck;
        int  enable;
        char dhcpname[32];
    } NetEthS;

    typedef struct {
        char platform[128];     // test about 10 platforms
        char solution[32];
        char devname[64];
        char kernelver[64];
        char serverver[64];
        char webver[32];
        char devtype[64];
        char devid[32];
        char plugver[32];
        int  devtype_select;
        char custom_ui[32];
        char custom_appid[8];
        int  ver_lite;
        char verifystr[128];
        char air_burn[8];
    } SysInfoS;
    typedef struct {
        char devclass[64];      //设备分类（IPC/NVR/SHAKE/CARD/FISHEYE/APIPC）
        char cpu[64];           //cpu型号
        char flash[64];         //Flash大小，单位M
        char version[64];       //完整版本号
        char compiledate[64];   //编译日期，比如20190724
        char customer[64];      //定制厂家，通用的为GEN
        char Sensor[32];        //sensor类型
        char DevType[32];
        int  upgrade_time;
    } OtaInfoS;
    typedef struct {
        char url[256];          //升级包URL
    } OtaUpgradeUrlS;

    typedef struct {
        int  baud;
        int  databits;
        int  stop;
        int  addr;
        int  hreverse;
        int  vreverse;
        char  protocol[32];
        char parity[32];
        char protocollist[128];
    } PtzSerialS;

    typedef struct {
        int   enable;
        char  ntpserver[64];
        int   ntpport;
        int   interval;
    } SysNtpS;

    typedef struct {
        int idx;
        int sec_east; //偏移量
        char str[16]; //时区string
    } TzoneS;

    typedef struct {
        int           gnum;
        VideoEnc0     enc[ENCODE_MAX_CHN];
    } VideoEncS;

    typedef struct {
        int   en;
        int   left;
        int   top;
        char  text[64];
    } CusOsdEleS;

    typedef struct {
        int        size;
        int        font;   //同 OsdStyleS
        OsdExpand0 cusosd[OSD_EXPAND_MAX_CHN];
    } OsdExpandS;

    typedef struct {
        int          enable;
        int          gnum;
        VideoMask0   mask[VIDEO_MASK_MAX_CHN];
    } VideoMaskS;

    typedef struct {
        char        DebugStr[256];
        char        ZoomStr[128];
        char        PresetStr[128];
        int         DebugEnable;
        int         ZoomEnable;
        int         PresetEnable;
        int         DebugChannel;
        int         ZoomChannel;
        int         PresetChannel;
        int         Debugupdate;
        int         Zoomupdate;
        int         Presetupdate;
        int         style_update;
        int         style_size;
        int         enable;
        int         Maxlen;
        int         delaytime;
        int         x;
        int         y;
        char        *osdstr;
    } OsdExpandS_Dome;

    typedef struct {
        int             inenable;             //音频开关0: 关， 1:开
        int             inputtype;            //音频输入方式 ， 0:Mic输入，1:Line-In输入
        int             involume;             //输入音量0~100
        int             ingain;               //输入音量0~31
        int             outvolume;            //输出音量0~100
        int             outgain;              //输入音量0~31
        int             definvolume;          //默认输入音量0~100
        int             defoutvolume;         //默认输出音量0~100
        int             talkvolume;           //对讲输出音量0~100
        float           talkamp;
        float           outamp;
        float           inamp;
        AudioFormatE    codetype;             //音频编码类型，目前只支持G711A和G711U
        AudAmrBpsE      amrbps;               //音频采样率。
    } AudioCfgS;

    typedef struct {
        int             card;                 //
        int             fdd;                  //动态打开锁定FDD
        char            token[32];            //token
    }Sim4gCfgS;

    typedef struct {
        int             start;                //音频测试开关0: 关， 1:开
        int             inputtype;            //音频输入方式 ， 0:Mic输入，1:Line-In输入
        int             involume;             //输入音量0~100
        int             outvolume;            //输出音量0~100
    } AudioTestCfgS;

    typedef enum {
        SoundSelectE_BEGAIN = -1,
        SoundSelectE_DEFAULT,
        SoundSelectE_DOG,
        SoundSelectE_OTHER,
        SoundSelectE_CUSTOM,
        SoundSelectE_END,
    } SoundSelectE;

    typedef struct {
        int            enable;
        int            thresh;
        int            level;
        char           mbdesc[512];  //18 number of 0000000000000111111110,
        unsigned int   times[7]; //times[0] = 0, times[1]=21;
    } MotionDetectS;

    typedef struct {
        int            enable;
        int            screenenable;// screen show human ability
        int            thresh;
        int            humandistance;//target distance
        int            drag;
        char           mbdesc[512];  //18 number of 0000000000000111111110,
        unsigned int   times[7]; //times[0] = 0, times[1]=21;
        int            mode;
        int            level;
        int            interval;
        int            pictstartFlag;
        int            pictwidth;
        int            pictheight;
        int            faceae;        // 人脸收光功能
        int            person_center; // 人形居中功能
    } HumanDetectionS;

     typedef struct {
        int            enable;
        int            show;
        int            screenenable;// screen show human ability
        int            thresh;
        int            cardistance;//target distance
        int            drag;
        char           mbdesc[512];  //18 number of 0000000000000111111110,
        unsigned int   times[7]; //times[0] = 0, times[1]=21;
        int            mode;
        int            level;
        int            interval;
        int            pictstartFlag;
        int            pictwidth;
        int            pictheight;
     } CarDetectionS;            //车辆侦测告警

     typedef struct {
        int            enable;
		int            screenenable;// screen show human ability
        int            thresh;
		int            petdistance;//target distance
		int            drag;
        char           mbdesc[512];  //18 number of 0000000000000111111110,
        unsigned int   times[7]; //times[0] = 0, times[1]=21;
		int            mode;
        int            level;
        int            interval;
     } PetDetectionS;            //宠物侦测

     typedef struct {
        unsigned int   times[7];    //times[0] = 0, times[1]=21;
        int            enable;
        int            thresh;
        int            level;
     } CryDetectionS;            //哭声检测

     typedef struct {
         int            enable;
         int            sdcardenable;
         int            screenenable;
         int            thresh;
         int            facedistance;
         int            quality;
         unsigned int   times[7]; //times[0] = 0, times[1]=21;
     } CarSnap;                  //车辆识别抓拍

     typedef struct {
        int            enable;
        char           serverip[32];
        int            serverport;
        int            interval;
        int            pictstartFlag;
        int            pictwidth;
        int            pictheight;
        int            thresh;
        int            level;
        char           mbdesc[512];  //18 number of 0000000000000111111110,
        unsigned int   times[7]; //times[0] = 0, times[1]=21;
    } HumanDetectionNVR;

    typedef struct {
        int    id;
        int    enable;
        int    x0;
        int    y0;
        int    x1;
        int    y1;
    } VideoMotion0;

    typedef struct {
        int    interval;
        int    alarmcenter;
        int    email;
        int    alarmout1;
        int    alarmout2;
        int    sound;
        int    soundsel;
        int    record;
        int    ftpup;
        int    snapshot;
        int    preset;  // 0 disable, otherwise preset number
    } MotionDetectLinkS;

    typedef struct {
        int    interval;
        int    alarmcenter;
        int    email;
        int    alarmout1;
        int    alarmout2;
        int    sound;
        int    soundsel;
        int    record;
        int    ftpup;
        int    snapshot;
        int    preset;  // 0 disable, otherwise preset number
    } HumanDetectLinkS;

    typedef struct {
        int    interval;
        int    alarmcenter;
        int    email;
        int    alarmout1;
        int    alarmout2;
        int    sound;
        int    soundsel;
        int    record;
        int    ftpup;
        int    snapshot;
        int    preset;  // 0 disable, otherwise preset number
    } CarDetectLinkS;

	typedef struct {
        int    interval;
        int    alarmcenter;
        int    email;
        int    alarmout1;
        int    alarmout2;
        int    sound;
        int    record;
        int    ftpup;
        int    snapshot;
        int    preset;  // 0 disable, otherwise preset number
    } PetDetectLinkS;

	typedef struct {
        int    interval;
        int    alarmcenter;
        int    email;
        int    alarmout1;
        int    alarmout2;
        int    sound;
        int    record;
        int    ftpup;
        int    snapshot;
        int    preset;  // 0 disable, otherwise preset number
    } CryDetectLinkS;

    typedef struct {
        int    interval;
        int    alarmcenter;
        int    email;
        int    alarmout1;
        int    alarmout2;
        int    sound;
        int    soundsel;
        int    record;
        int    ftpup;
        int    snapshot;
        int    preset;  // 0 disable, otherwise preset number
    } VglineLinkS;

    typedef struct {
        int    interval;
        int    alarmcenter;
        int    email;
        int    alarmout1;
        int    alarmout2;
        int    sound;
        int    soundsel;
        int    record;
        int    ftpup;
        int    snapshot;
        int    preset;  // 0 disable, otherwise preset number
    } VgrectLinkS;

    typedef struct {
        char   smtpserver[64];
        char   sendto[64];
        char   user[64];
        char   password[64];
    } EmailS;

    typedef struct {
        int               dhcp;
        char              ip[32];
        char              mask[32];
        char              gw[32];
        char              nicEnd[0];
        int               mode;
        char              ssid[64];
        char              weppasswd[64];
        char              token[32];
        char              nic[32];   //read only
        char              mac[64];
        int               quality;
        char              wifiscan[4096];
        int               status;
    } NetWifiS;

    typedef struct{
        char result[16];
        char interface[16];
        char ip[32];
    } DhcpNotifyS;

    typedef struct {
        int         beginhour;  //彩转黑时间开始 时
        int         beginmin;   //彩转黑时间开始 分
        int         endhour;    //彩转黑时间结束 时
        int         endmin;     //彩转黑时间结束 分
        int         type;       //readOnly 0:无红外 1:红外枪 2:红外球
        int         turnonlux;
        int         webturnonlux;
        int         lightmode;  //0:自动 1:近 2:中 3:远 灯
        int         switchmode; //1:硬光敏模式      2:软光敏模式 3:定时模式
        int         irtesten;
        int         irtestresult;
        EIRCUTMode  eIrcutMode;
        int         shinemode;
        int         shinetime;
        int         autolighten;
        int         lightgrade;
        int         whitectrl;
        int         fcolorbeginhour;  //全彩模式时间开始 时
        int         fcolorbeginmin;   //全彩模式时间开始 分
        int         fcolorendhour;    //全彩模式时间结束 时
        int         fcolorendmin;     //全彩模式时间结束 分
    } IrCtrlS;

    typedef struct {
        int     mode;//灯光模式。 0、红外；  1、白光；  2、双光源
        int     openlightlux;// 开灯阀值。
        int     closelightlux;//关灯阀值。
    }LightCfg;

    typedef struct {
        int     show;
        int     enable;
        int     whiteen;
        int     audioen;
        int     time;
        int     rest_time;
    } DriveOut;

    typedef struct {
        int    rtsp;
        int    http;
        int    ftp;
        int    voice;
        int    update;
    } NetUpnpS;

    typedef struct {
        char act[16];
        char path[128];
    } SdcardS;

    typedef struct {
        int colormode;  // 0:自动 1:白色2:黑色
        int font;       // 0 宋体
        int width;      // 8 ~ 96 (step以8对齐)
        int height;     // 8 ~ 96 (step以8对齐)
    } OsdStyleS;

    typedef struct {
        int             alarmseconds;
        int             schedminutes;
        int             diskreservemb;
        int             diskstrategy;
        int             filestrategy;
        int             rec_type;
        int             prerecord;
        int             prerecordtime;
        int             isrecording;
        int             sd_times;
        int             sd_stat;
        unsigned int    timestrategy[7];
    } RecordCtrlS;

    typedef struct {
        int       enable;
        int       alarmday;
        int       alarmhour;
    } AutoRebootS;

    typedef struct {
        int      lcdtype;
        int      lcdlogo;
        int      sensor;
    } SysBootArgS;


    typedef enum {
        GRAIN_NORMAL    = 0,          //grain 系标配
        GRAIN_ALL_FUN = 1,         //  grain 全功能
    } GRAIN_TYPE_E;

    typedef struct {
        int      follow;
        int      wiper;
        int      ircut;
        int      irlight;
        int      ircolor;
        int      alarmhost;
        int      gps;
        int      ptz;
        int      alarmin;
        int      alarmout;
        GRAIN_TYPE_E graintype;
        int webdeflang;
        int pixels;
        int sdinfo;
        int osd;
    } SysCustomS;

    typedef struct {
        int      gnum;
        SysUser0 user[USER_MAX_NUM];
    } SysUserS;

    typedef struct {
        char    realm[32];
    } AuthRealmS;

    typedef struct {
        int    ae;
        int    awb;
        int    blc;
        int    redgain;
        int    bluegain;
        int    lowlightEnhance;
        int    nightfacemode;//夜视人脸模式
    } Video3aS;

    typedef struct {
        int httpport;
        int rtspport;
        int ftpport;
        int audioport;
        int updateport;
    } NetPortS;

    typedef struct {
        int          interv;                  //定时抓拍间隔
        VencSizeE    vesize;
        int          alarminterv;             //报警抓拍间隔
        int          alarmnum;                //报警抓拍张数
        unsigned int timestrategy[7];
    } CaptureS;

    typedef struct {
        int          level;                 //打印级别
    } TLogLevel;

    typedef struct {
        int    interval;
        int    ao0en;
        int    ao1en;
        int    recorden;
        int    sounden;
        int    captureen;
    } IpLinkS;

    typedef struct {
        int  timeen;
        int  timeleft;
        int  timetop;
        int  bpsen;
        int  bpsleft;
        int  bpstop;
        int  nameen;
        int  nameleft;
        int  nametop;
        char name[128];
        int  gpsen;
        int  gpsleft;
        int  gpstop;
        int  osdcolor;
        int  osdlanguage;
        int  osdweek;
        int  hdtop;
        int  hdleft;
        int  dateformat;
    } OsdInfoS;

    typedef struct {
        int seq;
        int connector;
    } TimeOSD;

    typedef struct {
        int       width;
        int       height;
        int       maxbps;
        int       maxfps;
        VencSizeE size;
    } VencMaxParamS;

    typedef struct {
        int nightluma;
        int bright;
        int contrast;
        int hue;
        int saturation;
        int sharpness;
        int lampfrequency;
        int reverse;
        int gain;
        int stren;      // 对比度增强开关，NXP夜视有效，升迈暂时停用
        int brightlevel;
        int suppress;   // 强光抑制
    } ViInfoS;

    typedef struct {
        int id;
        int qp;
        int interval;
        int enable;
        int left;
        int top;
        int right;
        int bottom;
    } RoiArea0;

    typedef struct {
        int         factest;
        int         code;
        int         rtcstat;
        int         ecodestat;
        int         sdexist;
        int         rstkey;
        int         callstat;
        int         wifiexist;
        int         bleexist;
        int         wificonnected;
        int         wifiquality;
        int         wifipass;
        int         doubleboard;
        int         sim4g;
        int         stepdebug;
        char        eth0mac[64];
        char        wlan0mac[64];
    } DevTestS;

    typedef struct {
        int HD_Init;
        int VP_Init;
        int FA_Init;
        int IAAC_Init;
    }IVS_RUN;

    typedef struct {
        int gnum;
        RoiArea0 area[MAX_ROI_AREA];
    } RoiAreaS;

    typedef enum photometric_mode {
        E_PHOTOMTC_DISABLE = 0,
        E_PHOTOMTC_ENABLE,
        E_PHOTOMTC_DEFINE,
    } EPhotoMtcMode;

    typedef struct {
        EPhotoMtcMode eLSCMode;             //校正模式 0=禁止 1=使能  2=校正模式
        int ratio;                          //校正的强度
    } LensSCS;

    typedef enum dpc_mode {
        E_DPC_DISABLE = 0,
        E_DPC_ENABLE,
        E_DPC_DEFINE
    } EDpcMode;

    typedef struct {
        EDpcMode eDpcMode;                  //校正模式 0=禁止 1=使能  2=校正模式
    } DPCS;

    typedef struct {
        int profile;
        VencSizeE vesize;
        int level;
        int bIDREnable;
    } ProfileS;

    typedef struct {
        ProfileS ps[MAX_PROFILE_NUM];
    } VeProfileS;

    typedef struct media_session_info_s {
        char ip[32];
        char stream_type[16];
    } media_session_info_t;

    typedef struct {
        int enable;
        int mode;
    } DnrCfgS;

    typedef enum Auth_type_S {
        NONE_AUTH = 0,
        BASIC_AUTH,
        DIGEST_AUTH
    } AuthtypeE;

    typedef struct {
        int count;
        char zone[16];
        int offset;
    } tzoneS;

    typedef struct {
        int logen;      // 不要动位置,会被高级权限的库如 libs/encryt 引用
        int code;       // 不要动位置,会被高级权限的库如 libs/encryt 引用
        int stat;       // 不要动位置,会被高级权限的库如 libs/encryt 引用 SYS_STAT
        int run;        // 供双参数调试使用
        int jcpinfo;
        int timerinfo;
        int mcuinfo;
        char tracer[64];
        int search;     // 搜索
        int wifi;       // wifi
        int alilog;  //阿里日志等级
        int irdbg;
        int bv_day;
        int bv_ir;
        int cp_near;
    } ToggleS;

    typedef struct {
        char device_id[32];
    } UbootEnvS;

    typedef struct {
        char sensor[32];
        char flash[16];
        int  maxheight;
        int  lensircut;
        char cpu[16];
        int  feature;
        int  lang;
        int  hdetect;
    } BOOTARGS_CFG_S;

    typedef struct {
        char manufacturer[64];
        char owner[64];
        char civilcode[64];
        char sip_srv_ip[16];
        int  port;
        char srv_id[32];
        char dev_sysname[64];
        char dev_type[32];
        char video_channal_id[32];
        char alarm_id[32];
        int  reg_interval;
        int  hb_interval;
        char authname[32];
        char username[32];
        char password[32];
        int enable;
        int videochannel;
        int localport;
        int protocoltype;
        int streamtype;
        int connect_status;
    } GuoBiaoS;

    typedef struct {
        char  address[64];
        float longitude;
        float latitude;
    } GBAddrS;
    typedef enum sensor_type_e {
        SENSOR_NONE = 0,
        SENSOR_GC3003,
        SENSOR_GC4663,
        SENSOR_GC4653,
        SENSOR_GC5603,
        SENSOR_SC230AI,
        SENSOR_SC200AI,
        SENSOR_SC450AI,
        SENSOR_SC465SL,
        SENSOR_SC4336P,
        SENSOR_OS04D10,
        SENSOR_SP4329,
        SENSOR_SC235,
        SENSOR_MAX,
    } ESensorType;

    typedef enum audio_type_e {
        AUDIO_NONE = 0,
        AUDIO_WM8988,
        AUDIO_MAX,
    } EAudioType;

    typedef enum rtc_type_e {
        RTC_NONE = 0,
        RTC_PCF8563,
        RTC_MAX,
    } ERtcType;

    typedef enum auto_focus_type_e {
        AUTOFOCUS_NONE = 0,
        AUTOFOCUS_AN41908,
        AUTOFOCUS_MAX,
    } EAutoFocusType;
    /*
        系统能力集
    */
    typedef struct {
        ESensorType sensor;                 // sensor的类型
        ERtcType rtc;                       // 是否支持RTC
        EAudioType audio;                   // 是否支持音频
        EAutoFocusType af;
        int alarm_in;
        int alarm_out;
        int rs485;                          // 是否支持485
        int rs485_gpio;                     // 485控制GPIO的编号
        int key_reset;                      // 恢复出厂默认值按键
        int led_dectect;                    // 红外灯的侦测
        int led_ctrl;                       // 红外灯控制

    } SystemFunEnable;

    typedef struct {
        int alarmevent;
        int blackmargin;
        int colorbase;
        int hue;
        int gain;
        int dynamic;
        int sharpness;
        int ae_aw_blc;
        int position_3D;
        int ptz_ctrl;
        int follow;
        int customtype;
        int new_pelco;
        int rain;
        int irlight;
        int irmode;
        int shelter;
        int MCUupgrade;
        int G3;
        int dome;
        int alarmin;
        int alarmout;
        int audio;
        int graintype;
        int webdeflang;
        int hdetect;
        int cartect;
        int __4g;
        int wifi;
        int facetect;
        int platetect;
        int passenger;
        int facesnap;
    } ShowWebS;

    typedef struct {
        int len;
        unsigned char cmdbuf[128];
    } TransparenceDataS;

    typedef struct {
        int type;
        int ratio;
        int status;
    } LensDPCS;

    typedef struct {
        int enable;
        int port;
        char dev_id[48];
        char serverip[64];
    }HanBangServiceCfg;
//日夜切换后，获取视频信息参数的结构体，可后期扩展
typedef struct {
    int fps[2];     //fps[0]:主码流帧率 fps[1]:从码流帧率
    int isnight;    //0:白天，1:夜视
} DayNightChangeCfg;

typedef enum {
    OSD_SIZE_S = 0,
    OSD_SIZE_M,
    OSD_SIZE_L
} OsdSizeE;

typedef struct {
    unsigned char value;            //当前值
    unsigned char start;            //该值的设置范围的起始值
    unsigned char step;             //该值设置范围的步长
    unsigned char level;            //该值的最高偏移步数
} Area_t;

typedef struct {
    unsigned char hbit;             //双字节高位
    unsigned char lbit;             //双字节高位
} Double_t;

typedef struct{
	int enable;
	int volumn;
} AudioOutCfg;

typedef struct{
    int devicebind;
    int definition;
} DevConfS;

typedef struct {
    Area_t heat;                       //加热器当前阀值，及设置范围
    Area_t fan;                        //风扇当前阀值， 及设置范围
    Area_t level_speed;                //水平手动速度当前值，及设置范围
    Area_t apeak_speed;                //垂直手动速度当前值，及设置范围
    Area_t apeak_angle;                //垂直角度当前值，及设置范围
    Area_t ir_mode;                    //红外灯控制当前值，及设置范围
    unsigned char auto_reboot;         //定时重启当前设置
    unsigned char auto_hour;           //定时 小时
    unsigned char auto_min;            //定时 分钟
    unsigned char auto_save;           //28  保留
    Area_t save4[DomeParameter_Dwordlen - 7];          //保留
    Double_t save2[DomeParameter_wordlen -0];          //保留
    unsigned char preset_title;        //预置点标题显示使能
    unsigned char autopan_title;       //左右扫描标题显示使能
    unsigned char sequence_title;      //巡航扫描标题显示使能
    unsigned char pattern_title;       //轨迹标题显示使能
    unsigned char region_title;        //区域标题显示使能
    unsigned char direct_title;        //方向指示显示使能
    unsigned char lens_title;          //倍数指示
    unsigned char alarm_title;         //报警状态显示
    unsigned char dome_title;          //球机标题显示使能
    unsigned char temperature_title;   //温度显示使能
    unsigned char DCpower_title;       //电压显示使能
    unsigned char light_title;         //照度指示使能
    unsigned char systime_title;       //系统时间显示使能
    unsigned char sysdate_title;       //系统日期显示使能
    unsigned char font_color;          //字体颜色
    unsigned char osd_posionH;         //255:无， 0 - 15 硬件OSD显示位置y 水平位置
    unsigned char osd_posionV;         //255:无， 0 - 63 硬件OSD显示位置x 垂直位置
    unsigned char guard_action;        //看守功能
    unsigned char action_num;          //功能编号
    unsigned char sequence_num;        //巡航扫描队列数
    unsigned char pattern_num;         //轨迹扫描队列数
    unsigned char autopan_num;         //左右扫描队列数
    unsigned char preset_num;          //预置位个数
    unsigned char guard_time;          //守卫时间
    unsigned char keep_time;           //开机保持时间
    unsigned char temp_unit;           //温度单位   255无，0-摄氏度，1-华氏度
    unsigned char speed_capacity;      //智能调节       255无，0-关，1-开
    unsigned char senser_limit;            //光敏阀值        255无，1-10 级
    unsigned char sensa_delay;         //光敏切换延时  255无，1-20秒
    unsigned char ir_ctrl_mode;        //红外自动模式 255无，0-室外，1-室内
    unsigned char ir_senser_mode;      //红外切换模式255无，0-摄像机1-光敏
    unsigned char sup_language;        //支持的语言
    unsigned char cur_language;        //当前语言
    unsigned char autopan_speed;       //左右扫描速度0-停，1-255
    unsigned char max_alarmchn;        //最大报警通道数255-无，1-7
    unsigned char alarm_upload;        //报警上传模式  255无，0-请求模式， 1-连续模式， 2-变化模式
    unsigned char alarm_outstatus;     //报警输出状态  0：断开，1吸合
    unsigned char save3[DomeParameter_Bytelen - 37];
} Dome_info_t;

typedef struct {
    Area_t digital_zoom;              //数字变焦设置
    Area_t sensitivity;               //慢快门
    Area_t image_mode;                //图像帧率
    Area_t blcmode;                   //背光补偿设置， 255 无， 0:背光关，1:背光开，2宽动态开 3，雾透开
    Area_t irmode;                    //日夜转换
    Area_t aemode;                    //曝光模式
    Area_t aemode_shuttle;            //快门参数
    Area_t aemode_iris;               //光圈参数
    Area_t aemode_agc;                //增益参数
    Area_t aemode_bright;             //亮度参数
    Area_t save4[CameraParameter_Dwordlen-10];
    Double_t save2[CameraParameter_Wordlen-0];
    unsigned char awb_mode;           //白平衡模式     ：255＝无、0~5：自动、手动、自动跟踪、室内、室外、单次锁定
    unsigned char r_gain;             //红增益           ：0~255
    unsigned char b_gain;             //蓝增益           ：0~255
    unsigned char image_bright;       //亮度          ：255无、0～254：0～254
    unsigned char image_contrst;      //对比度           ：255无、0～254：0～254
    unsigned char image_saturation;   //饱和度           ：255无、0～254：0～254。
    unsigned char image_sharpness;    //锐度          ：255无、0～254：0～254
    unsigned char image_hmirror;      //图像水平镜像  ：255无、0=关，1＝开
    unsigned char image_vmirror;      //图像垂直镜像  ：255无、0=关，1＝开
    unsigned char image_negart;       //图像负片        ：255无、0=关，1＝开
    unsigned char day_gammalevel;     //白天伽马        ：255＝无、0~5：0~5级
    unsigned char night_gammalevel;   //夜晚伽马        ：255＝无、0~5：0~5级
    unsigned char image_agc;          //AGC限值：255=无，0~13；0dB,2dB,4dB,6dB,8dB,12dB,14dB,16dB,18dB,20dB,22dB,24dB,26dB,28dB。
    unsigned char zoom_speed;         //变焦速度          ：255无、0=高、1＝低、2＝中。
    unsigned char image_dnr;          //图像降噪        ：255＝无，1~5：1~5级
    unsigned char image_stability;    //图像防抖          ：255无、0=关，1＝开。
    unsigned char foucs_limit;        //最小聚焦距离    ：255无，0～4：1cm、10cm、30cm、1m、1.5m。
    unsigned char day_hour;           //转彩色时间时    ：0~23
    unsigned char day_min;            //转彩色时间分    ：0~59
    unsigned char night_hour;         //转黑白时间时    ：0~23
    unsigned char night_min;          //转黑白时间分    ：0~59
    unsigned char agc_bright_mode;    //增益/亮度模式 ：255无、0=增益优先模式，1=亮度优先模式。
    unsigned char preset_freeze;      //预置点视频冻结   ：255无、0＝关，1＝开。
    unsigned char joystick_auto;      //摇杆自动恢复    ：255无、0＝关、1＝全开、2＝聚焦、3＝光圈。
    unsigned char auto_focus;          //自动聚焦恢复时间：0=关，3~255：3~255秒
    unsigned char auto_iris;         //自动光圈恢复时间：0=关，3~255：3~255秒
    unsigned char save1[CameraParameter_Bytelen-26];
} Camera_info_t;

typedef struct {
    unsigned char preset_id;
    unsigned char preset_name[32]; //UTF-8编码，
} PresetInfo_t;

typedef struct {
    int preset_num;
    PresetInfo_t info[210];
}PresetInfoS;

typedef struct {
    int led_index;
    int ao_prompt;
} gpio_t;

typedef struct {
    int   reverse;        //0:低开 1:高开
} WhiteLedS;

typedef enum MP4_VIDEO_TYPE_E {
    VIDEO_H264 = 1,
    VIDEO_H265
} Mp4VideoTypeE;

typedef struct {
    int x0;
    int y0;
    int x1;
    int y1;
    int dx0;
    int dy0;
    int dx1;
    int dy1;
    int k;
    int enable;
    int blink;
    int thresh;
    int indoor;
    int level;
    int dir;
    unsigned int times[7];
} VglineS;

typedef struct {
    int x0;         // 左上
    int y0;
    int x1;         // 右上，依顺时针方向
    int y1;
    int x2;
    int y2;
    int x3;
    int y3;
    int enable;
    int blink;
    int indoor;
    int dir;
    int thresh;
    int level;
    unsigned int times[7];
} VgrectS;

typedef enum {
    AUDIO_AIRLINK_MODE      = 0,        // 01.pcm
    AUDIO_KEY_RESETING      = 1,        // 02.pcm
    AUDIO_EXEC_RESETING     = 2,        // 03.pcm
    AUDIO_IP_SUCESS         = 3,        // 04.pcm
    AUDIO_WIFIAP_FAIL       = 4,        // 05.pcm
    AUDIO_AUTH_FAIL         = 5,        // 06.pcm
    AUDIO_AUTH_SUCCESS      = 6,        // 07.pcm
    AUDIO_AUTH_1STTIME      = 7,        // 08.pcm
    AUDIO_FACTORY_MODE      = 8,
    AUDIO_FACTORY_TEST      = 9,
    AUDIO_FACTORY_UPGRADE   = 10,
    AUDIO_BURNING_SUCC      = 11,
    AUDIO_BURNING_FAIL      = 12,
    AUDIO_RUNINGOUT_ID      = 13,
    AUDIO_PASSWORD_ERR      = 14,
    AUDIO_SETUP_WIFI        = 15,
    AUDIO_WEAK_WIFI         = 16,
    AUDIO_8188_FAIL         = 17,
    AUDIO_WIFI_CONNECT      = 18,
    AUDIO_ALARM_ALARM       = 19,
    AUDIO_ALARM_DOG         = 20,
    AUDIO_ALARM_OTHER       = 21,
    AUDIO_ALARM_CUSTOM      = 22,
    AUDIO_DI_DI                 = 23,
    AUDIO_RECV_PWD              = 24,
    AUDIO_CON_FAIL              = 25,
#ifdef CUST_ALIAPP_MODE_TIANMAO       //阿里云天猫精灵版本提示音
    AUDIO_TM_WAIT               = 26, //等待配网
    AUDIO_TM_START              = 27, //配网启动
    AUDIO_TM_TIMEOUT            = 28, //等待配网达到10分钟
    AUDIO_TM_RESET              = 29, //重置
    AUDIO_TM_CON_WIFISTART      = 30, //尝试连接WiFi
    AUDIO_TM_CON_WIFISUC        = 31, //连接WiFi成功
    AUDIO_TM_CON_WIFIFAIL       = 32, //连接WiFi失败
    AUDIO_TM_CON_CLOUDSUC       = 33, //连接到云端成功
    AUDIO_TM_CON_CLOUDCONFLICT  = 34, //连接到云端失败,若已知被其他账号绑定的原因而失败
    AUDIO_TM_CON_CLOUDOTHERS    = 35, //连接到云端失败（若"被其他账号绑定"之外的原因而失败）
#endif
    AUDIO_SETUP_4G              = 36,
    AUDIO_CHECK_SUCCESS_4G      = 37,
    AUDIO_SIM_CHECK_SUCCESS_4G  = 38,
    AUDIO_CHECK_FAIL_4G         = 39,
    AUDIO_SIM_CHECK_FAIL_4G     = 40,
    AUDIO_SIM_CHECK_ICCID_4G    = 41,
    AUDIO_CSQ_WEAK_4G           = 42,
    AUDIO_CSQ_STRONG_4G         = 43,
    AUDIO_UPGRAD_SUCCESS_REBOOT = 44,
    AUDIO_UPGRADING_NO_OFF      = 45,
    AUDIO_SIM4G_CON_FAIL        = 46,
    AUDIO_AGING_TEST            = 47,
    AUDIO_SIM4G_SIGLE_NORMAL    = 48,
    AUDIO_SIM4G_SIGLE_WEEK      = 49,
    AUDIO_SIM4G_SCAN_QRCODE     = 50,
    AUDIO_SIM4G_BIND_SUCCESS    = 51,
    AUDIO_SIM4G_DI_DEV_BINDING  = 52,
    AUDIO_CUT2INFRARED          = 53,
    AUDIO_CUT2WHITE             = 54,
    AUDIO_CUT2DOUBLE            = 55,
    AUDIO_CUT2FACTORY           = 56,
    AUDIO_BOTTOM                = 57,

    AUDIO_DIG_0                 = 90,
    AUDIO_DIG_1                 = 91,
    AUDIO_DIG_2                 = 92,
    AUDIO_DIG_3                 = 93,
    AUDIO_DIG_4                 = 94,
    AUDIO_DIG_5                 = 95,
    AUDIO_DIG_6                 = 96,
    AUDIO_DIG_7                 = 97,
    AUDIO_DIG_8                 = 98,
    AUDIO_DIG_9                 = 99,

    AUDIO_CALL_UP               = 100,
    AUDIO_CALL_OFF              = 101,
    AUDIO_CALL_NOT_AVAILABLE    = 102,
    AUDIO_CALL_FINISH           = 103,
    AUDIO_CALL_BUSY             = 104,
    AUDIO_CALL_DROP             = 105,

    /*ASR*/
    AUDIO_ASR_LM_HERE           = 106,   // 我在的主人
    AUDIO_ASR_OPEN_DEV          = 107,   // 设备已打开
    AUDIO_ASR_OFF_DEV           = 108,   // 设备已关闭

    AUDIO_VOLUM_INC             = 109,   // 音量已调大
    AUDIO_VOLUM_DEC             = 110,   // 音量已调小
    AUDIO_VOLUM_OFF             = 111,   // 音量已关闭
    AUDIO_VOLUM_MAX             = 112,   // 音量调到最大

    AUDIO_LIGHT_ON              = 113,   // 灯光已打开
    AUDIO_LIGHT_OFF             = 114,   // 灯光已关闭
    AUDIO_LIGHT_INC             = 115,   // 灯光已调亮
    AUDIO_LIGHT_DEC             = 116,   // 灯光已调暗

    AUDIO_TURN_LEFT             = 117,   // 好的左转
    AUDIO_TURN_RIGHT            = 118,   // 好的右转
    AUDIO_TURN_UP               = 119,   // 好的向上
    AUDIO_TURN_DOWN             = 120,   // 好的向下

    AUDIO_TURN_MAX_LEFT         = 121,   // 已转至最左边
    AUDIO_TURN_MAX_RIGHT        = 122,   // 已转至最右边
    AUDIO_TURN_MAX_UP           = 123,   // 已转至最上边
    AUDIO_TURN_MAX_DOWN         = 124,   // 已转至最下边

    AUDIO_CALL_WAIT             = 125,   // 正在呼叫请稍等
    AUDIO_CALL_HANGUP           = 126,   // 已挂断通话
    AUDIO_PTZ_CALIBRATE         = 127,   // 云台已校准

    AUDIO_ASR_WAKEUP_AGAIN      = 128,   // 主人需要再唤醒我吧
    AUDIO_LIGHT_MAX             = 129,   // 亮度已调至最大
    AUDIO_VOLUME_MAX            = 130,   // 音量已调至最大
    AUDIO_NIGHT_LIGHT_OFF       = 131,   // 关闭小夜灯
    AUDIO_ASR_OK                = 132,   // 好的
    AUDIO_VOLUM_MUTE            = 133,   // 已静音

    AUDIO_COUNT,
} AUDIO_PROMPT;

typedef struct {
    int audio_type;
    int time;
}alarm_audio_t;

typedef struct {
    int light_mode;      //1.双光源 2 普通红外
    int is_zoom;         //1.变倍   2 不带变倍
    int is_ptz;          //1.带云台，2.不带云台
} ShowAppS;

typedef struct {
    int type;
    int reverse;        //控制方向定义:  1=正常;2 =上下反;3 =左右反;其它=全反;
    int o_speed;
    int o_seconds;
    int h_maxstep;      //水平转动最大步数:  默认= 3120;
    int v_maxstep;      //云台垂直转动最大步数:  默认= 1050;
    int h_speed;        //云台速度控制因子。用于调节总体速度;  60%--150 %，默认= 100%；
    int v_speed;        //云台垂直速度控制因子。用于调节垂直速度;    50%--100 %，默认= 66%；
    int h_Startstep;    //云台水平起始偏移步数:  默认= 50;
    int v_Startstep;    //云台垂直起始偏移步数:  默认=120;
    int WaitTime;       //预置位巡航 驻留时间；单位为秒。最小16，最大255。
    int SeqTimes;       //预置位巡航 扫描总时间，以分钟为单位。
    int max_h_Angle;    //云台水平最大旋转角度
    int max_v_Angle;    //云台垂直最大旋转角度
    int max_h_ViewAngle;//镜头最大水平视角
    int max_v_ViewAngle;//镜头最小水平视角
} motor_t;

typedef struct {
    int h_maxstep;          //水平转动最大步数:  默认= 3120;
    int v_maxstep;          //云台垂直转动最大步数:  默认= 1050;
    int max_h_Angle;        //云台水平最大旋转角度
    int max_v_Angle;        //云台垂直最大旋转角度
    int max_h_ViewAngle;    //镜头最大水平视角
    int max_v_ViewAngle;    //镜头最大垂直视角
    float Distance_k;
    int ImageH_Mirror;
    int ImageV_Mirror;
    int ZoomStepMax;
    int ZoomInLimitsize;
    int ZoomOutLimitsize;
    int cur_pos_P;
    int cur_pos_T;
    int cur_pos_Z;
    int ZoomLimit;
    int Tilt_follow_Enable;
    int follow_Idle_Limit_P;
    int follow_Idle_Limit_T;
    float DZoomLimit;
    float DZoomStepMax;
} Follow_t;

typedef struct {
    int x;
    int y;
    int revert;
}PelcodCfg;

typedef struct
{
    int  id;
    int  x;
    int  y;
    int  isdefault;
    int  enable;
    char name[60];
}presetlist;

typedef struct
{
    int         gnum;
    presetlist  preset[MAX_PRESET_NUM];
}presetcfg;

typedef struct
{
    int mode;           //控灯模式:0主板控制灯；1灯板硬件控制灯；
    int type;           //光敏类型:1硬光敏，0软光敏;
    int reverse;        //极性:type=1、mode=0时，为主板驱动灯的极性；
    int nighttoday;
    int daytonight;
    int nighttoday_mode1;//mode=1时，nighttoday的阈值
    int daytonight_mode1;
}DaynightCfgS;

typedef struct
{
    AUDIO_PROMPT  status;
    int play_time;
    char *pBuf;
    int nBuf;
    char path[64];
} play_amr_info_t;

typedef struct {
    int enable;
    int humanenable;
    int carenable;
    int petenable;
    int zoom;
    int screenenable;
    int thresh;
    int idle;
    int preset;
    int reverse;
    unsigned int times[7];
}follow_info_t;

typedef struct {
	char appves[128];
} Appvescfg;

typedef struct {
    Appvescfg appve[6];
    int       imaxqp[2];
    int       iminqp[2];
    VencSizeE webxvsz;      // web 主码流定制
    VencSizeE appxvsz;
} Appvecfg;

enum LAMP_TYPE {
    LAMP_IR     = 0,
    LAMP_WHITE  = 1,
    LAMP_DBL    = 2,
    LAMP_STAR   = 3,
};

enum ALG_TYPE {
    ALG_HARD    = 1,        //硬光敏
    ALG_SOFT    = 2,        //软光敏
    ALG_TIME    = 3,        //定时状态
    ALG_DAY     = 4,        //强制白天 = 强制关灯
    ALG_NIGHT   = 5,        //强制晚上 = 强制开灯
};

enum SHINE_MODE {
    SHINE_WHITE = 0,
    SHINE_IR    = 1,
    SHINE_DBL   = 2,
};

typedef struct {
    int             beginhour     ;
    int             beginmin      ;
    int             endhour       ;
    int             endmin        ;
    int             fcolorbeginhour ;  //全彩模式彩转黑时间开始 时
    int             fcolorbeginmin  ;  //全彩模式彩转黑时间开始 分
    int             fcolorendhour   ;  //全彩彩转黑时间结束 时
    int             fcolorendmin    ;  //全彩彩转黑时间结束 分
    int             truestar      ; // 1 为黑光臻全彩设备 0 为普通设备
    int             gt_1_forceday ; // 2 ALG_DAY, 0 ALG_NIGHT, 1 AUTO   ---> 这个变量暂时没用到
    int             pwm_percent   ; // 1~100
    int             pwm_ev        ; // 1~99999999
    int             showautolight ;
    int             adjustable    ;
    int             is_wh_triglow ;
    int             is_ir_triglow ;
    enum LAMP_TYPE  lamptype      ;
    enum ALG_TYPE   alg           ;
    enum SHINE_MODE shinemode     ;
    int             shinetime     ;
    int             turn_on_pct   ;
    int             turn_off_pct  ;
    int             irledmode     ; // 红外读高读低模式
    int             lampmode      ; // 灯板软硬光敏模式
    int             lightboard    ; // 灯板
    int             nightled      ; // 小夜灯亮度
    int             ircut_reverse ; // 放置在最后，方便 memcmp()
    float           fcopenevda    ;
    float           fcopenevst    ;
    float           fcopenearly   ;
    float           fcopenmiddle  ;
    float           fcopenlate    ;
    float           fctargeevst   ;
    float           fccloseevst   ;
    float           fctargetratio ;
    float           fclightmaxev  ;
    float           fclightminev  ;
    float           fccurev       ;
} LightExtCfg;

typedef struct
{
    int enable;
    int port;
    int keep;
    int ircut;
    int audio;
    int led_red;
    int led_white;
    int ptz;
    int speed;
    int zoom;
}ag_params_t;

struct lamp_run {
    int             is_day_curr;     //当前设备状态，1白天不开灯 2晚上白光灯关,红外灯开 3晚上白光灯开,红外灯关 0无效状态
    int             is_day_prev;     //设备上一个状态
    int             is_alarm;
    int             ircut_reverse_prev;     //ircrt状态
    int             infrared_reverse_prev;  //红外灯状态
    int             white_reverse_prev;     //白光灯状态
    int             hard_cnt;
    int             color_cnt;
    int             adjust_cnt;
    int             adjust_ev_prev;
    int             white_light_grade;
    int             infrared_light_grade;
    int             is_red_open;  //红外灯是否开启 0-关闭 1-开启。 用于崔总软光敏算法
    int             is_force_night;//是否启用防反复切
    int             lamptype_prev;  //红外灯是否开启 0-关闭 1-开启。 用于崔总软光敏算法
    enum ALG_TYPE   alg;
    JSTCHandle      hdl_shineoff;    //白光灯 关灯任务
    int             daynight_switch_time[6];
    int             is_audio_alarm;
 };

typedef struct {
    int            enable;
    int            mod;  //检测模式，画线检测还是画框检测
    int            x0;   // 左上
    int            y0;
    int            x1;   // 右上，依顺时针方向
    int            y1;
    int            x2;
    int            y2;
    int            x3;
    int            y3;
    int            x4;
    int            y4;
    int            x5;
    int            y5;
    int            dx0;
    int            dy0;
    int            dx1;
    int            dy1;
    int            dir;
    int            reverse;
    int            timeclear;
    int            screenenable;
    int            showenable;
    int            sense;
    int            distance;
    unsigned int   times[7]; //times[0] = 0, times[1]=21;
    int            httpupload;
    int            timeali;
    int            lingerTime;//用于徘徊判断，单位:S。
    int            lingerTouchTimes;
} PassengerDetectS;

typedef struct {
    int    enable;
    int    x;
    int    y;
    char   content[128];
}osd_custom_t;

typedef struct {
    int    beginhour;
    int    beginmin ;
    int    endhour  ;
    int    endmin   ;
} TimeSeg;

typedef struct {
    int     show;
    int     enable;
    int     type;
    int     place;
    TimeSeg timeseg;
    int     times;
    int     aindex;
    char    atext[128];
} AudioAlarmS;

typedef struct {
    int     enable;
    int     place;
    TimeSeg timeseg;
    int     time;
} LightAlarmS;

typedef struct {
    char name[24];
    char hardware[24];
    char location[24];
    int discoverable;
    int searchable;
    char lastreboot[16];
} OnvifInfoCfg;

typedef struct
{
    int chn;
    unsigned int multicast;
    int port;
    int action;     // 0 stop, 1 start
} RtpOverMulticast_t;

typedef struct {
    int     show;
    int     enable;
    int     place;
    TimeSeg timeseg;
    int     time;
} IOAlarmS;

typedef struct {
    int             enable;
    int             thresh;
    unsigned int    times[7];  //0:10,1:10,2:10,3:10,4:10,5:10,6:10,
} VMaskAlarmS;

typedef struct {
    int interval;
    int alarmcenter;
    int email;
    int alarmout1;
    int alarmout2;
    int sound;
    int capture;
    int record;
    int ftpup;
} VMaskAlarmLinkS;

typedef struct {
    int interval;
} AlarmInfocfg;

typedef struct {
    int video;
} priv_ctrl_t;

//隐私遮挡布防时间配置
typedef struct {
    char week[8];       // 计划星期布防
    int enable;         // 计划隐私遮挡开关
    int mask_enable;    // 手动隐私遮挡开关(重启继承)
    int cover_direction;// 隐私遮挡方向
    int beginhour;
    int beginmin;
    int endhour;
    int endmin;
} videomask_plan_t;

typedef struct {
    int show;
    int enable;
    int status;
    char modelId[64];
    char appId[64];
    char openId[64];
    int openStatus;
} sVideoCallCfg;

typedef struct {
    int  iso1;
    int  iso2;
    int  iso3;
    float  area1;
    float  area2;
    float  area3;
    int  compensation1;
    int  compensation2;
    int  compensation3;
} facial_convergence_t;

typedef struct {
    facial_convergence_t wh;
    facial_convergence_t ir;
} sFacialConvergence;

#define false  0
#define true   1

#ifdef __cplusplus
}
#endif

#endif

