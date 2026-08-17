/** Copyright (C) by Jabsco Company
*
* @File Name    : gb_mpeg_ps.cpp
* @Created Time : 2024-05-06
* @Version      : 1.0
* @Author       :
* @Description  : 国标 ps 打包
*/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "mpeg-util.h"
//#include "mpeg-crc32.h"

#include "gb_mpeg_ps.h"
#include "gb_common.h"

#define MAX_PES_HEADER  1024    // pack_header + system_header + psm
#define MAX_PES_PACKET  0xffc6  // 65408 海康最大 pes 包大小
#define VIDEO_BUFF_SIZE_BOUND 128  // video 编码 buff 尺寸，单位 1024 byte
#define AUDIO_BUFF_SIZE_BOUND 16    // audio 编码 buff 尺寸, 单位 128 byte

#define MAX_VIDEO_BUFF VIDEO_BUFF_SIZE_BOUND * 1024
#define MAX_AUDIO_BUFF AUDIO_BUFF_SIZE_BOUND * 128

GbMpegPsEncode::GbMpegPsEncode()
{
    PsReset();
}

/*
*  H264 H265 ps 打包
*
* @param[frame] 待打包视频帧
* @param[frame_size] 视频流帧大小
* @param[timestamp] 视频帧时间戳
* @param[stream_id] 流 ID，由 PsAddStream 返回
* @param[is_key] 是否是关键帧，关键帧必须带有编码信息
* @param[vps] H265
* @param[vpd_size] vps 大小
* @param[sps] H265/H264
* @param[sps_size] sps 大小
* @param[pps] H265/H264
* @param[pps_size] pps 大小
*
* @return 成功返回 SUCCESS 失败返回 FAILURE
*/
int GbMpegPsEncode::PsPackH26x(const char *frame, uint32_t frame_size, double timestamp, uint32_t stream_id, bool is_key,
                             const char *vps, int vps_size, const char *sps, int sps_size, const char *pps, int pps_size)
{
    if (frame == NULL || frame_size == 0) {
        GB_ERR("frame error\n");
        return FAILURE;
    }

    PsStreamHeader *stream_head = FindStream(stream_id);
    if (stream_head == NULL || !IsH26x(stream_head->stream_type)) {
        GB_ERR("stream id[%u] or stream type error\n", stream_id);
        return FAILURE;
    }

    if (is_key) {
        if (sps == NULL || sps_size <= 0 || pps == NULL || pps_size <= 0) {
            GB_ERR("sps[%p] size[%d] pps[%p] size[%d], error\n", sps, sps_size, pps, pps_size);
            return FAILURE;
        }

        if (stream_head->stream_type == STREAM_TYPE_H265 && (vps == NULL || vps_size <= 0)) {
            GB_ERR("vps[%p] size[%d], error\n", vps, vps_size);
            return FAILURE;
        }
    }

    //GB_INFO("frame_size:%u timestamp:%lf is_key:%d\n", frame_size, timestamp, is_key);

    uint8_t pack[MAX_VIDEO_BUFF] = {0};
    uint32_t pack_size = 0;

    // ps head
    system_clock_base_ = timestamp * 90000;
    system_clock_extension_ = 0;
    pack_size += PackHeaderWrite(pack);

    if(is_key) {
        // ps system head
        pack_size += SystemHeaderWrite(pack + pack_size);
        // ps system map
        pack_size += PsmWrite(pack + pack_size);
    }

    // 视频 pes 的 pts 时间戳和 ps head 的时间保持一致，只有第一个 pes 包才有 pts，生成完一次后清零
    uint64_t pts = system_clock_base_;
    if (is_key) { // 关键帧前几个 pes 是编码信息
        if (stream_head->stream_type == STREAM_TYPE_H265) { // vps
            pack_size += PesWriteData((uint8_t *)vps, vps_size, pack + pack_size, stream_id, 1, pts);
            pts = 0;
        }

        // sps
        pack_size += PesWriteData((uint8_t *)sps, sps_size, pack + pack_size, stream_id, 1, pts);
        pts = 0;

        // pps
        pack_size += PesWriteData((uint8_t *)pps, pps_size, pack + pack_size, stream_id, 1, pts);
    }

    uint32_t frame_remain_size = frame_size;
    uint8_t data_alignment = 1;
    bool is_first = true;

    while(frame_remain_size > 0) {
        uint8_t *pes = pack + pack_size;
        uint8_t *p = NULL;
        // 判断是否有内存再放一个 pes 包
        if (MAX_VIDEO_BUFF - pack_size > MAX_PES_PACKET + 6) {
            p = pes + PesWriteHeader(pes, stream_id, data_alignment, pts);

            // 判断还能拷贝多少内容
            uint32_t size = 0;
            uint32_t remain = MAX_VIDEO_BUFF - (p - pack);
            //DBG("frame_remain_size:%u remain:%u\n", frame_remain_size, remain);
            size = frame_remain_size > remain ? remain : frame_remain_size;
            if (size > MAX_PES_PACKET) {
                size = MAX_PES_PACKET;
            }
            //DBG("size:%u pack_size:%u\n", size, pack_size);
            nbo_w16(pes + 4, (uint16_t)((p - pes - 6) + size));
            memcpy(p, frame, size);
            frame_remain_size -= size;
            frame += size;
            pack_size += p - pes + size;
        } else {
            bool is_end = (frame_remain_size <= 0);
            // 处理数据
            //GB_DBG("pack :%p pack_size: %d\n", pack, pack_size);
            int ret = OnPsStream(pack, pack_size, true, is_key, is_first, is_end);
            if (ret != SUCCESS) {
                GB_ERR("on ps video stream error\n");
                return FAILURE;
            }
            pack_size = 0;
            data_alignment = 1;
            is_first = false;
        }
    }

    if (pack_size != 0) {
        //GB_DBG("pack :%p pack_size: %d\n", pack, pack_size);
        int ret = OnPsStream(pack, pack_size, true, is_key, is_first, true);
        if (ret != SUCCESS) {
            GB_ERR("on ps video stream error\n");
            return FAILURE;
        }
        pack_size = 0;
    }

    return SUCCESS;
}

