/**
 * @file record_scan_days.h
 * @brief 使用 getdents64 syscall 一次性扫描指定月份的所有天数子目录
 *
 * 对比原始方案（31次 access）：
 *   原始: for(i=1..31) access() → 最多31次系统调用
 *   当前: 1次 getdents64 syscall → 内存扫描所有匹配项
 *
 * 适用于 arm-linux-musleabi 环境（musl libc + ARM EABI）
 */

#ifndef __RECORD_SCAN_DAYS_H__
#define __RECORD_SCAN_DAYS_H__

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 扫描目录下指定月份前缀的所有天数子目录
 *
 * 例如 prefix = "202605"，会匹配 "20260501", "20260502", ..., "20260531"。
 * 结果以逗号分隔写入 out，如 "01,02,05,10,"。
 *
 * @param dir_path  目录路径，如 "/mnt/IPCamera"
 * @param prefix    月份前缀，如 "202605"
 * @param out       输出缓冲区
 * @param out_sz    输出缓冲区大小
 * @return          匹配到的天数（目录数量），<0 表示错误
 */
int record_scan_days_fast(const char *dir_path,
                   const char *prefix,
                   char *out,
                   size_t out_sz);

#ifdef __cplusplus
}
#endif

#endif /* __SCAN_DAYS_H__ */
