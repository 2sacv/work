/** Copyright (C) by Jabsco Company
*
* @File Name    : gb_broadcast.h
* @Created Time : 2024-05-06
* @Version      : 1.0
* @Author       :
* @Description  : 国标对讲服务
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <memory>

#include "mpeg-proto.h"
#include "g711.h"

#include "gb_broadcast.h"

GbBroadcast::GbBroadcast(JSScheduler &sch, GbProfile &profile) :
sch_(sch), profile_(profile), hdl_socker_read_(NULL), ssrc_(0), call_id_(NULL), ps_demuxer_(NULL), recv_size_(0)
{
    bzero(&g722dec_, sizeof(g722dec_));
    bzero(recv_buff_, sizeof(recv_buff_));
}

GbBroadcast::~GbBroadcast()
{
    StopBroadcast();
    GB_DBG("deinit broadcast\n");
}

void GbBroadcast::RrepareRecvAudio(SdpParse *sdp_parse, osip_call_id_t *call_id)
{
    if (sdp_parse == NULL || call_id == NULL) {
        GB_ERR("sdp_parse || call_id is NULL\n");
        return ;
    }

    SchedulerParam<GbBroadcast, SdpParse *> param(GetSharedPtr(), true, 0);
    param.set_data1(sdp_parse);
    param.set_data2(call_id);
    js_run_function(sch_, RrepareRecvAudioCb, &param, true);
}

void GbBroadcast::RrepareRecvAudioCb(void *data)
{
    SchedulerParam<GbBroadcast, SdpParse *> *param = (SchedulerParam<GbBroadcast, SdpParse *> *)data;
    std::shared_ptr<GbBroadcast> &this_ptr = param->get_this_ptr();
    SdpParse *sdp_parse = param->get_data1();
    osip_call_id_t *call_id = (osip_call_id_t *)param->get_data2();
    int ret = 0;
    // 广播策略以后来者为准，如果前面存在就先关掉
    this_ptr->StopBroadcast();

    do {
        ret = this_ptr->BroadcastConnectServer(sdp_parse);
        if (ret != SUCCESS) {
            break;
        }

        ret = this_ptr->BroadcastConnectAudioout();
        if (ret != SUCCESS) {
            break;
        }

        this_ptr->ps_demuxer_ = ps_demuxer_create(OnPsPacket, this_ptr.get());
        if (this_ptr->ps_demuxer_ == NULL) {
            break;
        }
        g722_decode_init(&this_ptr->g722dec_, 64000, G722_SAMPLE_RATE_8000);

        this_ptr->ssrc_ = sdp_parse->get_ssrc();

        osip_call_id_clone(call_id, &(this_ptr->call_id_));

        this_ptr->DebugOpenFile();

        GB_DBG("prepare recv audio success\n");
        return ;
    } while (0);

    this_ptr->StopBroadcast();
    GB_DBG("prepare recv audio fail\n");
    return ;
}

int GbBroadcast::BroadcastConnectServer(SdpParse *sdp_parse)
{
    // 网络连接方式判断
    GbSetup connect_setup = sdp_parse->get_setup();
    if (connect_setup == GB_ACTIVE && audioin_socket_.get_network_protocols() == GB_TCP) {
        // 服务器主动连接，需要设备监听
        do {
            GB_DBG("gb broadcast start listen\n");

            if (audioin_socket_.CreateSocket(sdp_parse->GetConnectType()) != SUCCESS)
                break;

            if (audioin_socket_.SetReuseAddr(1) != SUCCESS)
                break;

            if (audioin_socket_.Bind(profile_.get_broadcast_port()) != SUCCESS)
                break;

            if (audioin_socket_.Listen(1) != SUCCESS)
                break;

            js_create_reader_r(sch_, audioin_socket_.get_socket(), JS_READABLE, HandleListen, this, &hdl_socker_read_);
            return SUCCESS;
        } while (0);

    } else {
        // 服务器被动，设备需要主动连接
        do {
            GB_DBG("gb broadcast start connect server\n");

            int ret = audioin_socket_.ServerConnect(sdp_parse->GetConnectType(), profile_.get_broadcast_port(),
                                    sdp_parse->get_ip(), sdp_parse->get_media_port(), 10);
            if (ret != SUCCESS) {
                break;
            }

            js_create_reader_r(sch_, audioin_socket_.get_socket(), JS_READABLE, HandleRecvAudio, this, &hdl_socker_read_);

            if (sdp_parse->GetConnectType() == GB_UDP) {
                // 广播设备是只接收，udp 协议下如果是内网，需要发送探测包，让对端能知道链路信息
                char data[1024] = {0};
                int len = snprintf(data,sizeof(data),
                                    "RECORD RTSP/1.0\r\nCSeq:1\r\n"
                                    "Session:47c325660fb111e88000e84dd0b92e4\r\n"
                                    "User-Agent:HUAWEI CU/V100R002C01\r\nContent-Length:0\r\n"
                                    "Range:npt=now-\r\n"
                                    "x-NAT-Info:type=RTP;local_addr=%s;local_port=%d\r\n", profile_.get_local_ip(), profile_.get_broadcast_port());

                audioin_socket_.Send(data, len);
            }

            return SUCCESS;
        } while (0);

    }

    return FAILURE;
}

int GbBroadcast::BroadcastConnectAudioout()
{
    audioout_socket_.CreateSocket(GB_TCP);

    return audioout_socket_.ServerConnect("127.0.0.1", 8004, 60);
}

void GbBroadcast::HandleListen(int fd, int events, void *userdata)
{
    GbBroadcast *this_ptr = (GbBroadcast *)userdata;
    GbSocket client_socket;

    if (this_ptr->audioin_socket_.Aceept(client_socket) == SUCCESS) {
        // 接受连接成功，删掉监听，然后关闭监听套接字，然后把客户端套接字完整拷贝
        js_delete_reader_r(&this_ptr->hdl_socker_read_);
        this_ptr->audioin_socket_.CloseSocket();
        this_ptr->audioin_socket_.set_socket(client_socket);

        js_create_reader_r(this_ptr->sch_, this_ptr->audioin_socket_.get_socket(), JS_READABLE, HandleRecvAudio, this_ptr, &this_ptr->hdl_socker_read_);
    }
}

void GbBroadcast::HandleRecvAudio(int fd, int events, void *userdata)
{
    GbBroadcast *this_ptr = (GbBroadcast *)userdata;

    this_ptr->RecvAudio();
}

void GbBroadcast::RecvAudio()
{
    int ret = 0;

    ret = audioin_socket_.Recv((char *)recv_buff_ + recv_size_, sizeof(recv_buff_) - recv_size_);
    if (ret <= 0) {
        return ;
    }

    recv_size_ += ret;
    if (audioin_socket_.get_network_protocols() == GB_TCP) {
        int parse_size = ParseRtpOverTcp((const char*)recv_buff_, recv_size_);
        if (parse_size > 0) {
            recv_size_ -= parse_size;
            memmove(recv_buff_, recv_buff_ + parse_size, recv_size_);
        } else {
            // 解析字节为 0 , 要不就是接收数据太少;要不就是数据出错了，导致 rtp 长度异常
            GB_INFO("recv_size:%u\n", recv_size_);
        }
    } else {
        // udp 不会粘包，不需要额外处理
        ParseRtpOverUdp((const char *)recv_buff_, recv_size_);
        recv_size_ = 0;
    }
}

int GbBroadcast::OnRtpUnpack(RtpPayload payload, uint16_t seq, uint32_t timestamp, uint32_t ssrc, const char *body, uint32_t body_size)
{
    // 正常来说需要判断 ssrc 是否正常，以及检查帧号和时间戳，防止收包顺序不对
    // 这里先不做那么复杂，有需要再加

    if (payload == RTP_PCMA) {
        OnAudioG711A(body, body_size);
    } else if (payload == RTP_PS) {
        if (ps_demuxer_) {
            // ps 解码完成后会调用 OnPsPacket 处理数据
            ps_demuxer_input(ps_demuxer_, (const uint8_t* )body, (size_t)body_size);
        }
    } else {
        GB_ERR("payload error\n");
    }

    return SUCCESS;
}

/*对讲 debug 写音频文件*/
void GbBroadcast::DebugWriteAudio(const char *data, size_t data_size)
{
    if (debug_fd_ == -1) {
        return ;
    }

    int ret = Writefully(debug_fd_, data, data_size);
    if (ret < 0 || ret != (int)data_size) {
        GB_ERR("write audio fail ret :%d\n", ret);
        return ;
    }

    return ;
}