void GbMpegPsEncode::PsReset()
{
    program_mux_rate_ = 59572;
    rate_bound_ = 59572;
    audio_bound_ = 0;
    fixed_flag_ = 0;
    CSPS_flag_ = 0;
    system_audio_lock_flag_ = 1;
    system_video_lock_flag_ = 1;
    vedio_bound_ = 0;
    packet_rate_restriction_flag_ = 0;

    bzero(stream_, sizeof(stream_));
    stream_count_ = 0;

    program_stream_map_version_ = 0;
}

int GbMpegPsEncode::PsPackAudio(const char *frame, uint32_t frame_size, double timestamp, uint32_t stream_id)
{
    if (frame == NULL || frame_size == 0) {
        GB_ERR("frame error\n");
        return FAILURE;
    }

    PsStreamHeader *stream_head = FindStream(stream_id);
    if (stream_head == NULL || !IsAudio(stream_head->stream_type)) {
        GB_ERR("stream id[%u] or stream type error\n", stream_id);
        return FAILURE;
    }
    // 音频帧就是 pes 直接加数据，不需要 ps head
    uint8_t pack[MAX_AUDIO_BUFF] = {0};
    uint32_t pack_size = 0;

    uint64_t pts = timestamp * 90000; // 只有 33 位有效

    pack_size += PesWriteHeader(pack, stream_id, 1, pts);
    // 一般音频帧打到一个 pes 包里面，而且音频帧也不会太大，太大了一定是有问题
    if (frame_size > MAX_AUDIO_BUFF - pack_size) {
        GB_ERR("frame size[%d] out of range\n", frame_size);
        return FAILURE;
    }

    nbo_w16(pack + 4, (uint16_t)((pack_size - 6) + frame_size));
    memcpy(pack + pack_size, frame, frame_size);
    pack_size += frame_size;
    int ret = OnPsStream(pack, pack_size, false, false, true, true);
    if (ret != SUCCESS) {
        GB_ERR("on ps audio stream error\n");
        return FAILURE;
    }

    return SUCCESS;
}

/*添加一个节目流，成功返回节目流 ID , 失败返回 FAILURE*/
int GbMpegPsEncode::PsAddStream(StreamType stream_type, EncodeInfo &encode_info)
{
    if (stream_count_ >= PS_STREAM_MAX) {
        GB_ERR("number of streams reaches maximum:%u\n", stream_count_);
        return FAILURE;
    }

    PsStreamHeader &stream = stream_[stream_count_];
    if (IsH26x(stream_type)) {
        if (vedio_bound_ >= 15) {
            GB_ERR("vedio bound has reached the maximum\n");
            return FAILURE;
        }
        stream.stream_id = PES_SID_VIDEO + vedio_bound_;
        vedio_bound_++;
        stream.buffer_bound_scale = 1;
        stream.buffer_size_bound = VIDEO_BUFF_SIZE_BOUND;
        stream.stream_type = stream_type;
        stream.encode_info = encode_info;
    } else if (IsAudio(stream_type)) {
        if (audio_bound_ >= 31) {
            GB_ERR("audio bound has reached the maximum\n");
            return FAILURE;
        }

        stream.stream_id = PES_SID_AUDIO + audio_bound_;
        audio_bound_++;
        stream.buffer_bound_scale = 0;
        stream.buffer_size_bound = AUDIO_BUFF_SIZE_BOUND;
        stream.stream_type = stream_type;
        stream.encode_info = encode_info;
    } else {
        GB_ERR("stream type[%d] nukown\n", stream_type);
        return FAILURE;
    }

    stream_count_++;

    return stream.stream_id;
}

