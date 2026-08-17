/** Copyright (C) by Jabsco Company
*
* @File Name    : gb_rtp.cpp
* @Created Time : 2024-05-06
* @Version      : 1.0
* @Author       :
* @Description  : 国标 rtp 打包,解包
*/
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "mpeg-util.h"

#include "gb_rtp.h"
#include "gb_common.h"

static inline uint16_t rtp_read_uint16(const uint8_t* ptr)
{
    return (((uint16_t)ptr[0]) << 8) | ptr[1];
}

static inline uint32_t rtp_read_uint32(const uint8_t* ptr)
{
    return (((uint32_t)ptr[0]) << 24) | (((uint32_t)ptr[1]) << 16) | (((uint32_t)ptr[2]) << 8) | ptr[3];
}

/*
*  rtp 打包，每打完一个包都会调用 OnRtpPackage 处理包
*
* @param[stream] 要打包的流
* @param[stream_size] 流大小
* @param[is_video] 是否是视频帧
* @param[is_video_key] 是否是视频关键帧
* @param[is_first] 是否是第一帧
* @param[is_end] 是否是这次数据流的最后一个包
* @param[payload] 负载类型
*
* @return 成功返回 SUCCESS, 失败返回 FAILURE，OnRtpPackage 返回失败，该函数也会返回失败
*/
int GbRtpPack::RtpPack(const char *stream, uint32_t stream_size, bool is_video, bool is_video_key, bool is_first, bool is_end, RtpPayload payload)
{
    uint16_t mtu = GetMtu();

    if (stream == NULL || mtu > 1500 || mtu < 200) {
        GB_ERR("stream[%p] || mtu[%u] error\n", stream, mtu);
        return FAILURE;
    }

    rtp_header_t header = {.v = 0x02, .p = 0, .x = 0, .cc = 0, .m = 0, .pt = payload};
    uint8_t package[1500] = {0};  // mtu 最大不会超过 1500
    // mtu 包含 IP(20) 头和 TCP(20)/UDP(8) 头，另外再留一部分余量，再减去固定头大小
    uint16_t max_size = mtu - 86 - 12;
    uint16_t package_size = 0;
    uint32_t read_size = 0;
    int ret = 0;

    bool is_rtp_first = true;
    while (read_size < stream_size) {
        // 填写固定头，最前两个字节预留给包长度
        package[2] = (uint8_t)((header.v << 6) | (header.p << 5) | (header.x << 4) | header.cc);
        package[3] = (uint8_t)((header.m << 7) | header.pt);
        package[4] = (uint8_t)(seq_ >> 8);
        package[5] = (uint8_t)(seq_ & 0xFF);
        nbo_w32(package + 6, time_stamp_);
        nbo_w32(package + 10, ssrc_);

        package_size += 2 + 12; // rtp 固定头部是 12 字节

        if (stream_size - read_size > max_size) {
            memcpy(package + package_size, stream + read_size, max_size);
            read_size += max_size;
            package_size += max_size;
        } else {
            uint16_t read_len = stream_size - read_size;
            memcpy(package + package_size, stream + read_size, read_len);
            read_size += read_len;
            package_size += read_len;
        }
        nbo_w16(package, package_size - 2); // 包头写入包长度，TCP 要用到，UDP 跳过
        seq_++; // 每一个 rtp 包序列号要加一

        bool is_repack_end = (read_size >= stream_size);
        ret = OnRtpPackage(package, package_size, is_video, is_video_key, is_first && is_rtp_first, is_end && is_repack_end);
        if (ret < 0) {
            GB_ERR("OnRtpPackage error\n");
            return FAILURE;
        }
        is_rtp_first = false;
        package_size = 0;
    }

    if (is_video && is_end)
        time_stamp_ += time_step_; // 发完一个视频包, 时间戳需要往上加, 音频用视频的时间戳，不需要加

    return SUCCESS;
}