/*判断是否是是 debug 模式，如果是的话打印文件用以存储对讲数据，文件名用 ssrc 来进行区分*/
void GbBroadcast::DebugOpenFile()
{
    if (!is_okey("/tmp/gb_audio_test") || debug_fd_ != -1) {
        return ;
    }

    char file_name[64] = {0};
    snprintf(file_name, sizeof(file_name), "/mnt/gb_audio/audio_%u", ssrc_);

    GB_ERR("open audio debug file:%s\n", file_name);
    debug_fd_ = open(file_name, O_CREAT|O_WRONLY|O_APPEND);
    if (debug_fd_ == -1) {
        GB_ERR("open audio debug file fail\n");
        return ;
    }

    return ;
}

void GbBroadcast::OnPsPacket(void * param, int stream, int codecid, int flags, int64_t pts, int64_t dts, const void * data, uint32_t bytes)
{
    GbBroadcast *this_ptr = (GbBroadcast *)param;

    if(codecid == PSI_STREAM_AUDIO_G711A)
        this_ptr->OnAudioG711A((const char *)data, bytes);
    else if (codecid == PSI_STREAM_AUDIO_G711U)
        this_ptr->OnAudioG711U((const char *)data, bytes);
    else if(codecid == PSI_STREAM_AUDIO_G722)
        this_ptr->OnAudioG722((const char *)data, bytes);
    else {
        GB_ERR("codecied:%d is not support\n", codecid);
        // 不支持的流默认当成 g711a 处理
        this_ptr->OnAudioG711A((const char *)data, bytes);
    }

}

