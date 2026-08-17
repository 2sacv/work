/** Copyright (C) by Jabsco Company
*
* @File Name    : gb_mpeg_ps.h
* @Created Time : 2024-05-06
* @Version      : 1.0
* @Author       :
* @Description  : 国标 ps 打包
*/
#ifndef GB_MPEG_PS_H_
#define GB_MPEG_PS_H_

#include <stdint.h>
#include <time.h>
#include <stddef.h>

#include "mpeg-proto.h"

enum PsStreamId{
    STREAM_ID_AUDIO = PES_SID_VIDEO,
    STREAM_ID_VIDEO = PES_SID_AUDIO,
};

enum StreamType {
    STREAM_TYPE_UNKNOWN = PSI_STREAM_RESERVED,
    STREAM_TYPE_H264 = PSI_STREAM_H264,
    STREAM_TYPE_H265 = PSI_STREAM_H265,

    STREAM_TYPE_G711A = PSI_STREAM_AUDIO_G711A,
    STREAM_TYPE_G711U = PSI_STREAM_AUDIO_G711U,
};

#define PS_STREAM_MAX 2  // ps 中流的个数，最大视频 16 + 音频 32 = 48; 我们基本就一路音频一路视频，这里给 2

union EncodeInfo{
    struct {
        int bps;
        int fps;
        int width;
        int height;
    } video_info;
    struct {
        int sample_rata;
        int bit_width;
    } audio_info;
};

struct PsStreamHeader
{
    uint32_t stream_id : 8; // 视频流 0xe0-0xef 音频流 0xc0-0xdf
    uint32_t buffer_bound_scale : 1; // 音频填 0 视频填 1
    uint32_t buffer_size_bound : 13; // 目标解码器缓冲区限制 视频流时单位 1024 byte 默认 128; 音频流时 单位 128 byte 默认 12
    StreamType stream_type; // 流类型，H264 H265 G711 ...
    EncodeInfo encode_info;
};

class GbMpegPsEncode {
public:
    GbMpegPsEncode();
    virtual ~GbMpegPsEncode() {};


    int PsPackH26x(const char *frame, uint32_t frame_size, double timestamp, uint32_t stream_id, bool is_key = false,
                             const char *vps = NULL, int vpd_size = 0, const char *sps = NULL, int sps_size = 0, const char *pps = NULL, int pps_size = 0);
    int PsPackAudio(const char *frame, uint32_t frame_size, double timestamp, uint32_t stream_id);

    int PsAddStream(StreamType stream_type, EncodeInfo &encode_info);

    void PsReset();

    /*
    *  ps 流处理，纯虚函数，用户需要实现这个函数来处理 ps 流
    *
    * @param[ps] ps 流指针
    * @param[ps_size] ps 流大小
    * @param[is_video] 是否是视频帧
    * @param[is_video_key] 是否是视频关键帧
    * @param[is_first] 是否是本次的第一段数据
    * @param[is_end] 是否是最后一段数据
    *
    * @return 成功返回 SUCCESS 失败返回 FAILURE
    */
    virtual int OnPsStream(uint8_t *ps, uint32_t ps_size, bool is_video, bool is_video_key, bool is_first, bool is_end) = 0;

    // 获取 devid ，用户可以实现这个函数，不实现就用默认值
    virtual uint64_t GetDevid() {return 0xffffffffffffffff;};

private:
    PsStreamHeader *FindStream(uint32_t id);
    bool IsH26x(StreamType stream_type);
    bool IsAudio(StreamType stream_type);

    size_t PackHeaderWrite(uint8_t *data);
    size_t SystemHeaderWrite(uint8_t *data);
    size_t PsmWrite(uint8_t *data);
    size_t PesWriteHeader(uint8_t* data, uint8_t stream_id, uint8_t data_alignment, uint64_t pts);
    inline size_t PesWriteData(uint8_t *src, uint16_t src_size, uint8_t *dst, uint8_t stream_id, uint8_t data_alignment, uint64_t pts);

    uint16_t ProgramStreamInfoWrite(uint8_t* data);
    uint16_t VideoInfoWrite(uint8_t* data, EncodeInfo &encode_info);
    uint16_t AudioInfoWrite(uint8_t* data, EncodeInfo &encode_info);

private:
    // PS Header
    uint64_t system_clock_base_ : 33; // 基础系统时钟，这一帧的时间
    uint64_t system_clock_extension_ : 9; // 系统时钟扩展字段，大部分系统不需要这么精确; 一般填 0
    uint64_t program_mux_rate_ : 22; // 节目流速率，单位 50 byte/s; 默认 59572 = 2.84 M/s

    // PS System Header
    uint64_t rate_bound_ : 22; // 节目流速率限制，大于或者等于 program_mux_rate; 默认 59572 = 2.84 M/s
    uint64_t audio_bound_ : 6; // 0~32 节目流中音频流的最大数目; 默认 0
    uint64_t fixed_flag_ : 1; // 1:定码率; 0:变码率; 默认 0
    uint64_t CSPS_flag_ : 1; // 1:满足文档中对音视频流的一些限制; 默认 0
    uint64_t system_audio_lock_flag_ : 1; // 1:表示采样率和 system_clock_base 存在特定的比例关系; 默认 1
    uint64_t system_video_lock_flag_ : 1; // 1:表示频时间基和系统目标解码器的系统时钟频率之间存在特定的常量比率关系; 默认 1
    uint64_t vedio_bound_ : 5; // 0~16 节目流中视频流的最大数目; 默认 0
    uint64_t packet_rate_restriction_flag_ : 1; // 分组速率限制标记字段，CSPS 置 0 这个标志无意义; 默认 0

    PsStreamHeader stream_[PS_STREAM_MAX];
    uint32_t stream_count_;
    // PS System Map
    uint32_t program_stream_map_version_ : 5; // system map 的版本号，内容改变就需要增加，内部控制，外部不需要管

    // pes
    uint32_t PES_priority_ : 1;
};

#endif