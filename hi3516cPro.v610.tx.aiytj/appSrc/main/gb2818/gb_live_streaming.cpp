/** Copyright (C) by Jabsco Company
*
* @File Name    : gb_live_streaming.h
* @Created Time : 2024-05-06
* @Version      : 1.0
* @Author       :
* @Description  : 国标直播推流服务
*/

#include "shm_buf_pool.h"
#include "encodeapi.h"

#include "gb_sdp_parse.h"
#include "gb_common.h"
#include "gb_socket.h"
#include "gb_profile.h"
#include "gb_live_streaming.h"
#include "shm_buf.h"

GbVistor::GbVistor(SdpParse &sdp_parse, osip_call_id_t &call_id, GbProfile &profile) :
profile_(profile), call_id_(NULL), send_key_(false)
{
    /*获取媒体参数*/
    sdp_parse.ParseEncodeParam(video_codec_, video_venc_, video_fps_, fix_bps_, video_bps_, audio_codec_, audio_bps_, audio_sample_rate_);

    /*配置网络参数*/
    socket_.set_dst_ip(sdp_parse.get_ip());
    socket_.set_dst_port(sdp_parse.get_media_port());
    socket_.set_network_protocols(sdp_parse.GetConnectType());

    connect_setup_ = sdp_parse.get_setup();

    /*设置会话属性*/
    set_ssrc(sdp_parse.get_ssrc());

    if (osip_call_id_clone(&call_id, &call_id_) != OSIP_SUCCESS) {
        GB_ERR("call id clone fail\n");
    }
}

GbVistor::~GbVistor()
{
    if (call_id_ != NULL) {
        osip_call_id_free(call_id_);
    }
}

/*连接媒体服务器，成功返回 SUCCESS 失败返回 FAILURE*/
int GbVistor::StartConnect()
{
    int ret = 0;

    ret = socket_.ServerConnect(socket_.get_network_protocols(), profile_.get_media_port(),
                                socket_.get_dst_ip(), socket_.get_dst_port(), 10);

    if (ret == SUCCESS) {
        GB_DBG("connect media server success\n");
        return SUCCESS;
    }

    GB_ERR("connect media server fail\n");
    return FAILURE;
}
/*rtp 包处理函数*/
int GbVistor::OnRtpRepackage(const char *package, uint16_t package_size)
{
    int ret = 0;

    if (socket_.get_network_protocols() == GB_TCP) {
        ret = socket_.Send(package, package_size, 3);
    } else {
        ret = socket_.Send(package + 2, package_size - 2); // udp 跳过前两个长度字节
    }

    if (ret < 0)
        return ret;

    return SUCCESS;
}


GbLiveStreaming::GbLiveStreaming(GbProfile &profile, JSScheduler &sch, GbEncodeChannel channel, int video_idx, int audio_idx) :
profile_(profile), sch_(sch), audio_loop_(NULL), video_loop_(NULL), delay_start_(NULL), encode_channel_(channel), video_stream_id_(-1), audio_stream_id_(-1),
video_idx_(video_idx), vide_buf_(NULL), video_serial_(-1), video_type_(SHM_MEDIA_UNKOWN), bps_(0), fps_(0), width_(0), height_(0),
audio_idx_(audio_idx), audio_buf_(NULL), audio_serial_(0), audio_type_(SHM_MEDIA_UNKOWN), sample_rate_(0), bit_width_(0)
{
    bzero(vps_data_, sizeof(vps_data_));
    bzero(sps_data_, sizeof(sps_data_));
    bzero(pps_data_, sizeof(pps_data_));
    vps_data_size_ = 0;
    sps_data_size_ = 0;
    pps_data_size_ = 0;
}

GbLiveStreaming::~GbLiveStreaming()
{
    DeleteAllVistor();

    GB_DBG("Encode channel:%d release successful\n", encode_channel_);
}

void GbLiveStreaming::DeleteAllVistor()
{
    StopLiveStreaming();

    std::list<GbVistor *>::iterator it = vistor_list_.begin();
    for (; it != vistor_list_.end(); ) {
        delete *it;
        it = vistor_list_.erase(it); // erase 擦除元素后会返回下一个元素的迭代器
    }
}

