/*
 *       Filename:  aliyun_mmi_cb.h
 *    Description:  MMI 回调集声明 + 状态查询接口
 *        Version:  1.0
 *        Created:  07/10/2026 02:27:01 PM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  xiangyp (),
 *   Organization:
 */

#ifndef _ALIYUN_MMI_CB_H
#define _ALIYUN_MMI_CB_H
#ifdef __cplusplus
extern "C" {
#endif

typedef int (*cbMmiEvt) (void *param);

/* ---------- MMI SDK 回调函数 ---------- */

int cb_recv_usr_config(void *param);
int cb_recv_data_init(void *param);
int cb_recv_speech_ready(void *param);
int cb_recv_speech_prepare(void *param);
int cb_recv_speech_start(void *param);
int cb_recv_speech_interrupt(void *param);
int cb_recv_data_deinit(void *param);
int cb_recv_asr_start(void *param);
int cb_recv_asr_incomplete(void *param);
int cb_recv_asr_complete(void *param);
int cb_recv_asr_end(void *param);
int cb_recv_llm_incomplete(void *param);
int cb_recv_llm_complete(void *param);
int cb_recv_tts_start(void *param);
int cb_recv_tts_end(void *param);
int cb_recv_heartbeat(void *param);
int cb_recv_error(void *param);

int cb_recv_volume_unkown(void *param);
int cb_recv_volume_increase(void *param);
int cb_recv_volume_decrease(void *param);
int cb_recv_volume_set(void *param);
int cb_recv_volume_mute(void *param);
int cb_recv_volume_unmute(void *param);

/* ---------- 状态查询 ---------- */

int is_mmi_data_inited(void);
int is_mmi_speech_ready(void);
int is_mmi_ai_talking(void);

void mmi_cb_reset_ai_talking(void);

#ifdef __cplusplus
}
#endif
#endif /* _ALIYUN_MMI_CB_H */
