/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : encode_isp_video.h
 * @Created Time : 2022-12-26
 * @Version      : 1.0
 * @Author       : tangjx
 * @Description  :
*/

#ifndef __ENCODE_ISP_H__
#define __ENCODE_ISP_H__
#ifdef __cplusplus
extern "C" {
#endif

#define ENCODE_SENSOR_BIN_PATH "/ipc/sensor/"
#define ISP_PERIOD 1000
#define HIGHTEMP_WATCH_PERIOD (5*1000/ISP_PERIOD)

struct isp_cfg {
    Video3aS v3as;
    ViInfoS  vinfos;
    DnrCfgS  dnrcfgs;
};

struct isp_run {
    JSScheduler sch;
    JSTCHandle  hdl_loop;
    struct cmdstat *p_ctx;
};

typedef struct
{
    unsigned short isp_Brightness;                                                 //亮度           0~65535     记录上电时的参数，对应网页默认参数
    unsigned short isp_Contrast;                                                   //对比度         0~65535
    unsigned short isp_Saturation;                                                 //饱和度         0~65535
    unsigned char  isp_YCprocEn;                                                   //ycproc参数 亮度，对比度，饱和度等的使能开关
    unsigned char  isp_Sharpness[16];                            //锐化           0~255
    unsigned char  isp_HighlightSup;                                               //强光抑制       0~255
    unsigned char  isp_ExtraDgain;                                                 //全局isp gain   16~255
    unsigned char  isp_3DLevel[16];                         //可控降噪3D力度
}ENCODE_ISP_INFO_S;

int get_isp_mode(void);
int encode_isp_start();
int encode_isp_stop();

#ifdef __cplusplus
}
#endif
#endif

