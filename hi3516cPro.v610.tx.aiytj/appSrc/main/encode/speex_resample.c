/* 
 *       Filename:  speex_resample.c
 *    Description:  
 *        Version:  1.0
 *        Created:  01/15/2026 05:02:47 PM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  xiangyp (), 
 *   Organization:  
 */
#include "debug.h"
#include "ot_common_aio.h"
#include "speex_resampler.h"

#define CHN_MONO        (1)

typedef struct {
    SpeexResamplerState *p_hdl_8k_16k;
    SpeexResamplerState *p_hdl_16k_8k;
    SpeexResamplerState *p_hdl_24k_16k;
} sSpeexRun;

static sSpeexRun g_run_sp = {0};

//SPEEX_RESAMPLER_QUALITY_MAX         最高质量
//SPEEX_RESAMPLER_QUALITY_MIN         低质量
//SPEEX_RESAMPLER_QUALITY_DEFAULT     平衡质量与性能
//SPEEX_RESAMPLER_QUALITY_VOIP        适合实时通信场景
//SPEEX_RESAMPLER_QUALITY_DESKTOP
static int init_speex_8k_to_16k_resampler(void)
{
    int err = RESAMPLER_ERR_SUCCESS;

    if (NULL != g_run_sp.p_hdl_8k_16k) {
        goto exit;
    }

    g_run_sp.p_hdl_8k_16k =
        speex_resampler_init(CHN_MONO, OT_AUDIO_SAMPLE_RATE_8000,
                             OT_AUDIO_SAMPLE_RATE_16000, SPEEX_RESAMPLER_QUALITY_VOIP,
                             &err);
    if (RESAMPLER_ERR_SUCCESS != err) {
        SYSLOG("failed to init 8k to 16k speex resampler\n");
        g_run_sp.p_hdl_8k_16k = NULL;
    }

exit:

    return err;
}

int speex_resample_8k_to_16k(void *dst_data, size_t *dst_len, void *src_data,
                             size_t src_len, void *usr_data)
{
    int err = 0;
    spx_uint32_t input_len = src_len / sizeof(short);
    spx_uint32_t output_len = (input_len * OT_AUDIO_SAMPLE_RATE_16000) /
                              OT_AUDIO_SAMPLE_RATE_8000;
    
    goto_exit_if_fail(NULL != dst_data, exit, err = FAILURE,
                      "%s dst_data is null\n", __func__);
    goto_exit_if_fail(NULL != dst_len, exit, err = FAILURE,
                      "%s dst_len is null\n", __func__);
    goto_exit_if_fail(*dst_len >= output_len, exit, err = FAILURE,
                      "dst len is not longer enough\n");
    goto_exit_if_fail(NULL != src_data, exit, err = FAILURE,
                      "%s src_data is null\n", __func__);
    goto_exit_if_fail(src_len > 0, exit, err = FAILURE,
                      "%s src_len is less than 1\n", __func__);

    err = init_speex_8k_to_16k_resampler();
    goto_exit_if_fail(RESAMPLER_ERR_SUCCESS == err, exit, err = FAILURE,
                      "failed to init speex resampler 8k to 16k\n");

    err = speex_resampler_process_int(g_run_sp.p_hdl_8k_16k, 0, (short *)src_data,
                                      &input_len, (short *)dst_data, &output_len);
    goto_exit_if_fail(RESAMPLER_ERR_SUCCESS == err, exit, err = FAILURE,
                      "speex failed to resample 8k to 16k\n");
    //DBG("input len: %d, output_len: %d\n", input_len, output_len);

    *dst_len = output_len * sizeof(short);

exit:

    return err;
}

void destroy_speex_resampler_8k_to_16k(void)
{
    if (NULL != g_run_sp.p_hdl_8k_16k) {
        speex_resampler_destroy(g_run_sp.p_hdl_8k_16k);
        g_run_sp.p_hdl_8k_16k = NULL;
    }
}

static int init_speex_16k_to_8k_resampler(void)
{
    int err = RESAMPLER_ERR_SUCCESS;

    if (NULL != g_run_sp.p_hdl_16k_8k) {
        goto exit;
    }

    g_run_sp.p_hdl_16k_8k =
        speex_resampler_init(CHN_MONO, OT_AUDIO_SAMPLE_RATE_16000,
                             OT_AUDIO_SAMPLE_RATE_8000, SPEEX_RESAMPLER_QUALITY_VOIP,
                             &err);
    if (RESAMPLER_ERR_SUCCESS != err) {
        SYSLOG("failed to init 16k to 8k speex resampler\n");
        g_run_sp.p_hdl_16k_8k = NULL;
    }

exit:

    return err;
}

