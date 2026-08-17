/*
 *       Filename:  encode_audio_queue.h
 *    Description:
 *        Version:  1.0
 *        Created:  11/03/2022 05:12:25 PM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  xiangyp (),
 *   Organization:
 */

#ifndef _ENCODE_AUDIO_QUEUE_H
#define _ENCODE_AUDIO_QUEUE_H
#ifdef __cplusplus
extern "C" {
#endif

#include "jconfstruct.h"


#define NO_AUDIO_DATA       (1)

typedef enum {
    AUDIO_TYPE_NONE     = 0,
    AUDIO_TYPE_ALARM    = 1,
    AUDIO_TYPE_LIGHT    = 2,
    AUDIO_TYPE_NET      = 3,
    AUDIO_TYPE_UPGRADE  = 4,
    AUDIO_TYPE_4G       = 5,
    AUDIO_TYPE_FACTORY  = 6,
    AUDIO_TYPE_SYSTEM   = 7,
    AUDIO_TYPE_VIDCALL  = 8,
    AUDIO_TYPE_ASR      = 9,
} eAudioType;

typedef struct {
    unsigned char *audio;
    int len;
    int duration;
    eAudioType type;
} AudioDataS;

int get_amr_ms_by_name(AUDIO_PROMPT name);
int get_amr_play_duration_ms_by_path(const char *path, int *duration);
int get_amr_path_from_alarm_type(AUDIO_PROMPT name, char **path);

int audio_decode_local_amr(const char *path, unsigned char **audio, int *len, int *duration);

int encode_audio_queue_push_amr(AUDIO_PROMPT name, int fast_play);
int encode_audio_queue_get_amr(AudioDataS *audio);
void encode_audio_queue_clean(void);
int encode_audio_queue_start(void);
int encode_audio_queue_stop(void);
int play_conditionally(AUDIO_PROMPT status);

#ifdef __cplusplus
}
#endif
#endif
