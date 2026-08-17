/*
 *       Filename:  aliyun_mmi_dialog.c
 *    Description:  阿里云 MMI 对话引擎实现
 *                  WSS 连接(aliyun_mmi_websock) + WS 收发 + 录音数据泵
 *                  + TTS 下行播放泵
 *                  录音源: SHM_BUF_AUDIO 共享内存
 *                  TTS播放: 委托 aliyun_mmi_talk 模块
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
#include <pthread.h>
#include <libwebsockets.h>
#include "g711.h"
#include "g_log.h"
#include "utils.h"
#include "c_mmi.h"
#include "shm_buf.h"
#include "encodeapi.h"
#include "ring_buffer.h"
#include "shm_buf_pool.h"
#include "aliyun_mmi_cb.h"
#include "aliyun_mmi_cfg.h"
#include "speex_resample.h"
#include "aliyun_mmi_talk.h"
#include "aliyun_mmi_dialog.h"
#include "aliyun_mmi_server.h"
#include "aliyun_mmi_websock.h"
#include "encode_audio_output.h"

/* ================================================================
 *  内部常量
 * ================================================================ */

#define MS_INTV_WSS               (20)

/* TTS 下行播放：16K / 16bit / mono，200ms = 16000*2*0.2 = 6400 字节 */
#define TTS_SMPL_RATE             (16000)
#define TTS_BYTES_PER_SMPL        (2)
#define TTS_PREBUFF_MS            (200)
#define TTS_PREBUFF_BYTES         (TTS_SMPL_RATE * TTS_BYTES_PER_SMPL * TTS_PREBUFF_MS / 1000)

/* ================================================================
 *  文件级全局状态
 * ================================================================ */

typedef struct {
    char tts_ring[BYTES_MMI_BUF];        /* 环形缓冲本体 */
    shm_buf_t shmbuf_aud;
    JSScheduler sch_wss;
    JSTCHandle  hdl_wss;
    size_t bytes_read;                   /* 读指针：从 SDK 读取音频的写入位置 */
    size_t bytes_played;                 /* 播放指针：播放音频的读取位置 */
    int connected;
    int audio_serial;
    int played_done;
    int ok_to_play;                      /* 是否达到播放要求 */
} sMmiDialogRun;

static sMmiDialogRun g_dl_run = {
    .played_done = TRUE
};

static void cb_reset_player(void *usr_data)
{
    g_dl_run.bytes_read   = 0;
    g_dl_run.bytes_played = 0;
    g_dl_run.ok_to_play   = FALSE;
    g_dl_run.played_done  = TRUE;
}

/* 清空 TTS 下行播放环形缓冲，重置收播进度
 * 用于 ASR_START（用户打断）等场景，立即丢弃已缓冲未播数据
 * 需要和 mmi_dialog_play_speech 同一线程运行
*/
void mmi_dialog_reset_player(void)
{
    if (NULL != g_run_mmi.sch_rx_aud) {
        js_run_function(g_run_mmi.sch_rx_aud, cb_reset_player, NULL, TRUE);
    } else {
        cb_reset_player(NULL);
    }
}

static void cb_reset_played_done(void *usr_data)
{
    g_dl_run.played_done = FALSE;
}

void mmi_dialog_reset_played_done(void)
{
    if (NULL != g_run_mmi.sch_rx_aud) {
        js_run_function(g_run_mmi.sch_rx_aud, cb_reset_played_done, NULL, TRUE);
    } else {
        cb_reset_played_done(NULL);
    }
}

/* ================================================================
 *  内部: shm_buf 音频读取回调 → c_mmi_put_recorder_data
 * ================================================================ */
