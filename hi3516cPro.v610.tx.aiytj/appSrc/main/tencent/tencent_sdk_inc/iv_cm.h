/*****************************************************************************
 * Tencent is pleased to support the open source community by making IoT Video available.
 * Copyright (C) 2020 THL A29 Limited, a Tencent company. All rights reserved.
 * Licensed under the MIT License (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://opensource.org/licenses/MIT
 * Unless required by applicable law or agreed to in writing, software distributed under the License
 *is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 *or implied. See the License for the specific language governing permissions and limitations under
 *the License.
 *
 * @file    iv_cm.h
 * @brief   Description common function of sdk
 * @version v1.0.0
 *
 *****************************************************************************/

#ifndef __IV_CM_H__
#define __IV_CM_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stddef.h>
#include <stdint.h>

/******************************** Macro Definition ***************************/

#define MAX_FILE_NAME_LENGTH 128

/*define the aenc type of audio*/
typedef enum
{
    IV_CM_AENC_TYPE_PCM    = 0, /*pcm stream*/
    IV_CM_AENC_TYPE_G711A  = 1, /*g711a stream*/
    IV_CM_AENC_TYPE_G711U  = 2, /*g711u stream*/
    IV_CM_AENC_TYPE_G726   = 3, /*g726, IoT Video SDK Version 1.x.x don't support*/
    IV_CM_AENC_TYPE_AAC    = 4, /*aac stream*/
    IV_CM_AENC_TYPE_AMR    = 5, /*amr stream, IoT Video SDK Version 1.x.x don't support*/
    IV_CM_AENC_TYPE_ADPCMA = 6, /*adpcm, IoT Video SDK Version 1.x.x don't support*/

    IV_CM_AENC_TYPE_BUTT
} iv_cm_aenc_type_e;

/*define the aac type*/
typedef enum
{
    IV_CM_AAC_TYPE_NONE     = 0,
    IV_CM_AAC_TYPE_MAIN     = 1,
    IV_CM_AAC_TYPE_LC       = 2,
    IV_CM_AAC_TYPE_SSR      = 3,
    IV_CM_AAC_TYPE_LTP      = 4,
    IV_CM_AAC_TYPE_SBR      = 5,
    IV_CM_AAC_TYPE_SCALCBLE = 6,
    IV_CM_AAC_TYPE_TWINVQ   = 7,
    IV_CM_AAC_TYPE_CELP     = 8,
    IV_CM_AAC_TYPE_HVXC     = 9,

    IV_CM_AAC_TYPE_BUTT
} iv_cm_aac_type_e;

/*define the amr type*/
/*typedef enum
{
    IV_CM_AMR_FORMAT_MMS = 0,
    IV_CM_AMR_FORMAT_IF1 = 1,
    IV_CM_AMR_FORMAT_IF2 = 2,

    IV_CM_AMR_FORMAT_BUTT
} iv_cm_amr_type_e;*/

/*define the sample rate  of audio*/
typedef enum
{
    IV_CM_AENC_SAMPLE_RATE_8000  = 8000,  /* 8K samplerate*/
    IV_CM_AENC_SAMPLE_RATE_11025 = 11025, /* 11.025K samplerate*/
    IV_CM_AENC_SAMPLE_RATE_12000 = 12000, /* 12K samplerate*/
    IV_CM_AENC_SAMPLE_RATE_16000 = 16000, /* 16K samplerate*/
    IV_CM_AENC_SAMPLE_RATE_22050 = 22050, /* 22.050K samplerate*/
    IV_CM_AENC_SAMPLE_RATE_24000 = 24000, /* 24K samplerate*/
    IV_CM_AENC_SAMPLE_RATE_32000 = 32000, /* 32K samplerate*/
    IV_CM_AENC_SAMPLE_RATE_44100 = 44100, /* 44.1K samplerate*/
    IV_CM_AENC_SAMPLE_RATE_48000 = 48000, /* 48K samplerate*/
    IV_CM_AENC_SAMPLE_RATE_64000 = 64000, /* 64K samplerate*/
    IV_CM_AENC_SAMPLE_RATE_96000 = 96000, /* 96K samplerate*/

    IV_CM_AENC_SAMPLE_RATE_BUTT,
} iv_cm_aenc_sample_rate_e;

