#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include "utils.h"
#include "encode_aac.h"
#include "g_sys.h"
#include "g_run.h"
#include "encode_common.h"
#include "encode_audio_output.h"

aac_enc_info_t aac_enc_info = {0,};

int write_pcm(void *buf, int len, const char * filepath)
{
    static int i = 0;
    static int start = 1; 
    static FILE *fp = NULL;
    //if (pop_g_run(audio, RUN_AUDIOIN_SAVE)) start = !start; 

    if (start) {
        if (!fp) {
            fp = fopen(filepath, "w");
        }
        int wr = fwrite(buf, len, 1, fp);
        DBG("___i[%d]_ len:%d wr:%d\n", i++, len, wr);
    } else {
        if (fp) {
            fsync(fileno(fp));
            fclose(fp);
            fp = NULL;
            i = 0;
            DBG("___ write %s over\n", filepath);
        }
    }

    return 0;
}

//pcm8k转16k,只能采样率8k的16bit的音频变成采样率16k的16bit的声音
void pcmaudio_8k_to_16k(short* in, int in_len, short* out, int *out_len)
{
    if (!in || !out || in_len <= 0) {     // 输入参数校验
        return;
    }

    int out_idx = 0;
    const int input_samples = in_len / sizeof(short);  // 输入样本总数
    for (int i = 0; i < input_samples; ++i) 
    {
        // 当前样本
        short current = in[i];

        // 下一个样本(边界处理)
        short next = (i < input_samples - 1) ? in[i + 1] : current;

        // 线性插值计算中间值
        short interpolated = (short)((current + next) / 2);

        // 写入两个输出样本
        out[out_idx++] = current;       // 原始样本
        out[out_idx++] = interpolated;  // 插值样本
    }

    *out_len = out_idx * sizeof(short);  // 输出字节数

    return;
}

int aac_encode_init()
{
    // FAAC库版本信息
    char *faac_id_string = NULL;
    char *faac_copyright_string = NULL;
    faacEncGetVersion(&faac_id_string, &faac_copyright_string);
    DBG("FAAC Version: %s\n", faac_id_string);
    DBG("FAAC Copyright: %s\n", faac_copyright_string);

    // 初始化编码器参数
    aac_enc_info.sampleRate = 16000;
    aac_enc_info.numChannels = 1;
    aac_enc_info.inputSamples = 0;
    aac_enc_info.maxOutputBytes = 0;

    // 打开编码器
    aac_enc_info.enc_handle = faacEncOpen(aac_enc_info.sampleRate, aac_enc_info.numChannels, &aac_enc_info.inputSamples, &aac_enc_info.maxOutputBytes);
    if (!aac_enc_info.enc_handle) {
        ERR("Failed to open encoder\n");
        return -1;
    }

    // 获取并设置编码器配置
    faacEncConfigurationPtr faac_config = faacEncGetCurrentConfiguration(aac_enc_info.enc_handle);
    faac_config->inputFormat = FAAC_INPUT_16BIT; // 假设输入是16位PCM
    faac_config->mpegVersion = MPEG4;
    faac_config->aacObjectType = LOW;
    faac_config->bitRate = 32000;// 直接决定音频质量和大小
    faac_config->bandWidth = aac_enc_info.sampleRate / 2;// 音频带宽,不超过[采样率/2]
    faac_config->outputFormat = ADTS_STREAM;
    faac_config->useTns = 0;
    faac_config->allowMidside = 0;
    if (!faacEncSetConfiguration(aac_enc_info.enc_handle, faac_config)) {
        ERR("Failed to set encoder configuration\n");
        faacEncClose(aac_enc_info.enc_handle);
        return -1;
    }

    aac_enc_info.input_size = aac_enc_info.inputSamples*2; //frameLength是每帧每个channel的采样点数

    return 0;
}

int aac_encode_uninit(void)
{
    DBG("tencent_aac_encode_uninit\n");

    faacEncClose(aac_enc_info.enc_handle);
    aac_enc_info.enc_handle = NULL;

    return 0;
}