static void cb_dialog_read_audio(void *userdata, tSBFrame *p_frm)
{
    unsigned char aud_pcm_16k[AUDIO_IN_NUM_PERFRM] = {0};
    int idx = 0;
    short sample_pcm = 0;

    switch (p_frm->error) {
    case SHM_ERR_SUCCESS: {
        //没有播放完，或者已经播放完，但是在收数据了，不发送音频帧给大模型
        if (!g_dl_run.played_done || 
            (g_dl_run.played_done && is_mmi_ai_talking())) {
            pri_mmi(LVL_LOOP, "send mute ai frms\n");
            c_mmi_put_recorder_data((uint8_t *)aud_pcm_16k, p_frm->frame_size * 2);
            break;
        }

        if (g_dl_run.audio_serial <= 0) {
            g_dl_run.audio_serial = p_frm->frame_serial;
        }

        switch (p_frm->mediatype) {
        case SHM_MEDIA_AUDIO_ALAW: {
            for (idx = 0; idx < p_frm->frame_size; idx++) {
                sample_pcm = jco_alaw2linear(p_frm->framedata[idx]);
                aud_pcm_16k[idx * 2] = sample_pcm & 0xFF;
                aud_pcm_16k[idx * 2 + 1] = (sample_pcm >> 8) & 0xFF;
            }
            break;
        }
        case SHM_MEDIA_AUDIO_ULAW: {
            for (idx = 0; idx < p_frm->frame_size; idx++) {
                sample_pcm = jco_ulaw2linear(p_frm->framedata[idx]);
                aud_pcm_16k[idx * 2] = sample_pcm & 0xFF;
                aud_pcm_16k[idx * 2 + 1] = (sample_pcm >> 8) & 0xFF;
            }
            break;
        }
        default: {
            ERR("unsupported media type %d\n", p_frm->mediatype);
            goto exit;
        }
        }

        c_mmi_put_recorder_data((uint8_t *)aud_pcm_16k, p_frm->frame_size * 2);

        g_dl_run.audio_serial++;
        break;
    }
    case SHM_ERR_NOT_READY: {
        pri_mmi(LVL_LOOP, "audio shmbuf not ready\n");
        break;
    }
    case SHM_ERR_OVER_WRITE: {
        ERR("audio shmbuf over write, serial %d\n", g_dl_run.audio_serial);
        g_dl_run.audio_serial = 0;
        break;
    }
    default: {
        pri_mmi(LVL_LOOP, "audio shmbuf err %d\n", p_frm->error);
        break;
    }
    }

exit:

    return;
}

/* ================================================================
 *  内部: WSS service 阻塞线程 — 独立于定时器，持续等待网络事件
 * ================================================================ */

static void cb_loop_ws_service(void *usr_data)
{
    /* 独立线程，只负责收数据。lws_service 回调里把收到的数据
     * 直接注入 SDK，不与其他线程竞争。SDK 内部 mutex 负责保护。 */
    aliyun_mmi_ws_service(-1);
}

/* ================================================================
 *  公开 API: 建立 WSS 连接
 * ================================================================ */

int mmi_dialog_init(void)
{
    int ret = SUCCESS;

    if (g_dl_run.connected) {
        pri_mmi(LVL_LOOP, "already connected\n");
        goto exit;
    }

    /* 获取音频共享内存 */
    g_dl_run.shmbuf_aud = get_shm_buf_pool(SHM_BUF_AUDIO);
    goto_if_fatal_err(NULL != g_dl_run.shmbuf_aud, exit, ret = FAILURE,
                      "failed to get audio shmbuf\n");

    g_dl_run.audio_serial = 0;

    /* 从 SDK 获取 WSS 连接参数 */
    char *host   = c_mmi_get_wss_host();
    char *port_s = c_mmi_get_wss_port();
    char *api    = c_mmi_get_wss_api();
    char *header = c_mmi_get_wss_header();
    int   port   = port_s ? atoi(port_s) : 443;

    DBG("wss param: host=%s port=%d api=%s header=%s\n",
        host ? host : "NULL", port,
        api  ? api  : "NULL",
        header ? header : "NULL");

    goto_if_fatal_err(host && api, exit, ret = FAILURE,
                      "wss param invalid\n");

    /* 重置 TTS 下行播放环形缓冲读播进度。
     * 必须在发起 WSS 连接之前重置，否则连接建立后收到帧时，
     * 可能晚于帧到达才重置，导致读播进度错乱。 */
    g_dl_run.bytes_read   = 0;
    g_dl_run.bytes_played = 0;
    g_dl_run.ok_to_play   = FALSE;

    /* 初始化 websock 模块 */
    ret = aliyun_mmi_ws_init();
    goto_if_fatal_err(SUCCESS == ret, exit, ret = FAILURE,
                      "failed to ws_init\n");

    /* 发起 WSS 连接 */
    ret = aliyun_mmi_ws_connect(host, port, api, header);
    goto_if_fatal_err(SUCCESS == ret, exit, ret = FAILURE,
                      "failed to ws_connect\n");

    g_dl_run.connected = TRUE;

    DBG("wss connect initiated\n");

    g_dl_run.sch_wss = js_create_scheduler("sch_wss");
    goto_if_fatal_err(NULL != g_dl_run.sch_wss, exit, ret = FAILURE,
                      "failed to create sch_wss\n");

    js_create_timer_r(g_dl_run.sch_wss, MS_INTV_WSS, MS_INTV_WSS,
                      cb_loop_ws_service, NULL, &g_dl_run.hdl_wss);
    goto_if_fatal_err(NULL != g_dl_run.hdl_wss, exit, ret = FAILURE,
                      "failed to create wss timer\n");

exit:
    return ret;
}