/*define the bit width  of audio*/
typedef enum
{
    IV_CM_AENC_BIT_WIDTH_8  = 0, /* 8bit width */
    IV_CM_AENC_BIT_WIDTH_16 = 1, /* 16bit width*/
    IV_CM_AENC_BIT_WIDTH_24 = 2, /* 24bit width*/

    IV_CM_AENC_BIT_WIDTH_BUTT,
} iv_cm_aenc_bit_width_e;

/*define the aenc mode  of audio*/
typedef enum
{
    IV_CM_AENC_MODE_MONO   = 0, /*mono*/
    IV_CM_AENC_MODE_STEREO = 1, /*stereo*/

    IV_CM_AENC_MODE_BUTT
} iv_cm_aenc_mode_e;

typedef enum
{
    IV_CM_FILE_TYPE_VIDEO = 0, /*video file*/
    IV_CM_FILE_TYPE_PIC   = 1  /*pircture file*/

    /*User expandable*/
} iv_cm_file_type_e;

/*Defines the struct of aenc_pack*/
typedef struct iv_cm_aenc_pack_s {
    uint8_t *pu8Addr; /*address*/
    uint32_t u32Len;  /*the stream length*/
    uint64_t u64PTS;  /*PTS*/
    uint32_t u32Seq;  /*the index number of audio pack*/
} iv_cm_aenc_pack_s;

/*Defines the struct of aenc_stream*/
typedef struct iv_cm_aenc_stream_s {
    iv_cm_aenc_pack_s *pstAencPack[8]; /*aenc stream pack attribute*/
    uint32_t u32PackCount;             /*the frame number in this stream*/
} iv_cm_aenc_stream_s;

/*Defines the stream type*/
typedef enum
{
    IV_CM_STREAM_TYPE_AUDIO = 0,
    IV_CM_STREAM_TYPE_VIDEO = 1,

    IV_CM_STREAM_TYPE_BUTT
} iv_cm_stream_type_e;

/*Defines the venc type*/
typedef enum
{
    IV_CM_VENC_TYPE_H264 = 0, /*h264*/
    IV_CM_VENC_TYPE_H265 = 1, /*h265*/
    IV_CM_VENC_TYPE_MJPG = 2, /*mjpg*/

    IV_CM_VENC_TYPE_BUTT
} iv_cm_venc_type_e;

typedef enum
{
    IV_CM_PIXEL_VARIABLE     = 0, /*可变自适应*/
    IV_CM_PIXEL_240x320      = 1, /*240x320*/
    IV_CM_PIXEL_320x240      = 2, /*320x240*/
    IV_CM_PIXEL_480x352      = 3, /*480x352*/
    IV_CM_PIXEL_480x640      = 4, /*480x640*/

    IV_CM_PIXEL_TYPE_BUTT
} iv_cm_pixel_type_e;

typedef struct
{
    uint32_t width;
    uint32_t height;
} iv_cm_mjpg_pixel_s;
typedef enum
{
    IV_CM_FRAME_TYPE_I = 0,
    IV_CM_FRAME_TYPE_P = 1,
    // IV_CM_FRAME_TYPE_B = 2, // not support yet

    IV_CM_FRAME_TYPE_BUTT
} iv_cm_frame_type_e;

/*Defines the struct of venc pack*/
typedef struct iv_cm_venc_pack_s {
    uint8_t *pu8Addr;              /*the address of stream*/
    uint32_t u32Len;               /*the length of stream*/
    uint64_t u64PTS;               /*PTS*/
    iv_cm_frame_type_e eFrameType; /*frame type*/
    uint32_t u32Seq;               /*the index number of video pack*/
} iv_cm_venc_pack_s;

/*Defines the struct of venc stream*/
typedef struct iv_cm_venc_stream_s {
    iv_cm_venc_pack_s *pstVencPack[8]; /*stream pack attribute*/
    uint32_t u32PackCount;             /*the pack number in this stream*/
} iv_cm_venc_stream_s;

/*Defines the struct of avenc stream*/
typedef struct iv_cm_avenc_stream_s {
    iv_cm_aenc_stream_s stAencStream; /*aenc stream pack*/
    iv_cm_venc_stream_s stVencStream; /*venc stream*/
} iv_cm_avenc_stream_s;

