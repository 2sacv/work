/** Copyright (C) by Jabsco Company
*
* @File Name    : gb_record.h
* @Created Time : 2024-05-06
* @Version      : 1.0
* @Author       :
* @Description  : 国标录像查询推流控制
*/
#include <time.h>

#include "js_scheduler.h"
//#include "osip2/osip.h"
#include "osipparser2/osip_message.h"
#include "osipparser2/osip_parser.h"
#include "record_lib.h"
#include "mpeg-util.h"
#include "record_disk.h"

#include "gb_client_manager.h"
#include "gb_record_mng.h"
#include "gb_dialog_mng.h"

#define SECS_OF_DAY (60*60*24)

GbRecordPlay::GbRecordPlay(GbRecordMng &record_mng, GbProfile &profile, JSScheduler &sch, int no, SdpParse *sdp_parse, std::shared_ptr<InviteDialog> &invite_ptr) :
profile_(profile), no_(no), sch_(sch), push_loop_(NULL), video_serial_(-1), video_fps_(0), video_type_(eVideoType_None), video_timestamp_(0),
audio_exist_(false), audio_serial_(0), audio_timestamp_(0.0), start_time_(0), end_time_(0), play_status_(PLAY), play_scale_(1), invite_ptr_(invite_ptr),
video_stream_id_(-1), audio_stream_id_(-1), create_time_(mono_sec())
{
    bzero(vps_data_, sizeof(vps_data_));
    vps_data_size_ = 0;
    bzero(sps_data_, sizeof(sps_data_));
    sps_data_size_ = 0;
    bzero(pps_data_, sizeof(pps_data_));
    pps_data_size_ = 0;

    // 参数配置
    SetPlayScale(sdp_parse->get_download_speed());
    set_ssrc(sdp_parse->get_ssrc());

    // 网络连接
    GbSetup connect_setup = sdp_parse->get_setup();
    if (connect_setup == GB_ACTIVE && socket_.get_network_protocols() == GB_TCP) {
        ;// 服务器主动连接，需要设备监听，这个方式仅局域网或者公网 IP 有用，暂时不实现 wuhy
    } else {
        // 服务器被动，设备需要主动连接
        int ret = socket_.ServerConnect(sdp_parse->GetConnectType(), profile_.get_media_port(),
                                    sdp_parse->get_ip(), sdp_parse->get_media_port(), 10);
        if (ret < 0) {
            GB_ERR("gb record connect server fail\n");
            play_status_ = TEARDOWN;
            return ;
        }
    }

    // 查找打开录像文件
    int ret = OpenRecordFile(sdp_parse->get_start_time(), sdp_parse->get_end_time());
    if (ret != SUCCESS) {
        play_status_ = TEARDOWN;
        return ;
    }

    js_create_timer_r(sch_, 10, 10, PushRecord, this, &push_loop_);
}

GbRecordPlay::~GbRecordPlay()
{
    StopRecordPlay();

    socket_.CloseSocket();

    GB_DBG("deinit record play\n");
}

/*录像推流过程中参数变更*/
void GbRecordPlay::ParamChange(SdpParse *sdp_parse)
{
    /* 清除上一次播放 */
    StopRecordPlay();

    /* 查找打开录像文件 */
    int ret = OpenRecordFile(sdp_parse->get_start_time(), sdp_parse->get_end_time());
    if (ret != SUCCESS) {
        play_status_ = TEARDOWN;
        return ;
    }

    /* 参数配置 */
    SetPlayScale(sdp_parse->get_download_speed());
    set_ssrc(sdp_parse->get_ssrc());

    /* 网络连接 */
    if (sdp_parse->get_connect() == "new") {
        // 需要新建连接
        socket_.CloseSocket();
        ret = socket_.ServerConnect(sdp_parse->GetConnectType(), profile_.get_media_port(),
                                    sdp_parse->get_ip(), sdp_parse->get_media_port(), 10);
        if (ret < 0) {
            GB_ERR("gb record connect server fail\n");
            play_status_ = TEARDOWN;
            return ;
        }
    }

    js_create_timer_r(sch_, 10, 10, PushRecord, this, &push_loop_);
    return ;
}

void GbRecordPlay::SetPlayOffset(time_t offset)
{
    /*参数判断*/
    if (start_time_ + offset > end_time_) {
        GB_ERR("start_time[%lld] end_time[%lld] offset[%lld] error\n", start_time_, end_time_, offset);
        return ;
    }

    /* 清除上一次播放 */
    StopRecordPlay();

    /* 查找打开录像文件 */
    int ret = OpenRecordFile(start_time_ + offset, end_time_);
    if (ret != SUCCESS) {
        play_status_ = TEARDOWN;
        return ;
    }

    js_create_timer_r(sch_, 10, 10, PushRecord, this, &push_loop_);
    return ;
}

