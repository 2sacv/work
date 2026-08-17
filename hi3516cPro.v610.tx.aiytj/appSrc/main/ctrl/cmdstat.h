/*
 *       Filename:  cmdstat.h
 *        Created:  2023-04-17 20:49:21
 *         Author:  zhangjian wuhy,
 *   Organization:  JCO
 *                  1.  co-work with js_event.h,
 *                  2.  Replace dmap with 3 struct POINTER:
 *                      g_cfg_xxx, 模块需用到的所有 config, 和 jcp event cb 强相关
 *                      g_raw_xxx, config 的内部拷贝，只给 diff_cfg2cmd() 和 cb() 使用
 *                      g_run_xxx, 程序(算法)逻辑需要的常驻内存的变量集合，如君正 attr
 *                  3.  只有 以下 3 个业务需要 ctx 中的 mutex 锁，其它使用上面的指针
 *                      attach_config & attach_event
 *                      loop
 *                      diff
 */

#ifndef _ITASK_H
#define _ITASK_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "debug.h"
#include "jevent.h"     // p_cmd 必有 attach_event
#include "jconfig.h"    // p_cmd 必有 attach_config

typedef int (*pCbHandleCmd)(void *usr_data);

typedef struct {
    int          cmd;
    pCbHandleCmd cb_handle_cmd;
} sCmdFunc;

/*
 * cmdstat mean INDEPENDENT TASK
 * 参考 git stage 的设计，用 VAR 及 VAR_stage，在只加一把锁的情况下，保证线程安全。
 * VAR_stage 是要加锁才能访问的变量， VAR 则是 loop 内访问的变量。
 */
struct cmdstat {
    int  cmd_stage;
    int  cmd_self;                  // delay operator
    void (*diff_cfg2cmd)(void *);   // 对 cfg new 逻辑比对，set_command 设置 cmd_self，比如重启编码器
    pthread_mutex_t mutex;
    void *p_new;                    // js_run_function(, syncflag=0) async
    void *p_cfg;                    // 业务使用的配置
    void *p_run;                    // 业务逻辑
};

int  cmd_get_command(struct cmdstat *p_cmd);
void cmd_set_command(struct cmdstat *p_cmd, int bit);
void cmd_clr_command(struct cmdstat *p_cmd, int bit);

/*
 * encode_Auto_Len_osd_set_Zoom() 通过事件改写时可用到，比 js_run_function 实时性差
 **/
#define CB2CMDCFG_FREE(cb, cmd, p_member)               \
static void cb(int id, void *ptr, int size, void *ctx)  \
{                                                       \
    struct cmdstat *p_cmd = (struct cmdstat *)ctx;      \
    pthread_mutex_lock(&(p_cmd->mutex));                \
    p_cmd->cmd_stage |= cmd;                            \
    memcpy(p_member, ptr, size);                        \
    free(ptr);                                          \
    pthread_mutex_unlock(&(p_cmd->mutex));              \
    /* DBG("%s CMD: %s\n", __func__, #cmd); */          \
}

/*
 * ptr 由 js_scheduler 传入，js 内部维护一个 ptr 的引用，当所有 attach 的 cb 回调完成，自动 free()
 * 与 send_conf_data() 和 send_event_chn() 配套
 * (p_dst && p_src) 指针不做保护判断，防止业务缺陷无法暴露
 **/
#define CPY2CMDCFG(cmd, p_dst, p_src, size)             \
do {                                                    \
    struct cmdstat *p_cmd = (struct cmdstat *)ctx;      \
    pthread_mutex_lock(&(p_cmd->mutex));                \
    p_cmd->cmd_stage |= cmd;                            \
    memcpy(p_dst, p_src, size);                         \
    pthread_mutex_unlock(&(p_cmd->mutex));              \
    /* DBG("%s CMD: %s @%p\n", __func__, #cmd, p_src);*/\
} while(0)

/*
 * 可以不管 p_src 指向的数据内容，
 * 与 send_conf_nake() 和 send_event() 配套
 **/
#define CPY2CMD(cmd)             \
do {                                                    \
    struct cmdstat *p_cmd = (struct cmdstat *)ctx;      \
    pthread_mutex_lock(&(p_cmd->mutex));                \
    p_cmd->cmd_stage |= cmd;                            \
    pthread_mutex_unlock(&(p_cmd->mutex));              \
} while(0)

#ifdef __cplusplus
}
#endif
#endif
