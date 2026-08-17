/*
 *       Filename:  time_sync.c
 *    Description:
 *        Version:  1.0
 *        Created:  2022年11月29日 21时51分33秒
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  zhangjian (),
 *   Organization:
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/* file */
#include <fcntl.h>
#include <sys/file.h>

/* socket() */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "debug.h"
#include "system_ctrl.h"
#include "utils.h"

struct timesync {
    time_t utc_secs;
    time_t mono_secs;
    pthread_mutex_t mut;
};

static struct timesync t = { 0, 0 };

int start_timesync(time_t utc_secs)
{
    if (0 == t.utc_secs) {
        SYSLOG("%s epoch: %lld\n", __func__, utc_secs);
    }

    pthread_mutex_lock(&t.mut);
    t.utc_secs = utc_secs;
    t.mono_secs = mono_sec();
    pthread_mutex_unlock(&t.mut);

    return SUCCESS;
}

int clear_timesync()
{
    pthread_mutex_lock(&t.mut);
    t.utc_secs = 0;
    t.mono_secs = 0;
    pthread_mutex_unlock(&t.mut);

    return SUCCESS;
}

/* 目的：多客户端有序对时
 *       epoch.reboot
 *       record-repair
 *       web
 *       NVR
 *       P2P & NTP
 * 1. p2p 已对时，强制对时 diff
 * 2. p2p 未对时，上电前钟三分钟内不限制对时条件
 *                三分钟后，限制前后对时间隔
 **/
int check_timesync(time_t utc_secs)
{
    int ret = SUCCESS;
    time_t diff = 0, up_secs = mono_sec();

    pthread_mutex_lock(&t.mut);

    if (t.utc_secs) {
        diff = (utc_secs - t.utc_secs) - (up_secs - t.mono_secs);
        // 如果 ntp 已对过时，只允许 60 秒
        if (diff >= 60 || diff <= -60) {
            SYSLOG("invalid timesync: %lld diff: %lld\n", utc_secs, diff);
            ret = FAILURE;
        } else {
            ret = SUCCESS;
        }
    } else {
        static struct timesync prev_t = {0};
        if (up_secs < 180) {
            // 随便对，只记录
            ret = SUCCESS;
        } else {
            if (prev_t.utc_secs == 0) {
                // 随便对，首次
                SYSLOG("not-p2p timesync@1");
                ret = SUCCESS;
            } else {
                diff = (utc_secs - prev_t.utc_secs) - (up_secs - prev_t.mono_secs);
                // 如果已对过时，只允许 60 秒
                if (diff >= 60 || diff <= -60) {
                    SYSLOG("invalid timesync: %lld diff: %lld\n", utc_secs, diff);
                    ret = FAILURE;
                } else {
                    ret = SUCCESS;
                }
            }
        }

        if (ret == SUCCESS) {
            prev_t.utc_secs = utc_secs;
            prev_t.mono_secs = up_secs;
        }
    }

    pthread_mutex_unlock(&t.mut);

    return ret;
}