/*
*  rtp 重打包，在原始 rtp 包上面填入自己的 ssrc timestamp seq , 每打完一个包都会调用 OnRtpRepackage 处理包
*
* @param[rtp_package] rtp 包，最开始两个字节是记录长度
* @param[package_size] rtp 包大小
* @param[is_video] 是否是视频帧
* @param[is_end] 是否是当前帧的最后一个 rtp 包
*
* @return 成功返回 SUCCESS, 失败返回 FAILURE，OnRtpRepackage 返回失败，该函数也会返回失败
*/
int GbRtpRepack::RtpRepack(uint8_t *rtp_package, uint16_t package_size, bool is_video, bool is_end)
{
    if (rtp_package == NULL || package_size < 14) {
        GB_ERR("repack error: %p %u\n", rtp_package, package_size);
        return FAILURE;
    }

    rtp_package[4] = (uint8_t)(seq_ >> 8);
    rtp_package[5] = (uint8_t)(seq_ & 0xFF);
    nbo_w32((uint8_t *)rtp_package + 6, time_stamp_);
    nbo_w32((uint8_t *)rtp_package + 10, ssrc_);

    seq_++;

    if (is_video && is_end)
        time_stamp_ += time_step_;

    return OnRtpRepackage((const char *)rtp_package, package_size);
}

/*
*  基于 TCP 的 RTP 解析，TCP 会存在粘包现象，一包中可能数据是不完整的
*
* @param[data] 要解析的数据流
* @param[data_size] 数据流的大小
*
* @return 返回解析了的数据大小，如果 RTP 包不完整的话会小于 data_size
*/
int GbRtpUnpack::ParseRtpOverTcp(const char *data, uint32_t data_size)
{
    // tcp 会粘包，前两个字节表示 rtp 包长度
    uint16_t rtp_len = 0;
    uint32_t read_size = 0;

    while(read_size < data_size) {
        // 读取长度
        rtp_len = rtp_read_uint16((uint8_t *)data + read_size);
        if (read_size + rtp_len + 2 > data_size)
            return read_size;

        read_size += 2;
        ParseRtpPackage(data + read_size, rtp_len);
        read_size += rtp_len;
    }

    return read_size;
}

/*
*  基于 UDP 的 RTP 解析，UDP 不存在粘包
*
* @param[data] 要解析的数据流
* @param[data_size] 数据流的大小
*
*/
void GbRtpUnpack::ParseRtpOverUdp(const char *data, uint32_t data_size)
{
    ParseRtpPackage(data, data_size);
}

/*处理一个完整的 RTP 包*/
void GbRtpUnpack::ParseRtpPackage(const char *rtp_package, uint32_t package_size)
{
    rtp_header_t header = {0};
    const char *pack = rtp_package;

    // 处理固定头
    header.v = (pack[0] >> 6) & 0x3;
    header.p = (pack[0] >> 5) & 0x1;
    header.x = (pack[0] >> 4) & 0x1;
    header.cc = pack[0] & 0xf;
    header.m = (pack[1] >> 7) & 0x1;
    header.pt = pack[1] & 0x7f;
    header.seq = rtp_read_uint16((uint8_t *)pack + 2);
    header.timestamp = rtp_read_uint32((uint8_t *)pack + 4);
    header.ssrc = rtp_read_uint32((uint8_t *)pack + 8);

    const char *body = rtp_package + 12; // 跳过固定头
    uint32_t body_size = package_size - 12;

    // 处理可变头
    if (header.cc != 0) {
        int csrc_len = header.cc * 4; // 每个 CSRC 占用 4 字节
        body += csrc_len;
        body_size -= csrc_len;
    }

    if (header.x == 1) {
        // 扩展头部的第一个四字节的后两个字节是长度
        uint16_t exten_len = rtp_read_uint16((uint8_t *)body + 2);
        exten_len = (exten_len + 1) * 4; // 扩展长度是指示有多少个四字节，加一是描述扩展头部信息的四字节也要算进去
        body += exten_len;
        body_size -= exten_len;
    }

    // 判断数据是否合法
    if (rtp_package + package_size != body + body_size) {
        GB_ERR("rtp parse fail\n");
        return;
    }

    OnRtpUnpack((RtpPayload)header.pt, header.seq, header.timestamp, header.ssrc, body, body_size);

    return;
}