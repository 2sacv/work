/** Copyright (C) by Jabsco Company
*
* @File Name    : gb_broadcast.h
* @Created Time : 2024-05-06
* @Version      : 1.0
* @Author       :
* @Description  : 国标对讲服务
*/

#ifndef GB_BROADCAST_H_
#define GB_BROADCAST_H_

#include <stdint.h>

#include "g722.h"
#include "osipparser2/osip_message.h"
#include "js_scheduler.h"
#include "mpeg-ps.h"

#include "gb_socket.h"
#include "gb_profile.h"
#include "gb_rtp.h"
#include "gb_sdp_parse.h"

/*对讲就是 broadcast + 实时流的音频*/

class GbBroadcast : public GbRtpUnpack , public std::enable_shared_from_this<GbBroadcast>{
public:
    std::shared_ptr<GbBroadcast> GetSharedPtr() {
        // 使用 shared_from_this() 安全地获取 shared_ptr
        return shared_from_this();
    }

    GbBroadcast(JSScheduler &sch, GbProfile &profile);
    ~GbBroadcast();

    void RrepareRecvAudio(SdpParse *sdp_parse, osip_call_id_t *call_id);
    void StopRecvAduio(osip_call_id_t *call_id);
private:
    static void RrepareRecvAudioCb(void *data);
    static void StopRecvAduioCb(osip_call_id_t *call_id);
    static void HandleListen(int fd, int events, void *userdata);
    static void HandleRecvAudio(int fd, int events, void *userdata);
    static void StopRecvAduioCb(void *data);
    void RecvAudio();
    static void OnPsPacket(void * param, int stream, int codecid, int flags, int64_t pts, int64_t dts, const void * data, uint32_t bytes);
    int BroadcastConnectServer(SdpParse *sdp_parse);
    int BroadcastConnectAudioout();
    void StopBroadcast();
    int OnRtpUnpack(RtpPayload payload, uint16_t seq, uint32_t timestamp, uint32_t ssrc, const char *body, uint32_t body_size);

    void OnAudioG711A(const char *data, size_t data_size);
    void OnAudioG711U(const char *data, size_t data_size);
    void OnAudioG722(const char *data, size_t data_size);

    // 对讲保存文件
    void DebugWriteAudio(const char *data, size_t data_size);
    void DebugOpenFile();
private:
    JSScheduler &sch_; // 事件和接收消息处理
    GbProfile &profile_;  // 配置

    GbSocket audioout_socket_;  // 设备音频输出套接字
    GbSocket audioin_socket_;   // 接收音频套接字

    JSRWHandle  hdl_socker_read_;

    uint32_t ssrc_;
    osip_call_id_t *call_id_;

    ps_demuxer_t *ps_demuxer_; // ps 解码
    g722_decode_state_t g722dec_; // g722 解码

    int debug_fd_ = -1;  // 调试模式保存音频套接字

    uint8_t recv_buff_[2048]; // tcp 可能会粘包，缓冲区要能保留一个完整的 rtp 包
    uint32_t recv_size_; // 有效数据大小
};

#endif