/*
 *       Filename:  aliyun_mmi_cfg.h
 *    Description:  阿里云百炼 MMI 配置管理
 *        Version:  1.0
 *        Created:  07/10/2026 10:31:53 AM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  xiangyp (), 
 *   Organization:  
 */

#ifndef __ALIYUN_MMI_CFG_H__
#define __ALIYUN_MMI_CFG_H__

#ifdef __cplusplus
extern "C" {
#endif

#define MMI_HOSTING_FULL
//#define MMI_HOSTING_SEMI
#define MMI_PREPAID_MODE
//#define MMI_POSTPAID_MODE

#define BYTES_MMI_TRIPLE        (1024)
#define BYTES_MMI_BUF           (64 * 1024)

#ifdef MMI_HOSTING_FULL
    #define CNT_TRIPLE_KEY_MEMBS            (5)
    #define FMT_SCANF_TRIPLE_KEY(_key)      \
            "%[^;];%[^;];%[^;];%[^;];%[^;]",\
            (_key)->app_id,                 \
            (_key)->app_secret,             \
            (_key)->device_name,            \
            (_key)->api_key,                \
            (_key)->workspace_id
    
    #define FMT_SNPRINTF_TRIPLE_KEY(_key)   \
            "%s;%s;%s;%s;%s",               \
            (_key)->app_id,                 \
            (_key)->app_secret,             \
            (_key)->device_name,            \
            (_key)->api_key,                \
            (_key)->workspace_id
#else
    #define CNT_TRIPLE_KEY_MEMBS            (4)
    #define FMT_SCANF_TRIPLE_KEY(_key)      \
            "%[^;];%[^;];%[^;];%[^;]",      \
            (_key)->app_id,                 \
            (_key)->app_secret,             \
            (_key)->device_name,            \
            (_key)->workspace_id

    #define FMT_SNPRINTF_TRIPLE_KEY(_key)   \
            "%s;%s;%s;%s",                  \
            (_key)->app_id,                 \
            (_key)->app_secret,             \
            (_key)->device_name,            \
            (_key)->workspace_id
#endif

typedef struct {
    char app_id[128];
    char app_secret[128];
    char device_name[128];
#ifdef MMI_HOSTING_FULL
    char api_key[256];
#endif
    char workspace_id[128];
} sMmiTripleKey;

#define C_MMI_CONFIG_CUSTOM()                   \
{                                               \
    .evt_cb = NULL,                             \
    .work_mode = C_MMI_MODE_DUPLEX,             \
    .text_mode = C_MMI_TEXT_MODE_BOTH,          \
    .incremental_response = 0,                  \
    .response_text = 0,                         \
    .voice_id = "aimei",                        \
    .story_voice_id = "aimei",                  \
    .upstream_mode = C_MMI_STREAM_MODE_PCM,     \
    .downstream_mode = C_MMI_STREAM_MODE_PCM,   \
    .recorder_rb_size = BYTES_MMI_BUF,          \
    .player_rb_size = BYTES_MMI_BUF,            \
    .transmit_rate_limit = 0,                   \
    .enable_cbr = 0,                            \
    .frame_size = 60,                           \
    .bit_rate = 32,                             \
    .us_sample_rate = 16000,                    \
    .ds_sample_rate = 16000,                    \
    .vocabulary_id = NULL,                      \
    .volume = 50,                               \
    .speech_rate = 100,                         \
    .pitch_rate = 100,                          \
    .user_id = NULL,                            \
}

int aliyun_mmi_load_triple_key(sMmiTripleKey *p_key);
int aliyun_mmi_dump_triple_key(sMmiTripleKey *p_key);

#ifdef __cplusplus
}
#endif
#endif /* __ALIYUN_MMI_CFG_H__ */