/*设置播放速度*/
void GbRecordPlay::SetPlayScale(double play_scale)
{
    /*国标文档规定，播放速度只有 0.25/0.5/1/2/4 五档，这里向上取整*/
    if (play_scale <= 0.25) {
        play_scale_ = 0.25;
    } else if (play_scale <= 0.5) {
        play_scale_ = 0.5;
    } else if (play_scale <= 1) {
        play_scale_ = 1;
    } else if (play_scale <= 2) {
        play_scale_ = 2;
    } else {
        play_scale_ = 4;
    }

    GB_DBG("set play scale:%lf\n", play_scale_);
    return ;
}

/*打开录像文件，成功返回 SUCCESS 失败返回 FAILURE*/
int GbRecordPlay::OpenRecordFile(time_t start_time, time_t end_time)
{
    /*首先查找对应文件*/
    GbRecordQuery query(0, start_time, end_time, MAX_RECS_OF_DAY, true);

    sRec1File *record_info = query.GetBestMatchVideo(start_time, end_time);
    if (record_info == NULL) {
        GB_ERR("not find record, start_time[%lld] end_time[%lld]\n", start_time, end_time);
        return FAILURE;
    }
    start_time_ = record_info->start_time;
    end_time_ = record_info->stop_time;

    GB_DBG("play start time[%lld] end time[%lld]\n", start_time, end_time);
    GB_DBG("record start time[%lld] end time[%lld]\n", start_time_, end_time_);

    /*打开文件*/
    char file_path[128] = {0};
    if (GetRecordFilePath(file_path, sizeof(file_path), start_time, record_info->file_name) != SUCCESS) {
        GB_ERR("get record path fail\n");
        return FAILURE;
    }

    GB_DBG("file path:%s\n", file_path);

    mp4_read_.Close();
    int ret = mp4_read_.Open(file_path);
    if (ret < 0) {
        GB_ERR("open record file[%s] fail\n", file_path);
        return FAILURE;
    }

    /*获取 mp4 流信息*/
    tagMP4VideoInfo video_info;
    tagMP4AudioInfo audio_info;

    mp4_read_.GetVideoInfo(video_info);
    mp4_read_.GetAudioInfo(audio_info);

    video_fps_ = video_info.fps;
    video_type_ = video_info.video_type;
    audio_exist_ = audio_info.bValid;

    GB_DBG("record video info fps:%lf width:%d height:%d\n", video_info.fps, video_info.width, video_info.height);
    GB_DBG("record audio info valid:%d type:%d\n", audio_info.bValid, audio_info.type);

    /*获取 vps sps pps 信息*/
    if (video_info.video_type == eVideoType_H265) {
        nbo_w32((uint8_t *)vps_data_, 0x00000001);
        ret = mp4_read_.GetExtData(eExtData_vps, (LPBYTE)vps_data_ + 4, sizeof(vps_data_), vps_data_size_);
        if (ret < 0) {
            GB_ERR("record get vps info error\n");
            return FAILURE;
        }
        vps_data_size_ += 4;
    }

    nbo_w32((uint8_t *)sps_data_, 0x00000001);
    ret = mp4_read_.GetExtData(eExtData_sps, (LPBYTE)sps_data_ + 4, sizeof(sps_data_), sps_data_size_);
    if (ret < 0) {
        GB_ERR("record get sps info error\n");
        return FAILURE;
    }
    sps_data_size_ += 4;

    nbo_w32((uint8_t *)pps_data_, 0x00000001);
    ret = mp4_read_.GetExtData(eExtData_pps, (LPBYTE)pps_data_ + 4, sizeof(pps_data_), pps_data_size_);
    if (ret < 0) {
        GB_ERR("record get pps info error\n");
        return FAILURE;
    }
    pps_data_size_ += 4;

    /*添加 ps 编码信息*/
    PsReset(); // 添加之前先复位 ps 编码信息

    StreamType stream_type = STREAM_TYPE_UNKNOWN;
    EncodeInfo encode_info = {0};

    /*添加一路视频 ps 编码信息*/
    encode_info.video_info.bps = 0; // 这个是一个描述信息，获取不到填 0 就行
    encode_info.video_info.fps = video_info.fps;
    encode_info.video_info.width = video_info.width;
    encode_info.video_info.height = video_info.height;
    if (video_info.video_type == eVideoType_H264) {
        stream_type = STREAM_TYPE_H264;
    } else {
        stream_type = STREAM_TYPE_H265;
    }
    video_stream_id_ = PsAddStream(stream_type, encode_info);

    /*添加一路音频 ps 编码信息*/
    if (audio_exist_) {
        bzero(&encode_info, sizeof(encode_info));
        // 采样率和位宽，audio info 里面没有，这里写死
        encode_info.audio_info.sample_rata = 8000; // IV_CM_AENC_SAMPLE_RATE_8000
        encode_info.audio_info.bit_width = 16; // IV_CM_AENC_BIT_WIDTH_16
        if (audio_info.type == RTP_TYPE_ALAW) {
            stream_type = STREAM_TYPE_G711A;
        } else {
            stream_type = STREAM_TYPE_G711U;
        }
        audio_stream_id_ = PsAddStream(stream_type, encode_info);
    }

    /*文件开头可能大于指定开始时间，跳转到指定时间播放*/
    if (start_time > start_time_) {
        int frame_no = (start_time - start_time_) * video_fps_;
        GB_DBG("record utc:%lld   sdp start uct:%lld   seek to frame:%d\n", start_time_, start_time, frame_no);
        mp4_read_.SeekToFrame(frame_no);
    }

    mp4_read_.SeekToIFrame();

    /*初始化参数，准备开始推流*/
    video_serial_ = -1;
    audio_serial_ = 0;
    play_status_ = PLAY;

    return SUCCESS;
}
/*获取录像文件目录，成功返回 SUCCESS，失败返回 FAILURE*/
int GbRecordPlay::GetRecordFilePath(char *buf, uint32_t buf_size, time_t start_time, const char *file_name)
{
    /*查看内存卡是否存在*/
    char mmcpath[128] = {0};
    if (storage_get_mmcpath(mmcpath) == -1) {
        GB_DBG("not find mmc path\n");
        return FAILURE;
    }

    /*时间转换成日期，只要年月日*/
    struct tm gbk_time = {0};
    start_time += 28800; // 国标默认东八区
    gmtime_r(&start_time, &gbk_time);

    uint32_t yyyymmdd = (gbk_time.tm_year + 1900)*10000 + (gbk_time.tm_mon +1)*100 + gbk_time.tm_mday;

    snprintf(buf, buf_size, "%s/IPCamera/%u/%s", mmcpath, yyyymmdd, file_name);

    return SUCCESS;
}

