/******************************************************************************
    Copyright (C), 2008-2018, JABSCO ELECTRONIC Tech. Co., Ltd
 
    File Name    : encode_captureaudio.h
    Version      : 1.0
    Author       : JABSCO Video Server Software Group
    Created      : 2015-03-25
    Description  : 
    History      : 
                        created by tianjun. 2015-03-25
******************************************************************************/

#ifndef __ENCODE_AAC_H__
#define __ENCODE_AAC_H__

#ifdef __cplusplus
extern "C" {
#endif
#define AAC_MAX_LEN 2048
#include "faac.h"
#include "faaccfg.h"


typedef struct {
    unsigned long sampleRate;
    unsigned int numChannels;
    unsigned long inputSamples;
    unsigned long maxOutputBytes;
    int input_size;
    double timeStamp; // AAC音频时间戳
    int curbytes;//当前audio_buf里剩余数据大小
    char aac_buf[AAC_MAX_LEN];// 用于AAC编码
    char pcm_buf[AAC_MAX_LEN*3];// 用于保存pcm数据
    faacEncHandle   enc_handle;
} aac_enc_info_t;

int write_pcm(void *buf, int len, const char * filepath);
int pcm_audio_to_aac(void* pcm_data, int pcm_size, void* aac_data, int aac_size);
void pcmaudio_8k_to_16k(short* in, int in_len, short* out, int *out_len);
int aac_encode_init(void);
int aac_encode_uninit(void);

#ifdef __cplusplus
}
#endif

#endif//__ENCODE_AAC_H__
