/**
 * @file record_scan_days.c
 * @brief 实现 scan_days_fast —— 用 getdents64 一次性扫描月份子目录
 *
 * 对比:
 * ┌──────────────────────┬──────────────────┬────────────────────┐
 * │ 方案                 │ 系统调用次数     │ 时间复杂度         │
 * ├──────────────────────┼──────────────────┼────────────────────┤
 * │ 原始 access() 循环   │ 最多 31 次       │ O(31) syscalls     │
 * │ 本方案 getdents64    │ 1次（或少数几次）│ O(n) 内存扫描      │
 * └──────────────────────┴──────────────────┴────────────────────┘
 *
 * 适用环境: arm-linux-musleabi-gcc (musl libc + ARM EABI)
 *
 * linux_dirent64 结构（与内核 ABI 一致）:
 *   d_ino    : 8 bytes (unsigned long long)  inode 号
 *   d_off    : 8 bytes (long long)           下一项偏移
 *   d_reclen : 2 bytes (unsigned short)      本记录总长度
 *   d_type   : 1 byte  (unsigned char)       文件类型 (DT_DIR=4)
 *   d_name   : 变长                           以 '\0' 结尾的文件名
 */

#define _GNU_SOURCE         /* musl: 启用 fstatat / O_DIRECTORY / AT_SYMLINK_NOFOLLOW / DT_* */

#include "record_scan_days.h"

#include <stdio.h>        /* snprintf */
#include <string.h>       /* strlen, strncmp, memset */
#include <fcntl.h>        /* open, O_RDONLY, O_DIRECTORY, AT_SYMLINK_NOFOLLOW */
#include <unistd.h>       /* close */
#include <sys/syscall.h>  /* SYS_getdents64 / __NR_getdents64 */
#include <sys/stat.h>     /* fstatat */
#include <dirent.h>       /* DT_DIR, DT_UNKNOWN */
#include <errno.h>
#include <stdint.h>       /* uint8_t */
#include <stdlib.h>       /* malloc, free */

/* 兜底定义：部分 libc 环境可能缺少 DT_ 宏 */
#ifndef DT_UNKNOWN
#define DT_UNKNOWN  0
#endif
#ifndef DT_DIR
#define DT_DIR      4
#endif

/* ─────────────────────────────────────────
 * 内核 dirent64 结构（避免依赖内核头文件）
 * ───────────────────────────────────────── */
struct linux_dirent64 {
    unsigned long long d_ino;     /* 64-bit inode */
    long long          d_off;     /* 64-bit offset */
    unsigned short     d_reclen;  /* 本记录长度 */
    unsigned char      d_type;    /* 文件类型 */
    char               d_name[];  /* 文件名（变长） */
};

/* ─────────────────────────────────────────
 * 兼容性：不同 libc 对 getdents64 syscall 编号的命名不同
 * ───────────────────────────────────────── */

/* musl 使用 __NR_getdents64，uclibc 也是；glibc 有 SYS_getdents64 */
#if !defined(__NR_getdents64) && defined(SYS_getdents64)
#define __NR_getdents64 SYS_getdents64
#endif

/* 各架构 getdents64 syscall 编号 */
#if !defined(__NR_getdents64)
/* 尝试推断 */
#if defined(__arm__)
  #define __NR_getdents64 217               /* ARM EABI */
#elif defined(__aarch64__)
  #define __NR_getdents64 61                /* ARM64 */
#elif defined(__mips64)
  #define __NR_getdents64 5219              /* MIPS64 (n64: __NR_Linux=5000 + 219) */
#elif defined(__mips__)
  #define __NR_getdents64 4219              /* MIPS32 (o32: __NR_Linux=4000 + 219) */
#elif defined(__i386__)
  #define __NR_getdents64 220
#elif defined(__x86_64__)
  #define __NR_getdents64 217
#else
  #error "Unsupported architecture: cannot determine __NR_getdents64"
#endif
#endif

/* ─────────────────────────────────────────
 * 缓冲区大小：4KB，用 malloc 动态分配
 * 4KB 足够容纳 31 天的目录项，且对齐友好。
 * ───────────────────────────────────────── */

#ifndef SCAN_BUF_SIZE
#define SCAN_BUF_SIZE  4096   /* 4 KB */
#endif

/* ─────────────────────────────────────────
 * 内部辅助：验证目录项名称是否为2位数字
 * ───────────────────────────────────────── */