void GbBroadcast::OnAudioG711A(const char *data, size_t data_size)
{
    if (data == NULL || data_size <= 0) {
        GB_ERR("data[%p] || data_size[%u] error\n", data, data_size);
        return ;
    }

    DebugWriteAudio(data, data_size);
    // G711A 需要先转回 pcma 再发送
    short decbuf[1500] = {0};
    const char *p = data;

    while (data_size > 0) {
        uint32_t i = 0;
        for (; i < sizeof(decbuf); ++i) {
            decbuf[i] = jco_alaw2linear(*p++);
            if (--data_size == 0) {
                break;
            }
        }

        audioout_socket_.Send((const char *)decbuf, i * 2);
    }
}

void GbBroadcast::OnAudioG711U(const char *data, size_t data_size)
{
    if (data == NULL || data_size <= 0) {
        GB_ERR("data[%p] || data_size[%u] error\n", data, data_size);
        return ;
    }

    DebugWriteAudio(data, data_size);

    // G711U 需要先转回 pcma 再发送
    short decbuf[1500] = {0};
    const char *p = data;

    while (data_size > 0) {
        uint32_t i = 0;
        for (; i < sizeof(decbuf); ++i) {
            decbuf[i] = jco_ulaw2linear(*p++);
            if (--data_size == 0) {
                break;
            }
        }

        audioout_socket_.Send((const char *)decbuf, i * 2);
    }
}

void GbBroadcast::OnAudioG722(const char *data, size_t data_size)
{
    if (data == NULL || data_size <= 0) {
        GB_ERR("data[%p] || data_size[%u] error\n", data, data_size);
        return ;
    }

    DebugWriteAudio(data, data_size);

    short decbuf[2048] = {0};
    int declen = 0;
    if (data_size * 2 > sizeof(decbuf)) {
        GB_ERR("large len %u\n", data_size);
    }

    declen = g722_decode(&g722dec_, decbuf, (const uint8_t*)data, data_size);

    //GB_DBG("dec g722 bytes:%d declen:%d!\n", bytes, declen);
    audioout_socket_.Send((const char *)decbuf, declen * 2);

    return ;
}

void GbBroadcast::StopRecvAduio(osip_call_id_t *call_id)
{
    if (call_id == NULL) {
        GB_ERR("call_id is NULL\n");
        return ;
    }

    SchedulerParam<GbBroadcast, osip_call_id_t *> param(GetSharedPtr(), true, 0);
    param.set_data1(call_id);

    js_run_function(sch_, StopRecvAduioCb, &param, true);
}

void GbBroadcast::StopRecvAduioCb(void *data)
{
    SchedulerParam<GbBroadcast, osip_call_id_t *> *param = (SchedulerParam<GbBroadcast, osip_call_id_t *> *)data;
    std::shared_ptr<GbBroadcast> &this_ptr = param->get_this_ptr();
    osip_call_id_t *call_id = (osip_call_id_t *)param->get_data1();

    if (osip_call_id_match(call_id, this_ptr->call_id_) == OSIP_SUCCESS) {
        this_ptr->StopBroadcast();
    }

    return ;
}

void GbBroadcast::StopBroadcast()
{
    js_delete_reader_r(&hdl_socker_read_);
    audioin_socket_.CloseSocket();
    audioout_socket_.CloseSocket();

    ssrc_ = 0;
    if (call_id_) {
        osip_call_id_free(call_id_);
        call_id_ = NULL;
    }

    if (ps_demuxer_) {
        ps_demuxer_destroy(ps_demuxer_);
        ps_demuxer_ = NULL;
    }

    if (debug_fd_ != -1) {
        fsync(debug_fd_);
        close(debug_fd_);
        debug_fd_ = -1;
    }

    bzero(&g722dec_, sizeof(g722dec_));
    bzero(recv_buff_, sizeof(recv_buff_));
    recv_size_ = 0;
}