int speex_resample_16k_to_8k(void *dst_data, size_t *dst_len, void *src_data,
                             size_t src_len, void *usr_data)
{
    int err = 0;
    spx_uint32_t input_len = src_len / sizeof(short);
    spx_uint32_t output_len = (input_len * OT_AUDIO_SAMPLE_RATE_8000) /
                              OT_AUDIO_SAMPLE_RATE_16000;
    
    goto_exit_if_fail(NULL != dst_data, exit, err = FAILURE,
                      "%s dst_data is null\n", __func__);
    goto_exit_if_fail(NULL != dst_len, exit, err = FAILURE,
                      "%s dst_len is null\n", __func__);
    goto_exit_if_fail(*dst_len >= output_len, exit, err = FAILURE,
                      "dst len is not longer enough\n");
    goto_exit_if_fail(NULL != src_data, exit, err = FAILURE,
                      "%s src_data is null\n", __func__);
    goto_exit_if_fail(src_len > 0, exit, err = FAILURE,
                      "%s src_len is less than 1\n", __func__);

    err = init_speex_16k_to_8k_resampler();
    goto_exit_if_fail(RESAMPLER_ERR_SUCCESS == err, exit, err = FAILURE,
                      "failed to init speex resampler 16k to 8k\n");

    err = speex_resampler_process_int(g_run_sp.p_hdl_16k_8k, 0, (short *)src_data,
                                      &input_len, (short *)dst_data, &output_len);
    goto_exit_if_fail(RESAMPLER_ERR_SUCCESS == err, exit, err = FAILURE,
                      "speex failed to resample 16k to 8k\n");

    *dst_len = output_len * sizeof(short);

exit:

    return err;
}

void destroy_speex_resampler_16k_to_8k(void)
{
    if (NULL != g_run_sp.p_hdl_16k_8k) {
        speex_resampler_destroy(g_run_sp.p_hdl_16k_8k);
        g_run_sp.p_hdl_16k_8k = NULL;
    }
}

static int init_speex_24k_to_16k_resampler(void)
{
    int err = RESAMPLER_ERR_SUCCESS;

    if (NULL != g_run_sp.p_hdl_24k_16k) {
        goto exit;
    }

    g_run_sp.p_hdl_24k_16k =
        speex_resampler_init(CHN_MONO, OT_AUDIO_SAMPLE_RATE_24000,
                             OT_AUDIO_SAMPLE_RATE_16000, SPEEX_RESAMPLER_QUALITY_VOIP,
                             &err);
    if (RESAMPLER_ERR_SUCCESS != err) {
        SYSLOG("failed to init 24k to 16k speex resampler\n");
        g_run_sp.p_hdl_24k_16k = NULL;
    }

exit:

    return err;
}

int speex_resample_24k_to_16k(void *dst_data, size_t *dst_len, void *src_data,
                              size_t src_len, void *usr_data)
{
    int err = 0;
    spx_uint32_t input_len = src_len / sizeof(short);
    spx_uint32_t output_len = (input_len * OT_AUDIO_SAMPLE_RATE_16000) /
                              OT_AUDIO_SAMPLE_RATE_24000;

    goto_exit_if_fail(NULL != dst_data, exit, err = FAILURE,
                      "%s dst_data is null\n", __func__);
    goto_exit_if_fail(NULL != dst_len, exit, err = FAILURE,
                      "%s dst_len is null\n", __func__);
    goto_exit_if_fail(*dst_len >= output_len, exit, err = FAILURE,
                      "dst len is not longer enough\n");
    goto_exit_if_fail(NULL != src_data, exit, err = FAILURE,
                      "%s src_data is null\n", __func__);
    goto_exit_if_fail(src_len > 0, exit, err = FAILURE,
                      "%s src_len is less than 1\n", __func__);

    err = init_speex_24k_to_16k_resampler();
    goto_exit_if_fail(RESAMPLER_ERR_SUCCESS == err, exit, err = FAILURE,
                      "failed to init speex resampler 24k to 16k\n");

    err = speex_resampler_process_int(g_run_sp.p_hdl_24k_16k, 0, (short *)src_data,
                                      &input_len, (short *)dst_data, &output_len);
    goto_exit_if_fail(RESAMPLER_ERR_SUCCESS == err, exit, err = FAILURE,
                      "speex failed to resample 24k to 16k\n");

    *dst_len = output_len * sizeof(short);

exit:

    return err;
}

void destroy_speex_resampler_24k_to_16k(void)
{
    if (NULL != g_run_sp.p_hdl_24k_16k) {
        speex_resampler_destroy(g_run_sp.p_hdl_24k_16k);
        g_run_sp.p_hdl_24k_16k = NULL;
    }
}