/*通过 ID 查找流*/
PsStreamHeader *GbMpegPsEncode::FindStream(uint32_t id)
{
    for (uint32_t i = 0; i < stream_count_; ++i) {
        if (stream_[i].stream_id == id)
            return stream_ + i;
    }

    return NULL;
}

/*查询流类型是否是 H264/H265*/
bool GbMpegPsEncode::IsH26x(StreamType stream_type)
{
    switch (stream_type)
    {
    case STREAM_TYPE_H264:
    case STREAM_TYPE_H265:
        return true;

    default:
        return false;
    }
}
/*查询流类型是否是音频*/
bool GbMpegPsEncode::IsAudio(StreamType stream_type)
{
    switch (stream_type)
    {
    case STREAM_TYPE_G711A:
    case STREAM_TYPE_G711U:
        return true;

    default:
        return false;
    }
}

// 写入包头，返回写入的字节数
size_t GbMpegPsEncode::PackHeaderWrite(uint8_t *data)
{
    // pack_start_code
    nbo_w32(data, 0x000001BA);

    // 33-system_clock_reference_base + 9-system_clock_reference_extension
    // '01xxx1xx xxxxxxxx xxxxx1xx xxxxxxxx xxxxx1xx xxxxxxx1'
    data[4] = 0x44 | (((system_clock_base_ >> 30) & 0x07) << 3) | ((system_clock_base_ >> 28) & 0x03);
    data[5] = ((system_clock_base_ >> 20) & 0xFF);
    data[6] = 0x04 | (((system_clock_base_ >> 15) & 0x1F) << 3) | ((system_clock_base_ >> 13) & 0x03);
    data[7] = ((system_clock_base_ >> 5) & 0xFF);
    data[8] = 0x04 | ((system_clock_base_ & 0x1F) << 3) | ((system_clock_extension_ >> 7) & 0x03);
    data[9] = 0x01 | ((system_clock_extension_ & 0x7F) << 1);

    // program_mux_rate
    // 'xxxxxxxx xxxxxxxx xxxxxx11'
    data[10] = (uint8_t)(program_mux_rate_ >> 14);
    data[11] = (uint8_t)(program_mux_rate_ >> 6);
    data[12] = (uint8_t)(0x03 | ((program_mux_rate_ & 0x3F) << 2));

    // stuffing length
    // '00000xxx'
    data[13] = 0xFa;

    // 四字节对齐，填充两个字节
    data[14] = 0xFF;
    data[15] = 0xFF;
    return 16;
}

size_t GbMpegPsEncode::SystemHeaderWrite(uint8_t *data)
{
    size_t i, j;

    // system_header_start_code
    nbo_w32(data, 0x000001BB);

    // rate_bound
    // 1xxxxxxx xxxxxxxx xxxxxxx1
    data[6] = 0x80 | ((rate_bound_ >> 15) & 0x7F);
    data[7] = (rate_bound_ >> 7) & 0xFF;
    data[8] = 0x01 | ((rate_bound_ & 0x7F) << 1);

    // 6-audio_bound + 1-fixed_flag + 1-CSPS_flag
    data[9] = ((audio_bound_ & 0x3F) << 2) | ((fixed_flag_ & 0x01) << 1) | (CSPS_flag_ & 0x01);

    // 1-system_audio_lock_flag + 1-system_video_lock_flag + 1-maker + 5-video_bound
    data[10] = 0x20 | ((system_audio_lock_flag_ & 0x01) << 7) | ((system_video_lock_flag_ & 0x01) << 6) | (vedio_bound_ & 0x1F);

    // 1-packet_rate_restriction_flag + 7-reserved
    data[11] = 0x7F | ((packet_rate_restriction_flag_ & 0x01) << 7);

    i = 12;
    for (j = 0; j < stream_count_; j++) {
        data[i++] = (uint8_t)stream_[j].stream_id;
        // '11' + 1-P-STD_buffer_bound_scale + 13-P-STD_buffer_size_bound
        // '11xxxxxx xxxxxxxx'
        data[i++] = 0xC0 | ((stream_[j].buffer_bound_scale & 0x01) << 5) | ((stream_[j].buffer_size_bound >> 8) & 0x1F);
        data[i++] = stream_[j].buffer_size_bound & 0xFF;
    }

    // header length
    nbo_w16(data + 4, (uint16_t)(i - 6));
    return i;
}