/*通过 call_id 查找对话，成功返回 vistor 指针, 失败返回 NULL*/
GbVistor *GbLiveStreaming::FindVistor(osip_call_id_t *call_id)
{
    std::list<GbVistor *>::iterator it = vistor_list_.begin();
    for (; it != vistor_list_.end(); ++it) {
        GbVistor *vistor = *it;
        if (vistor != NULL && (osip_call_id_match(call_id, vistor->call_id_) == OSIP_SUCCESS)) {
            return vistor;
        }
    }

    return NULL;
}
/*添加 vistor 成功返回 SUCCESS, 失败返回 FAILURE*/
int GbLiveStreaming::AddVistor(GbVistor *vistor)
{
    if (FindVistor(vistor->call_id_) != NULL) {
        // 已经存在，不重复添加
        GB_ERR("current session already exists\n");
        return FAILURE;
    }

    /*当 vistor 全部被删除后，推流器会停止推流，需要重新打开推流*/
    if (video_loop_ == NULL) {
        int ret = DelayStartLiveStreaming();
        if (ret != SUCCESS) {
            GB_ERR("Start live streaming error\n");
            return FAILURE;
        }
    }

    // 服务器那边下发的连接方式分为主动连接和被动连接
    if (vistor->connect_setup_ == GB_ACTIVE && vistor->socket_.get_network_protocols() == GB_TCP) {
        // 服务器主动连接，设备需要监听，先不实现 wuhy
        ;
    } else {
        // 服务器被动，设备需要主动连接
        if (vistor->StartConnect() != SUCCESS) {
            return FAILURE;
        }
    }

    vistor_list_.push_back(vistor);
    GB_DBG("add vistor:%p cur vistor unmber:%d\n", vistor, vistor_list_.size());

    return SUCCESS;
}

/*通过 call_id 删除 vistor ，成功返回 SUCCESS, 失败返回 FAILURE*/
int GbLiveStreaming::DeleteVistor(osip_call_id_t *call_id)
{
    GbVistor *vistor = FindVistor(call_id);
    if (vistor != NULL) {
        vistor_list_.remove(vistor);
        delete vistor;

        if (vistor_list_.size() == 0) { // 所有 vistor 都删除了，停止推流
            StopLiveStreaming();
        }

        return SUCCESS;
    }

    return FAILURE;
}

int GbLiveStreaming::DelayStartLiveStreaming()
{
    // 获取媒体信息可能会失败，用定时器执行
    if (delay_start_ == NULL) {
        js_create_timer_r(sch_, 20, 2 * 1000, CbStartLivingStreaming, this, &delay_start_);
    }

    return SUCCESS;
}
/*开始推流*/
void GbLiveStreaming::CbStartLivingStreaming(void *data)
{
    GbLiveStreaming *streaming = (GbLiveStreaming *)data;

    // 如果已经存在推流先删除
    js_delete_timer_r(&streaming->video_loop_);
    js_delete_timer_r(&streaming->audio_loop_);

    do {
        // 1. 获取媒体信息
        if (streaming->GetMediaInfo() != SUCCESS) {
            break;
        }

        streaming->video_serial_ = -1;
        streaming->audio_serial_ = 0;
        js_create_timer_r(streaming->sch_, 20, VIDEO_LOOP, ReadVideo, data, &streaming->video_loop_);
        if (streaming->audio_buf_ != NULL) {
            js_create_timer_r(streaming->sch_, 40, AUDIO_LOOP, ReadAudio, data, &streaming->audio_loop_);
        }

        js_delete_timer_r(&streaming->delay_start_); // 开始推流成功，删除定时处理
    } while (0);

}

