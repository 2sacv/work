/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : encode_video.h
 * @Created Time : 2022-12-5
 * @Version      : 1.0
 * @Author       : tangjx
 * @Description  :
*/

#ifndef __ENCODE_VIDEO_H__
#define __ENCODE_VIDEO_H__
#ifdef __cplusplus
extern "C" {
#endif

#define GET_STREAM_TIMEOUT  (1000)
#define SCENE_CHANGE_INTER  (60*60*1000)

enum
{
    E_MAIN_CHN,
    E_SUB_CHN,
    E_MAX_VENC_CHN,
};

enum {
    CMD_VENC_ENC         = 1 << 0,
    CMD_VENC_PROF        = 1 << 1,
    CMD_VENC_APPVE       = 1 << 2,
    CMD_VENC_UPDATE      = 1 << 3,
    CMD_EVENT_LUMACHG    = 1 << 6,

    CMD_CODEC_RESET0     = 1 << 7,  //反初始化再初始化主码流
    CMD_CODEC_QUICKSET0  = 1 << 8,  //动态修改主码流编码参数
    CMD_CODEC_RESET1     = 1 << 9,  //反初始化再初始化子码流
    CMD_CODEC_QUICKSET1  = 1 << 10, //动态修改子码流编码参数
    CMD_CEDEC_CHANGE     = 1 << 11, //切换编码方式
};

int encode_video_get_jpeg(char *buf, int *size);
int encode_video_init();
int encode_video_uninit();

#ifdef __cplusplus
}
#endif
#endif



