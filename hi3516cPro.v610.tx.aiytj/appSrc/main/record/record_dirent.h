/*
 *       Filename:  record_dirent.h
 *    Description:  查找固定文件名格式的文件，升序排序，减少 strdup 来加速处理
 *                  可单独查询 9999 文件
 */

#ifndef _RECORD_DIRENT_H
#define _RECORD_DIRENT_H
#ifdef __cplusplus 
extern "C" {
#endif

#define MAX_TMPS_OF_DAY         16     // 修复太久耗时
#define MAX_RECS_OF_DAY         512    // 24*60/3.5 = 412  考虑到通断电，及异常上电 +100
#define CHARS_OF_MP4NAME        20     // 实际=18 = 17+'\0' = S-104137-0090.mp4  S-115511-9999.mp4

typedef struct {
    uint32_t start_hms;    // 录像开始时间，UTC时间，单位为秒
    uint32_t file_secs;     // 录像的文件秒数
    char name[CHARS_OF_MP4NAME];  // 录像的文件名
} sCache1File;

typedef enum {
    REC_FILE_ALL = 0,
    REC_FILE_TMP,
    REC_FILE_MP4,
} eRecFileType;

int lookupdir(const char *dir, sCache1File *p_mp4s[], int sz, int *num, int flag);

int countfiles(const char *dir);

#ifdef __cplusplus
}
#endif
#endif

