/** Copyright (C) by Jabsco Company
*
* @File Name    : gb_rtp.h
* @Created Time : 2024-05-06
* @Version      : 1.0
* @Author       :
* @Description  : 国标 rtp 打包,解包
*/
#ifndef GB_RTP_H_
#define GB_RTP_H_

#include <stdint.h>

enum RtpPayload {
    RTP_G723   = 4,
    RTP_PCMA   = 8,
    RTP_G722   = 9,
    RTP_G729   = 18,
    RTP_SVAC_A = 20, // SVAC 音频
    RTP_PS     = 96,
    RTP_MPEG_4 = 97,
    RTP_H264   = 98,
    RTP_SVAC   = 99,
};

typedef struct _rtp_header_t
{
    uint32_t v:2;       /* protocol version */
    uint32_t p:1;       /* padding flag */
    uint32_t x:1;       /* header extension flag */
    uint32_t cc:4;      /* CSRC count */
    uint32_t m:1;       /* marker bit */
    uint32_t pt:7;      /* payload type */
    uint32_t seq:16;    /* sequence number */
    uint32_t timestamp; /* timestamp */
    uint32_t ssrc;      /* synchronization source */
} rtp_header_t;

class GbRtpPack {
public:
    GbRtpPack(uint32_t ssrc = 0, uint32_t time_stamp = 0, uint16_t time_step = 3600, uint16_t seq = 0) :
                ssrc_(ssrc), time_stamp_(time_stamp), time_step_(time_step), seq_(seq) {};
    virtual ~GbRtpPack() {};

    int RtpPack(const char *stream, uint32_t stream_size, bool is_video, bool is_video_key, bool is_first, bool is_end, RtpPayload payload);    // rtp 打包

    void set_ssrc(uint32_t ssrc) {
        ssrc_ = ssrc;
    }

    void set_time_step(uint32_t time_step) {
        time_step_ = time_step;
    }

    void RtpReset() {
        ssrc_ = 0;
        time_stamp_ = 0;
        time_step_ = 3600;
        seq_ = 0;
    }
    /*
    *  rtp 包处理，纯虚函数，用户需要实现这个函数来处理 RTP 包
    *
    * @param[package] rtp 包数据，默认前两个字节是包大小，UDP 需要跳过这两个字节，TCP 不需要
    * @param[package_size] rtp 包大小
    * @param[is_video] 是否是视频帧，给重打包使用的，正常不用管
    * @param[is_video_key] 是否是视频关键帧
    * @param[is_first] 是否是第一帧
    * @param[is_end] 是否是最后一包, 给重打包用的，正常不用管
    *
    * @return 成功返回 SUCCESS 失败返回 FAILURE
    */
    virtual int OnRtpPackage(uint8_t *package, uint16_t package_size, bool is_video, bool is_video_key, bool is_first, bool is_end) = 0;

    /*
    * 获取 mtu 值，rtp 一包大小不能超过 mtu 限制，纯虚函数，用户需要实现这个函数
    *
    * @return 返回 mtu 大小
    */
    virtual uint16_t GetMtu() = 0;

private:

    uint32_t ssrc_;
    uint32_t time_stamp_; // 时间戳，初始时间戳只能构造的时候初始化，后续会进行自增
    uint16_t time_step_; // 时间一次的增量 默认 3600
    uint16_t seq_;  // 序列号，初始序列号只有构造的时候可以初始化，后续会进行自增
};

class GbRtpRepack {
public:
    GbRtpRepack(uint32_t ssrc = 0, uint32_t time_stamp = 0, uint16_t time_step = 3600, uint16_t seq = 0) :
                ssrc_(ssrc), time_stamp_(time_stamp), time_step_(time_step), seq_(seq) {};
    virtual ~GbRtpRepack() {};

    int RtpRepack(uint8_t *rtp_package, uint16_t package_size, bool is_video, bool is_end);

    void set_ssrc(uint32_t ssrc) {
        ssrc_ = ssrc;
    }

    void set_time_step(uint32_t time_step) {
        time_step_ = time_step;
    }
    /*
    * rtp 重打包数据处理，纯虚函数，用户需要实现这个函数来处理 RTP 数据包
    *
    * @param[package] rtp 包数据，默认前两个字节是包大小，UDP 需要跳过这两个字节，TCP 不需要
    * @param[package_size] rtp 包大小
    *
    * @return 成功返回 SUCCESS 失败返回 FAILURE
    */
    virtual int OnRtpRepackage(const char *package, uint16_t package_size) = 0;

private:
    uint32_t ssrc_;
    uint32_t time_stamp_; // 时间戳，初始时间戳只能构造的时候初始化，后续会进行自增
    uint32_t time_step_; // 时间一次的增量 默认 3600
    uint16_t seq_;  // 序列号，初始序列号只有构造的时候可以初始化，后续会进行自增
};

class GbRtpUnpack {
public:
    GbRtpUnpack() {};
    virtual ~GbRtpUnpack() {};

    int ParseRtpOverTcp(const char *data, uint32_t data_size);
    void ParseRtpOverUdp(const char *data, uint32_t data_size);

    /*
    * RTP 解包数据处理函数，用户需要实现这个函数来处理解包数据
    *
    * @param[payload] 负载类型
    * @param[seq] 序列号
    * @param[timestamp] 时间戳
    * @param[ssrc] ssrc 校验码
    * @param[body] 去掉头之后的消息部分
    * @param[body_size] 消息部分大小
    *
    * @return 成功返回 SUCCESS 失败返回 FAILURE
    */
    virtual int OnRtpUnpack(RtpPayload payload, uint16_t seq, uint32_t timestamp, uint32_t ssrc, const char *body, uint32_t body_size) = 0;

private:
    void ParseRtpPackage(const char *rtp_package, uint32_t package_size);
};

#endif