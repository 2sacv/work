/*
 *       Filename:  aliyun_mmi_dialog.h
 *    Description:  阿里云 MMI 对话引擎声明（WSS + WS 收发 + 录音泵）
 *        Version:  1.0
 *        Created:  07/11/2026 14:30:00 PM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  xiangyp (),
 *   Organization:
 */

#ifndef __ALIYUN_MMI_DIALOG_H__
#define __ALIYUN_MMI_DIALOG_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 建立 WSS 连接
 *        通过 c_mmi_get_wss_host/port/api/header 获取连接参数
 * @return SUCCESS/FAILURE
 */
int mmi_dialog_init(void);

/**
 * @brief 断开 WSS + 停止 TTS 播放管线
 */
void mmi_dialog_deinit(void);

/**
 * @brief 从 SDK 获取待发送数据并发送，发完后唤醒 service 线程
 * @return SUCCESS/FAILURE
 */
int mmi_dialog_send_packed_data(void);

/**
 * @brief TTS 播放泵：从 SDK 播放缓冲区取 PCM → speaker
 * @return SUCCESS/FAILURE
 */
int mmi_dialog_play_speech(void);

/**
 * @brief 驱动一次录音数据泵（轮询调用）
 *        从录音 ringbuffer 取数据 → c_mmi_put_recorder_data
 * @return SUCCESS/FAILURE
 */
int mmi_dialog_pack_audio_in_frames(void);

/**
 * @brief 清空 TTS 下行播放环形缓冲，重置收播进度
 *        用于 ASR_START（用户打断）等场景，立即丢弃已缓冲未播数据
 */
void mmi_dialog_reset_player(void);

void mmi_dialog_reset_played_done(void);

#ifdef __cplusplus
}
#endif
#endif /* __ALIYUN_MMI_DIALOG_H__ */