int GbLiveStreaming::GetMediaInfo()
{
    do {
        // 视频初始化
        vide_buf_ = get_shm_buf_pool(video_idx_);
        if (vide_buf_ == NULL) {
            GB_ERR("Get video shm buf error\n");
            break;
        }

        shm_buf_get_media_info(vide_buf_, &video_type_, &bps_, &fps_, &width_, &height_);
        if (!width_ || !height_ || video_type_ == SHM_MEDIA_UNKOWN) {
            GB_ERR("Get video info error, %d %d %d\n", width_, height_, video_type_);
            break;
        }

        shm_buf_get_vps_sps_pps(vide_buf_,
                                vps_data_, &vps_data_size_,
                                sps_data_, &sps_data_size_,
                                pps_data_, &pps_data_size_);
        if (video_type_ == SHM_MEDIA_VIDEO_H264 && (sps_data_size_ == 0 || pps_data_size_ == 0)) {
            GB_ERR("get h264 info error\n");
            break;
        } else if (video_type_ == SHM_MEDIA_VIDEO_H265 && (vps_data_size_ == 0 || sps_data_size_ == 0 || pps_data_size_ == 0)) {
            GB_ERR("get h265 info error\n");
            break;
        }
        GB_DBG("vpsDataSize:%d spsDataSize:%d ppsDataSize:%d!\n", vps_data_size_, sps_data_size_, pps_data_size_);

        PsReset(); // 清除 ps 编码信息

        // 添加一路视频 ps 编码信息
        EncodeInfo encode_info = {0};
        encode_info.video_info.bps = bps_;
        encode_info.video_info.fps = fps_;
        encode_info.video_info.width = width_;
        encode_info.video_info.height = height_;
        StreamType stream_type = STREAM_TYPE_UNKNOWN;

        if (video_type_ == SHM_MEDIA_VIDEO_H264) {
            stream_type = STREAM_TYPE_H264;
        } else {
            stream_type = STREAM_TYPE_H265;
        }

        video_stream_id_ = PsAddStream(stream_type, encode_info);

        // 音频初始化
        int width = 0, height = 0; // 音频宽高不会有值，这两个不用管
        audio_buf_ = get_shm_buf_pool(SHM_BUF_AUDIO);
        if (audio_buf_ == NULL) {
            GB_ERR("Get audio shm buf error\n");
            return SUCCESS; // 有极小概率音频上电会初始化失败，音频获取不到不用管
        }

        shm_buf_get_media_info(audio_buf_, &audio_type_, &sample_rate_, &bit_width_, &width, &height);
        if (!sample_rate_ || !bit_width_ || audio_type_ == SHM_MEDIA_UNKOWN) {
            GB_ERR("Get audio info error, %d %d %d\n", sample_rate_, bit_width_, audio_type_);
            //break;
        }

        // 添加一路音频 ps 编码信息
        bzero(&encode_info, sizeof(encode_info));
        encode_info.audio_info.sample_rata = sample_rate_;
        encode_info.audio_info.bit_width = bit_width_;

        if (audio_type_ == SHM_MEDIA_AUDIO_ALAW) {
            stream_type = STREAM_TYPE_G711A;
        } else {
            stream_type = STREAM_TYPE_G711U;
        }

        audio_stream_id_ = PsAddStream(stream_type, encode_info);

        return SUCCESS;
    } while(0);

    return FAILURE;
}

void GbLiveStreaming::ReadVideo(void *data)
{
    if (data == NULL)
        return ;

    GbLiveStreaming *streaming = (GbLiveStreaming *)data;
    shm_buf_read_frame_ex(streaming->vide_buf_, streaming->video_serial_, CbDoReadVideo, streaming);
}

void GbLiveStreaming::ReadAudio(void *data)
{
    if (data == NULL)
        return ;

    GbLiveStreaming *streaming = (GbLiveStreaming *)data;
    shm_buf_read_frame_ex(streaming->audio_buf_, streaming->audio_serial_, CbDoReadAudio, streaming);
}

void GbLiveStreaming::CbDoReadVideo(void *userdata, tSBFrame* cur_frame)
{
    if (userdata == NULL)
        return ;

    GbLiveStreaming *streaming = (GbLiveStreaming *)userdata;
    streaming->DoReadVideo(cur_frame);
}