/*取流推录像*/
void GbRecordPlay::PushRecord(void *data)
{
    GbRecordPlay *play = (GbRecordPlay *)data;

    play->PushRecordCb();
}

/*取录像推流处理*/
void GbRecordPlay::PushRecordCb()
{
    char video_buf[MAX_RECORD_VIDEO_SIZE] = {0};
    char audio_buf[MAX_RECORD_AUDIO_SIZE] = {0};
    int read_size = 0;
    uint32_t duration = 0;
    bool is_key = false;
    int ret = 0;
    double cur_time = mono_stamp();

    if (play_status_ != PLAY)
        return ;

    if (video_serial_ == -1) { // 取 I 帧的时候拉平音视频时间戳
        video_timestamp_ = audio_timestamp_ = cur_time;
    }

    // 视频推流判断
    if (cur_time >= video_timestamp_ ) {
        do {
            nbo_w32((uint8_t *)video_buf, 0x00000001);
            ret = mp4_read_.ReadVideoFrame(LPBYTE(video_buf + 4), sizeof(video_buf), read_size, duration);
            if (ret < 0 || read_size <= 0) {
                GB_DBG("NO:%d video has been read\n", no_);
                goto read_complete_;
            }
            read_size += 4;

            // 判断是否是 I 帧
            if(((7 == (video_buf[4]&0x1f) || 5 == (video_buf[4]&0x1f)) && video_type_ == eVideoType_H264) || (0x26 == video_buf[4] && video_type_ == eVideoType_H265)) {
                is_key = true;
                //GB_INFO("record I frame size:%d\n", read_size);
            }

            if (video_serial_ == -1 && !is_key) {
                break;
            }

            video_serial_++;
            // 视频的时间粒度是 90000
            video_timestamp_ += duration / (play_scale_ * 90000.0);
            ret = PsPackH26x(video_buf, read_size, video_timestamp_, video_stream_id_, is_key,
                            vps_data_, vps_data_size_, sps_data_, sps_data_size_, pps_data_, pps_data_size_); // ps 打包
            if (ret != SUCCESS) {
                GB_ERR("NO:%d ps pack h26x error\n", no_);
                goto err_;
            }
        } while (0);
    }

    // 音频推流判断，音频一定要在视频发完第一个 I 帧之后才能发，否则 ps 流解析会异常
    if (cur_time >= audio_timestamp_ && audio_exist_ && video_serial_ != -1) {
        do {
            ret = mp4_read_.ReadAudioFrame(LPBYTE(audio_buf), sizeof(audio_buf), read_size, duration);
            if (ret < 0 || read_size <= 0) {
                GB_DBG("NO:%d audio has been read\n", no_);
                audio_timestamp_ += 60 * 60; // 音频取完了，后续不用再进来了，推流结束以视频为准
                break;
            }
            // 音频是按照 8000 的粒度
            audio_timestamp_ += duration / (play_scale_ * 8000);
            ret = PsPackAudio(audio_buf, read_size, audio_timestamp_, audio_stream_id_);
            if (ret != SUCCESS) {
                GB_ERR("NO:%d ps pack audio error\n", no_);
                goto err_;
            }
        } while (0);
    }

    return ;
read_complete_:
    play_status_ = COMPLETE;
    GbClientManager::Enqueue(RECORD_END); // 入队事件，通知主线程录像播放完成
    return ;
err_:
    StopRecordPlay();
    return ;
}