/* ================================================================
 *  公开 API: 从 SDK 获取待发送帧并发送，发完后唤醒 service 线程
 * ================================================================ */

int mmi_dialog_send_packed_data(void)
{
    static uint8_t ws_send_buf[BYTES_MMI_BUF] = {0};
    uint8_t opcode    = 0;
    uint32_t send_len = 0;
    int ret = SUCCESS;

    if (!g_dl_run.connected) {
        goto exit;
    }

    if (!aliyun_mmi_ws_is_connected()) {
        pri_mmi(LVL_LOOP, "ws not conn\n");
        goto exit;
    }

    send_len = c_mmi_get_send_data(&opcode, ws_send_buf, sizeof(ws_send_buf));

    if (send_len > 0) {
        ret = aliyun_mmi_ws_send(opcode, ws_send_buf, send_len);
        if (SUCCESS != ret) {
            ERR("ws send failed, ret=%d\n", ret);
        }

        /* 发送完数据后唤醒 service 线程，让它处理 WRITEABLE 回调 */
        lws_cancel_service(aliyun_mmi_ws_get_context());
    }

exit:
    return ret;
}

/* ================================================================
 *  公开 API: TTS 播放泵
 * ================================================================ */

int mmi_dialog_play_speech(void)
{
    char buf_to_play[PCM_SMPL_PER_FRM_16K] = {0};
    size_t bytes_got = 0, bytes_to_play = 0, bytes_read_max = 0;
    int ret = SUCCESS;

    if (!g_dl_run.connected) {
        goto exit;
    }

    /* 1. 计算读入空间：读指针到播放指针之间的可用空间，防止覆盖未播数据。
     *    读指针 bytes_read 回绕后可能 < 播放指针 bytes_played，需处理回绕。 */
    if (g_dl_run.bytes_read >= g_dl_run.bytes_played) {
        bytes_read_max = sizeof(g_dl_run.tts_ring) - g_dl_run.bytes_read;
    } else {
        /* 读指针回绕到播放指针前面：可用空间 = 播放指针 - 读指针 */
        bytes_read_max = g_dl_run.bytes_played - g_dl_run.bytes_read;
    }

    /* 2. 从 SDK 读音频写入环形缓冲（读指针 = bytes_read） */
    if (bytes_read_max > 0) {
        bytes_got = c_mmi_get_player_data(
                        (uint8_t *)&g_dl_run.tts_ring[g_dl_run.bytes_read],
                        bytes_read_max);
        if (bytes_got > 0) {
            pri_mmi(LVL_DBG, "got %d new bytes\n", bytes_got);
        } else {
            pri_mmi(LVL_LOOP, "got %d new bytes\n", bytes_got);
        }
        g_dl_run.bytes_read += bytes_got;
        g_dl_run.bytes_read %= sizeof(g_dl_run.tts_ring);
    }

    /* 3. 计算待播放量（播放指针 bytes_played 到读指针 bytes_read 之间） */
    if (g_dl_run.bytes_read < g_dl_run.bytes_played) {
        bytes_to_play = g_dl_run.bytes_read + sizeof(g_dl_run.tts_ring) -
                        g_dl_run.bytes_played;
    } else {
        bytes_to_play = g_dl_run.bytes_read - g_dl_run.bytes_played;
    }

    /* 4. 达到播放要求后，大于 0 即播放；否则累积 200ms 语音 */
    if (g_dl_run.ok_to_play) {
        if (bytes_to_play <= 0) {
            pri_mmi(LVL_LOOP, "tts buffer empty\n");

            if (!is_mmi_ai_talking()) {
                pri_mmi(LVL_DBG, "clear ok to play\n");
                g_dl_run.ok_to_play = FALSE;
            }
            goto exit;
        }
    } else {
        if (bytes_to_play < TTS_PREBUFF_BYTES) {
            pri_mmi(LVL_LOOP, "tts buffer not ready\n");
            goto exit;
        } else {
            pri_mmi(LVL_DBG, "ok to play\n");
            g_dl_run.ok_to_play = TRUE;
        }
    }

    pri_mmi(LVL_LOOP, "tts %d bytes to play\n", bytes_to_play);
    if (bytes_to_play >= sizeof(buf_to_play)) {
        bytes_to_play = sizeof(buf_to_play);
    }

    /* src_ringbuffer_memcpy 内部会通过 RING_BUFFER_ADD 推进 bytes_played，
     * 此处不能再手动推进，否则播放指针会双倍前进导致数据重复播放。 */
    src_ringbuffer_memcpy(buf_to_play, g_dl_run.tts_ring, &g_dl_run.bytes_played,
                          sizeof(g_dl_run.tts_ring), bytes_to_play);

    ret = aliyun_mmi_push_audio_data(buf_to_play, bytes_to_play);
    goto_exit_if_fail(SUCCESS == ret, exit,
                      ret = FAILURE, "failed to push tts: %d\n", ret);

exit:

    return ret;
}

