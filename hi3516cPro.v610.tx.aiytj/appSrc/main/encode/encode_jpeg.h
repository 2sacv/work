/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : encode_jpeg.h
 * @Created Time : 2021-04-21
 * @Version      : 1.0
 * @Author       : cheby
 * @Description  :
*/

#ifndef __ENCODE_JPEG_H__
#define __ENCODE_JPEG_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "shm_buf.h"
#include "jconfstruct.h"
#include "jconfig.h"

// buf 小了会导致图片下方缺少数据部分呈灰色
#define LEN_JPEG (180*1024)  // 户外实测在 31K 左右, 少数可达 38K

enum {
    CMD_JPEG_CAPTURE  = 1 << 0,
    CMD_JPEG_VIDEO    = 1 << 1,
    CMD_JPEG_FREEZE   = 1 << 2,
    CMD_JPEG_PERSON   = 1 << 4,
    CMD_JPEG_ALARM    = 1 << 5,
};

typedef struct {
    unsigned int   jpglen;
    unsigned short width;
    unsigned short height;
    unsigned char  buf[0];
}PicInfoS;

typedef struct {
    int        chn;
    JEventType event_type;
}JpegAlarm;

struct jpeg_run{
    JSScheduler sch;
    JSTCHandle  hdl_loop;
    struct cmdstat *p_ctx;
    char *buf;
};

int encode_snapshot_ex(char *jpegbuf, int *bufsize);
int encode_loop_jpeg_start(void);
int encode_loop_jpeg_stop(void);
unsigned long Yuv420PToJpegInMemo(unsigned char ** jpgBuf, unsigned long *jpgSize, unsigned char* yuvData, int image_width, int image_height, int quality);

#ifdef __cplusplus
}
#endif
#endif