/*ps 流处理*/
int GbRecordPlay::OnPsStream(uint8_t *ps, uint32_t ps_size, bool is_video, bool is_video_key, bool is_first, bool is_end)
{
    return RtpPack((const char*)ps, ps_size, is_video, is_video_key, is_first, is_end, RTP_PS);
}

/*rtp 包处理函数*/
int GbRecordPlay::OnRtpPackage(uint8_t *package, uint16_t package_size, bool is_video, bool is_video_key, bool is_first, bool is_end)
{
    int ret = 0;

    if (socket_.get_network_protocols() == GB_TCP) {
        ret = socket_.Send((const char *)package, package_size, 3);
    } else {
        ret = socket_.Send((const char *)package + 2, package_size - 2); // udp 跳过前两个长度字节
    }

    if (ret < 0) {
        GB_ERR("send rtp error\n");
        return FAILURE;
    }

    return SUCCESS;
}

/*设置播放状态*/
void GbRecordPlay::SetPlayStatus(PlayStatus play_status)
{
    if (play_status_ == TEARDOWN || play_status_ == COMPLETE) {
        return ; // 停止状态不允许变更
    }

    if (play_status_ == PAUSE && play_status == PLAY) {
        // 从暂停状态恢复到播放状态，需要同步一次时间
        video_timestamp_ = audio_timestamp_ = mono_sec();
    }

    GB_DBG("set play status:%d\n", play_status);

    play_status_ = play_status;

    return;
}

/*停止录像推流*/
void GbRecordPlay::StopRecordPlay()
{
    js_delete_timer_r(&push_loop_);
    mp4_read_.Close();

    play_status_ = TEARDOWN;
    GB_DBG("NO:%d stop record play\n", no_);
}

/*----------------------------------------record qurey--------------------------------------*/

/**
 * 用二分查找符合指定时间的录像，符合条件指录像的开始时间小于等于该时间，结束时间大于等于该时间
 *
 * @param[reclist] 录像列表
 * @param[file_unm] 列表里面文件个数
 * @param[file_unm] 目标时间
 *
 * @return 满足条件的录像下标或者 -1 失败
 */
int BinarySearchRecord(sRec1File *reclist, uint32_t file_unm, time_t dst_time)
{
    int left = 0;
    int right = file_unm;

    while(left <= right) {
        int mid = left + (right - left) / 2;

        // 满足条件，返回该元素
        if ((time_t)reclist[mid].start_time <= dst_time && (time_t)reclist[mid].stop_time >= dst_time) {
            return mid;
        } else if ((time_t)reclist[mid].stop_time < dst_time ) { // 更新左节点
            left = mid + 1;
        } else { // 更新右节点
            right = mid - 1;
        }
    }

    return FAILURE;
}

/**
 * 录像查询任务构造函数
 *
 * @param[start_time] 查询的开始 UTC 时间
 * @param[end_time] 查询的结束 UTC 时间
 * @param[sn] 国标 xml 会话的标识，这里用来标识这个查询任务，后续通过 sn 号查询出对应的任务
 * @param[qurey_unmber] 要查询录像的数量，0 为全部查询
 * @param[need_size] 是否需要查询真实大小
 *
 */