void GbLiveStreaming::DoReadVideo(tSBFrame* cur_frame)
{
    // shmbuf 的帧号是不会回退的，如果取过帧了，并且当前要取的帧序号大于 buf 里面最新的帧号超过一定值，表示 buf 被重置了
    //  会存在 video_serial_ - curframe_serial = 1 的情况，我们会 ++video_serial_ 取下一帧
    if(cur_frame->mediatype == SHM_MEDIA_UNKOWN ||
        (video_serial_ > 0 && video_serial_ > cur_frame->curframe_serial + 2)) {

        GB_DBG("[%p]the vbuf has been reset! cur_frame->mediatype:%d cur_frame->curframe_serial:%d vserial:%d\n",
               this, cur_frame->mediatype, cur_frame->curframe_serial, video_serial_);
        goto err;
    }

    switch(cur_frame->error) {
        case SHM_ERR_SUCCESS:
            if (cur_frame->frame_type == SHM_FRAME_VIDEO_I) {
                if (width_ != cur_frame->v.width || height_ != cur_frame->v.height) {
                    goto err; // 当前帧的宽高不等于获取到的宽高，表明通道已经 reset 了，需要刷新参数
                }
            }
            PsPackH26x(cur_frame->framedata, cur_frame->frame_size, cur_frame->frame_timestamp,
                        video_stream_id_, cur_frame->frame_type == SHM_FRAME_VIDEO_I,
                        vps_data_, vps_data_size_, sps_data_, sps_data_size_, pps_data_, pps_data_size_); // ps 打包
            video_serial_ = cur_frame->frame_serial + 1; // 取下一帧
            break;
        case SHM_ERR_NOT_READY:
            break ;

        case SHM_ERR_OVER_WRITE:
            GB_DBG("[%p]read vframe serial:%d over write!\n", this, video_serial_);
            video_serial_ = -1;
            break ;

        default:
            GB_ERR("read vframe data error:%d!\n", cur_frame->error);
            break;
    }

    return ;
err:
    // 获取帧信息出错，重新获取信息
    DelayStartLiveStreaming();
    return;
}

void GbLiveStreaming::CbDoReadAudio(void *userdata, tSBFrame* cur_frame)
{
    if (userdata == NULL)
        return ;

    GbLiveStreaming *streaming = (GbLiveStreaming *)userdata;
    streaming->DoReadAudio(cur_frame);
}

void GbLiveStreaming::DoReadAudio(tSBFrame* cur_frame)
{
    unsigned char audio_stream_8k[AUDIO_IN_NUM_PERFRM] = {0};
    unsigned int i = 0, j = 0;

    if(cur_frame->mediatype == SHM_MEDIA_UNKOWN ||
        (audio_serial_ > 0 && audio_serial_ > cur_frame->curframe_serial + 2)) {

        GB_DBG("[%p]the abuf has been reset! cur_frame->mediatype:%d cur_frame->curframe_serial:%d vserial:%d\n",
               this, cur_frame->mediatype, cur_frame->curframe_serial, audio_serial_);
        goto err;
    }

    switch(cur_frame->error) {
        case SHM_ERR_SUCCESS:
            for (j = 0; j < cur_frame->frame_size; j += 2) {
                audio_stream_8k[i++] = cur_frame->framedata[j];
            }

            PsPackAudio((char *)audio_stream_8k, cur_frame->frame_size / 2, cur_frame->frame_timestamp, audio_stream_id_);
            audio_serial_ = cur_frame->frame_serial + 1; // 取下一帧;
            break;

        case SHM_ERR_NOT_READY:
            break;

        case SHM_ERR_OVER_WRITE:
            GB_DBG("[%p]read aframe serial:%d over write!\n", this, audio_serial_);
            audio_serial_ = 0; // 读慢了，取最新帧
            break ;

        default:
            GB_DBG("[%p]read aframe data error:%d!\n", this, cur_frame->error);
            break ;
    }
    return ;
err:
    // 音频改变，重新获取信息
    DelayStartLiveStreaming();
    return;
}

int GbLiveStreaming::OnPsStream(uint8_t *ps, uint32_t ps_size, bool is_video, bool is_video_key, bool is_first, bool is_end)
{
    return RtpPack((const char*)ps, ps_size, is_video, is_video_key, is_first, is_end, RTP_PS);
}

