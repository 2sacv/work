/*
 *       Filename:  aliyun_mmi_talk.h
 *    Description:  阿里云 MMI TTS 音频播放模块声明
 *                  init 创建 speaker socket, push 直写, stop/uninit 关闭
 *                  云端下发 PCM，无需转码
 *        Version:  1.0
 *        Created:  07/11/2026 14:30:00 PM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  xiangyp (),
 *   Organization:
 */

#ifndef __ALIYUN_MMI_TALK_H__
#define __ALIYUN_MMI_TALK_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化: 创建 speaker socket
 * @return SUCCESS/FAILURE
 */
int aliyun_mmi_talk_init(void);

/**
 * @brief 写入 PCM 数据到 speaker socket
 * @param buf  音频数据（PCM）
 * @param size 数据长度
 * @return SUCCESS/FAILURE
 */
int aliyun_mmi_push_audio_data(char *buf, int size);

/**
 * @brief 反初始化: 关闭 speaker socket
 * @return SUCCESS/FAILURE
 */
int aliyun_mmi_talk_uninit(void);

#ifdef __cplusplus
}
#endif
#endif /* __ALIYUN_MMI_TALK_H__ */