GbRecordQuery::GbRecordQuery(uint32_t sn, time_t start_time, time_t end_time, uint32_t qurey_unmber, bool need_size) :
total_(0), strat_time_(start_time), end_time_(end_time), sn_(sn), qurey_count_(0), create_time_(mono_sec())
{
    if (start_time < SECS_OF_DAY || end_time < SECS_OF_DAY || start_time > end_time) {
        GB_ERR("record query param error\n");
        return ;
    }

    if (qurey_unmber == 0 || qurey_unmber > MAX_RECS_OF_DAY)
        qurey_unmber = MAX_RECS_OF_DAY;

    bzero(record_list_, sizeof(record_list_));

    int tz_off = 28800; // 时区偏移写死东八区，gb 只用中国时间
    sRec1File *record_list = record_list_;
    do {
        char dir_path[128] = {0};
        time_t cur_start_time = start_time;

        // 国标录像查询要支持跨天，这里对开始时间进行处理
        // 不满一天的加上剩余时间，满一天的加上一天的秒数
        time_t time_left = (start_time + tz_off) % SECS_OF_DAY; // 计算一天剩余时间
        start_time += time_left ? SECS_OF_DAY - time_left : SECS_OF_DAY;

        int yyyymmdd = record_get_ymd_of_epoch(cur_start_time);
        if (record_get_path_of_ymd(yyyymmdd, dir_path, sizeof(dir_path), 0) < 0) {
            GB_DBG("gb get record path dir failed\n");
            break;
        }

        int file_num = record_query_list(cur_start_time, qurey_unmber, record_list, 0);
        if (file_num <= 0) {
            GB_DBG("gb not find file:%d\n", yyyymmdd);
            continue;
        }

        // 返回的结果是从开始时间到当天零点，还需要截取大于结束时间的部分
        int got = BinarySearchRecord(record_list, file_num, end_time);
        if (got > 0) {
            file_num = got;
        }

        if (need_size) {
            // 查出来的结果的 file_size 是指文件长度，国标下载服务需要获取文件大小，这里获取一下
            for (int i = 0; i < file_num; i++) {
                char file_path[256];
                struct stat st;
                snprintf(file_path, sizeof(file_path), "%s/%s", dir_path, record_list[i].file_name);
                if (stat(file_path, &st) == 0) {
                    record_list[i].file_secs = st.st_size;
                }
            }
        }

        total_ += file_num;
        record_list += file_num;
        qurey_unmber -= file_num;
    } while(start_time < end_time && qurey_unmber > 0);

    return ;
}

GbRecordQuery::~GbRecordQuery()
{
    GB_INFO("Deinit record qurey\n");
}

/*继续获取下一条录像，没有返回 NULL*/
sRec1File *GbRecordQuery::ResumeGetRcordInfo()
{
    if (qurey_count_ < total_) {
        return &record_list_[qurey_count_++];
    }

    return NULL;
}

/*
* 录像有概率重和一部分，该函数获取一条录像，优先精准匹配，如果不能精准匹配的话，选取开始时间最早的
*/
sRec1File *GbRecordQuery::GetBestMatchVideo(time_t start_time, time_t end_time)
{
    if (total_ == 0) {
        return NULL;
    }

    if (total_ == 1) {
        GB_DBG("total[%u] == 1\n", total_);
        return  &record_list_[0];
    }

    int dst = -1;
    for (uint32_t i = 0; i < total_; ++i) {

        time_t record_start_time = (time_t)record_list_[i].start_time;
        time_t record_end_time   = (time_t)record_list_[i].stop_time;
        // 精准匹配，成功直接返回
        if (record_start_time == start_time &&
                record_end_time == end_time) {
            GB_DBG("%i is eligible\n", i);
            return &record_list_[i];
        }

        // 非精准匹配，选择最近的开始时间大于结束时间的录像，不能等于
        if (dst == -1 && start_time > record_end_time) {
            dst = i;
        }
    }

    if (dst == -1)
        dst = 0;

    return &record_list_[dst];
}

/*----------------------------------------record mng--------------------------------------*/

/*录像查询，播放控制器构造函数*/
GbRecordMng::GbRecordMng(GbProfile &profile, JSScheduler &sch) : profile_(profile), sch_(sch), play_list_(MAX_PLAY), query_list_(MAX_QUERY)
{
    GB_DBG("Init record mng\n");
}

GbRecordMng::~GbRecordMng()
{
    ClearQureyList();
    for (int i = 0; i < MAX_PLAY; ++i) {
        DeletePlay(i);
    }
    GB_DBG("Deinit record mng\n");
}

/**
 * 添加一个录像查询任务
 *
 * @param[sn] 国标 xml 会话的标识，这里用来标识这个查询任务，后续通过 sn 号查询出对应的任务
 * @param[start_time] 查询的开始 UTC 时间
 * @param[end_time] 查询的结束 UTC 时间
 *
 * @return FAILURE 或者查询任务的数组下标
 */
int GbRecordMng::AddRecordQuery(uint32_t sn, time_t start_time, time_t end_time)
{
    int pos = FindQureyEmptySlot();
    if (pos < 0) {
        GB_ERR("not find qurey empty slot\n");
        return FAILURE;
    }

    GbRecordQuery *qurey = new GbRecordQuery(sn, start_time, end_time);
    if (qurey == NULL) {
        GB_ERR("new record qurey fail\n");
        return FAILURE;
    }
    query_list_[pos] = qurey;

    return pos;
}

