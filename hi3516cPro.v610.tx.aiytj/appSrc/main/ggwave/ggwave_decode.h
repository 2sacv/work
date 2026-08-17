#ifndef GGWAVE_DECODE_H
#define GGWAVE_DECODE_H
#ifdef __cplusplus
extern "C" {
#endif

int ggwave_pcm2str(char *waveform, int waveformlen, char *decoded, int decoded_len);

int ggwave_uninstance();


#ifdef __cplusplus
}
#endif
#endif