static inline int is_day_str(const char *s)
{
    return (s[0] >= '0' && s[0] <= '9'
         && s[1] >= '0' && s[1] <= '9'
         && s[2] == '\0');
}

/* ─────────────────────────────────────────
 * 内部辅助：确认目录项是目录（处理 DT_UNKNOWN）
 * ───────────────────────────────────────── */

static int confirm_is_dir(int dirfd, const char *name, unsigned char d_type)
{
    /* 如果内核直接返回了类型 */
    if (d_type == DT_DIR) return 1;
    if (d_type != DT_UNKNOWN) return 0;  /* 确定不是目录 */

    /* DT_UNKNOWN：需要 fallback，用 fstatat 确认 */
    struct stat st;
    if (fstatat(dirfd, name, &st, AT_SYMLINK_NOFOLLOW) == 0) {
        return S_ISDIR(st.st_mode) ? 1 : 0;
    }
    return 0;
}

/* ─────────────────────────────────────────
 * 内部辅助：插入排序（升序，最多31项）
 * ───────────────────────────────────────── */

static void sort_days(uint8_t *days, int count)
{
    for (int i = 1; i < count; i++) {
        uint8_t key = days[i];
        int j = i - 1;
        while (j >= 0 && days[j] > key) {
            days[j + 1] = days[j];
            j--;
        }
        days[j + 1] = key;
    }
}

/* ─────────────────────────────────────────
 * 公开接口实现
 * ───────────────────────────────────────── */

int record_scan_days_fast(const char *dir_path,
                   const char *prefix,
                   char *out,
                   size_t out_sz)
{
    if (!dir_path || !prefix || !out || out_sz == 0) {
        errno = EINVAL;
        return -1;
    }

    size_t prefix_len = strlen(prefix);
    if (prefix_len == 0) {
        errno = EINVAL;
        return -1;
    }

    /* 打开目录 */
    int dirfd = open(dir_path, O_RDONLY | O_DIRECTORY);
    if (dirfd < 0) {
        return -1;  /* errno 由 open 设置 */
    }

    /* ── malloc 4KB 缓冲区（malloc 默认已对齐到 ≥8 字节）── */
    char *buf = (char *)malloc(SCAN_BUF_SIZE);
    if (!buf) {
        close(dirfd);
        errno = ENOMEM;
        return -1;
    }

    /* 天数收集数组（最多31项） */
    uint8_t days[32];
    int     count = 0;

    /* ───── 循环读取所有目录项 ───── */
    for (;;) {
        long nread = syscall(__NR_getdents64, dirfd, buf, SCAN_BUF_SIZE);

        if (nread == 0) break;          /* 目录读完 */
        if (nread < 0) {
            close(dirfd);
            free(buf);
            count = -1;
            goto out;
        }

        long offset = 0;
        while (offset < nread) {
            struct linux_dirent64 *d = (struct linux_dirent64 *)(buf + offset);

            /* 安全检查：offset + d_reclen 不应越界 */
            if (d->d_reclen == 0) break;
            if (offset + d->d_reclen > nread) break;

            /* 匹配前缀 + 验证后缀为2位数字 */
            const char *name = d->d_name;
            if (strncmp(name, prefix, prefix_len) == 0) {
                const char *day_str = name + prefix_len;
                if (is_day_str(day_str)) {
                    /* 确认是目录（必要时 fstatat） */
                    if (confirm_is_dir(dirfd, name, d->d_type)) {
                        /* 解析为数字: "01"→1, "31"→31 */
                        uint8_t day = (uint8_t)((day_str[0] - '0') * 10
                                                + (day_str[1] - '0'));
                        if (count < 31) {
                            days[count++] = day;
                        }
                    }
                }
            }

            offset += d->d_reclen;
        }
    }

    close(dirfd);
    free(buf);

    /* ───── 排序（升序） + 格式化输出 ───── */
    sort_days(days, count);

    char *p   = out;
    char *end = out + out_sz - 1;
    *p = '\0';

    for (int i = 0; i < count; i++) {
        int written = snprintf(p, (size_t)(end - p + 1), "%d,", days[i]);
        if (written > 0 && p + written <= end) {
            p += written;
        } else {
            break;  /* 输出缓冲区满 */
        }
    }

    if (p != out) {
        *(--p) = '\0';
    }

out:
    return count;
}
