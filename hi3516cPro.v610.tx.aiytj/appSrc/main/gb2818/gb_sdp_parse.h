/** Copyright (C) by Jabsco Company
*
* @File Name    : gb_sdp_parse.h
* @Created Time : 2024-05-06
* @Version      : 1.0
* @Author       :
* @Description  : 国标 sdp 解析
*/

#ifndef GB_SDP_PARSE_H_
#define GB_SDP_PARSE_H_

#include <stdlib.h>
#include <stdint.h>

#include <string>
#include <map>

#include "jconfstruct.h"

#include "gb_socket.h"

enum GbVideoCodecType {
    GB_VIDEO_CODEC_UNKNOWN = 0,
    // 编码格式 gb28181-2016 文档
    GB_MPEG_4,
    GB_H264,
    GB_SVAC,
    GB_3GP,
    GB_H265,
    GB_VIDEO_CODEC_UNMBER,
};

enum GbResolution {
    GB_RESOLUTION_UNKNOWN = 0,
    // 分辨率 gb28181-2016 文档
    GB_QCIF,
    GB_CIF,
    GB_4CIF,
    GB_D1,
    GB_720P,
    GB_1080P,
    // 未定义的部分采用 WxH 例如 3M = 2048*1536
    GB_RESOLUTION_UNMBER,
};

enum GbBitrateControl {
    GB_BITRATE_CONTROL_UNKNOWN = 0,
    // 码率控制 gb28181-2016 文档
    GB_CBR, // 固定码率
    GB_VBR, // 可变码率
    GB_BITRATE_CONTROL_UNMBER,
};

enum GbAudioCodecType {
    GB_AUDIO_CODEC_UNKNOWN = 0,
    // 音频编码格式 gb28181-2016 文档
    GB_G711,
    GB_G723_1,
    GB_G729,
    GB_G722_1,
    GB_AUDIO_CODEC_UNMBER,
};

enum GbSetup {
    GB_ACTIVE = 0,
    GB_PASSIVE,
};

class SdpParse {
public:
    static GbVideoCodecType CodecToVideoFormat(VENC_FORMAT_E codec);
    static VENC_FORMAT_E VideoFormatToCodec(GbVideoCodecType codec);
    static const char *VencsizeToResolution(VencSizeE vencsize);
    static VencSizeE ResolutionToVencsize(const char *str);
    static GbBitrateControl FixbpsToBitRateType(int fixbps);
    static int BitRateTypeToFixbps(GbBitrateControl bit_ctr);
public:
    SdpParse() : t_starttime_(0), t_endtime_(0), m_mediaport_(0), a_download_speed_(1), y_ssrc_(0) {};

    int Decode( const char *body);
    void PrintfAll(); // 参数打印
    /*获取会话名称 play playback talk*/
    const std::string &get_name() const {
        return s_name_;
    }
    /*获取服务器 ip 地址*/
    const char *get_ip() const {
        return c_ip_.c_str();
    }
    /*获取开始结束时间，录像回放和下载使用*/
    time_t get_start_time() const {
        return t_starttime_;
    }
    time_t get_end_time() const {
        return t_endtime_;
    }
    /*获取服务器端口*/
    uint32_t get_media_port() const {
        return m_mediaport_;
    }
    /*获取传输协议*/
    const std::string &get_transport() const {
        return m_transport_;
    }
    /*获取传输协议类型 TCP/UDP*/
    NetworkProtocols GetConnectType() {
        if (m_transport_ == "TCP/RTP/AVP")
            return GB_TCP;
        else if (m_transport_ == "RTP/AVP")
            return GB_UDP;
        else
            return GB_TCP; // 默认 TCP
    }
    /*获取负载类型表*/
    const std::map< std::string, std::string >& get_rtpmap() const {
        return m_rtpmap_;
    }
    /*获取下载速度*/
    double get_download_speed() const {
        return a_download_speed_;
    }
    /*获取是否新建连接*/
    std::string &get_connect() {
        return a_connect_;
    }
    /*获取连接方式*/
    GbSetup get_setup() {
        if (a_setup_ == "active")
            return GB_ACTIVE;

        return GB_PASSIVE;
    }
    /*获取 ssrc*/
    uint32_t get_ssrc() const {
        return y_ssrc_;
    }
    /*获取 f 字段*/
    std::string &get_encode_proterty() {
        return f_encode_proterty_;
    }
    /*解析 f 字段*/
    int ParseEncodeParam(GbVideoCodecType &video_codec, GbResolution &video_venc, int &video_fps, GbBitrateControl &bitrate_ctrl, int &video_bps, GbAudioCodecType &audio_codec, int &audio_bps, int &audio_sample_rate);

private:
    std::string                          s_name_;       //s 会话名称
    std::string                          c_ip_;  //c 媒体服务器的 ip 地址
    time_t                               t_starttime_;    // t 开始 UTC 时间
    time_t                               t_endtime_;      // t 结束 UTC 时间
    std::string                          m_mediatype_;    // m video:音视频混合; audio:音频
    uint32_t                             m_mediaport_;     // m 传输端口
    std::string                          m_transport_;    // m 传输层协议，TCP/RTP/AVP: TCP 载 RTP 流; RTP/AVP: UDP 载 RTP 流
    std::map< std::string, std::string > m_rtpmap_;       // m & a 支持的负载类型 "96 PS/90000"
    double                               a_download_speed_; // a 下载速度
    std::string                          a_connect_; // a connection:new 表示固定采用新建 TCP 连接方式,这里截取 new 部分
    std::string                          a_setup_;     // a 连接方式，主动发起连接还是被动接受连接 “active”为主动, “passive”为被动
    uint32_t                             y_ssrc_;         // y 校验码,总共十位
    std::string                          f_encode_proterty_;   // f
};

#endif