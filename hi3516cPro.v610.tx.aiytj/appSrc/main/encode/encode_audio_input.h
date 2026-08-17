#ifndef _ENCODE_AUDIO_INPUT_H
#define _ENCODE_AUDIO_INPUT_H
#ifdef __cplusplus 
extern "C" {
#endif

#define AUDIO_IN_DEV_ID             (0)
#define AUDIO_IN_CHNID              (0)
#define AUDIO_IN_AENC_CHN           (0)

#include "ot_common_aio.h"

#define DECL_BIGGER(_dst, _src, type_b, type_s, struct_memb) type_b struct_memb;
#define DECL_SMALLER(_dst, _src, type_b, type_s, struct_memb) type_s struct_memb;
#define COPY_BIG2SMALL(_dst, _src, type_b, type_s, struct_memb) (_dst)->struct_memb = (_src)->struct_memb;
#define COPY_SMALL2BIG(_dst, _src, type_b, type_s, struct_memb) (_dst)->struct_memb = (_src)->struct_memb;

#define PNR_MEMB_LIST(_dst, _src, list)                          \
        list(_dst, _src, int, td_bool, usr_mode)                 \
        list(_dst, _src, int, td_s16 , min_gain_limit)           \
        list(_dst, _src, int, td_s16 , snr_prior_limit)          \
        list(_dst, _src, int, td_s16 , ht_threshold)             \
        list(_dst, _src, int, td_s16 , hs_threshold)             \
        list(_dst, _src, int, td_s16 , alpha_ph)                 \
        list(_dst, _src, int, td_s16 , alpha_psd)                \
        list(_dst, _src, int, td_s16 , prior_snr_fixed)          \
        list(_dst, _src, int, td_s16 , cep_threshold)            \
        list(_dst, _src, int, td_s16 , cep_amp)                  \
        list(_dst, _src, int, td_s16 , low_freq_protect)         \
        list(_dst, _src, int, td_s16 , speech_protect_threshold) \
        list(_dst, _src, int, td_s16 , hem_enable)               \
        list(_dst, _src, int, td_s16 , tcs_enable)

#define NR_MEMB_LIST(_dst, _src, list)                           \
        list(_dst, _src, int, td_bool, usr_mode)                 \
        list(_dst, _src, int, td_s16 , min_gain_limit)           \
        list(_dst, _src, int, td_s16 , snr_prior_limit)          \
        list(_dst, _src, int, td_s16 , ht_threshold)             \
        list(_dst, _src, int, td_s16 , hs_threshold)             \
        list(_dst, _src, int, td_s16 , prior_snr)                \
        list(_dst, _src, int, td_s16 , snr_smooth_factor)        \
        list(_dst, _src, int, td_s16 , speech_prob_smooth_factor)\
        list(_dst, _src, int, td_s16 , noise_pwr_smooth_factor)  \
        list(_dst, _src, int, td_s8  , low_freq_suppress_enable) \
        list(_dst, _src, int, td_s8  , low_freq_gain_suppress)   \
        list(_dst, _src, int, td_s16 , env_mode)                 \
        list(_dst, _src, int, td_s16 , cep_alpha)                \
        list(_dst, _src, int, td_s16 , cep_threshold)            \
        list(_dst, _src, int, td_s16 , cep_amp)                  \
        list(_dst, _src, int, td_s8  , gain_sm_mode)             \
        list(_dst, _src, int, td_s8  , gain_sm_alpha1)           \
        list(_dst, _src, int, td_s8  , gain_sm_alpha2)           \
        list(_dst, _src, int, td_s8  , gain_sm_alpha3)

#define AGC_MEMB_LIST(_dst, _src, list)                          \
        list(_dst, _src, int, td_bool, usr_mode)                 \
        list(_dst, _src, int, td_s16 , target_level)             \
        list(_dst, _src, int, td_s16 , max_gain)                 \
        list(_dst, _src, int, td_s16 , min_gain)                 \
        list(_dst, _src, int, td_s16 , up_gradient_ratio)        \
        list(_dst, _src, int, td_s16 , down_gradient_ratio)      \
        list(_dst, _src, int, td_s16 , decay)                    \
        list(_dst, _src, int, td_s32 , vad_threshold)            \
        list(_dst, _src, int, td_s16 , vad_ctrl)

#define FMP_MEMB_LIST(_dst, _src, list)                          \
        list(_dst, _src, int, td_bool, usr_mode)                 \
        list(_dst, _src, int, td_s8  , comfort_flag)             \
        list(_dst, _src, int, td_s8  , comfort_intensity)

#define AEC_MEMB_LIST(_dst, _src, list)                          \
        list(_dst, _src, int, td_bool, usr_mode)                 \
        list(_dst, _src, int, td_u16 , pure_delay)               \
        list(_dst, _src, int, td_u16 , switch_nlp)               \
        list(_dst, _src, int, td_u16 , band1)                    \
        list(_dst, _src, int, td_u16 , band2)                    \
        list(_dst, _src, int, td_u16 , band3)                    \
        list(_dst, _src, int, td_u16 , band4)                    \
        list(_dst, _src, int, td_u16 , gain_lower_limit1)        \
        list(_dst, _src, int, td_u16 , gain_lower_limit2)        \
        list(_dst, _src, int, td_u16 , gain_lower_limit3)        \
        list(_dst, _src, int, td_u16 , gain_lower_limit4)        \
        list(_dst, _src, int, td_u16 , gain_lower_limit5)        \
        list(_dst, _src, int, td_u16 , ols_on)                   \
        list(_dst, _src, int, td_u16 , speaker_nl_on)            \
        list(_dst, _src, int, td_u16 , block_num)                \
        list(_dst, _src, int, td_u16 , echo_boost1)              \
        list(_dst, _src, int, td_u16 , echo_boost2)              \
        list(_dst, _src, int, td_u16 , echo_boost3)              \
        list(_dst, _src, int, td_u16 , echo_boost4)              \
        list(_dst, _src, int, td_u16 , echo_boost5)

#define WNR_MEMB_LIST(_dst, _src, list)                          \
        list(_dst, _src, int, td_bool, usr_mode)                 \
        list(_dst, _src, int, td_s8  , min_gain_limit)

#define HS_MEMB_LIST(_dst, _src, list)                           \
        list(_dst, _src, int, td_bool, usr_mode)                 \
        list(_dst, _src, int, td_s32 , hold_time)                \
        list(_dst, _src, int, td_s32 , min_gain)                 \
        list(_dst, _src, int, td_s32 , threshold)                \
        list(_dst, _src, int, td_s32 , smooth_time)              \
        list(_dst, _src, int, td_s32 , freq_move)

typedef struct {
    int enable;
    PNR_MEMB_LIST(NULL, NULL, DECL_BIGGER);
} sVqeV2PnrCfg;

typedef struct {
    int enable;
    NR_MEMB_LIST(NULL, NULL, DECL_BIGGER);
} sVqeV2NrCfg;

typedef struct {
    int enable;
    AGC_MEMB_LIST(NULL, NULL, DECL_BIGGER);
} sVqeV2AgcCfg;

typedef struct {
    int enable;
    int usr_mode;
    int gain_db[OT_TALKVQEV2_EQ_BAND_NUM];
} sVqeV2EqCfg;

typedef struct {
    int enable;
    FMP_MEMB_LIST(NULL, NULL, DECL_BIGGER);
} sVqeV2FmpCfg;

typedef struct {
    int enable;
    AEC_MEMB_LIST(NULL, NULL, DECL_BIGGER);
} sVqeV2AecCfg;

typedef struct {
    int enable;
    WNR_MEMB_LIST(NULL, NULL, DECL_BIGGER);
} sVqeV2WnrCfg;

typedef struct {
    int enable;
    HS_MEMB_LIST(NULL, NULL, DECL_BIGGER);
} sVqeV2HsCfg;

typedef struct {
    int          open_mask;
    sVqeV2PnrCfg pnr_cfg;
    sVqeV2NrCfg  nr_cfg;
    sVqeV2AgcCfg agc_cfg;
    sVqeV2EqCfg  eq_cfg;
    sVqeV2FmpCfg fmp_cfg;
    sVqeV2AecCfg aec_cfg;
    sVqeV2WnrCfg wnr_cfg;
    sVqeV2HsCfg  hs_cfg;
} sAiVqeV2Cfg;

typedef struct {
    int   agc_enable;
    float agc_level;
    int   agc_max_gain;
    int   agc_increment;
    int   agc_decrement;
    int   nr_enable;
    int   nr_decrement;
    int   aec_enable;
    int   aec_filter_len;
    int   aec_suppress;
    int   aec_suppress_active;
} sAiSpeexCfg;

int encode_audio_in_init(void);

int encode_audio_in_uninit(void);

void encode_audio_in_set_aec(int enable);

ot_aio_attr *fet_ai_attr(ot_aio_attr *iattr);

#ifdef __cplusplus
}
#endif
#endif