/*处理 rtp 数据*/
int GbLiveStreaming::OnRtpPackage(uint8_t *package, uint16_t package_size, bool is_video, bool is_video_key, bool is_first, bool is_end)
{
    int ret = 0;

    if (vistor_list_.size() == 0) { // vistor 被删完了, 停止推流
        StopLiveStreaming();
        return FAILURE;
    }

    // 遍历所有 vistor 通知处理 rtp 数据
    std::list<GbVistor *>::iterator it = vistor_list_.begin();
    for (; it != vistor_list_.end(); ) {
        GbVistor *vistor = *it;
        if (vistor == NULL) {
            it = vistor_list_.erase(it); // 擦除空的 list
            continue;
        }

        if (!vistor->send_key_) {
            if (!is_video_key && !is_first) {
                continue;
            }

            vistor->send_key_ = true;
        }

        ret = vistor->RtpRepack(package, package_size, is_video, is_end);
        if (ret < 0) {
            // rtp 推流失败，删除会话
            GB_DBG("delete visitor %p\n", *it);
            it = vistor_list_.erase(it);
            continue;
        }

        ++it;
    }

    return SUCCESS;
}

/*停止推流，只有 vistor 删除完了，或者是析构的时候能调用*/
int GbLiveStreaming::StopLiveStreaming()
{
    js_delete_timer_r(&audio_loop_);
    js_delete_timer_r(&video_loop_);
    js_delete_timer_r(&delay_start_);

    // 参数复位
    vide_buf_ = NULL;
    video_serial_ = -1;
    video_type_ = SHM_MEDIA_UNKOWN;
    bps_ = fps_ = width_ = height_ = 0;

    bzero(vps_data_, sizeof(vps_data_));
    bzero(sps_data_, sizeof(sps_data_));
    bzero(pps_data_, sizeof(pps_data_));
    vps_data_size_ = 0;
    sps_data_size_ = 0;
    pps_data_size_ = 0;

    audio_buf_ = 0;
    audio_idx_ = 0;
    audio_serial_ = 0;
    audio_type_ = SHM_MEDIA_UNKOWN;
    sample_rate_ = bit_width_ = 0;

    PsReset(); // 复位 ps 编码器
    RtpReset(); // 复位 RTP 编码器
    return SUCCESS;
}


GbLiveStreamingMng::GbLiveStreamingMng(JSScheduler &sch, GbProfile &profile) : sch_(sch), profile_(profile), chn_mng(GB_ENCODE_MAX)
{
    // 初始化编码通道
    chn_mng[GB_ENCODE_MAIN] = new GbLiveStreaming(profile, sch, GB_ENCODE_MAIN, SHM_BUF_MAIN, SHM_BUF_AUDIO);
    chn_mng[GB_ENCODE_SUB] = new GbLiveStreaming(profile, sch, GB_ENCODE_SUB, SHM_BUF_SUB, SHM_BUF_AUDIO);
}

GbLiveStreamingMng::~GbLiveStreamingMng()
{
    // sch_ 是外面传进来的，由外面释放
    for (int i = 0; i < GB_ENCODE_MAX; ++i) {
        if (chn_mng[i] != NULL) {
            delete chn_mng[i];
            chn_mng[i] = NULL;
        }
    }
    GB_DBG("Gb live streaming manage deinit success\n");
}



void GbLiveStreamingMng::StartPushStreaming(SdpParse *sdp_parse, osip_call_id_t *call_id)
{
    if (sdp_parse == NULL || call_id == NULL) {
        GB_ERR("sdp_parse[%p] || call_id[%p] is NULL\n", sdp_parse, call_id);
        return;
    }

    SchedulerParam<GbLiveStreamingMng, SdpParse *> param(GetSharedPtr(), true, 0);
    param.set_data1(sdp_parse);
    param.set_data2(call_id);

    js_run_function(sch_, StartPushStreamingCb, &param, true);
}

