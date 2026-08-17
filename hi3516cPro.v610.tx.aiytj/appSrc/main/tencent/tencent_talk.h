#ifdef PLATFORM_TENCENT

#ifndef TENCENT_TALK_H_
#define TENCENT_TALK_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "faad.h"
#include "encode_audio_output.h"

typedef enum {
    H264    = 1,
    MPEG    = 2,
    MJPEG   = 3,
    H265    = 4,
    H265_HISILICON    = 5,
    MJPEG_DIFT        = 6,
    G711A   = 101,
    ULAW    = 102,
    G711U   = 103,
    PCM     = 104,
    ADPCM   = 105,
    G721    = 106,
    G723    = 107,
    G726_16 = 108,
    G726_24 = 109,
    G726_32 = 110,
    G726_40 = 111,
    AAC     = 112,
    JPG     = 200,
    PNG     = 201,
} AliMediaTypeE;

typedef struct {
    char *pBuf;
    int nBuf;
} play_audio_info_t;

typedef struct {
    int                 dec_init;
    NeAACDecHandle      dec_handle;
} aac_dec_info_t;

int tencent_talk_stop(void);
void talk_set_dec_init_val(int val);
int tencent_push_audio_data(char *buf, int size);
int talk_create_speaker_socket(void);
int destroy_speaker_socket_fd(void);
int tencent_aac_decode_uninit(void);
int tencent_talk_init(void);
int tencent_talk_uninit(void);

#ifdef __cplusplus
}
#endif

#endif
#endif //PLATFORM_TENCENT