/*通过 sn 查找录像查询任务，成功返回下标，失败返回 FAILURE*/
int GbRecordMng::FindQureyBySn(uint32_t sn)
{
    for (int i = 0; i < MAX_QUERY; ++i) {
        GbRecordQuery *qurey = query_list_[i];
        if (qurey != NULL && qurey->get_sn() == sn) {
            return i;
        }
    }

    return FAILURE;
}

void GbRecordMng::DeleteQureyBySn(uint32_t sn)
{
    int pos = FindQureyBySn(sn);
    if (pos < 0)
        return;

    DeleteQurey(pos);

    return;
}

/*通过下标获取录像总数*/
int GbRecordMng::GetQureyTotal(uint32_t pos)
{
    if (pos >= MAX_QUERY) {
        GB_ERR("pos[%u] error\n", pos);
        return 0;
    }

    if (query_list_[pos] != NULL)
        return (int)query_list_[pos]->get_total();

    return 0;
}

/*清除指定下标删除录像查询*/
void GbRecordMng::DeleteQurey(uint32_t pos)
{
    if (pos > MAX_QUERY) {
        GB_ERR("pos[%u] error\n", pos);
        return ;
    }

    GbRecordQuery *query = query_list_[pos];
    if (query == NULL)
        return ;

    GB_DBG("delete record query[%u]\n", pos);
    delete query;
    query_list_[pos] = NULL;

    return ;
}

/**
 * 通过下标继承查找录像，一次返回一个录像
 *
 * @param[pos] 录像对象的下标
 *
 * @return 查找完成或者出错将返回 NULL, 否则返回描述录像信息的结构体指针
 */
sRec1File *GbRecordMng::ResumeGetRcordInfo(uint32_t pos)
{
    if (pos > MAX_QUERY)
        return NULL;

    GbRecordQuery *query = query_list_[pos];
    if (query == NULL)
        return NULL;

    return query->ResumeGetRcordInfo();
}

/**
 * 发现空闲位置，同时会清理超时的会话
 *
 * @return 空闲位置下标或者 FAILURE
 */
int GbRecordMng::FindQureyEmptySlot()
{
    time_t current_time = mono_sec();
    time_t oldest_time = 0;
    time_t oldest_place = 0;

    for (int i = 0; i < MAX_QUERY; ++i) {
        GbRecordQuery *qurey = query_list_[i];

        if (qurey != NULL) {
            // 已有录像，判断是否超时需要清理
            if (current_time - qurey->get_create_time() > QUERY_TIMEOUT) {
                DeleteQurey(i);
                return  i;
            } else if (oldest_time == 0 || qurey->get_create_time() < oldest_time) {
                oldest_time = qurey->get_create_time();
                oldest_place = i;
            }
            // 未超时继续下一个
        } else {
            //当前位置空闲
            return i;
        }
    }

    // 理论上来说新请求需要被优先响应，如果没找到空位的话就清除时间最老的会话
    if (oldest_time != 0) {
        DeleteQurey(oldest_place);
        return oldest_place;
    }

    return FAILURE;
}

/*清除全部录像会话*/
void GbRecordMng::ClearQureyList()
{
    for (int i = 0; i < MAX_QUERY; ++i) {
        DeleteQurey(i);
    }

    return;
}
/*开始录像推流*/
void GbRecordMng::StartPushRecord(SdpParse *sdp_parse, std::shared_ptr<InviteDialog> &invite_ptr)
{
    if (sdp_parse == NULL) {
        GB_ERR("sdp_parse[%p] is NULL\n", sdp_parse);
        return ;
    }

    SchedulerParam<GbRecordMng, SdpParse *> param(GetSharedPtr(), true, 0);
    param.set_data1(sdp_parse);
    param.set_data2(&invite_ptr);

    js_run_function(sch_, StartPushRecordCb, &param, true);
}

void GbRecordMng::StartPushRecordCb(void *data)
{
    SchedulerParam<GbRecordMng, SdpParse *> *param = (SchedulerParam<GbRecordMng, SdpParse *> *)data;
    std::shared_ptr<GbRecordMng> &this_ptr = param->get_this_ptr();
    SdpParse *sdp_parse = param->get_data1();
    std::shared_ptr<InviteDialog> &invite_ptr = *(std::shared_ptr<InviteDialog> *)param->get_data2();

    int pos = this_ptr->FindPlayByCallId(invite_ptr->get_call_id());
    if (pos > 0) {
        // 会话已经存在，拖动进度条会走同一个会话
        this_ptr->play_list_[pos]->ParamChange(sdp_parse);
        return ;
    }

    // 添加新会话
    pos = this_ptr->FindPlayEmptySlot();
    if (pos < 0) {
        GB_ERR("not find play empty slot\n");
        return ;
    }

    GbRecordPlay *play = new GbRecordPlay(*this_ptr, this_ptr->profile_, this_ptr->sch_, pos, sdp_parse, invite_ptr);
    if (play == NULL) {
        GB_ERR("new record play fail\n");
        return ;
    }

    this_ptr->play_list_[pos] = play;
    GB_DBG("add record play:%d\n", pos);

    return ;
}

