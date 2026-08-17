#ifdef PLATFORM_TENCENT
#ifndef _TENCENT_MEDIA_MANAGE_H_
#define _TENCENT_MEDIA_MANAGE_H_

#include "iv_av.h"
#include "tencent_record_play.h"
#include "tencent_server.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int uint32_t;

#define SEI_MAX_LEN 64
#define FRAME_HEAD_PRE_SIZE (3*512+SEI_MAX_LEN)
#define STREAM_PUSH_TIMER  (30)

typedef struct {
    char running;
    uint32_t visitor;
    char *blk_buf;
    int blk_size;
    FILE *fp;
} tx_download_info_s;

// 对讲一次只允许一个visitor进行
typedef struct {
    int         running;        // 正在运行
    uint32_t    sensor_id;      // 腾讯下发的sensor号
    uint32_t    visitor;        // 分享者序列号
} tx_talk_visitor_info_s;

typedef struct {
    int      sender;  // 0-app 1-client
    uint32_t visitor;
    uint32_t sensor_id;
    iv_avt_video_res_type_e quality;
    iv_avt_video_res_type_e video_res_type;
    iv_cm_time_fragment_s *pb_time;
} tx_info_s;

int is_media_useable(void);
void tencent_check_drop_cache(void);
void tencent_talk_get_enc_info(uint32_t visitor, uint32_t channel,
                                iv_avt_video_res_type_e video_res_type,iv_cm_av_data_info_s *p_av_data_info,void *args);
void tencent_talk_start_real_play(uint32_t visitor, uint32_t channel, iv_avt_video_res_type_e video_res_type, void *args);
void tencent_talk_stop_real_play(uint32_t visitor, uint32_t channel, iv_avt_video_res_type_e video_res_type);
int  tencent_talk_start_recv_stream(uint32_t visitor, uint32_t channel, iv_avt_stream_type_e stream_type, iv_cm_av_data_info_s *p_av_data_info);
int  tencent_talk_stop_recv_stream(uint32_t visitor, uint32_t channel, iv_avt_stream_type_e stream_type);
int  tencent_talk_recv_stream(uint32_t visitor, uint32_t channel, iv_avt_stream_type_e stream_type, void *pStream);
void tencent_record_cmd_handle(record_cmd_param_s param);
int  tencent_talk_download_proc(iv_avt_download_status_e status, uint32_t visitor, uint32_t channel, void *args);

int tencent_media_manage_init();
int tencent_media_manage_uninit(void);

#ifdef __cplusplus
}
#endif

#endif //_TENCENT_MEDIA_MANAGE_H_
#endif //PLATFORM_TENCENT