void GbLiveStreamingMng::StopPushStreaming(osip_call_id_t *call_id)
{
    if (call_id == NULL) {
        GB_ERR("call_id is NULL\n");
    }

    SchedulerParam<GbLiveStreamingMng, osip_call_id_t *> param(GetSharedPtr(), true, 0);
    param.set_data1(call_id);

    js_run_function(sch_, StopPushStreamingCb, &param, true);
}

void GbLiveStreamingMng::StopAllPushStreaming()
{
    SchedulerParam<GbLiveStreamingMng, osip_call_id_t *> param(GetSharedPtr(), true, 0);

    js_run_function(sch_, StopAllPushStreamingCb, &param, true);
}

bool GbLiveStreamingMng::FindCallId(osip_call_id_t *call_id)
{
    /*遍历所有推流器，查找观看者*/
    for (int i = 0; i < GB_ENCODE_MAX; ++i) {
        if (chn_mng[i]->FindVistor(call_id) != NULL)
            return true;
    }

    return false;
}

/*推流失败 live streaming 会删除 vistor，对 vistor 操作要保证在一个调度里面运行*/
void GbLiveStreamingMng::StartPushStreamingCb(void *data)
{
    SchedulerParam<GbLiveStreamingMng, SdpParse *> *param =
                (SchedulerParam<GbLiveStreamingMng, SdpParse*> *)data;

    SdpParse *sdp_parse = param->get_data1();
    osip_call_id_t *call_id = (osip_call_id_t *)param->get_data2();
    std::shared_ptr<GbLiveStreamingMng> &this_ptr = param->get_this_ptr();

    // 会话已经存在，不重复添加
    if (this_ptr->FindCallId(call_id)) {
        GB_ERR("Current session already exists\n");
        return ;
    }

    // 创建会话，vistor 的构造函数会解析 sdp 的编码信息，和添加 call_id 信息
    GbVistor *vistor = new GbVistor(*sdp_parse, *call_id, this_ptr->profile_);
    if (vistor == NULL) {
        GB_ERR("New vistor fail\n");
        return ;
    }

    // 判断会话分辨率
    GbEncodeChannel current_channel_ = GB_ENCODE_MAIN;
    if (vistor->video_venc_  < GB_1080P) { // 子码流
        current_channel_ = GB_ENCODE_SUB;
    } else { // 主码流
        current_channel_ = GB_ENCODE_MAIN;
    }

    /*添加观看者，添加成功直接返回，失败释放 vistor 返回 FAILURE*/
    for (int i = 0; i < GB_ENCODE_MAX; ++i) {
        if (this_ptr->chn_mng[i]->encode_channel_ == current_channel_) {
            int ret = this_ptr->chn_mng[i]->AddVistor(vistor);
            if (ret == SUCCESS)
                return ; // 成功，返回
        }
    }

    // 未找到匹配的通道，添加失败
    if (vistor != NULL)
        delete vistor;

    GB_ERR("Add vistor fail\n");

    return ;
}

/*推流失败 live streaming 会删除 vistor，对 vistor 操作要保证在一个调度里面运行*/
void GbLiveStreamingMng::StopPushStreamingCb(void *data)
{
    SchedulerParam<GbLiveStreamingMng, osip_call_id_t *> *param =
            (SchedulerParam<GbLiveStreamingMng, osip_call_id_t *> *) data;
    osip_call_id_t *call_id = param->get_data1();
    std::shared_ptr<GbLiveStreamingMng> &this_ptr = param->get_this_ptr();

    /*遍历所有推流器，删除观看者*/
    for (int i = 0; i < GB_ENCODE_MAX; ++i) {
        this_ptr->chn_mng[i]->DeleteVistor(call_id);
    }

    return ;
}

void GbLiveStreamingMng::StopAllPushStreamingCb(void *data)
{
    SchedulerParam<GbLiveStreamingMng, osip_call_id_t *> *param =
            (SchedulerParam<GbLiveStreamingMng, osip_call_id_t *> *) data;

    std::shared_ptr<GbLiveStreamingMng> &this_ptr = param->get_this_ptr();

    /*遍历所有推流器，删除观看者*/
    for (int i = 0; i < GB_ENCODE_MAX; ++i) {
        this_ptr->chn_mng[i]->DeleteAllVistor();
    }
}
