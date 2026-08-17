#ifdef PLATFORM_TENCENT
#ifndef _TENCENT_LIVING_STREAM_H_
#define _TENCENT_LIVING_STREAM_H_

#include "iv_av.h"
#include "js_scheduler.h"
#include "jconfstruct.h"
#include "tencent_talk.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int uint32_t;

#define  LIVE_REPORT_VITCOUNT   "visitor_count"
#define  LIVE_REPORT_NET_STATUS "net_status"
#define  LIVE_REPORT_NET_DBM    "net_dbm"
#define  LIVE_REPORT_QUALITY    "quality"

typedef enum{
    AUDIO_UNENABLE = 0,
    AUDIO_ENALE,
} AudioEnableE;

typedef enum{
   VIDEO_CHN_MAIN,
   VIDEO_CHN_SUB,
} VideoChn;

typedef enum{
    STREAM_LIVING = 0,
    STREAM_CLOUDSTORAGE,
} Streamtype;

typedef struct{
    int          idx;
    int          mediatype;
    int          bps;
    int          fps;
    int          width;
    int          height;
    unsigned int profile;
    int          spssize;
    char         spsdata[512];
    int          ppssize;
    char         ppsdata[512];
    int          vpssize;
    char         vpsdata[512];
} VideoInfoS;

typedef struct {
    JSTCHandle       a_handle;        // 视频处理器
    int              audio_frame_serial; // 处理音频带初始包序号
    int              frame_serial;    // 处理音频带初始包序号
    AudioCfgS        info;            // 音频信息
    uint64_t         timestamp_ms;    // 音频时间戳(ms)
    AudioEnableE     enable;          // 音频开关状态
    AliMediaTypeE    input_format;    // 音频输入格式G711A/G711U/AAC
} AudioFrameInfoS;

typedef struct {
    JSTCHandle      v_handle;        // 视频处理器
    uint32_t        video_channel;   // 视频通道号
    int             frame_serial;    // 处理视频的初始包序号
    uint64_t        timestamp_ms;    // 视频时间戳(ms)
    VideoInfoS      info;            // 视频信息
} VideoFrameInfoS;

typedef struct {
    int congst_low;
    int congst_warn;
    int congst_high;
    int running;                   // 正在运行
    int waiting_key_frame;         // 等待关键帧
    int request_key_frame;         // 请求关键帧
    int iframe_requested;          // 拥塞期间是否已请求I帧(per-visitor)
    uint32_t        video_channel;  // 视频通道号
    uint32_t        sensor_id;      // 腾讯下发的sensor号
    uint32_t        visitor;        // 分享者序列号
    iv_avt_video_res_type_e video_res_type; // 流类型
    iv_avt_video_res_type_e quality; // 清晰度
} tx_lv_cs_visitor_info_s;

typedef struct {
    int cnt;                // 直播流下的vistor个数信息
    int is_living;          // 是否存在直播推流
    int running;            // stream running
    Streamtype type;        // stream type
    AudioFrameInfoS audio;  // 音频
    VideoFrameInfoS video;  // 视频
} tx_lv_cs_push_info_s;

void tencent_report_property();

void tencent_live_stream_init(tx_lv_cs_push_info_s *lv_cs_push_info);

int tencent_get_video_channel(int sensor_id, iv_avt_video_res_type_e video_res_type, int sender);

void tencent_push_live_video(void *userdata);

void tencent_push_live_audio(void *userdata);

int tencent_start_push_stream(uint32_t visitor, uint32_t channel, iv_avt_video_res_type_e video_res_type, int sender);

int tencent_stop_push_stream(uint32_t visitor, uint32_t channel, iv_avt_video_res_type_e video_res_type);

void tencent_video_res_type_change(uint32_t sensor_id, iv_avt_video_res_type_e video_res_type);

void tencent_quality_change(int id, void *p_src, int size, void *ctx);

void tencent_live_event_cb(int id, void *p_src, int size, void *ctx);

int tencent_live_report_property(const char *const json_format, ...);

int tencent_living_running_count(void);

int tencent_living_init();

int tencent_living_uninit();

void tencent_set_congst_low(int i, int j);
void tencent_set_congst_warn(int i, int j);
void tencent_set_congst_high(int i, int j);

#ifdef __cplusplus
}
#endif

#endif //_TENCENT_LIVING_STREAM_H_
#endif //PLATFORM_TENCENT