/*下载时获取文件大小*/
uint32_t GbRecordMng::DownloadGetFileSize(time_t start_time, time_t end_time)
{
    /*
        获取下载文件大小时不能只查一条，当两条录像开始和结束时间重合时
        现有机制是会查到前面那条，但我们实际要找的是开始时间和结束时间完全符合的后面那条录像
    */
    GbRecordQuery query(0, start_time, end_time, 2, true);

    sRec1File *record_info = query.GetBestMatchVideo(start_time, end_time);
    if (record_info == NULL) {
        GB_ERR("not find record, start_time[%lld] end_time[%lld]\n", start_time, end_time);
        return 0;
    }

    return record_info->file_secs;
}

/*通过 call id 删除录像播放器*/
void GbRecordMng::DeleteRecordPlayByCallId(osip_call_id_t *call_id)
{
    if (call_id == NULL) {
        GB_ERR("call id error\n");
        return ;
    }
    SchedulerParam<GbRecordMng, osip_call_id_t *> param(GetSharedPtr(), true, 0);
    param.set_data1(call_id);

    js_run_function(sch_, DeleteRecordPlayByCallIdCb, &param, true);
}

void GbRecordMng::DeleteRecordPlayByCallIdCb(void *data)
{
    SchedulerParam<GbRecordMng, osip_call_id_t *> *param = (SchedulerParam<GbRecordMng, osip_call_id_t *> *)data;
    std::shared_ptr<GbRecordMng> &this_ptr = param->get_this_ptr();
    osip_call_id_t *call_id = param->get_data1();

    int pos = this_ptr->FindPlayByCallId(call_id);
    if (pos < 0) {
        return ;
    }

    this_ptr->DeletePlay(pos);
    return ;
}
/*获取播放完成录像的 dialog 指针*/
void GbRecordMng::GetCompletPtrCb(void *data)
{
    SchedulerParam<GbRecordMng, std::shared_ptr<InviteDialog> *> *param =
                        (SchedulerParam<GbRecordMng, std::shared_ptr<InviteDialog> *> *)data;
    std::shared_ptr<GbRecordMng> &this_ptr = param->get_this_ptr();
    std::shared_ptr<InviteDialog> &invite_ptr = *(std::shared_ptr<InviteDialog> *)param->get_data1();

    for (int i = 0; i < MAX_PLAY; ++i) {
        GbRecordPlay *play = this_ptr->play_list_[i];
        if (play->get_play_status() == COMPLETE) {
            invite_ptr = play->get_invite_ptr();
            GB_DBG("get play complete record no: %d\n", i);
            break;
        }
    }

    if (invite_ptr == nullptr) {
        GB_ERR("not find play complete record\n");
    }
    return ;
}
std::shared_ptr<InviteDialog> GbRecordMng::GetCompletPtr()
{
    std::shared_ptr<InviteDialog> invite_ptr;
    SchedulerParam<GbRecordMng, std::shared_ptr<InviteDialog> *> param(GetSharedPtr(), true, 0);
    param.set_data1(&invite_ptr);

    js_run_function(sch_, GetCompletPtrCb, &param, true);

    return invite_ptr;
}
/*设置播放状态*/
void GbRecordMng::SetRecordPlayStatus(osip_call_id_t *call_id, PlayStatus status)
{
    if (call_id == NULL) {
        GB_ERR("call id error\n");
        return ;
    }

    SchedulerParam<GbRecordMng, osip_call_id_t *> param(GetSharedPtr(), true, 0);
    param.set_data1(call_id);
    param.set_data2(&status);

    js_run_function(sch_, SetRecordPlayStatusCb, &param, true);
}

void GbRecordMng::SetRecordPlayStatusCb(void *data)
{
    SchedulerParam<GbRecordMng, osip_call_id_t *> *param = (SchedulerParam<GbRecordMng, osip_call_id_t *> *)data;
    std::shared_ptr<GbRecordMng> &this_ptr = param->get_this_ptr();
    osip_call_id_t *call_id = param->get_data1();
    PlayStatus status = *(PlayStatus *)param->get_data2();

    int pos = this_ptr->FindPlayByCallId(call_id);
    if (pos < 0) {
        return ;
    }

    if (status == TEARDOWN) {
        // 服务器发送停止，需要结束会话释放资源
        this_ptr->DeletePlay(pos);
        return ;
    }

    // 其它状态同步到 play 对象
    this_ptr->play_list_[pos]->SetPlayStatus(status);
    return ;
}

