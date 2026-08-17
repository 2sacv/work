/**
 * Copyright (C) by Jabsco Company
 * 
 * @File Name    : encodeapi.h
 * @Created Time : 2022-12-5
 * @Version      : 1.0
 * @Author       : tangjx
 * @Description  : 
 */

#ifndef _ENCODE_API_H_
#define _ENCODE_API_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <math.h>
#include "jconfstruct.h"

typedef enum {
    CH_FS_MAIN0     = 0,          // MAIN 0
    CH_FS_SUB0      = 1,          // SUB  0
    /* fs h26x above */
    CH_FS_H26X_END ,
    CH_FS_ALL = CH_FS_H26X_END,
    CH_FS_IVE = CH_FS_H26X_END,
    CH_FS_END
}CH_FS_E;

typedef enum {
    SHM_BUF_MAIN = CH_FS_MAIN0,
    SHM_BUF_SUB  = CH_FS_SUB0,
    /* fs h26x above */
    SHM_BUF_AUDIO = CH_FS_H26X_END,
    SHM_BUF_AUDIO_AAC,
    SHM_BUF_END
}IDX_SHMBUF_E;

typedef enum {
    ISP_DAY             = 0, // 白天
    ISP_COLOR_NIGHT     = 1, // 全彩夜视
    ISP_INFRARED_NIGHT  = 2, // 红外夜视
    ISP_MAX,
} eIspColor;

#define FLOAT_TO_LLU(a)   (unsigned long long int)((((a) < 0) ? -(a) : (a)) * pow(10,3))

#define AUDIO_OUT_PERFRM      (320)
#define AUDIO_IN_PERFRM       (480)
#define AUDIO_OUT_NUM_PERFRM  (AUDIO_OUT_PERFRM * 2)
#define AUDIO_IN_NUM_PERFRM   (AUDIO_IN_PERFRM * 2)

typedef enum
{
    PLAY_AMR_TYPE_BEGIN = -1,
    PLAY_AMR_TYPE_VG,
    PLAY_AMR_TYPE_MOTION,
    PLAY_AMR_TYPE_OTHER,
    PLAY_AMR_TYPE_CUT2INFRARED,
    PLAY_AMR_TYPE_CUT2WHITE,
    PLAY_AMR_TYPE_CUT2DOUBLE,
    PLAY_AMR_TYPE_CUT2FACTORY,
    PLAY_AMR_TYPE_END
} PLAY_AMR_TYPE_E;

/* special osdtype: the osd text of each channel are the same
** normal osdtype: osd text of each channel are independent
*/
typedef enum
{
    OSD_TYPE_BEGIN = -1,
    OSD_TYPE_TIME,          // special osdtype: currnet time show
    OSD_TYPE_BPSADNFPS,     // special osdtype: bitrate per second and framerate per second
    OSD_TYPE_TEXT,          // special osdtype: osd name,etc.
    OSD_TYPE_SYNC,          // normal osdtype: osd text
    OSD_TYPE_END            // 
} OSD_TYPE_E;

/* osd item struct */
typedef struct OsdItem_t
{
    int enable;         //  [0 ,1], 1-enable,  0-disable
    int chnId;          //  [0, 1], 0-main stream, 1-second stream
    int osdId;          //  [0,63] , auto obtain osdid when it is -1
    int coord_x;        // [0, video width-1],  x coordinate
    int Coord_y;        // [0, video height-1], y coordinate
    int font_width;     // [8, -], font width   - 8,16,24,32,48,64 etc in common use.
    int font_height;    // [8, -], font height  - 8,16,24,32,48,64 etc in common use.
    char osdText[128];  // osd text content
    OSD_TYPE_E osdType; // osd type, OSD_TYPE_SYNC in common use.
    int colorConvert;   // [0,2], white black convert
}OsdItem_s;

#define Pelco_d_IE_Cmd_Buf_Len	64
#define Pelco_d_Cmd_Buf_Len		300
#define Pelco_d_Data_Len    	128
#define Pelco_d_Cmd_Len    		Pelco_d_Data_Len+6

typedef union 	
{
    unsigned char  Bytes[Pelco_d_Cmd_Len];
    struct
    {
        unsigned char  Head;
        unsigned char  Address;
        unsigned char  Cmd0;
        unsigned char  Cmd1;
        unsigned char  LEN;
        unsigned char  Data[Pelco_d_Data_Len];
        unsigned char  Sum;
    }Byte; 
    struct
    {
        unsigned char  Head;
        unsigned char  Address;
        unsigned char  Cmd0;
        unsigned char  Cmd1;
        unsigned char  Data0;
        unsigned char  Data1;
        unsigned char  Sum;
    }CmdByte; 	
}Pelco_d_RData;

typedef struct
{
    unsigned short 	Zoom;    
    unsigned short 	Digit_Zoom;
    unsigned short 	SysytmWorkMode;
} Zoom_St;

int encode_Pre_body_Check_Pause(int enable);
int encode_get_follow_enable(void);

//encode_video
int encode_immediate_iframe(CH_FS_E          eChannel);

int encode_get_init_status();

#ifdef __cplusplus
}
#endif

#endif

