#ifndef _ENCODE_AUDIO_H
#define _ENCODE_AUDIO_H
#ifdef __cplusplus 
extern "C" {
#endif

#include "ot_common_aio.h"

#define PCM_AMPLIFIED_TIMES     (1.0)

int encode_audio_init(void);
int encode_audio_uninit(void);
void amplify_pcm_volume(td_u8 *data, td_u32 bytes_data,
                        ot_audio_bit_width bitwidth, float multiplier);

#ifdef __cplusplus
}
#endif
#endif

