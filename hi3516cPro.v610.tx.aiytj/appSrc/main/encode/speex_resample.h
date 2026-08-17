/*
 *       Filename:  speex_resample.h
 *    Description:  
 *        Version:  1.0
 *        Created:  01/15/2026 05:02:59 PM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  xiangyp (), 
 *   Organization:  
 */

#ifndef _SPEEX_RESAMPLE_H
#define _SPEEX_RESAMPLE_H
#ifdef __cplusplus 
extern "C" {
#endif

int speex_resample_8k_to_16k(void *dst_data, size_t *dst_len, void *src_data,
                             size_t src_len, void *usr_data);
void destroy_speex_resampler_8k_to_16k(void);

int speex_resample_16k_to_8k(void *dst_data, size_t *dst_len, void *src_data,
                             size_t src_len, void *usr_data);
void destroy_speex_resampler_16k_to_8k(void);

int speex_resample_24k_to_16k(void *dst_data, size_t *dst_len, void *src_data,
                             size_t src_len, void *usr_data);
void destroy_speex_resampler_24k_to_16k(void);

#ifdef __cplusplus
}
#endif
#endif
