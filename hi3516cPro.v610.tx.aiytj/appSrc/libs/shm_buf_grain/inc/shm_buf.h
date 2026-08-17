/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    :
 * Created Time : 2014-11-05
 * Version      : 1.0
 * Author       : luoshunfa
 * Description  :
 */

#ifndef _SHM_BUF_H
#define _SHM_BUF_H

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_FRAME_BYTES (400*1024)
#define MAX_FRAME_HEAD_BYTES (1*1024)  // sps pps vps

#define VIDEO_DATA_BUF_SIZE     MAX_FRAME_BYTES
#define RECORD_BUF_SIZE         MAX_FRAME_BYTES
#define REC_VIDEO_BUF_SIZE      MAX_FRAME_BYTES


    typedef enum {
        SHM_ERR_SUCCESS         = 0,
        SHM_ERR_FAILDE          = -1,
        SHM_ERR_DATA_TOO_LARGE  = -2,
        SHM_ERR_BUF_TOO_SMALL   = -3,
        SHM_ERR_NOT_READY       = -4,
        SHM_ERR_OVER_WRITE      = -5,
        SHM_ERR_LOCK_ERR        = -6,
    }eShmErr;

    typedef enum {
        SHM_FRAME_DUMMY         = 0,
        SHM_FRAEM_AUDIO         = 1,
        SHM_FRAME_VIDEO_B       = 10,
        SHM_FRAME_VIDEO_P       = 11,
        SHM_FRAME_VIDEO_I       = 12,
        SHM_FRAME_H264_SPS      = 20,
        SHM_FRAME_H264_PPS      = 21,
        SHM_FRAME_H264_SEI      = 22,

        SHM_FRAME_H265_VPS      = 30,
        SHM_FRAME_H265_SPS      = 31,
        SHM_FRAME_H265_PPS      = 32,
        SHM_FRAME_H265_SEI      = 33,
    } eShmFrameType;

    typedef enum {
        SHM_MEDIA_UNKOWN        = 0,
        SHM_MEDIA_AUDIO_ALAW    = 1,
        SHM_MEDIA_AUDIO_ULAW    = 2,
        SHM_MEDIA_AUDIO_AMR     = 3,
        SHM_MEDIA_AUDIO_AAC     = 4,
        SHM_MEDIA_VIDEO_MJPEG   = 10,
        SHM_MEDIA_VIDEO_MPEG4   = 11,
        SHM_MEDIA_VIDEO_H264    = 12,
        SHM_MEDIA_VIDEO_H265    = 13,
    } eShmMediaType;

    typedef void * shm_buf_t;
    shm_buf_t * shm_buf_new(int shmbufsize);
    void shm_buf_del(shm_buf_t sb);
    void shm_buf_reset(shm_buf_t sb);

    eShmErr shm_buf_set_media_info(shm_buf_t sb, eShmMediaType mediatype, int bps, int fps, int width, int height);
    eShmErr shm_buf_get_media_info(shm_buf_t sb, eShmMediaType *mediatype, int *bps, int *fps, int *width, int *height);
    eShmErr shm_buf_get_vps_sps_pps(shm_buf_t sb, char vpsdata[], int* vpsdata_size, char spsdata[], int* spsdata_size, char ppsdata[], int* ppsdata_size);
    eShmErr shm_buf_set_vps_sps_pps(shm_buf_t sb, unsigned int profile_level, char vpsdata[], int vpsdata_size, char spsdata[], int spsdata_size, char ppsdata[], int ppsdata_size);
    eShmErr shm_buf_get_sps_pps(shm_buf_t sb, char spsdata[], int* spsdata_size, char ppsdata[], int* ppsdata_size);
	eShmErr shm_buf_set_sps_pps(shm_buf_t sb, unsigned int profile_level, char spsdata[], int spsdata_size, char ppsdata[], int ppsdata_size);

	eShmErr shm_buf_set_media_info_audio(shm_buf_t sb, eShmMediaType mediatype, int samples, int bits);
	eShmErr shm_buf_get_media_info_audio(shm_buf_t sb, eShmMediaType *mediatype, int *samples, int *bits);

    eShmErr shm_buf_write_frame(shm_buf_t sb, char *dataptr, int nbytes, eShmFrameType frametype, double timestamp);

    /* note:
     *   1. when read inserial's frame success, will call cb auto, cb func must not stay long time, or will cause problems.
     *   2. inserial is -1, will read newest key frame, and outserail is the key frame 's serail
     *   3. inserial is -2, will read oldest key frame, and outserial is the key frame 's serail
     *   4. inserial is 0, will read the newest frame, and out serial is the frame's serail
     *   5. otherwise, the inserial is must increment
     */
    typedef void (*JSBReadCB)(void *userdata, int outserail, char *frame, int framesize, eShmFrameType frametype, double timestamp);
    eShmErr shm_buf_read_frame(shm_buf_t sb, int inserial, JSBReadCB cb, void *userdata);

    typedef struct {
        int             bps;
        int             fps;
        unsigned short  width;
        unsigned short  height;
        char *          vpsdata;
        int             vpsdata_size;
        char *          spsdata;
        int             spsdata_size;
        char *          ppsdata;
        int             ppsdata_size;
    } tSBVFrameInfo;

    typedef struct sdk_a_frame_info_s {
        int             samples;     //采样频率 8000
        unsigned char   bits;        //位宽     8 or 16 bits
        unsigned char   channels;    //通道数   1 or 2
        unsigned char   res[2];
    } tSBAFrameInfo;

    typedef struct {
        eShmErr         error;

        eShmMediaType   mediatype;
        union {
            tSBVFrameInfo v;        //视频帧信息
            tSBAFrameInfo a;        //音频帧信息
        };

        char *          framedata;
        eShmFrameType   frame_type;
        unsigned int    frame_size;
        double          frame_timestamp;
        int             frame_serial;

        int             curframe_serial;
    } tSBFrame;
    typedef void (*JSBReadCBEx)(void *userdata, tSBFrame* tFrame);

	// if read i frame, frame data just i frame data
    void shm_buf_read_frame_ex(shm_buf_t sb, int inserial, JSBReadCBEx cb, void *userdata);
	// if read i frame, frame data include vps sps pps
	void shm_buf_read_frame_add_vspps(shm_buf_t sb, int inserial, JSBReadCBEx cb, void *userdata);

#ifdef __cplusplus
}
#endif

#endif

