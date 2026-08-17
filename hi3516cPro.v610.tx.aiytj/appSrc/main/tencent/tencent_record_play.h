#ifdef PLATFORM_TENCENT
#ifndef _TENCENT_RECORD_PLAY_H_
#define _TENCENT_RECORD_PLAY_H_

#include "libmp4.h"
#include "record_lib.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "js_scheduler.h"
#include "jconfstruct.h"
#include "iv_cm.h"
#include "iv_av.h"

#define IDR_FRMAE_SEND_TIME   (4)

typedef enum {
    REPLAY_SPEED_1X = 1,
    REPLAY_SPEED_2X = 2,
    REPLAY_SPEED_4X = 4,
} ReplaySpeedE;

/* 录像相关的指令 */
typedef enum {
    STORAGE_RECORD_START = 0,   //开始播放，对于录像点播有效
    STORAGE_RECORD_PAUSE,       //暂停，对于录像点播有效
    STORAGE_RECORD_UNPAUSE,     //继续播放，对于录像点播有效
    STORAGE_RECORD_SEEK,        //定位，对于录像点播有效
    STORAGE_RECORD_STOP,        //停止，对于录像点播有效
    STORAGE_RECORD_SET_PARAM,   //设置点播倍速等参数信息
} push_streaming_cmd_type_e;

/* 用于录像相关指令的调度 */
typedef struct {
    struct {
        int record_id;              //录像回放ID
        push_streaming_cmd_type_e cmd_type;//命令类型
    } common;
    struct {
        unsigned int timestamp_ms;  //seek的时间戳，单位：ms.
    } seek;
    struct {
        unsigned int speed;         //倍速信息
        unsigned int key_only;      //0-推送全数据帧，1-仅推送I帧
    } set_param;
} record_cmd_param_s;

typedef struct {
    int                       id;           //录像回放序列号
    push_streaming_cmd_type_e play_status;  //当前播放状态
    unsigned int              timestamp_ms; //阿里云来的时间查找参数
    unsigned int              key_only;     //仅推送I帧
    unsigned int              speed;        //录像播放倍速
} RecordplayCmdS;

typedef struct {
    unsigned int start_time_s;      //播放当天0点的UTC时间,单位：s
    unsigned int stop_time_s;       //播放当天24点的UTC时间,单位：s
    unsigned int seek_time_s;       //播放的UTC时间相对于start_time的相对时间 单位：s
    unsigned int seek_timestamp_ms; //录像seek时间戳, 第一次点播、自动跳段、手动跳段时更新

    char file_dir[64];
    sRec1File *file_list;           //播放当天录像文件list
    int file_count;                 //播放当天录像文件数量
    int play_num;                   //当前播放录像文件序号
    int visitor;                    //播放visitor 分享者的录像
    int sensor_id;                  //播放sensor_id通道的录像
} RecordInfoS;


typedef struct {
    int serial;                             //录像的当前帧序号, 为-1请求读取I帧
    CMP4Read *           pCMP4Read;         //MP4文件读取指针

    struct {
        MP4_VIDEO_TYPE_E type;              //MP4文件视频类型
        MP4_VIDEO_TYPE_E pre_type;          //MP4文件前段视频类型
        sVpsSpsPpsInfo   vsp_sps_pps_info;  //MP4文件H265帧头信息
        sSpsPpsInfo      sps_pps_info;      //MP4文件H264帧头文件
        float            fps;               //录像视频帧率
        unsigned int     timestamp_ms;      //录像视频时间戳
        unsigned int     sei_timestamp_ms;  //录像视频时间戳
        uint8_t          sei_buffer[64];
        int              sei_size;
        int              video_type_change; // 录像回放视频编码改变
    } video;

    struct {
        bool             exist;           //MP4文件音频数据是否存在
        unsigned int     fps;             //录像音频帧率
        unsigned int     timestamp_ms;    //录像音频时间戳
    } audio;
} Mp4InfoS;

typedef struct {
    push_streaming_cmd_type_e play_status;      //当前播放状态
    unsigned int              speed;            //录像播放倍速1/2/4X
    unsigned int              key_only;         //仅推送I帧
    unsigned long             current_time_ms;  //推流时当前时间点
    unsigned long             start_time_ms;    //推流时开始时间点
    unsigned int              duration_v_t;     //推流时视频的累积时间点
    unsigned int              duration_a_t;     //推流时音频累积的时间点
} RecordPlayCtrlS;

typedef struct{
    int         id;             //阿里云下发的服务序列号
    JSTCHandle  handle;         //录像推流timer handle
    int         time;           //录像推流timer interval
} RecordPlayMngS;

typedef struct {
    int idrfrm_send_cnt;        //头一个 i 帧发送次数
    int video_frame_size;
    int audio_frame_size;
    int frame_type;
    int frame_count;
    int mp4_info_sync_done;
    unsigned int video_timestamp_ms;
    unsigned int audio_timestamp_ms;
    unsigned int duration_v;
    unsigned int duration_a;
} RecordRunInfoS;

typedef struct {
    RecordRunInfoS  run;
    RecordPlayMngS  mng;        //由media mnger管理id及handle
    RecordPlayCtrlS ctrl;       //控制回放推流
    Mp4InfoS        mp4_info;   //播放某个MP4文件相关信息
    RecordInfoS     rec_info;   //播放当天UTC时间、录像文件等信息
} RecordPlayChnS;

int is_replay_chn_full(void);

int is_replay_chn_running(int id);

float replay_get_video_fps(int record_id);

int replay_init_chn(int channel, int record_id, iv_cm_time_fragment_s *pb_time,
        iv_cm_av_data_info_s *pstAvDataInfo);

int replay_uninit_chn(int id);

void replay_stop_allchn();

void* replay_get_chn(int id);

int get_record_process(int record_id, iv_avt_video_res_type_e video_res_type);

int replay_find_record_list(RecordInfoS* info, int channel);

void replay_push_record(void *userdata);

void replay_push_cmd(void * userdata);

#ifdef __cplusplus
}
#endif

#endif //_TENCENT_RECORD_PLAY_H_
#endif //PLATFORM_TENCENT

