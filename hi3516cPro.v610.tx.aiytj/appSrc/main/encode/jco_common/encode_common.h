/******************************************************************************
    Copyright (C), 2008-2018, JABSCO ELECTRONIC Tech. Co., Ltd

    File Name    : encode_common.h
    Version      : 1.0
    Author       : JABSCO Video Server Software Group
    Created      : 2015-02-10
    Description  :
    History      :
                        created by tianjun. 2015-02-10
******************************************************************************/

#ifndef __ENCODE_COMMON_H__
#define __ENCODE_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <strings.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <signal.h>
#include <sys/mman.h>
#include <signal.h>
#include <math.h>
#include <errno.h>
#include <memory.h>
#include <time.h>
#include <fcntl.h>
#include <math.h>
#include <sys/reboot.h>
#include <fcntl.h>
#include <errno.h>
#include <utime.h>
#include <ctype.h>
#include <netinet/in.h>
#include <openssl/md5.h>
#include <sys/syscall.h>

#include "encode_typedef.h"
#include "encodeapi.h"
#include "jconfstruct.h"
#include "pthread_manage.h"
#include "utils.h"
#include "linux_list.h"

#define RGB_TO_YCRYCB(r,g,b)                                                              \
        ((((unsigned int)(( 0.257f * r + 0.564f * g + 0.098f * b) + 16.0f))<<24)|       \
        (((unsigned int)(( 0.439f * r - 0.368f * g - 0.071f * b) + 128.0f))<<16) |       \
        (((unsigned int)(( 0.257f * r + 0.564f * g + 0.098f * b) + 16.0f))<<8) |         \
        (((unsigned int)((-0.148f * r - 0.291f * g + 0.439f * b) + 128.0f))<<0))

#define ENC_GET2MULTIPLE(S)     ((((S)&0x00000007)==0)?(S):((((S)>>1)+1)<<1))
#define ENC_GET4MULTIPLE(S)     ((((S)&0x00000007)==0)?(S):((((S)>>2)+1)<<2))
#define ENC_GET8MULTIPLE(S)     ((((S)&0x00000007)==0)?(S):((((S)>>3)+1)<<3))
#define ENC_GET16MULTIPLE(S)    ((((S)&0x0000000F)==0)?(S):((((S)>>4)+1)<<4))
#define ENC_GET32MULTIPLE(S)    ((((S)&0x0000001F)==0)?(S):((((S)>>5)+1)<<5))
#define ENC_GET64MULTIPLE(S)    ((((S)&0x0000003F)==0)?(S):((((S)>>6)+1)<<6))

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array)       ((int)(sizeof(array) / sizeof(array[0])))
#endif

#define ENC_CEIL_DIV(A,B)       (((A)+((B)-1))/(B))
#define ENC_FLOOR_DIV(A,B)      ((A)/(B))

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef BIT
#define BIT(x)  (1<<(x))
#endif

#define         P1080_WIDTH             1920
#define         P1080_HEIGHT            1080
#define         NR_MAX_ENC_CHN          4

/*IVES 帧源参数*/
#define RAW_W       640
#define RAW_H       360
#define IVES_FPS    20
#define ENCODE_DRAW_RECT_MAX_NUM  10

#if 0 //特别备注花点和刷遍的值是根据创建时颜色顺序有关系的，目前逻辑如下
    { OSD_COLOR_IDX_TRANSPARENT, COLOR_ARGB_TRANSPARENT },
    { OSD_COLOR_IDX_WHITE,       COLOR_ARGB_WHITE       },
    { OSD_COLOR_IDX_BLACK,       COLOR_ARGB_BLACK       },
    { OSD_COLOR_IDX_RED,         COLOR_ARGB_RED         },
#endif

#define COLOR_ARGB_TRANSPARENT  0x00000000  /* 完全透明 */
#define COLOR_ARGB_WHITE        0xFFFFFFFF  /* 不透明白色 */
#define COLOR_ARGB_BLACK        0xFF000000  /* 不透明黑色 */
#define COLOR_ARGB_RED          0xFFFF0000  /* 不透明红色 */
#define COLOR_ARGB_GREEN        0xFF00FF00  /* 不透明绿色 */
#define OSD_COLOR_RED           0xFF0000
#define OSD_COLOR_GREEN         0x00FF00
#define OSD_COLOR_BLUE          0x0000FF

