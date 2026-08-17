/** Copyright (C) by Jabsco Company
*
* @File Name    : gb_live_streaming.h
* @Created Time : 2024-05-06
* @Version      : 1.0
* @Author       :
* @Description  : 国标直播推流服务
*/

#ifndef GB_LIVE_STREAMING_H_
#define GB_LIVE_STREAMING_H_

#include <stdint.h>

#include <vector>
#include <list>

#include "shm_buf.h"
#include "osipparser2/osip_message.h"
#include "js_scheduler.h"

#include "gb_sdp_parse.h"
#include "gb_common.h"
#include "gb_socket.h"
#include "gb_profile.h"
#include "gb_rtp.h"
#include "gb_mpeg_ps.h"

#define MAX_VISITOR 4
#define VIDEO_LOOP 40
#define AUDIO_LOOP 50

enum GbEncodeChannel{
    GB_ENCODE_MAIN = 0,
    GB_ENCODE_SUB,
    GB_ENCODE_MAX,
};

class GbVistor : public GbRtpRepack
{
    friend class GbLiveStreaming;
    friend class GbLiveStreamingMng;

public:
    GbVistor(SdpParse &sdp_parse, osip_call_id_t &call_id_, GbProfile &profile);
    ~GbVistor();

    int HandleRtpDate();
    int StartConnect();

    int OnRtpRepackage(const char *package, uint16_t package_size);

private:
    GbProfile &profile_;  // 配置

    GbSocket socket_;  // 推流 socket
    GbSetup connect_setup_; // 连接方式，这里记录的是服务器那边的参数
    osip_call_id_t *call_id_; // 会话 ID 停止推流时候使用, 同时也是会话的标识

    bool send_key_;  // 发送过关键帧

    // 媒体信息，这个是 sdp 报文里面的，不代表实际参数
    GbVideoCodecType video_codec_;
    GbResolution video_venc_;
    int video_fps_;
    GbBitrateControl fix_bps_;
    int video_bps_;
    GbAudioCodecType audio_codec_;
    int audio_bps_;
    int audio_sample_rate_;
};

// 推流类，实际用来控制推流逻辑的类
class GbLiveStreaming  : public GbRtpPack, public GbMpegPsEncode
{

    friend class GbLiveStreamingMng;

public:
    GbLiveStreaming(GbProfile &profile, JSScheduler &sch, GbEncodeChannel channel, int video_idx, int audio_idx);
    ~GbLiveStreaming();

    GbVistor *FindVistor(osip_call_id_t *call_id);
    int DeleteVistor( osip_call_id_t *call_id);
    int AddVistor(GbVistor *vistor);

    int OnRtpPackage(uint8_t *package, uint16_t package_size, bool is_video, bool is_video_key, bool is_first, bool is_end);
    uint16_t GetMtu() {
        return profile_.get_mtu();
    }
    int OnPsStream(uint8_t *ps, uint32_t ps_size, bool is_video, bool is_video_key, bool is_first, bool is_end);

    void DeleteAllVistor(); // sip 链接断开时应该删除全部推流
private:
    int DelayStartLiveStreaming();
    int StopLiveStreaming();
    static void ReadVideo(void *data);
    static void ReadAudio(void *data);
    static void CbDoReadVideo(void *userdata, tSBFrame* tFrame);
    static void CbDoReadAudio(void *userdata, tSBFrame* tFrame);
    static void CbStartLivingStreaming(void *data);
    void DoReadVideo(tSBFrame* tFrame);
    void DoReadAudio(tSBFrame* tFrame);

    int GetMediaInfo();

private:
    GbProfile &profile_;  // 配置

    JSScheduler sch_;       // 推流调度器
    JSTCHandle  audio_loop_;
    JSTCHandle  video_loop_;
    JSTCHandle  delay_start_;

    GbEncodeChannel encode_channel_;

    // ps 编码
    int video_stream_id_;
    int audio_stream_id_;

    // 视频参数
    int video_idx_; // shm cache 视频节点, 由管理器 mng 初始化

    shm_buf_t vide_buf_; //  shm buff 句柄
    int video_serial_; // 视频帧序号
    eShmMediaType video_type_; // 编码类型 H264 H265 ...
    int bps_;  // 码率
    int fps_;  // 帧率
    int width_; // 视频宽
    int height_; // 视频高
    char vps_data_[128]; // H26x 压缩参数
    int vps_data_size_;
    char sps_data_[128];
    int sps_data_size_;
    char pps_data_[128];
    int pps_data_size_;

    // 音频参数
    int audio_idx_; // shm cache 音频节点, 由管理器 mng 初始化
    shm_buf_t audio_buf_; // shm buff 句柄
    int audio_serial_; // 音频帧序号
    eShmMediaType audio_type_;  // 编码类型 G711A G711U ...
    int sample_rate_; // 采样率 8K 16K ...
    int bit_width_; // 采样精度 8bit 16bit ...

    std::list<GbVistor *> vistor_list_; // 推流列表
};

// 推流管理类，管理推流类，有几路通道就会有几个推流类
class GbLiveStreamingMng : public std::enable_shared_from_this<GbLiveStreamingMng>{
public:
    std::shared_ptr<GbLiveStreamingMng> GetSharedPtr() {
        // 使用 shared_from_this() 安全地获取 shared_ptr
        return shared_from_this();
    }

    GbLiveStreamingMng(JSScheduler &sch, GbProfile &profile);
    ~GbLiveStreamingMng();

    void StartPushStreaming(SdpParse *sdp_parse, osip_call_id_t *call_id);
    void StopPushStreaming(osip_call_id_t *call_id);

    void StopAllPushStreaming();
private:
    static void StartPushStreamingCb(void *data);
    static void StopPushStreamingCb(void *data);
    static void StopAllPushStreamingCb(void *data);

    bool FindCallId(osip_call_id_t *call_id);
private:
    JSScheduler &sch_;     // 推流调度器
    GbProfile &profile_;  // 配置
    std::vector<GbLiveStreaming *> chn_mng; // 通道控制
};

#endif