/* ================================================================
 *  公开 API: 驱动一次录音数据泵到 SDK 发送缓冲
 * ================================================================ */

int mmi_dialog_pack_audio_in_frames(void)
{
    int ret = SUCCESS;

    if (!g_dl_run.connected) {
        pri_mmi(LVL_LOOP, "dialog not conn yet\n");
        goto exit;
    }

    if (!g_dl_run.shmbuf_aud) {
        pri_mmi(LVL_LOOP, "shmbuf aud is null\n");
        goto exit;
    }

    shm_buf_read_frame_ex(g_dl_run.shmbuf_aud, g_dl_run.audio_serial,
                          cb_dialog_read_audio, NULL);

exit:
    return ret;
}

/* ================================================================
 *  公开 API: 断开连接
 * ================================================================ */

void mmi_dialog_deinit(void)
{
    if (!g_dl_run.connected) {
        goto exit;
    }

    /* 1. 唤醒 service 线程，让它退出 lws_service 阻塞 */
    if (aliyun_mmi_ws_get_context()) {
        lws_cancel_service(aliyun_mmi_ws_get_context());
    }

    /* 2. 删定时器 + scheduler，等待 service 线程完全退出。
     *    此后本线程独占 context，ws_disconnect 可安全阻塞驱动 close。 */
    js_delete_timer_r(&g_dl_run.hdl_wss);
    js_delete_scheduler(g_dl_run.sch_wss);
    g_dl_run.sch_wss = NULL;

    /* 3. 断开 WSS 连接（保留 context，不释放 OpenSSL 全局锁表）。
     *    内部阻塞调用 lws_service 直到服务端 close 回包到达。 */
    aliyun_mmi_ws_disconnect();

    /* 4. 停止 TTS 播放管线 */
    aliyun_mmi_talk_uninit();

    destroy_speex_resampler_24k_to_16k();

    g_dl_run.connected   = FALSE;
    g_dl_run.shmbuf_aud  = NULL;
    g_dl_run.audio_serial = 0;

    /* 重置 TTS 下行播放环形缓冲读播进度 */
    g_dl_run.bytes_read   = 0;
    g_dl_run.bytes_played = 0;
    g_dl_run.ok_to_play   = FALSE;

    DBG("deinit\n");

exit:
    return;
}

/* ================================================================
 *  公开 API: 查询连接状态
 * ================================================================ */

int aliyun_mmi_dialog_is_connected(void)
{
    return g_dl_run.connected && aliyun_mmi_ws_is_connected();
}