#ifndef ALIGN_UP
#define ALIGN_UP(x, align) ((((x) + ((align) - 1)) / (align)) * (align))
#endif

#ifndef ALIGN_DOWN
#define ALIGN_DOWN(x, align) (((x) / (align)) * (align))
#endif

#ifndef AX_MAX
#define AX_MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef AX_MIN
#define AX_MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#define EDBG(fmt, args...) \
    do{  \
        fprintf(stdout, NONE"[-ENC-] [%s:%5d] --- " fmt,__FILE__,__LINE__,##args); \
    }while(0)


#define EERR(fmt, args...) \
    do{ \
    fprintf(stderr,"[ERR %s:%5d] " fmt, (char *)__FILE__,__LINE__,## args);    \
} while(0)

#define ENCODE_RET_JUDGE(ret)         \
    do{  \
        if (S_OK != ret){   \
            EERR("[%s] bRet[0x%x] is ERROR! Please Check Code \n",__func__, ret);} \
    }while(0)

#define ENCODE_RET_CHECK(ret, fmt, args...)         \
    do{  \
        if (S_OK != ret){   \
            EERR("ret[0x%x]" fmt "\n", ret, ## args);} \
    }while(0)

#define ENCODE_RET_BREAK(ret, fmt, args...) {  \
        if (S_OK != ret){   \
            EERR("ret[0x%x]" fmt "\n", ret, ## args);\
            break;\
        }\
    }

#define ENCODE_NULL_BREAK(pPtr)  {       \
        if (NULL == pPtr) { \
            EERR("pPtr is NULL! Please Check Code \n"); \
            break;\
        }\
    }

#define ENCODE_NULL_JUDGE(pPtr)         \
    do{  \
        if (NULL == pPtr) { \
            EERR("pPtr is NULL! Please Check Code \n");} \
    }while(0)

#define ENCODE_CONDI_BREAK(condi, ret, fmt, args...)         \
    do{  \
        if (condi){   \
            EERR("ret[0x%x]" fmt "\n", ret, ## args);   \
            break;\
        } \
    }while(0)

#define ENCODE_CONDI_CHECK(condi, ret, fmt, args...)         \
    do{  \
        if (condi){   \
            EERR("ret[0x%x]" fmt "\n", ret, ## args);   \
        } \
    }while(0)

#define ENCODE_NULL_RET_BREAK(pPtr, ret)  {       \
        if (NULL == pPtr) { \
            EERR("pPtr is NULL! Please Check Code \n"); \
            ret = S_FAIL; \
            break;\
        }\
    }

#define ENCODE_PROCESS_RUN    0
#define ENCODE_PROCESS_PAUSH  1

typedef enum
{
    E_VIDEO_CHANNEL_BEGIN = -1,
    E_VIDEO_CHANNEL_MAIN = 0,
    E_VIDEO_CHANNEL_SUB,
    E_VIDEO_CHANNEL_JPG,
    E_VIDEO_CHANNEL_MAX,
} EVideoChannel;

typedef enum
{
    E_ENCODE_CHANNEL_BEGIN = -1,
    E_ENCODE_CHANNEL_H26X = 0,
    E_ENCODE_CHANNEL_JPEG,
    E_ENCODE_CHANNEL_MAX,
}EEncodeChannel;

typedef enum
{
    E_ENCODE_GROUP_BEGIN = -1,
    E_ENCODE_GROUP_MAIN = 0,
    E_ENCODE_GROUP_SUB,
    E_ENCODE_GROUP_MAX,
}EEncodeGroup;

typedef enum day_night_type_e {
    E_LUM_BEGIN = -1,
    E_LUM_NIGHT = 0,
    E_LUM_DAY_LOW = 1,
    E_LUM_DAY_MID = 2,
    E_LUM_DAY_HIGH = 3,
    E_LUM_MAX,
} EDayNightType;

typedef enum ev_type_e {
    E_EV_LOW = 0,
    E_EV_MID = 1,
    E_EV_HIGH = 2,
    E_EV_MAX,
} EEVType;

typedef enum light_mode_e {
    E_Light_Infrared = 0,
    E_Light_White_Light = 1,
    E_Light_Double_Lamp = 2,
    E_Light_MAX,
} ELightMode;

typedef enum {
    E_TV_FORMAT_BEGIN = -1,
    E_TV_FORMAT_PAL = 0,
    E_TV_FORMAT_NTSC,
    E_TV_FORMAT_MAX,
} ETVFormat;

typedef struct venc_frame_resolution {
    DWORD                   dwResWidth;                 // 分辨率的宽
    DWORD                   dwResHeight;                // 分辨率的高
} TVencFrameResolution;

typedef enum {
    E_H264_PROFILE_BEGIN = -1,
    E_H264_PROFILE_BASE,
    E_H264_PROFILE_MAIN,
    E_H264_PROFILE_HIGH,
    E_H264_PROFILE_END
} EH264Profile;

typedef struct video_enc {
    int           id;
    int           enable;
    VENC_FORMAT_E codec;
    VencSizeE     vencsize;
    int           standard; //0,P制; 1, N制。（现都高清，只支持0）
    int           fps;
    int           bps;
    int           gop;
    int           fixfps;   // 质量优先，0  码率优先
    int           fixbps;   // 1 定码流，0 变码流  2 SMART
    int           profile;  // mjpg 的编码质量1 ~ 10，h264 0:High 1:Main 2:Base
    int           level;    // 10|20|30|40|50
    int           idrframe;
    int           quality;
} TVideoEnc;

typedef struct image_enc {
    DWORD         dwGnum;
    TVideoEnc     tTVideoEnc[E_VIDEO_CHANNEL_MAX];
} TImageEnc;

typedef enum time_interval_e {
    E_TIME_INTERVAL_BEGIN = -1,
    E_TIME_INTERVAL_OK,
    E_TIME_INTERVAL_ERR,
} ETimeInterval;

typedef enum count_calc_e {
    E_COUNT_CALC_BEGIN = -1,
    E_COUNT_CALC_OK,
    E_COUNT_CALC_ERR,
} ECountCalc;

typedef int (*FunIRcutOp)(void);

typedef enum {

    E_IR_PHOTOSENSITIVE = 0,            // 光敏模式
    E_IR_DAY_NIGHT,                      // 日夜模式
    E_IR_LUMA,                          // 非红外枪 图像亮度模式
    E_IR_NONE,                          // 无红外功能
} EInfraredMode;

typedef enum {
    E_LUMA_BEGIN = -1,
    E_LUMA_LOW = 0,                     //  低亮
    E_LUMA_HIGH,                        //  高亮
    E_LUMA_MAX,
} ELumaMode;

typedef enum {
    E_COLOR_CHANGE_YES = 0,             // 彩转黑使能
    E_COLOR_CHANGE_NO,                  // 彩转黑禁止
    E_COLOR_CHANGE_COLOR,               // 彩色模式
    E_COLOR_CHANGE_MONO,                // 黑白模式
} EColorChangeMode;

typedef enum {
    E_LIGHT_NIGHT = 0,                  // 当前处于夜视模式
    E_LIGHT_DAY,                        // 当前处于白天模式
} EDayNightMode;

typedef enum {
    E_LED_AUTO = 0,                     //自动
    E_LED_NEAR,                         // 近
    E_LED_MIDDLE,                       // 中
    E_LED_FAR,                          // 远
} ELedMode;
/*
typedef enum {
    E_IRCUT_NORMAL = 0,                 // IRCUT 方向正常
    E_IRCUT_REVERSE,                    // IRCUT 方向反向
    E_IRCUT_DISABLE,                    // IRCUT 功能禁止
} EIRCUTMode;
*/
typedef enum {
    E_TRIG_LOW = 0,                   // 控制亮灯模式 低电平
    E_TRIG_HIGH,                      // 控制亮灯模式 高电平
}E_TRIG_MODE;

typedef enum {
    E_WHITE_PROC_BEGIN = -1,
    E_WHITE_PROC_OFF,
    E_WHITE_PROC_ON,
    E_WHITE_PROC_HOLD,
    E_WHITE_PROC_MAX,
} E_WHITE_PROC_STATUS;

typedef enum {
    OSD_COLOR_IDX_TRANSPARENT = 0,
    OSD_COLOR_IDX_WHITE,
    OSD_COLOR_IDX_BLACK,
    OSD_COLOR_IDX_RED,
} E_OSD_COlOR_IDX;

typedef struct {
    int                 enable;
    E_TRIG_MODE         e_trig_mode;
    int                 gpio_white;
    E_WHITE_PROC_STATUS e_white_proc_status;
    int                 hold_count;
} white_ctrl_t;

typedef struct InfraredCtrlS {
    EInfraredMode       eInfraredMode;      // 红外模式
    DWORD               dwTurnOnLux;        // 亮度的灵敏度
    EDayNightMode       eDayNightMode;      // 当前是夜视还是白天
    ELedMode            eLedMode;           // 灯模式
    EIRCUTMode          eIrcutMode;
    EColorChangeMode    eColorChangeMode;   // 图像颜色是否彩转黑
    SDWORD              sdwTransThrd;
    int                 blackbegin_hour;    //彩转黑开始：时
    int                 blackbegin_min;     //彩转黑开始：分
    int                 blackend_hour;      //彩转黑结束：时
    int                 blackend_min;       //彩转黑结束：分
    BOOL                bChangeInfrare;
    int                 tsInfraredInt;      //光敏检测时间间隔
    int                 count_max;          //光敏检测次数
    int                 autolighten;
    int                 lightgrade;
    int                 openlightlux;         //开灯灵敏度
    int                 closelightlux;        //关灯灵敏度
    int                 whitectrl;
    int                 fdGPIO;
    int                 fdADC;
    FunIRcutOp          fOpRedGlass;
    FunIRcutOp          fOpWhiteGlass;

    EDayNightType       eDayOrNight;
    struct timespec     tsInfr;
    int                 lum;
    int                 lum_old;
    int                 count_old;
    int                 time_begin;
    int                 time_end;
    int                 shinemode;
    int                 shinetime;
    int                 paraminit;
    int                 ircutinit;
    BOOL                bIrcutTest;
    int                 IrcutTestResult;
    white_ctrl_t        white_ctrl;
} InfraredCtrlS;

typedef struct font_t
{
    DWORD               dwFontWidth;        // 字体的宽
    DWORD               dwFontHeight;       // 字体的高
    DWORD               dwFontColor;        // 字体颜色
    CHAR                cFontFileName[64];  // 字库文件名称

}TFont;

typedef struct lattice_t
{
    struct list_head    list;
    TFont               tFont;                      // 使用饿字库的信息
    DWORD               dwWidth;                    // 点阵的宽
    DWORD               dwHeight;                   // 点阵的高
    DWORD               dwUnicode;                  // Unicode编码
    UCHAR               *pcBuffer;                  // 位图的BUFFER
}TLattice;

typedef struct rect_t
{
    DWORD               dwWidth;            // 矩形的宽
    DWORD               dwHeight;           // 矩形的高
    DWORD               dwPitch;            //
    DWORD               dwOffX;             // 拷贝内部偏移坐标
    DWORD               dwOffY;
    UCHAR               *pcBuffer;          // 矩形的BUFFER
}TRect;


typedef struct deforg_t
{
    int             strength;
} TDeForg;

typedef struct dpc_t
{
    int             ratio;
} TDPC;


typedef struct test_t
{
    int             enable;
} TTEST;

#define MAX_LATTICE_NUMBER  128

int encode_thread_get_run(void);
int encode_thread_set_quit(void);

int encode_get_color(int index,unsigned int *r,unsigned int *g,unsigned int *b);

int encode_count_stat(int msec);
int MMPF_OsCounterGetMs(void);
ETimeInterval encode_time_interval(struct timespec *ptime_pre,int interval_ms);
ECountCalc encode_count_calc(int *pcount_pre,int count_max);
int encode_memory_2d_copy(TRect *ptDesRect,TRect *ptSrcRect,DWORD dwSize);
int encode_debug_show_lattice(TLattice *ptLattice);
int encode_debug_show_rect(TRect *ptTRect);

void YCBCR_TO_RGB(int Y,int Cb,int Cr,int *R,int *G,int *B);
void RGB_TO_YCBCR(int R,int G,int B,int *Y,int *Cb,int *Cr);

int xslt_media_type(VideoEnc0 *p);

#ifdef __cplusplus
}
#endif

#endif//__ENCODE_COMMON_H__

