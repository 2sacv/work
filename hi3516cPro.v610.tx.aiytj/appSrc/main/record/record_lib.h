#ifndef _RECORD_LIB_H
#define _RECORD_LIB_H

#include "libmp4.h"
#include "record_dirent.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DEV_DOME_DIRECT_RECORD  99
#define DEV_DOME_RECORD         0
#define DEV_DIRECT_RECORD       1

#define SECS_OF_DAY             (60*60*24)

typedef struct {
    char		start_time[12]; // 时:分:秒 12:12:12
    char		end_time[12];
    uint32_t	status;
    uint32_t	record_no;
    size_t		week_count;     // <= 7  表示week数组中有几天
    uint32_t	week[7];
} sRecPlan;

typedef struct {
    int record_en;
    int begin_hour;
    int begin_min;
    int end_hour;
    int end_min;
} sRecTime;

typedef enum {
    REC_TYPE_PLAN = 0,          // 计划录像
    REC_TYPE_ALARM = 1,         // 报警录像
    REC_TYPE_INITIATIVE = 2,    // 主动录像
    REC_TYPE_ANY = 99,          // 所有类型。
} eRecType;                     // 如果不在这个范围内，则为用户自定义录像类型，由APP和设备侧自主协商含义，SDK不再明确列出具体类型.

typedef struct {
    unsigned int start_time;    // 录像开始时间，UTC时间，单位为秒
    unsigned int stop_time;     // 录像结束时间，UTC时间，单位为秒
    unsigned int file_secs;     // 录像的文件秒数
    char file_name[CHARS_OF_MP4NAME];  // 录像的文件名
    //eRecType record_type;       // 录像类型
    //int seek_timestamp;         // seek时间点
    int is_tmp_file;
} sRec1File;

typedef struct {
    int sps_size;
    unsigned char sps_buf[256];
    int pps_size;
    unsigned char pps_buf[256];
} sSpsPpsInfo;

typedef struct {
    int vps_size;
    unsigned char vps_buf[256];
    int sps_size;
    unsigned char sps_buf[256];
    int pps_size;
    unsigned char pps_buf[256];
} sVpsSpsPpsInfo;

/*打开MP4文件*/
CMP4Read *record_open_file(char *path, bool *bAudio, unsigned int *audio_fps, tagMP4AudioInfo *audio_info,int *h26x, double *video_fps, tagMP4VideoInfo* video_info);
/*关闭MP4文件*/
int record_close_file(CMP4Read *pCMP4Read);
/*读视频流数据*/
int record_read_vframe(CMP4Read *pCMP4Read, int *serial, char *buf, size_t buf_sz, int *frame_sz, int *keyframe, size_t *duration, int h26x);
/*读音频流数据*/
int record_read_aframe(CMP4Read *pCMP4Read, char *buf, size_t buf_sz, int *frame_sz, unsigned int *duration);
/*录像文件定位*/
int record_seek_file(CMP4Read *pCMP4Read, int offset_time);
/*获取当前帧数*/
int record_get_curfarme(CMP4Read *pCMP4Read);
/*获取pps sps 264*/
int record_get_pps_sps(CMP4Read *pCMP4Read, sSpsPpsInfo *info);
/*获取vps pps sps 265*/
int record_get_vps_pps_sps(CMP4Read *pCMP4Read, sVpsSpsPpsInfo *info);
/*获取MP4时长*/
int record_get_mp4_secs(const char *path, uint32_t *secs);
/*根据帧数计算时间戳*/
int record_get_timestamp_ms(CMP4Read *pCMP4Read);
/*获取录像列表_按天>>新结构体*/
int record_query_list(int64_t start_utc, int query_n, sRec1File *olist, int dev);
/*获取录像路径*/
int record_get_path_of_ymd(int yyyymmdd_in, char path[], int len, int channel);
/*获取录像时间*/
int record_get_ymd_of_epoch(int64_t start_utc);
/*获取录像时间_按月*/
int record_get_daybits_of_month(int month);
/*更新录像缓存*/
void record_clr_cache_list(int yyyymmdd);
/*获取录像计划*/
int record_get_plan(sRecPlan *record_info, int *plan_num);
/*设置录像计划*/
int record_set_plan(sRecPlan *info);
/*临时文件重命名9999->len*/
int record_set_len_of_tmpfile(sRec1File *respinfo);


#ifdef __cplusplus
}
#endif

#endif //_TENCENT_RECORD_H_

