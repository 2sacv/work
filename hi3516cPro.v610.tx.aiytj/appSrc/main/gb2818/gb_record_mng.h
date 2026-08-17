/** Copyright (C) by Jabsco Company
*
* @File Name    : gb_record_mng.h
* @Created Time : 2024-05-06
* @Version      : 1.0
* @Author       :
* @Description  : 国标录像推流管理
*/

#ifndef GB_RECORD_MNG_H_
#define GB_RECORD_MNG_H_

#include <time.h>

#include <vector>

#include "js_scheduler.h"
#include "osip2/osip.h"
#include "record_lib.h"
#include "encode_common.h"
#include "libmp4.h"
#include "shm_buf.h"
#include "encode_audio_output.h"

#include "gb_client_manager.h"
#include "gb_socket.h"
#include "gb_rtp.h"
#include "gb_mpeg_ps.h"
#include "gb_profile.h"
#include "gb_sdp_parse.h"
#include "gb_dialog_mng.h"

#define MAX_QUERY 2  // 最大同时查询数量
#define MAX_PLAY 2   // 最大同时播放数量

#define QUERY_TIMEOUT 5 // 查询超时时间 单位: S
#define MAX_RECORD_VIDEO_SIZE  VIDEO_DATA_BUF_SIZE  // 录像视频帧最大尺寸
#define MAX_RECORD_AUDIO_SIZE  PCM_FRM_BYTES_16K    // 录像音频帧最大尺寸

enum PlayStatus{
    PLAY = 0, // 正在播放
    PAUSE,    // 暂停
    TEARDOWN, // 停止状态
    COMPLETE, // 录像播放完全
};

class GbClientManager;
class GbRecordMng;

class GbRecordPlay : public GbRtpPack, public GbMpegPsEncode {
public:
    GbRecordPlay(GbRecordMng &record_mng, GbProfile &profile, JSScheduler &sch, int no, SdpParse *sdp_parse, std::shared_ptr<InviteDialog> &invite_ptr);
    ~GbRecordPlay();

    bool IsRunning() const {
        return play_status_ != TEARDOWN;
    }
    /*ps 编码和 RTP 编码相关*/
    int OnPsStream(uint8_t *ps, uint32_t ps_size, bool is_video, bool is_video_key, bool is_first, bool is_end);
    uint16_t GetMtu() {
        return profile_.get_mtu();
    }
    int OnRtpPackage(uint8_t *package, uint16_t package_size, bool is_video, bool is_video_key, bool is_first, bool is_end);

    /*获取会话标识*/
    osip_call_id_t *get_call_id() const {
        return invite_ptr_->get_call_id();
    }

    /*获取会话信息*/
    std::shared_ptr<InviteDialog> &get_invite_ptr() {
        return invite_ptr_;
    }

    /*获取当前播放状态*/
    PlayStatus get_play_status() {
        return play_status_;
    }

    /*同一个会话，多个 INVITE 下发，调用该函数进行修改*/
    void ParamChange(SdpParse *sdp_parse);

    /*设置播放状态*/
    void SetPlayStatus(PlayStatus play_status);

    /*设置播放时间，用于随机拖放，偏移是相对录像文件开始时间*/
    void SetPlayOffset(time_t offset);

    /*设置播放速度, 文档规定的范围为 0.25、0.5、1、2、4*/
    void SetPlayScale(double play_scale);

    time_t get_create_time() const {
        return create_time_;
    }

private:
    static void PushRecord(void *data);
    void PushRecordCb();
    /*打开录像文件*/
    int OpenRecordFile(time_t start_time, time_t end_time);
    /*停止播放录像文件*/
    void StopRecordPlay();
    /*获取录像文件目录*/
    int GetRecordFilePath(char *buf, uint32_t buf_size, time_t start_time, const char *file_name);

private:
    GbProfile &profile_;
    int no_;  // 标识，用来区分录像推流会话

    JSScheduler &sch_;       // 推流调度器
    JSTCHandle  push_loop_; // 推流定时器
    GbSocket socket_;  // 推流 socket

    /*video info*/
    int        video_serial_;    // 视频帧序号
    double     video_fps_;
    eVideoType video_type_;
    double     video_timestamp_;
    char       vps_data_[128]; // H26x 压缩参数
    int        vps_data_size_;
    char       sps_data_[128];
    int        sps_data_size_;
    char       pps_data_[128];
    int        pps_data_size_;

    /*audio info*/
    bool       audio_exist_;     //MP4文件音频数据是否存在
    int        audio_serial_;    // 音频帧序号
    double     audio_timestamp_;    //录像音频时间戳

