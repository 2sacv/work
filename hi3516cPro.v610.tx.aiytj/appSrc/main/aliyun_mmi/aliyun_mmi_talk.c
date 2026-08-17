/*
 *       Filename:  aliyun_mmi_talk.c
 *    Description:  阿里云 MMI TTS 音频播放模块实现
 *                  init → speaker socket, push → write(fd, pcm, size),
 *                  stop → close(fd), uninit → stop
 *                  云端下发 PCM，无需转码/缓存
 *        Version:  1.0
 *        Created:  07/11/2026 14:30:00 PM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  xiangyp (),
 *   Organization:
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "g_log.h"
#include "utils.h"
#include "confapi.h"
#include "aliyun_mmi_talk.h"
#include "encode_audio_output.h"

/* ================================================================
 *  运行时状态
 * ================================================================ */

typedef struct {
    int spkr_fd;
} sMmiTalkRun;

static sMmiTalkRun g_talk = {-1};

/* ================================================================
 *  公开 API: 初始化 → 建立 speaker socket
 * ================================================================ */

int aliyun_mmi_talk_init(void)
{
    struct sockaddr_in addr = {0};
    int audio_port = 0;
    int ret        = SUCCESS;

    if (g_talk.spkr_fd > 0) {
        goto exit;
    }

    conf_get_speekportcfg(&audio_port);
    goto_exit_if_fail(audio_port > 0, exit, ret = FAILURE,
                      "failed to get speekport\n");

    addr.sin_family = AF_INET;
    addr.sin_port   = htons(audio_port);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    g_talk.spkr_fd = socket(AF_INET, SOCK_STREAM, 0);
    goto_exit_if_fail(g_talk.spkr_fd > 0, exit, ret = FAILURE,
                      "failed to create socket\n");

    ret = connect(g_talk.spkr_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr));
    if (ret < 0) {
        ERR("failed to connect audio port: %s\n", strerror(errno));
        close(g_talk.spkr_fd);
        g_talk.spkr_fd = -1;
        ret = FAILURE;
        goto exit;
    }

    pri_mmi(LVL_DBG, "init fd=%d\n", g_talk.spkr_fd);
    ret = SUCCESS;

    encode_ao_set_talking_samplerate(OT_AUDIO_SAMPLE_RATE_16000);

exit:
    return ret;
}

/* ================================================================
 *  公开 API: 推送 PCM 数据 → 直写 speaker socket
 * ================================================================ */

int aliyun_mmi_push_audio_data(char *buf, int size)
{
    int wr = 0;
    int ret = SUCCESS;

    goto_exit_if_fail(buf && size > 0, exit, ret = FAILURE,
                      "push_audio param invalid\n");
    goto_exit_if_fail(g_talk.spkr_fd >= 0, exit, ret = FAILURE,
                      "push_audio socket not ready\n");

    pri_mmi(LVL_LOOP, "write %d bytes audio data\n", size);
    wr = write(g_talk.spkr_fd, buf, size);
    goto_exit_if_fail(wr >= 0, exit, ret = FAILURE,
                      "failed to write: %s\n", strerror(errno));

exit:
    return ret;
}

/* ================================================================
 *  公开 API: 反初始化 → 关闭 speaker socket
 * ================================================================ */

int aliyun_mmi_talk_uninit(void)
{
    if (g_talk.spkr_fd > 0) {
        close(g_talk.spkr_fd);
        g_talk.spkr_fd = -1;
        pri_mmi(LVL_DBG, "uninit\n");
    }

    return SUCCESS;
}