/*Defines the av infomation of av data*/
typedef struct iv_cm_av_data_info_s {
    iv_cm_aenc_type_e eAudioType;              /*audio codec type*/
    iv_cm_aac_type_e u32AudioCodecOption;      /*audio codec subtype*/
    iv_cm_aenc_mode_e eAudioMode;              /*audio mode*/
    iv_cm_aenc_bit_width_e eAudioBitWidth;     /*audio bit width,just support 16 bit*/
    iv_cm_aenc_sample_rate_e eAudioSampleRate; /*audio sample rate*/
    uint32_t u32SampleNumPerFrame;             /*audio sample num per Frame*/

    iv_cm_venc_type_e eVideoType; /*video type(0:h264, 1:h265)*/
    uint32_t u32VideoWidth;       /*video width*/
    uint32_t u32VideoHeight;      /*video height*/
    uint32_t u32Framerate;        /*frame rate*/

    uint32_t u32Reserve[4]; /*reserve*/
} iv_cm_av_data_info_s;

/*the Encryption type of wifi*/
typedef enum
{
    IV_CM_WIFI_ENC_TYPE_OPEN = 0, /*open wifi*/
    IV_CM_WIFI_ENC_TYPE_WEP  = 1, /*wep wifi*/
    IV_CM_WIFI_ENC_TYPE_WPA  = 2, /*wpa wifi*/

    IV_CM_WIFI_ENC_TYPE_BUTT
} iv_cm_wifi_enc_type_e;

typedef struct {
    uint8_t *buf;                            // buffer address
    size_t size;                             // buffer size
    void (*buf_free_fn)(uint8_t *, size_t);  // buffer free callback
} iv_cm_memory_s;

typedef struct {
    uint32_t type;          // custom type value in request
    uint64_t begin_time_s;  // start timestamp in second of playback, unix timestampe
    uint64_t end_time_s;    // end timestamp in second of playback, unix timestampe
} iv_cm_time_fragment_s;

typedef struct {
    uint32_t count;                      // file count of the day
    iv_cm_time_fragment_s file_list[0];  // play back file array
} iv_cm_pb_list_s;

// query record by month
typedef struct {
    uint32_t year;
    uint32_t month;
    uint32_t type;  // custom type value in request
    uint32_t day;   // Use each bit for one day. For example day=0010 0000 1000 0000=8320 means
                    // 8/14 day have playback data.
} iv_cm_query_rd_by_month_s;

typedef struct {
    iv_cm_time_fragment_s time_fragment;
    void (*free_fn)(void *ptr);  // memory free callback
    iv_cm_pb_list_s *rd_array;
} iv_cm_query_rd_by_day_s;

typedef struct {
    iv_cm_file_type_e file_type;           // file type
    char file_name[MAX_FILE_NAME_LENGTH];  // file name
    uint32_t file_size;                    // file size, unit Byte
    uint32_t begin_time_s;                 // start timestamp in second of playback, unix timestampe
    uint32_t end_time_s;                   // end timestamp in second of playback, unix timestampe
    iv_cm_memory_s extra_info;             // user define information, json format
} iv_cm_file_info_s;

typedef struct {
    uint32_t count;                  // file count
    iv_cm_file_info_s file_list[0];  // file list
} iv_cm_file_list_s;

typedef struct {
    iv_cm_time_fragment_s time_fragment;
    void (*free_fn)(void *ptr);  // memory free callback
    iv_cm_file_list_s *file_array;
} iv_cm_query_file_list_s;

typedef struct {
    char file_name[MAX_FILE_NAME_LENGTH];  // name of downloaded file
    int file_offset;                       // offset of downloaded file
} iv_cm_download_param_s;

typedef struct {
    uint64_t seek_time_ms;
} iv_cm_pb_seek_s;

typedef struct {
    uint32_t speed;  // 0:正常播放 1:表示只发I帧, 2:表示I帧取2发一, 3:表示I帧取3发1
} iv_cm_pb_ff_s;

typedef struct {
    uint32_t time_ms;  // 表示pts的间隔
} iv_cm_pb_speed_s;

typedef struct {
    uint64_t begin_time_s;  // end timestamp of rewind
    uint64_t end_time_s;    // start timestamp of rewind
} iv_cm_pb_rewind_s;
//--------------------------------------------
/***************************** Macro Definition end **************************/

#ifdef __cplusplus
}
#endif

#endif /* IOT_VIDEO_COMMON_H_ */