    /*播放信息*/
    CMP4Read   mp4_read_;   //MP4文件读取指针
    time_t start_time_; // 录像开始时间
    time_t end_time_; // 录像结束时间
    PlayStatus play_status_; // 播放状态
    double play_scale_;  // 播放速度
    osip_call_id_t *call_id_; // 会话 ID ,结束推流的时候要用到
    std::shared_ptr<InviteDialog> invite_ptr_; // 会话信息，用于查询和推送录像播放结束命令

    /* ps 流信息 */
    int video_stream_id_;
    int audio_stream_id_;

    time_t create_time_; // 当前录像播放创建时间
};

/*录像查找，通过 sn 号进行区分，继承查找*/
class GbRecordQuery {
public:
    GbRecordQuery(uint32_t sn, time_t start_time, time_t end_time, uint32_t unmber = 0,  bool need_size = false);
    ~GbRecordQuery();

    sRec1File *ResumeGetRcordInfo(); // 继续获取下一条录像

    // 获取一条最适配的录像
    sRec1File *GetBestMatchVideo(time_t start_time, time_t end_time);
    uint32_t get_total() const {
        return total_;
    }

    uint32_t get_sn() const {
        return sn_;
    }

    time_t get_create_time() const {
        return create_time_;
    }

private:
    sRec1File record_list_[MAX_RECS_OF_DAY]; // 录像列表
    uint32_t total_;    // 总录像个数
    time_t strat_time_; // 查询录像开始 UTC 时间
    time_t end_time_;  // 查询录像结束 UTC 时间
    uint32_t sn_; // 录像很多时需要分多条消息回复，SN 号要保持一致
    uint32_t qurey_count_; // 已经查询的录像计数

    time_t create_time_; // 当前查询创建时间
};

/*录像查询，播放控制器*/
class GbRecordMng : public std::enable_shared_from_this<GbRecordMng>{
public:
    GbRecordMng(GbProfile &profile, JSScheduler &sch);
    ~GbRecordMng();

    std::shared_ptr<GbRecordMng> GetSharedPtr() {
        // 使用 shared_from_this() 安全地获取 shared_ptr
        return shared_from_this();
    }

    /*录像查询*/
    int AddRecordQuery(uint32_t sn, time_t start_time, time_t end_time);  // 添加录像查询
    int FindQureyBySn(uint32_t sn); // 通过 sn 号查找录像查询任务
    void DeleteQureyBySn(uint32_t sn); // 通过 sn 号删除录像查询
    int GetQureyTotal(uint32_t pos); // 通过下标获取录像总数
    void DeleteQurey(uint32_t pos); // 通过下标删除录像查询
    sRec1File *ResumeGetRcordInfo(uint32_t pos); // 通过下标继续获取一条记录

    /*录像推流*/
    void StartPushRecord(SdpParse *sdp_parse, std::shared_ptr<InviteDialog> &invite_ptr);
    void DeleteRecordPlayByCallId(osip_call_id_t *call_id);
    uint32_t DownloadGetFileSize(time_t start_time, time_t end_time);
    void ClearPlayList(); // 清空录像播放列表列表

    std::shared_ptr<InviteDialog> GetCompletPtr();
    void SetRecordPlayStatus(osip_call_id_t *call_id, PlayStatus status); // 设置播放状态
    void SetRecordPlayScale(osip_call_id_t *call_id, double scale); // 设置播放速度
    void SetRecordPlayOffset(osip_call_id_t *call_id, time_t offset); // 设置播放偏移

private:
    /*录像查询*/
    int FindQureyEmptySlot(); // 在录像查询列表中找到一个空位
    void ClearQureyList(); // 清空查询列表

    /*录像推流*/
    int FindPlayEmptySlot(); // 在录像播放列表中找到一个空位
    int FindPlayByCallId(osip_call_id_t *call_id);
    void DeletePlay(uint32_t pos);
    static void StartPushRecordCb(void *data);
    static void DeleteRecordPlayByCallIdCb(void *data);
    static void ClearPlayListCb(void *data); // 清空录像播放列表列表
    static void SetRecordPlayStatusCb(void *data);
    static void SetRecordPlayScaleCb(void *data);
    static void SetRecordPlayOffsetCb(void *data);
    static void GetCompletPtrCb(void *data);
private:
    GbProfile &profile_;  // 配置
    JSScheduler &sch_;       // 推流调度器

    std::vector<GbRecordPlay *> play_list_;
    std::vector<GbRecordQuery *> query_list_;
};

#endif