/*写入自定义节目流描述信息，返回写入的字节数*/
uint16_t GbMpegPsEncode::ProgramStreamInfoWrite(uint8_t* data)
{
    // 这里采用 0x50 + 长度(1 byte) + 公司名(6 byte) + 0x52 + program_stream_map_version_(2 byte) +  0x54 + deviceid(6 byte) + 填充(2 byte)
    uint16_t version = program_stream_map_version_ * 128;
    uint64_t devid = GetDevid(); // devid 取低 6 字节

    data[0] = 0x50;
    data[1] = 0x12;
    memcpy(data + 2, "jabsco", 6);
    data[8] = 0x52;
    nbo_w16(data + 9, version);

    data[11] = 0x54;
    data[12] = (uint8_t)(devid & 0xff);
    data[13] = (uint8_t)((devid >> 8) & 0xff);
    data[14] = (uint8_t)((devid >> 16) & 0xff);
    data[15] = (uint8_t)((devid >> 24) & 0xff);
    data[16] = (uint8_t)((devid >> 32) & 0xff);
    data[17] = (uint8_t)((devid >> 40) & 0xff);
    data[18] = 0xff;
    data[19] = 0xff;

    return 20;
}

/*写入自定义视频描述信息*/
uint16_t GbMpegPsEncode::VideoInfoWrite(uint8_t* data, EncodeInfo &encode_info)
{
    // 0x56 + 长度(1 byte) + bps(4 byte) + fps(4 byte) + width(4 byte) + height(4 byte) + 填充(2 byte)
    data[0] = 0x56;
    data[1] = 0x12;
    nbo_w32(data + 2, encode_info.video_info.bps);
    nbo_w32(data + 6, encode_info.video_info.bps);
    nbo_w32(data + 10, encode_info.video_info.bps);
    nbo_w32(data + 14, encode_info.video_info.bps);
    data[18] = 0xff;
    data[19] = 0xff;

    return 20;
}

/*写入自定义音频描述信息*/
uint16_t GbMpegPsEncode::AudioInfoWrite(uint8_t* data, EncodeInfo &encode_info)
{
    // 0x58 + + 长度(1 byte) + sampel_rate(4 byte) + bit_width(4 byte) + 填充(2 byte)
    data[0] = 0x58;
    data[1] = 0x0a;
    nbo_w32(data + 2, encode_info.audio_info.sample_rata);
    nbo_w32(data + 6, encode_info.audio_info.bit_width);
    data[10] = 0xff;
    data[11] = 0xff;

    return 12;
}

size_t GbMpegPsEncode::PsmWrite(uint8_t *data)
{
    // Table 2-41 - Program stream map(p79)
    size_t i,j;
    uint16_t extlen;
    unsigned int crc;

    nbo_w32(data, 0x00000100);
    data[3] = 0XBC;

    // current_next_indicator '1'
    // single_extension_stream_flag '1'
    // reserved '0'
    // program_stream_map_version 'xxxxx'
    program_stream_map_version_ += 2; // 每一个包版本加二
    data[6] = 0xc0 | (program_stream_map_version_ & 0x1F);
    // reserved '0000000'
    // marker_bit '1'
    data[7] = 0x01;

    extlen = 0;
    extlen += (uint16_t)ProgramStreamInfoWrite(data + 10);

    // program_stream_info_length 16-bits
    nbo_w16(data + 8, extlen); // program_stream_info_length = 0

    // elementary_stream_map_length 16-bits
    //nbo_w16(data+10+extlen, psm->stream_count*4);

    j = 12 + extlen;
    for(i = 0; i < stream_count_; i++) {
        // stream_type:8
        data[j++] = stream_[i].stream_type;
        // elementary_stream_id:8
        data[j++] = stream_[i].stream_id;
        // descriptor()
        uint16_t len = 0;
        if (IsH26x(stream_[i].stream_type)) {
            len = VideoInfoWrite(data+j+2, stream_[i].encode_info);
        } else if (IsAudio(stream_[i].stream_type)) {
            len = AudioInfoWrite(data+j+2, stream_[i].encode_info);
        }

        // elementary_stream_info_length:16
        nbo_w16(data+j, len);

        j += 2 + len;
    }

    // elementary_stream_map_length 16-bits
    nbo_w16(data + 10 + extlen, (uint16_t)(j - 12 - extlen));
    // program_stream_map_length:16
    nbo_w16(data + 4, (uint16_t)(j-6+4)); // 4-bytes crc32

    // crc32
    crc = mpeg_crc32(0xffffffff, data, (uint32_t)j);
    data[j+3] = (uint8_t)((crc >> 24) & 0xFF);
    data[j+2] = (uint8_t)((crc >> 16) & 0xFF);
    data[j+1] = (uint8_t)((crc >> 8) & 0xFF);
    data[j+0] = (uint8_t)(crc & 0xFF);

    return j+4;
}

