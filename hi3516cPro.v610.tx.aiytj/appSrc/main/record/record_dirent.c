/*
 * 内核使用 32bits offset ，最大文件可达 2GB
 **/
#ifdef _LARGEFILE_SOURCE
#undef _LARGEFILE_SOURCE
#endif

#ifdef _FILE_OFFSET_BITS
#undef _FILE_OFFSET_BITS
#endif

#define _GNU_SOURCE

#include <dirent.h>     /* Defines DT_* constants */
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>

#include <sys/stat.h>
#include <sys/syscall.h>

#include <string.h>
#include "debug.h"
#include "g_sys.h"
#include "g_run.h"
#include "record_dirent.h"

#define DIR_BUF_SIZE 4096 // 20*512

struct linux_dirent {
   long           d_ino;
   off_t          d_off;
   unsigned short d_reclen;
   char           d_name[];
};

static int partition(sCache1File* arr[], int low, int high) 
{
    sCache1File* pivot = arr[high]; // 基准指针
    int i = low - 1;
    for (int j = low; j <= high - 1; j++) {
        if (arr[j]->start_hms <= pivot->start_hms) {
            i++;
            // 交换指针地址
            sCache1File* tmp = arr[i];
            arr[i] = arr[j];
            arr[j] = tmp;
        }
    }
    // 将基准放到正确位置
    sCache1File* tmp = arr[i + 1];
    arr[i + 1] = arr[high];
    arr[high] = tmp;
    return i + 1;
}

static void quick_sort(sCache1File* arr[], int low, int high)
{
    if (low < high) {
        int pi = partition(arr, low, high);
        quick_sort(arr, low, pi - 1);
        quick_sort(arr, pi + 1, high);
    }
}

/* return value:
 *  -1  失败
 *  0   成功
 *  1   部分成功
 *
 * flag:
 * 0    全部
 * 1    tmp临时文件，REC_FILE_TMP
 **/
int lookupdir(const char *dir, sCache1File *p_mp4s[], int sz, int *num, int flag)
{
    int ret = 0, count_num = 0, cnt = 0;
    struct dirent *entry = NULL;
    DIR *dirp = NULL;
    dirp = opendir(dir);
    if (NULL == dirp) {
        ERR("opendir failed dir:%s\n", dir);
        return -1;
    }

    while ((entry = readdir(dirp)) != NULL) {
        const char *name = entry->d_name;
        dbg_record("name:%s\n", name);
        count_num++;
        if (name[0] == 'S' && name[13] == '.' && name[16] == '4') {
            p_mp4s[cnt]->start_hms = atoi(name + 2);
            p_mp4s[cnt]->file_secs = atoi(name + 9);

            if (p_mp4s[cnt]->start_hms > 235959 || p_mp4s[cnt]->file_secs > 9999) {
                continue;
            }

            if (flag == REC_FILE_TMP) {
                if (p_mp4s[cnt]->file_secs != 9999) {
                    continue;
                }
            } else if (flag == REC_FILE_MP4) {
                if (p_mp4s[cnt]->file_secs == 9999) {
                    continue;
                }
            }
            snprintf(p_mp4s[cnt]->name, sizeof(p_mp4s[cnt]->name), "%s", name);

            if (++cnt >= sz) {      // 数组已满
                DBG("%s got a __big record @%s\n", __func__, dir);
                ret = 1;
                break;
            }
        }

        if (count_num > (MAX_RECS_OF_DAY+2)) { // 万一文件系统出错
            ERR("got filesys err of %d\n", cnt);
            cnt = 0;
            ret = -1;
            goto __exit;
        }
    }

    if (cnt > 0) {
        quick_sort(p_mp4s, 0, cnt-1);
    }

    if (get_g_run(record, RUN_RECORD_DENTRY)) {
        for (int i = 0; cnt > 0 && i < cnt; i++) {
            printf("%d: %s (start_hms=%u %04u)\n", i+1, p_mp4s[i]->name, p_mp4s[i]->start_hms, p_mp4s[i]->file_secs);
        }
    }

__exit:
    closedir(dirp);
    *num = cnt;
    return ret; // 部分成功
}

int countfiles(const char *dir)
{
    int fd = open(dir, O_RDONLY | O_DIRECTORY);
    if (fd < 0) {
        perror("open");
        return -1;
    }

    int count = 0;
    char buf[DIR_BUF_SIZE]; // 缓冲区
    long nread;

    while (1) {
        // 系统调用读取目录项
        nread = syscall(SYS_getdents, fd, buf, sizeof(buf));
        if (nread < 0) {
            perror("getdents");
            close(fd);
            return -1;
        }
        if (nread == 0) break; // 遍历结束

        // 解析缓冲区中的目录项
        char *ptr = buf;
        while (ptr < buf + nread) {
            struct linux_dirent *dent = (struct linux_dirent *)ptr;
            ptr += dent->d_reclen; // 移动到下一个条目

            // 跳过 "." 和 ".."
            if (strcmp(dent->d_name, ".") == 0 || strcmp(dent->d_name, "..") == 0) {
                continue;
            }

            count++;
        }
    }

    close(fd);
    return count;
}
