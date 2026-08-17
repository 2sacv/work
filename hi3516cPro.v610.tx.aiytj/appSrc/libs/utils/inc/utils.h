/*
 * Copyright (C) by Jabsco Company
 * 
 * File Name    : utils.h
 * Created Time : 2014-02-26
 * Version      : 1.0
 * Author       : luoshunfa
 * Description  : 
 */

 
#ifndef _utils_H_
#define _utils_H_

#include <arpa/inet.h>
#include <pthread.h>
#include <ctype.h>
#include "debug.h"

#ifdef __cplusplus
	  extern "C"{
#endif

struct val3_map {
    const char *key;    // end with '='
    const char *fmt;    // %d or %s
    void *p_val;
}; 

typedef enum {
    E_COLOR_BLACK       =  0,
    E_COLOR_DARK_GRAY3  =  1,
    E_COLOR_DARK_GRAY2  =  2,
    E_COLOR_DARK_GRAY1  =  3,
    E_COLOR_LIGHT_GRAY3 =  4,
    E_COLOR_LIGHT_GRAY2 =  5,
    E_COLOR_LIGHT_GRAY1 =  6,
    E_COLOR_WHITE       =  7,
    E_COLOR_RED         =  8,
    E_COLOR_GREEN       =  9,
    E_COLOR_BLUE        = 10,
    E_COLOR_LIGHT_BLUE  = 11,
    E_COLOR_YELLOW      = 12,
    E_COLOR_PURPLE_RED  = 13,
} eColorType;

typedef struct {
    int color;
    int rgb;
} sColor2RGB;

int gettid(void);

/*
 * 字串处理
 **/
int get_val(const char *haystack, const char *key, char *val);

int get_val2(const char *haystack, const char *key, const char *fmt, void *val);

void get_val3(const char *path, struct val3_map maps[], size_t sz);

void drop_tail_space(char *str);

void replace_str(char *str, const char *find, const char *replace);

char* safe_strncpy(char *dst, const char *src, size_t size);

void hex_printf(const char *tag, char *str, size_t size);

int get_cpu_temperature();

char *itoa10(int i, char *a);

int atoi10(const char *a);


/*
 * 文件&目录处理
 **/
int LoadFile(const char *srcFile, char *buf, int len);

int LoadFile2(const char *srcFile, const char *format, ...);

int Writefully(int fd, const void* buf, int nbytes);

int Readfully(int fd, void* buf, int nbytes);

int TouchFile(const char *dstFile);

int Write2File(const char *dstFile, const char *buf, int len);
#define WriteFile(_file, _buf) Write2File(_file, _buf, strlen(_buf))

int DumpFile(const char *dstFile, const char *buf, int len);

int DumpFile2(const char *dstFile, const char *format, ...);

int DumpKmsg(const char *format, ...);

int DropCache(const char *func);

int CompactMemo(const char *from);

int get_num_of_kbytes(int i_bud);

int AppendFile(const char *dstFile, const char *buf);

int AppendFile2(const char *dstFile, const char *format, ...);

int CopyFile(const char *dstFile, const char *srcFile);

int bytes_of_file(const char *filepath);

int rmdir_recursive(const char *path);

int is_okey(const char *file);

int is_okey2(const char *file);

int is_mountpoint(const char *dir);

int is_pattern_exist(const char *dir_path, const char *pattern, char *full_path);

int df_used_p(const char *path);

/*
 * system 命令处理
 **/
int UtilSystemCmd(const char *szCmd);

int UtilSystemCmd2(const char *format, ...);

int ReadCmdResult(const char* szCmd, char *read,int len);

int GetAppCount(char *szAppName);

/*
 * 系统函数二次封装
 **/
#ifndef __VALGRIND
void *system_malloc(int size);
void system_free(void *pPtr);
void *system_valloc(int size);
#else
#define system_malloc malloc
#define system_valloc valloc
#define system_free free
#endif

size_t system_fwrite(const void *ptr, size_t size, size_t nmemb,FILE *stream);
size_t system_fread(void *ptr, size_t size, size_t nmemb, FILE *stream);

FILE *vpopen(const char* cmdstring, const char *type);
int vpclose(FILE *fp);

/*
 * gpio
 **/
int gpio_open_export(int gpio);

int gpio_open_unexpot(int gpio);

int gpio_open_set_direction(int gpio, const char* direct);

int gpio_open_get_direction(int gpio, char* direct);

int gpio_open_get_value(int gpio, int *value);

int gpio_open_set_value(int gpio, int value);

int gpio_open_set_pullup(int gpionum);

int gpio_open_clear_pullup(int gpionum);

int gpio_open_set_pulldown(int gpionum);

int gpio_open_clear_pulldown(int gpionum);

/*
 * 24bit 布防时间处理
 **/
int timestr_to_intarray(char *str, unsigned int *arry);

int intarray_to_timestr(char *str, unsigned int *arry);

BOOL TimeJudge(unsigned int *tda);

/*
 * time & timer
 **/
void ms_sleep(time_t msec);
void ms_clock_reset(struct timespec *clock);
int  ms_clock_is_timeup(struct timespec *clock, int ms_timeout);
int  ms_clock_is_timeup2(struct timespec *clock, int ms_timeout, int *sec_left);


double  mono_stamp();
int64_t mono_usec();

int64_t mono_msec();
int64_t ms_since_previous(struct timespec *prev);
int64_t ms_since_previous2(struct timespec *prev);

time_t  mono_sec();
time_t  sec_since_previous(struct timespec *prev);
time_t  sec_since_previous2(struct timespec *prev);

char  *get_timestr();
char  *get_timestr2(time_t epo, char *outstr, size_t sz);
double get_usec_of_day(void);
int64_t get_secs_of_today_midngt(void);

/*
 * misc
 **/
char *j_inet_ntoa(struct in_addr in);

char *j_crypt(const char *key, const char *salt);

int fsck_exfat();
int use_exfat();

uint32_t get_rgb_color(int color);

#ifdef __cplusplus
	  }
#endif

#endif