/*
*  写 pes 数据，包括写头 + 数据，不一定会写入所有的 src_size，
*  如果判断超过 MAX_PES_PACKET，会留下一部分不写，具体写入多少以返回值为准
*
* @param[src] 源数据，要写入的数据
* @param[src_size] 要写入数据大小
* @param[dst] 目标 buff，ps 流 buff
* @param[other] 其它参数是 ps 头要求的数据，看对应函数注释
*
* @return 写入数据的大小
*/
size_t GbMpegPsEncode::PesWriteData(uint8_t *src, uint16_t src_size, uint8_t *dst, uint8_t stream_id, uint8_t data_alignment, uint64_t pts)
{
    uint8_t *p = NULL;

    p = dst + PesWriteHeader(dst, stream_id, data_alignment, pts);

    if (p - dst - 6 + src_size > MAX_PES_PACKET) {
        src_size = MAX_PES_PACKET - (p - dst - 6);
    }

    memcpy(p, src, src_size);
    p += src_size;
    nbo_w16(dst + 4, (uint16_t)((p - dst - 6)));

    return p - dst;
}

/*
*  写 pes 头
*
* @param[data] buff 指针
* @param[stream_id] 流 ID
* @param[data_alignment] 告知解码器当前包中的有效负载数据是否与某个特定的对齐边界相关联，
*                        海康的做法是:  1. 每一个独立帧(sps、I、P ...)起始都会置 1
*                                       2. 如果数据超过了缓冲区要进行拆包，第一个包置 1，第二个包置 0
* @param[pts] 为 0 表示不设置; pts 显示时间戳，告知编码器在什么时候解码这帧数据，一般只有一帧的起始才需要，比如 I 帧，是 vps/ 拥有
*
* @return 写入数据的大小
*/
size_t GbMpegPsEncode::PesWriteHeader(uint8_t *data, uint8_t stream_id, uint8_t data_alignment, uint64_t pts)
{
    uint8_t len = 0;
    uint8_t flags = 0x00;
    uint8_t *p = NULL;

    // packet_start_code_prefix 0x000001
    data[0] = 0x00;
    data[1] = 0x00;
    data[2] = 0x01;
    data[3] = stream_id;

    // '10' 标志位
    // PES_scrambling_control '00'  非加密
    // PES_priority '1' 海康的做法，默认都是高优先级
    // data_alignment_indicator '1'
    // copyright '0'
    // original_or_copy '0'
    data[6] = 0x88;
    if(data_alignment)
        data[6] |= 0x04;
    //if (IDR | subtitle | raw data)
        //data[6] |= 0x04;

    // PTS_DTS_flag 'xx'
    // ESCR_flag '0'
    // ES_rate_flag '0'
    // DSM_trick_mode_flag '0'
    // additional_copy_info_flag '0'
    // PES_CRC_flag '0'
    // PES_extension_flag '0'
    if (pts != 0) {
        flags |= 0x80;
        len += 5;
    }
    data[7] = flags;
    // PES_header_data_length : 8

    p = data + 9;
    if(flags & 0x80)
    {
        *p++ = ((flags >> 2) & 0x30)/* 0011/0010 */ | (((pts >> 30) & 0x07) << 1) /* PTS 30-32 */ | 0x01 /* marker_bit */;
        *p++ = (pts >> 22) & 0xFF; /* PTS 22-29 */
        *p++ = ((pts >> 14) & 0xFE) /* PTS 15-21 */ | 0x01 /* marker_bit */;
        *p++ = (pts >> 7) & 0xFF; /* PTS 7-14 */
        *p++ = ((pts << 1) & 0xFE) /* PTS 0-6 */ | 0x01 /* marker_bit */;
    }

    *p++ = 0xff;  // 填充两个字节
    *p++ = 0xfe;
    len += 2;

    data[8] = len;

    return p - data;
}