/*设置播放速度*/
void GbRecordMng::SetRecordPlayScale(osip_call_id_t *call_id, double scale)
{
    if (call_id == NULL) {
        GB_ERR("call id error\n");
        return ;
    }

    SchedulerParam<GbRecordMng, osip_call_id_t *> param(GetSharedPtr(), true, 0);
    param.set_data1(call_id);
    param.set_data2(&scale);

    js_run_function(sch_, SetRecordPlayScaleCb, &param, true);
}

void GbRecordMng::SetRecordPlayScaleCb(void *data)
{
    SchedulerParam<GbRecordMng, osip_call_id_t *> *param = (SchedulerParam<GbRecordMng, osip_call_id_t *> *)data;
    std::shared_ptr<GbRecordMng> &this_ptr = param->get_this_ptr();
    osip_call_id_t *call_id = param->get_data1();
    double scale = *(double *)param->get_data2();

    int pos = this_ptr->FindPlayByCallId(call_id);
    if (pos < 0) {
        return ;
    }

    this_ptr->play_list_[pos]->SetPlayScale(scale);

    return ;
}

/*设置播放偏移*/
void GbRecordMng::SetRecordPlayOffset(osip_call_id_t *call_id, time_t offset)
{
    if (call_id == NULL) {
        GB_ERR("call id error\n");
        return ;
    }

    SchedulerParam<GbRecordMng, osip_call_id_t *> param(GetSharedPtr(), true, 0);
    param.set_data1(call_id);
    param.set_data2(&offset);

    js_run_function(sch_, SetRecordPlayOffsetCb, &param, true);
}

void GbRecordMng::SetRecordPlayOffsetCb(void *data)
{
    SchedulerParam<GbRecordMng, osip_call_id_t *> *param = (SchedulerParam<GbRecordMng, osip_call_id_t *> *)data;
    std::shared_ptr<GbRecordMng> &this_ptr = param->get_this_ptr();
    osip_call_id_t *call_id = param->get_data1();
    time_t offset = *(time_t *)param->get_data2();

    int pos = this_ptr->FindPlayByCallId(call_id);
    if (pos < 0) {
        return ;
    }

    this_ptr->play_list_[pos]->SetPlayOffset(offset);

    return ;
}

/*发现播放队列空位，成功返回下标，失败返回 FAILURE*/
int GbRecordMng::FindPlayEmptySlot()
{
    time_t oldest_time = 0;
    time_t oldest_place = 0;

    for (int i = 0; i < MAX_PLAY; ++i) {
        GbRecordPlay *play = play_list_[i];

        if (play != NULL) {
            // 已有录像，判断是否需要清理
            if (!play->IsRunning()) {
                DeletePlay(i);
                return  i;
            } else if (oldest_time == 0 || play->get_create_time() < oldest_time) {
                oldest_time = play->get_create_time();
                oldest_place = i;
            }
        } else {
            //当前位置空闲
            return i;
        }
    }

    // 理论上来说新请求需要被优先响应，如果没找到空位的话就清除时间最老的会话
    if (oldest_time != 0) {
        DeletePlay(oldest_place);
        return oldest_place;
    }

    return FAILURE;
}

/*通过 call id 查找录像播放器，成功返回下标，失败返回 FAILURE*/
int GbRecordMng::FindPlayByCallId(osip_call_id_t *call_id)
{
    for (int i = 0; i < MAX_PLAY; ++i) {
        GbRecordPlay *play = play_list_[i];
        if (play != NULL && osip_call_id_match(call_id, play->get_call_id()) == OSIP_SUCCESS) {
            return i;
        }
    }

    return FAILURE;
}

/*通过下标删除录像播放器*/
void GbRecordMng::DeletePlay(uint32_t pos)
{
    if (pos > MAX_PLAY) {
        GB_ERR("pos[%d] error\n", pos);
        return ;
    }

    GbRecordPlay *play = play_list_[pos];
    if (play == NULL)
        return ;

    GB_DBG("delete record play[%u]\n", pos);
    delete play;
    play_list_[pos] = NULL;

    return ;
}
/*清空录像播放列表*/
void GbRecordMng::ClearPlayList()
{
    SchedulerParam<GbRecordMng, int> param(GetSharedPtr(), true, 0);

    js_run_function(sch_, ClearPlayListCb, &param, true);
}

void GbRecordMng::ClearPlayListCb(void *data)
{
    SchedulerParam<GbRecordMng, int> *param = (SchedulerParam<GbRecordMng, int> *)data;
    std::shared_ptr<GbRecordMng> &this_ptr = param->get_this_ptr();

    for (int i = 0; i < MAX_PLAY; ++i) {
        this_ptr->DeletePlay(i);
    }
}