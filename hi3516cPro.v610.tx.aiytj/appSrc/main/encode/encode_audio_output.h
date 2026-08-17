/*
 *       Filename:  encode_audio_output.h
 *    Description:  
 *        Version:  1.0
 *        Created:  10/26/2022 08:52:59 AM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  xiangyp (), 
 *   Organization:  
 */

#ifndef _ENCODE_AUDIO_OUTPUT_H
#define _ENCODE_AUDIO_OUTPUT_H
#ifdef __cplusplus 
extern "C" {
#endif

#define AUDIO_OUT_DEV_ID           (0)
#define AUDIO_OUT_CHN_ID           (0)
#define GPIO_AUDIO_ON              (0)
#define GPIO_AUDIO_OFF             (1)
#define GPIO_AUDIO_OUT             (56)

#define PCM_SMPL_PER_FRM_8K        (640)
#define PCM_SMPL_PER_FRM_16K       (640*2)
#define AAC_SMPL_PER_FRM_16K       (1024)

#define PCM_FRM_BYTES_8K            (PCM_SMPL_PER_FRM_8K * 2)
#define PCM_FRM_BYTES_16K           (PCM_SMPL_PER_FRM_16K* 2)          // 2560
#define AAC_FRM_BYTES_16K           (AAC_SMPL_PER_FRM_16K* 2)          // 2048

#define PCM_FRM_DURATION_8K         (PCM_SMPL_PER_FRM_8K *1000/ 8000)  //80ms, 8k采样率，与16k一致
#define PCM_FRM_DURATION_16K        (PCM_SMPL_PER_FRM_16K*1000/16000)  //80ms, 16k采样率
#define AAC_FRM_DURATION_16K        (AAC_SMPL_PER_FRM_16K*1000/16000)  //64ms, write_aac_shm_buf()

#define BYTES_AUDIO_PERFRM_MAX      (PCM_SMPL_PER_FRM_16K * 2)

int get_audioout_status(void);

int audio_output_sdk_init(void);

int audio_output_sdk_uninit(void);

int encode_audio_out_init(void);

void encode_audio_out_uninit(void);

int encode_audio_stop_playing(void);

void encode_ao_set_talking_samplerate(int samplerate);

#ifdef __cplusplus
}
#endif
#endif
