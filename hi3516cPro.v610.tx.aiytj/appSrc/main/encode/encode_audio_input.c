/*
 *       Filename:  encode_audio_input.c
 *    Description:
 *        Version:  1.0
 *        Created:  10/29/2022 03:46:54 PM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  xiangyp (),
 *   Organization:
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include "ot_type.h"
#include "ot_acodec.h"
#include "ot_common_aio.h"
#include "ss_mpi_sys_bind.h"
#include "ss_mpi_audio.h"
#include "securec.h"

#include "g_log.h"
#include "g_run.h"
#include "debug.h"
#include "utils.h"
#include "jconfig.h"
#include "conf_list.h"
#include "confapi.h"
#include "shm_buf.h"
#include "shm_buf_pool.h"
#include "cmdstat.h"
#include "js_scheduler.h"
#include "encodeapi.h"
#include "net_qrcode.h"
#include "encode_common.h"
#include "encode_audio_output.h"
#include "encode_audio_input.h"
#include "encode_bind.h"
#include "encode_aac.h"
#include "speex_preprocess.h"
#include "speex_resampler.h"
#include "speex_echo.h"
#include "system_sch.h"
#include "encode_videomask.h"
#include "encode_audio.h"

#define AUDIO_IN_HEAD_LEN           (4)

#define START_DELAY                 (40)
#define AUDIO_IN_LOOP_INTERVAL      (20)
#define AUDIO_IN_TALK_DEVIATION     (22)

#define AUDIO_IN_ACODEC_FILE        "/dev/acodec"

struct audio_in_run{
    JSScheduler g_sch_audio;
    JSTCHandle  g_hdl_audioin;
    JSRWHandle  g_rw_audioin;
    struct cmdstat *p_ctx;
    SpeexPreprocessState *speex_state;
    SpeexEchoState *speex_aec;
    int aec_enabled;
};

struct audio_in_cfg{
    sAiVqeV2Cfg vqev2cfg;
    sAiSpeexCfg speexcfg;
    AudioCfgS audiocfg;
    ot_aio_attr iattr;
    DevConfS devconf;
};

enum {
    CMD_AI_CFG              = 1 << 0,
    CMD_AUDIO_IN_CFG        = 1 << 1,
    CMD_AUDIO_IN_ENABLE     = 1 << 2,
    CMD_ALI_CFG             = 1 << 3,
    CMD_DEV_CFG             = 1 << 4,
    CMD_VQEV2_CFG           = 1 << 5,
    CMD_SPEEX_CFG           = 1 << 6,
};

typedef enum {
    AUDIO_VQE_TYPE_NONE = 0,
    AUDIO_VQE_TYPE_RECORD,
    AUDIO_VQE_TYPE_TALK,
    AUDIO_VQE_TYPE_TALKV2,
    AUDIO_VQE_TYPE_MAX,
} audio_vqe_type;

typedef struct {
    char name[16];
    int enable;
    int mask;
    void *value;
} sEnb2Mask;

extern aac_enc_info_t aac_enc_info;

static struct audio_in_cfg cfg = {0};
static struct audio_in_cfg raw = {0};
static struct audio_in_run run = {0};
static struct audio_in_cfg *g_cfg_ai = &cfg;
static struct audio_in_cfg *g_raw_ai = &raw;
static struct audio_in_run *g_run_ai = &run;

static void audio_in_speex_uninit(void);

static void cb_audioincfg_sync(int id, void *p_src, int size, void *ctx)
{
    CPY2CMDCFG(CMD_AI_CFG, &g_raw_ai->audiocfg, p_src, size);
}

static void cb_aivqev2cfg_sync(int id, void *p_src, int size, void *ctx)
{
    CPY2CMDCFG(CMD_VQEV2_CFG, &g_raw_ai->vqev2cfg, p_src, size);
}

static void cb_aispeexcfg_sync(int id, void *p_src, int size, void *ctx)
{
    CPY2CMDCFG(CMD_SPEEX_CFG, &g_raw_ai->speexcfg, p_src, size);
}

static void cb_devicebind_sync(int id, void *p_src, int size, void *ctx)
{
    CPY2CMDCFG(CMD_ALI_CFG, &g_raw_ai->devconf, p_src, size);
}

static void encode_ai_trans2negative_vqev2_param(sAiVqeV2Cfg *p_cfg)
{
    p_cfg->agc_cfg.target_level -= 120;
    p_cfg->agc_cfg.target_level = RANGE(p_cfg->agc_cfg.target_level, -120, 0);

    p_cfg->agc_cfg.max_gain -= 120;
    p_cfg->agc_cfg.max_gain = RANGE(p_cfg->agc_cfg.max_gain, -120, 240);

    p_cfg->agc_cfg.min_gain -= 120;
    p_cfg->agc_cfg.min_gain = RANGE(p_cfg->agc_cfg.min_gain, -120, 0);

    p_cfg->agc_cfg.decay -= 650;
    p_cfg->agc_cfg.decay = RANGE(p_cfg->agc_cfg.decay, -650, 0);

    DBG("agc target_level: %d, max_gain: %d, min_gain: %d, decay: %d\n",
        p_cfg->agc_cfg.target_level, p_cfg->agc_cfg.max_gain,
        p_cfg->agc_cfg.min_gain, p_cfg->agc_cfg.decay);
}

static void encode_ai_sync_vqev2_mask(sAiVqeV2Cfg *p_cfg)
{
    char status_msg[128] = {0};
    int offset = 0;

    sEnb2Mask v2_maps[] = {
        {"PNR", p_cfg->pnr_cfg.enable, OT_AI_TALKVQEV2_MASK_PNR, NULL},
        {"NR" , p_cfg->nr_cfg.enable,  OT_AI_TALKVQEV2_MASK_NR, NULL},
        {"AGC", p_cfg->agc_cfg.enable, OT_AI_TALKVQEV2_MASK_AGC, NULL},
        {"FMP", p_cfg->fmp_cfg.enable, OT_AI_TALKVQEV2_MASK_FMP, NULL},
        {"AEC", p_cfg->aec_cfg.enable |
                g_run_ai->aec_enabled, OT_AI_TALKVQEV2_MASK_AEC, NULL},
        {"WNR", p_cfg->wnr_cfg.enable, OT_AI_TALKVQEV2_MASK_WNR, NULL},
        {"HS" , p_cfg->hs_cfg.enable,  OT_AI_TALKVQEV2_MASK_HS, NULL}
    };

    offset += snprintf(status_msg, sizeof(status_msg) - 1, "AI VQEV2 status");
    for (int idx = 0; idx < ARRAY_SIZE(v2_maps); idx++) {
        if (v2_maps[idx].enable) {
            p_cfg->open_mask |= v2_maps[idx].mask;
        } else {
            p_cfg->open_mask &= (~v2_maps[idx].mask);
        }

        offset += snprintf(status_msg + offset, sizeof(status_msg) - offset - 1,
                           ", %s: %d", v2_maps[idx].name, v2_maps[idx].enable);
    }

    DBG("%s\n", status_msg);
}

static int audio_in_reset_shmbuf_from_conf(void)
{
    int ret = 0, audio_input_type = 0;

    if (AudioFormatE_G711A == g_cfg_ai->audiocfg.codetype) {
        audio_input_type = SHM_MEDIA_AUDIO_ALAW;
    } else {
        audio_input_type = SHM_MEDIA_AUDIO_ULAW;
    }

    reset_shm_buf_pool(SHM_BUF_AUDIO);
    reset_shm_buf_pool(SHM_BUF_AUDIO_AAC);
    /* 适配声波配网,16k转8k */
    //if (g_cfg_ai->iattr.sample_rate == OT_AUDIO_SAMPLE_RATE_16000) {
    //    ret = shm_buf_set_media_info(get_shm_buf_pool(SHM_BUF_AUDIO),
    //                                 audio_input_type,
    //                                 g_cfg_ai->iattr.sample_rate/2,
    //                                 g_cfg_ai->iattr.bit_width, 0, 0);
    //} else {
        ret = shm_buf_set_media_info(get_shm_buf_pool(SHM_BUF_AUDIO),
                                     audio_input_type,
                                     g_cfg_ai->iattr.sample_rate,
                                     g_cfg_ai->iattr.bit_width, 0, 0);
    //}

    ret = shm_buf_set_media_info(get_shm_buf_pool(SHM_BUF_AUDIO_AAC), SHM_MEDIA_AUDIO_AAC,
                   g_cfg_ai->iattr.sample_rate, g_cfg_ai->iattr.bit_width, 0, 0);

    return ret;
}

static td_void ai_init_record_vqe_param(ot_ai_record_vqe_cfg *ai_vqe_record_attr)
{
    ai_vqe_record_attr->work_sample_rate = g_cfg_ai->iattr.sample_rate;
    ai_vqe_record_attr->frame_sample = AUDIO_IN_PERFRM;
    ai_vqe_record_attr->work_state = OT_VQE_WORK_STATE_COMMON;
    ai_vqe_record_attr->in_chn_num = 1;
    ai_vqe_record_attr->out_chn_num = 1;
    ai_vqe_record_attr->record_type = OT_VQE_RECORD_NORMAL;
    ai_vqe_record_attr->drc_cfg.usr_mode = TD_FALSE;
    ai_vqe_record_attr->rnr_cfg.usr_mode = TD_FALSE;
    ai_vqe_record_attr->hdr_cfg.usr_mode = TD_FALSE;
    ai_vqe_record_attr->hpf_cfg.usr_mode = TD_TRUE;
    ai_vqe_record_attr->hpf_cfg.hpf_freq = OT_AUDIO_HPF_FREQ_80;

    ai_vqe_record_attr->open_mask =
        OT_AI_RECORDVQE_MASK_DRC | OT_AI_RECORDVQE_MASK_HDR | OT_AI_RECORDVQE_MASK_HPF | OT_AI_RECORDVQE_MASK_RNR;
}

static td_void ai_init_talk_vqe_param(ot_ai_talk_vqe_cfg *ai_vqe_talk_attr)
{
    ai_vqe_talk_attr->work_sample_rate = g_cfg_ai->iattr.sample_rate;
    ai_vqe_talk_attr->frame_sample = AUDIO_IN_PERFRM;
    ai_vqe_talk_attr->work_state = OT_VQE_WORK_STATE_COMMON;
    ai_vqe_talk_attr->aec_cfg.usr_mode = TD_FALSE;
    ai_vqe_talk_attr->anr_cfg.usr_mode = TD_FALSE;
    ai_vqe_talk_attr->hpf_cfg.usr_mode = TD_TRUE;
    ai_vqe_talk_attr->hpf_cfg.hpf_freq = OT_AUDIO_HPF_FREQ_150;

    ai_vqe_talk_attr->agc_cfg.usr_mode = TD_TRUE;
    ai_vqe_talk_attr->agc_cfg.target_level = -2;
    ai_vqe_talk_attr->agc_cfg.noise_floor = -40;
    ai_vqe_talk_attr->agc_cfg.max_gain = 30;
    ai_vqe_talk_attr->agc_cfg.adjust_speed = 10;
    ai_vqe_talk_attr->agc_cfg.improve_snr = 2;
    ai_vqe_talk_attr->agc_cfg.use_hpf = 0;
    ai_vqe_talk_attr->agc_cfg.output_mode = 0;
    ai_vqe_talk_attr->agc_cfg.noise_suppress_switch = 1;

    ai_vqe_talk_attr->open_mask = OT_AI_TALKVQE_MASK_AEC | OT_AI_TALKVQE_MASK_AGC | OT_AI_TALKVQE_MASK_ANR | OT_AI_TALKVQE_MASK_HPF;
}

static td_void ai_init_talk_vqe_v2_param(ot_ai_talk_vqe_v2_cfg *ai_vqe_talk_v2_attr)
{
    ai_vqe_talk_v2_attr->work_sample_rate = g_cfg_ai->iattr.sample_rate;
    ai_vqe_talk_v2_attr->frame_sample = AUDIO_IN_PERFRM;
    ai_vqe_talk_v2_attr->work_state = OT_VQE_WORK_STATE_COMMON;
    ai_vqe_talk_v2_attr->in_chn_num = 1;
    ai_vqe_talk_v2_attr->out_chn_num = 1;

    if (0 == g_cfg_ai->devconf.devicebind) {
        ai_vqe_talk_v2_attr->open_mask = OT_AI_TALKVQEV2_MASK_AGC | OT_AI_TALKVQEV2_MASK_FMP |
                                         OT_AI_TALKVQEV2_MASK_LIMITER  | OT_AI_TALKVQEV2_MASK_WNR;
    } else {
        ai_vqe_talk_v2_attr->open_mask |= OT_AI_TALKVQEV2_MASK_LIMITER;
    }
}

static void encode_ai_sync_vqev2cfg(ot_ai_talk_vqe_v2_cfg *p_vqev2,
                                    sAiVqeV2Cfg *p_vqecfg)
{
    PNR_MEMB_LIST(&p_vqev2->pnr_cfg, &p_vqecfg->pnr_cfg, COPY_BIG2SMALL);
    NR_MEMB_LIST(&p_vqev2->nr_cfg, &p_vqecfg->nr_cfg, COPY_BIG2SMALL);
    AGC_MEMB_LIST(&p_vqev2->agc_cfg, &p_vqecfg->agc_cfg, COPY_BIG2SMALL);
    FMP_MEMB_LIST(&p_vqev2->fmp_cfg, &p_vqecfg->fmp_cfg, COPY_BIG2SMALL);
    AEC_MEMB_LIST(&p_vqev2->aec_cfg, &p_vqecfg->aec_cfg, COPY_BIG2SMALL);
    WNR_MEMB_LIST(&p_vqev2->wnr_cfg, &p_vqecfg->wnr_cfg, COPY_BIG2SMALL);
    HS_MEMB_LIST(&p_vqev2->hs_cfg, &p_vqecfg->hs_cfg, COPY_BIG2SMALL);
    p_vqev2->open_mask = p_vqecfg->open_mask;
}

static td_s32 encode_audio_start_ai_vqe(audio_vqe_type vqe_type)
{
    td_s32 ret = TD_SUCCESS;

    ret = ss_mpi_ai_disable_vqe(AUDIO_IN_DEV_ID, AUDIO_IN_CHNID);
    ENCODE_RET_CHECK(ret, "ss_mpi_ai_disable_vqe failed!\n");

    switch (vqe_type) {
        case AUDIO_VQE_TYPE_NONE: {
            ret = TD_SUCCESS;
            break;
        }

        case AUDIO_VQE_TYPE_RECORD: {
            ot_ai_record_vqe_cfg ai_record_vqe= {0};
            ai_init_record_vqe_param(&ai_record_vqe);
            ret = ss_mpi_ai_set_record_vqe_attr(AUDIO_IN_DEV_ID, AUDIO_IN_CHNID, &ai_record_vqe);
            break;
        }

        case AUDIO_VQE_TYPE_TALK:{
            ot_ai_talk_vqe_cfg ai_talk_vqe = {0};
            ai_init_talk_vqe_param(&ai_talk_vqe);
            ret = ss_mpi_ai_set_talk_vqe_attr(AUDIO_IN_DEV_ID, AUDIO_IN_CHNID, AUDIO_OUT_DEV_ID, AUDIO_OUT_CHN_ID, &ai_talk_vqe);
            break;
        }

        case AUDIO_VQE_TYPE_TALKV2: {
            ot_ai_talk_vqe_v2_cfg ai_vqev2 = {0};
            encode_ai_sync_vqev2cfg(&ai_vqev2, &g_cfg_ai->vqev2cfg);
            ai_init_talk_vqe_v2_param(&ai_vqev2);
            DBG("vqev2 synced open_mask: 0x%08x\n", ai_vqev2.open_mask);
            ret = ss_mpi_ai_set_talk_vqe_v2_attr(AUDIO_IN_DEV_ID, AUDIO_IN_CHNID,
                                                 AUDIO_OUT_DEV_ID, AUDIO_OUT_CHN_ID,
                                                 &ai_vqev2);
            break;
        }

        default: {
            ret = TD_FAILURE;
            break;
        }
    }

    ret = ss_mpi_ai_enable_vqe(AUDIO_IN_DEV_ID, AUDIO_IN_CHNID);
    ENCODE_RET_CHECK(ret, "ss_mpi_ai_enable_vqe failed!\n");

    return ret;
}

static int dyn_write_mic_pcm(void *buf, int len)
{
    static FILE *fp = NULL;

    const char *filepath = is_okey("/opt/long") ? "/mnt/mic_ai.pcm" : "/tmp/mic_ai.pcm";
    int start = get_g_run(audio, RUN_AUDIOIN_SAVE);

    if (start) {
        if (!fp) {
            fp = fopen(filepath, "w");
            DBG("write %s start\n", filepath);
        }
        fwrite(buf, len, 1, fp);
    } else {
        if (fp) {
            fsync(fileno(fp));
            fclose(fp);
            fp = NULL;
            DBG("write %s stop\n", filepath);
        }
    }

    return 0;
}

static int dyn_write_mic_speex_pcm(void *buf, int len)
{
    static FILE *fp = NULL;

    const char *filepath = is_okey("/opt/long") ? "/mnt/mic_speex.pcm" : "/tmp/mic_speex.pcm";
    int start = get_g_run(audio, RUN_AUDIOIN_SAVE);

    if (start) {
        if (!fp) {
            fp = fopen(filepath, "w");
            DBG("write %s start\n", filepath);
        }
        fwrite(buf, len, 1, fp);
    } else {
        if (fp) {
            fsync(fileno(fp));
            fclose(fp);
            fp = NULL;
            DBG("write %s stop\n", filepath);
        }
    }

    return 0;
}

static int dyn_write_ref_pcm(void *buf, int len)
{
    static FILE *fp = NULL;

    const char *filepath = is_okey("/opt/long") ? "/mnt/mic_ref.pcm" : "/tmp/mic_ref.pcm";
    int start = get_g_run(audio, RUN_AUDIOIN_SAVE);

    if (start) {
        if (!fp) {
            fp = fopen(filepath, "w");
            DBG("write %s start\n", filepath);
        }
        fwrite(buf, len, 1, fp);
    } else {
        if (fp) {
            fsync(fileno(fp));
            fclose(fp);
            fp = NULL;
            DBG("write %s stop\n", filepath);
        }
    }

    return 0;
}

/*
 * aac 统一按 16K sampleRate 处理
 * pcm 当前 jz 底层 fps 是 12.5(当前测试出帧间隔不是严格80ms)
 * pcm 归一处理(8K -> 16K sampleRate 且 12.5 fps)，每次出 1280 sample
 * aac 编码最小 sample 数是 1024
 *
 * 因此，每 4 帧 pcm，可以出 5 帧 aac，即 1280 * 4 = 1024 * 5 = 5120
 *
 * 引入 AAC_FRM_DURATION_16K 宏，即 Sec Per Frame = 0.064S = 64ms
 * 同理 PCM_FRM_DURATION_16K 宏，即 Milli-Sec Per Frame = 80ms = 1280/16000
 */
int write_aac_shm_buf(ot_audio_frame audio_frame)
{
    int ret = SUCCESS;
    int aac_len = 0;
    unsigned char audio_aac_buf[AAC_MAX_LEN*4] = {0};
    shm_buf_t pBuf = get_shm_buf_pool(SHM_BUF_AUDIO_AAC);
    static int audio_len = 0;
    static unsigned char audio_buf[PCM_SMPL_PER_FRM_16K*3] = {0};

    memcpy_s(audio_buf + audio_len, PCM_SMPL_PER_FRM_16K*3-audio_len, audio_frame.virt_addr[0], audio_frame.len);
    audio_len += audio_frame.len;
    do {
        if(audio_len < aac_enc_info.input_size) {
            break;
        }

        aac_len = faacEncEncode(aac_enc_info.enc_handle, (signed int*)audio_buf,
                                aac_enc_info.inputSamples, audio_aac_buf, aac_enc_info.maxOutputBytes);
        //ERR("frame_len_16:%d i:%d aac_len:%d inputSamples:%d maxOutputBytes:%d \r\n",frame_len_16,i,aac_len,aac_enc_info.inputSamples,aac_enc_info.maxOutputBytes);
        if (aac_len < 0){
            ERR("faacEncEncode failed\r\n");
            break;
        }

        memmove_s(audio_buf, PCM_SMPL_PER_FRM_16K*3,  audio_buf+aac_enc_info.input_size, PCM_SMPL_PER_FRM_16K*3-aac_enc_info.input_size);
        audio_len -= aac_enc_info.input_size;

        aac_enc_info.timeStamp = audio_frame.time_stamp/1000000.0;

        if (aac_len > 0) {
            ret = shm_buf_write_frame(pBuf, (char *)audio_aac_buf, aac_len,
                                      SHM_FRAEM_AUDIO, aac_enc_info.timeStamp);

            break_if_fail(SUCCESS == ret, ret);
        }
    } while (0);

    return ret;
}

static void audio_in_process(int fd, int ev, void *instances)
{
    int ret = SUCCESS, is_get_stream = FALSE, is_get_aiframe = FALSE;
    ot_audio_frame ai_frame = {0};
    ot_aec_frame aec_frame = {0};
    ot_audio_stream audio_stream = {0};
    //unsigned char audio_stream_8k[AUDIO_IN_NUM_PERFRM+1] = {0};

    static int audio_len = 0;
    static unsigned char audio_buf[AUDIO_IN_PERFRM] = {0};
    static int audio_ggwave_len = 0;
    static unsigned char audio_ggwave_buf[AUDIO_IN_NUM_PERFRM*4+1] = {0};
    static unsigned char speex_aec_buf[AUDIO_IN_NUM_PERFRM * 2] = {0};

    do {
        ret = ss_mpi_ai_get_frame(AUDIO_IN_DEV_ID, AUDIO_IN_CHNID, &ai_frame, &aec_frame, 10);
        if (TD_SUCCESS != ret) {
            //ERR("ss_mpi_aenc_get_stream failed\n");
            break;
        }
        is_get_aiframe = TRUE;

        if (videomask_enabled()) {
            memset(ai_frame.virt_addr[0], 0, ai_frame.len);
            memset(aec_frame.ref_frame.virt_addr[0], 0, aec_frame.ref_frame.len);
        }

        dyn_write_mic_pcm(ai_frame.virt_addr[0], ai_frame.len);

        if (NULL != g_run_ai->speex_aec && NULL != aec_frame.ref_frame.virt_addr[0]) {
            speex_echo_cancellation(g_run_ai->speex_aec,
                                    (const spx_int16_t *)ai_frame.virt_addr[0],
                                    (const spx_int16_t *)aec_frame.ref_frame.virt_addr[0],
                                    (spx_int16_t *)speex_aec_buf);
            memcpy(ai_frame.virt_addr[0], speex_aec_buf, ai_frame.len);
        }

        if (NULL != g_run_ai->speex_state) {
            speex_preprocess_run(g_run_ai->speex_state, (spx_int16_t *)ai_frame.virt_addr[0]); 
        }

        amplify_pcm_volume(ai_frame.virt_addr[0], ai_frame.len,
                           g_cfg_ai->iattr.bit_width, g_cfg_ai->audiocfg.inamp);

        dyn_write_mic_speex_pcm(ai_frame.virt_addr[0], ai_frame.len);
        dyn_write_ref_pcm(aec_frame.ref_frame.virt_addr[0], aec_frame.ref_frame.len);

        write_aac_shm_buf(ai_frame);

        if(0 == g_cfg_ai->devconf.devicebind && AUDIO_IN_NUM_PERFRM == ai_frame.len) {
            memcpy_s(audio_ggwave_buf + audio_ggwave_len, AUDIO_IN_NUM_PERFRM * 4 - audio_ggwave_len,
                     ai_frame.virt_addr[0], ai_frame.len);
            audio_ggwave_len += ai_frame.len;
            if(audio_ggwave_len == AUDIO_IN_NUM_PERFRM*4) {
                ggwave_push_audio((uint8_t *)audio_ggwave_buf, audio_ggwave_len);
                memset_s(audio_ggwave_buf, sizeof(audio_ggwave_buf), 0, sizeof(audio_ggwave_buf));
                audio_ggwave_len = 0;
            }
        }

        ret = ss_mpi_aenc_send_frame(AUDIO_IN_AENC_CHN, &ai_frame, &aec_frame);
        if (TD_SUCCESS != ret) {
            ERR("ss_mpi_aenc_send_frame failed ret: 0x%x\n", ret);
            break;
        }

        ret = ss_mpi_aenc_get_stream(AUDIO_IN_AENC_CHN, &audio_stream, 10);
        if (TD_SUCCESS != ret) {
            ERR("ss_mpi_aenc_get_stream failed ret: 0x%x\n", ret);
            break;
        }

        is_get_stream = TRUE;
        audio_stream.time_stamp = ai_frame.time_stamp;

        double ts_sec = audio_stream.time_stamp/1000000.0;
        shm_buf_t pBuf = get_shm_buf_pool(SHM_BUF_AUDIO);
        //if (g_cfg_ai->iattr.sample_rate == OT_AUDIO_SAMPLE_RATE_16000) {
        //    int i = 0;
        //    for (int j = AUDIO_IN_HEAD_LEN; j < audio_stream.len; j+=2) {
        //        audio_stream_8k[i++] = audio_stream.stream[j];
        //    }
        //    audio_stream_8k[i] = '\0';

        //    memcpy_s(audio_buf + audio_len, sizeof(audio_buf)-audio_len, audio_stream_8k, (audio_stream.len-AUDIO_IN_HEAD_LEN)/2);
        //    audio_len += (audio_stream.len-AUDIO_IN_HEAD_LEN)/2;
        //} else {
            memcpy_s(audio_buf + audio_len, sizeof(audio_buf) - audio_len,
                     audio_stream.stream + AUDIO_IN_HEAD_LEN,
                     audio_stream.len - AUDIO_IN_HEAD_LEN);
            audio_len += (audio_stream.len - AUDIO_IN_HEAD_LEN);
        //}

        pri_audio(LVL_LOOP, "audio a/ulaw perfrm size: %d\n", audio_stream.len);
        if (audio_len == sizeof(audio_buf)) {
            ret = shm_buf_write_frame(pBuf, (char *)audio_buf,
                                      sizeof(audio_buf), SHM_FRAEM_AUDIO, ts_sec);
            memset_s(audio_buf, sizeof(audio_buf), 0, sizeof(audio_buf));
            audio_len = 0;
        }
    } while(0);

    if (TRUE == is_get_stream) {
        ret = ss_mpi_aenc_release_stream(AUDIO_IN_AENC_CHN, &audio_stream);
        if (TD_SUCCESS != ret) {
            ERR("ss_mpi_aenc_release_stream failed\n");
        }
    }

    if(is_get_aiframe) {
        ret = ss_mpi_ai_release_frame(AUDIO_IN_DEV_ID, AUDIO_IN_CHNID, &ai_frame, &aec_frame);
        if (TD_SUCCESS != ret) {
            ERR("ss_mpi_aenc_release_stream failed\n");
        }

    }
}

static int encode_audio_in_start(void)
{
    td_s32 ret = TD_SUCCESS;
    td_s32 aenc_fd = -1;
    do {
        aenc_fd = ss_mpi_ai_get_fd(AUDIO_IN_DEV_ID, AUDIO_IN_CHNID);
        if (aenc_fd < 0) {
            ret = TD_FAILURE;
            ERR("get aenc fd failed\n");
            break;
        }

        js_create_reader_r(g_run_ai->g_sch_audio, aenc_fd, JS_READABLE, audio_in_process, NULL, &g_run_ai->g_rw_audioin);
    } while(0);

    return ret;
}

static int encode_audio_in_stop(void)
{
    if (NULL != g_run_ai->g_rw_audioin) {
        js_delete_reader_r(&g_run_ai->g_rw_audioin);
        g_run_ai->g_rw_audioin = NULL;
    }

    return 0;
}

static td_s32 inner_codec_get_i2s_fs(ot_audio_sample_rate sample_rate, ot_acodec_fs *i2s_fs)
{
    ot_acodec_fs i2s_fs_sel;

    switch (sample_rate) {
        case OT_AUDIO_SAMPLE_RATE_8000:
            i2s_fs_sel = OT_ACODEC_FS_8000;
            break;

        case OT_AUDIO_SAMPLE_RATE_16000:
            i2s_fs_sel = OT_ACODEC_FS_16000;
            break;

        case OT_AUDIO_SAMPLE_RATE_32000:
            i2s_fs_sel = OT_ACODEC_FS_32000;
            break;

        case OT_AUDIO_SAMPLE_RATE_11025:
            i2s_fs_sel = OT_ACODEC_FS_11025;
            break;

        case OT_AUDIO_SAMPLE_RATE_22050:
            i2s_fs_sel = OT_ACODEC_FS_22050;
            break;

        case OT_AUDIO_SAMPLE_RATE_44100:
            i2s_fs_sel = OT_ACODEC_FS_44100;
            break;

        case OT_AUDIO_SAMPLE_RATE_12000:
            i2s_fs_sel = OT_ACODEC_FS_12000;
            break;

        case OT_AUDIO_SAMPLE_RATE_24000:
            i2s_fs_sel = OT_ACODEC_FS_24000;
            break;

        case OT_AUDIO_SAMPLE_RATE_48000:
            i2s_fs_sel = OT_ACODEC_FS_48000;
            break;

        case OT_AUDIO_SAMPLE_RATE_64000:
            i2s_fs_sel = OT_ACODEC_FS_64000;
            break;

        case OT_AUDIO_SAMPLE_RATE_96000:
            i2s_fs_sel = OT_ACODEC_FS_96000;
            break;

        default:
            ERR("%s: not support sample_rate:%d\n", __FUNCTION__, sample_rate);
            return TD_FAILURE;
    }

    *i2s_fs = i2s_fs_sel;
    return TD_SUCCESS;
}

int encode_audio_in_get_volume(int volume_in)
{
    int volume_min = 0, volume_max = 100;
    int volume_acodec_min = 10, volume_acodec_max = 60;

    int volume = (volume_in - volume_min) * (volume_acodec_max - volume_acodec_min) /
                 (volume_max - volume_min) + volume_acodec_min;

    DBG("volume_in: %d, volume codec: %d\n", volume_in, volume);

    return volume;
}

td_s32 encode_inner_codec_cfg_audio(ot_audio_sample_rate sample_rate, td_s32 vol)
{
    td_s32 ret = TD_SUCCESS;
    td_s32 fd_acodec = -1;
    ot_acodec_fs i2s_fs_sel;
    ot_acodec_mixer input_mode;

    do {
        fd_acodec = open(AUDIO_IN_ACODEC_FILE, O_RDWR);
        if (fd_acodec < 0) {
            ERR("can't open audio codec %s\n", AUDIO_IN_ACODEC_FILE);
            ret = TD_FAILURE;
            break;
        }

        ret = ioctl(fd_acodec, OT_ACODEC_SOFT_RESET_CTRL);
        ENCODE_RET_BREAK(ret, "reset audio code failed!\n");

        ret = inner_codec_get_i2s_fs(sample_rate, &i2s_fs_sel);
        ENCODE_RET_BREAK(ret, "inner_codec_get_i2s_fs failed!\n");

        usleep(40*1000);

        ret = ioctl(fd_acodec, OT_ACODEC_SET_I2S1_FS, &i2s_fs_sel);
        ENCODE_RET_BREAK(ret, "set acodec sample rate failed!\n");

        /* refer to hardware, demo board is pseudo-differential (IN_D), socket board is single-ended (IN1) */
        input_mode = OT_ACODEC_MIXER_IN_D;
        ret = ioctl(fd_acodec, OT_ACODEC_SET_MIXER_MIC, &input_mode);
        ENCODE_RET_BREAK(ret, "select acodec input_mode failed!\n");

        /*
        * The input volume range is [-78, 80]. Both the analog gain and digital gain are adjusted.
        * A larger value indicates higher volume.
        * For example, the value 80 indicates the maximum volume of 80 dB,
        * and the value -78 indicates the minimum volume (muted status).
        * The volume adjustment takes effect simultaneously in the audio-left and audio-right channels.
        * The recommended volume range is [20, 50].
        * Within this range, the noises are lowest because only the analog gain is adjusted,
        * and the voice quality can be guaranteed.
        */

        int acodec_input_vol = encode_audio_in_get_volume(vol);
        ret = ioctl(fd_acodec,  OT_ACODEC_SET_INPUT_VOLUME, &acodec_input_vol);
        ENCODE_RET_BREAK(ret, "set acodec micin volume failed!\n");

        DBG("set inner audio codec ok: sample_rate = %d, acodec_input_vol:%d\n", sample_rate, acodec_input_vol);
    }while(0);

    if(fd_acodec) {
        close(fd_acodec);
        fd_acodec = -1;
    }

    return ret;
}

td_s32 encode_audio_set_input_volume(int             volume)
{
    td_s32 ret = TD_SUCCESS;
    td_s32 fd_acodec = -1;

    do {
        fd_acodec = open(AUDIO_IN_ACODEC_FILE, O_RDWR);
        if (fd_acodec < 0) {
            ERR("can't open audio codec %s\n", AUDIO_IN_ACODEC_FILE);
            ret = TD_FAILURE;
            break;
        }

        int acodec_input_vol = encode_audio_in_get_volume(volume);
        DBG("acodec_input_vol:%d\n", acodec_input_vol);
        ret = ioctl(fd_acodec,  OT_ACODEC_SET_INPUT_VOLUME, &acodec_input_vol);
        ENCODE_RET_BREAK(ret, "set acodec micin volume failed!\n");
    } while(0);

    if(fd_acodec) {
        close(fd_acodec);
        fd_acodec = -1;
    }

    return ret;
}

ot_aio_attr *fet_ai_attr(ot_aio_attr *iattr)
{
    iattr->sample_rate = OT_AUDIO_SAMPLE_RATE_16000;
    iattr->point_num_per_frame = AUDIO_IN_PERFRM;

    iattr->bit_width = OT_AUDIO_BIT_WIDTH_16;
    iattr->snd_mode = OT_AUDIO_SOUND_MODE_MONO;
    iattr->frame_num = 20;
    iattr->chn_cnt = 1;
    iattr->clk_share = 1;
    iattr->expand_flag = 0;
    iattr->i2s_type = OT_AIO_I2STYPE_INNERCODEC;
    iattr->work_mode = OT_AIO_MODE_I2S_MASTER;

    pri_audio(LVL_LOOP, "audin samplerate = %d\nbitwidth = %d\nsoundmode = %d\n"
              "frmNum = %d\nnumPerFrm = %d\nchnCnt = %d\n",
              iattr->sample_rate, iattr->bit_width, iattr->snd_mode,
              iattr->frame_num, iattr->point_num_per_frame, iattr->chn_cnt);

    return iattr;
}

static int audio_in_set_pub_attr(void)
{
    int ret = TD_SUCCESS;

    do {
        ret = ss_mpi_ai_set_pub_attr(AUDIO_IN_DEV_ID, fet_ai_attr(&g_cfg_ai->iattr));
        ENCODE_RET_BREAK(ret, "ss_mpi_ai_set_pub_attr failed!\n");

        ret = ss_mpi_ai_enable(AUDIO_IN_DEV_ID);
        ENCODE_RET_BREAK(ret, "ss_mpi_ai_enable failed!\n");

        ret = ss_mpi_ai_enable_chn(AUDIO_IN_DEV_ID, AUDIO_IN_CHNID);
        ENCODE_RET_BREAK(ret, "ss_mpi_ai_enable_chn failed!\n");

        ot_ai_chn_param ai_chn_param = {0};

        ret = ss_mpi_ai_get_chn_param(AUDIO_IN_DEV_ID, AUDIO_IN_CHNID, &ai_chn_param);
        ENCODE_RET_BREAK(ret, "ss_mpi_ai_get_chn_param failed!\n");

        ai_chn_param.usr_frame_depth = 10; /* 10: frame depth */

        ret = ss_mpi_ai_set_chn_param(AUDIO_IN_DEV_ID, AUDIO_IN_CHNID, &ai_chn_param);
        ENCODE_RET_BREAK(ret, "ss_mpi_ai_set_chn_param failed!\n");

        ret = encode_inner_codec_cfg_audio(g_cfg_ai->iattr.sample_rate, g_cfg_ai->audiocfg.involume);
        ENCODE_RET_BREAK(ret, "encodencode_inner_codec_cfg_audioe_audio_start_ai_vqe failed!\n");

        ret = encode_audio_start_ai_vqe(AUDIO_VQE_TYPE_TALKV2);
        ENCODE_RET_CHECK(ret, "encode_audio_start_ai_vqe failed!\n");

        DBG("audio_in_set_pub_attr success\n");
    } while(0);

    return ret;
}

static int audio_in_create_enc_chn(void)
{
    int ret = 0;
    do {
        ot_aenc_chn_attr aenc_attr = {0};
        ot_aenc_attr_g711 aenc_g711 = {0};

        /* set AENC chn attr */
        aenc_attr.buf_size = 30; /* 30:size */
        aenc_attr.point_num_per_frame = AUDIO_IN_PERFRM;
        aenc_attr.value = &aenc_g711;

        switch (g_cfg_ai->audiocfg.codetype) {
        case AudioFormatE_G711A: {
            aenc_attr.type = OT_PT_G711A;
            break;
        }
        case AudioFormatE_G711U: {
            aenc_attr.type = OT_PT_G711U;
            break;
        }
        default : {//AudioFormatE_G711U
            aenc_attr.type = OT_PT_G711U;
            break;
        }
        }

        ret = ss_mpi_aenc_create_chn(AUDIO_IN_AENC_CHN, &aenc_attr);
        ENCODE_RET_BREAK(ret, "ss_mpi_aenc_create_chn failed!\n");

        //ret = encode_ai_bind_aenc(AUDIO_IN_DEV_ID, AUDIO_IN_CHNID, AUDIO_IN_AENC_CHN);
        //ENCODE_RET_BREAK(ret, "encode_ai_bind_aenc failed!\n");
    } while(0);

    return ret;
}

static int audio_in_destroy_enc_chn(void)
{
    int ret = 0;
    do {
        //ret = encode_ai_unbind_aenc(AUDIO_IN_DEV_ID, AUDIO_IN_CHNID, AUDIO_IN_AENC_CHN);
        //ENCODE_RET_BREAK(ret, "encode_ai_unbind_aenc failed!\n");

        ret = ss_mpi_aenc_destroy_chn(AUDIO_IN_AENC_CHN);
        ENCODE_RET_BREAK(ret, "ss_mpi_aenc_destroy_chn failed!\n");
    } while(0);

    return ret;
}

static int audio_in_sdk_init(void)
{
    int ret = 0;

    do {
        ret = audio_in_set_pub_attr();
        break_if_fail(SUCCESS == ret, ret);

        audio_in_create_enc_chn();
        break_if_fail(SUCCESS == ret, ret);

        DBG("audio_in_sdk_init success\n");
    } while(0);

    return ret;
}

static int audio_in_speex_init(sAiSpeexCfg *p_cfg)
{
    char status_msg[256] = {0};
    sAiSpeexCfg cfg = {0};
    int ret = 0, offset = 0;
    int samplerate = OT_AUDIO_SAMPLE_RATE_16000;
    int enable_cnt = 0;

    memcpy(&cfg, p_cfg, sizeof(cfg));

    cfg.agc_decrement *= -1;
    cfg.agc_decrement = RANGE(cfg.agc_decrement, -100, -1);
    cfg.nr_decrement *= -1;
    cfg.nr_decrement = RANGE(cfg.nr_decrement, -90, -5);
    cfg.aec_suppress *= -1;
    cfg.aec_suppress = RANGE(cfg.aec_suppress, -60, -1);
    cfg.aec_suppress_active *= -1;
    cfg.aec_suppress_active = RANGE(cfg.aec_suppress_active, -60, -1);
    DBG("agc_decrement: %d, nr_decrement: %d\n",
        cfg.agc_decrement, cfg.nr_decrement);
    DBG("aec_suppress: %d, aec_suppress_active: %d\n",
        cfg.aec_suppress, cfg.aec_suppress_active);

    if (NULL != g_run_ai->speex_state && NULL != g_run_ai->speex_aec) {
        speex_preprocess_ctl(g_run_ai->speex_state,
                             SPEEX_PREPROCESS_SET_ECHO_STATE, NULL);
        speex_echo_state_destroy(g_run_ai->speex_aec);
    }

    if (cfg.aec_enable) {
        DBG("init speex aec\n");
        g_run_ai->speex_aec = speex_echo_state_init(AUDIO_IN_PERFRM, cfg.aec_filter_len);
        goto_if_fatal_err(NULL != g_run_ai->speex_aec, exit, ret = TD_FAILURE,
                          "failed to init speex echo\n");

        speex_echo_ctl(g_run_ai->speex_aec, SPEEX_ECHO_SET_SAMPLING_RATE, &samplerate);
    } else {
        g_run_ai->speex_aec = NULL;
    }

    sEnb2Mask speex_maps[] = {
        {"NR: [enable"    , cfg.nr_enable,  SPEEX_PREPROCESS_SET_DENOISE,              &cfg.nr_enable},
        {"decrement"      , cfg.nr_enable,  SPEEX_PREPROCESS_SET_NOISE_SUPPRESS,       &cfg.nr_decrement},
        {"]\nAGC: [enable", cfg.agc_enable, SPEEX_PREPROCESS_SET_AGC,                  &cfg.agc_enable},
        {"level"          , cfg.agc_enable, SPEEX_PREPROCESS_SET_AGC_LEVEL,            &cfg.agc_level},
        {"gain"           , cfg.agc_enable, SPEEX_PREPROCESS_SET_AGC_MAX_GAIN,         &cfg.agc_max_gain},
        {"increment"      , cfg.agc_enable, SPEEX_PREPROCESS_SET_AGC_INCREMENT,        &cfg.agc_increment},
        {"decrement"      , cfg.agc_enable, SPEEX_PREPROCESS_SET_AGC_DECREMENT,        &cfg.agc_decrement},
        {"]\nAEC: [enable", cfg.aec_enable, SPEEX_PREPROCESS_SET_ECHO_STATE,           g_run_ai->speex_aec},
        {"suppress_neg"   , cfg.aec_enable, SPEEX_PREPROCESS_SET_ECHO_SUPPRESS,        &cfg.aec_suppress},
        {"suppress_act"   , cfg.aec_enable, SPEEX_PREPROCESS_SET_ECHO_SUPPRESS_ACTIVE, &cfg.aec_suppress_active},
    };

    offset += snprintf(status_msg, sizeof(status_msg) - 1, "AI SPEEX status:\n");
    for (int idx = 0; idx < ARRAY_SIZE(speex_maps); idx++) {
        if (NULL != speex_maps[idx].value) {
            offset += snprintf(status_msg + offset, sizeof(status_msg) - offset - 1,
                               "%s: %d, ", speex_maps[idx].name,
                               *((int *)(speex_maps[idx].value)));
        } else {
            offset += snprintf(status_msg + offset, sizeof(status_msg) - offset - 1,
                               "%s: null, ", speex_maps[idx].name);
        }

        if (!speex_maps[idx].enable) {
            continue;
        }

        enable_cnt++;

        if (NULL == g_run_ai->speex_state) {
            g_run_ai->speex_state = speex_preprocess_state_init(AUDIO_IN_PERFRM,
                                                                samplerate);
            goto_if_fatal_err(NULL != g_run_ai->speex_state, exit, ret = TD_FAILURE,
                              "failed to init speex preprocessor\n");
        }

        speex_preprocess_ctl(g_run_ai->speex_state, speex_maps[idx].mask,
                             speex_maps[idx].value);
    }

    if (0 == enable_cnt) {
        audio_in_speex_uninit();
    }

    DBG("%s]\n", status_msg);

    ret = TD_SUCCESS;

exit:
    return ret;  
}

static void audio_in_speex_uninit(void)
{
    if (NULL != g_run_ai->speex_aec) {
        speex_echo_state_destroy(g_run_ai->speex_aec);
        g_run_ai->speex_aec = NULL;
    }

    if (NULL != g_run_ai->speex_state) {
        speex_preprocess_state_destroy(g_run_ai->speex_state);
        g_run_ai->speex_state = NULL;
    }
}

static int audio_in_sdk_uninit(void)
{
    int ret = TD_SUCCESS;
    do {
        ret = audio_in_destroy_enc_chn();
        ENCODE_RET_BREAK(ret, "audio_in_destroy_enc_chn failed!\n");

        ret = ss_mpi_ai_disable_chn(AUDIO_IN_DEV_ID, AUDIO_IN_CHNID);
        ENCODE_RET_BREAK(ret, "ss_mpi_ai_disable_chn failed!\n");

        ret = ss_mpi_ai_disable(AUDIO_IN_DEV_ID);
        ENCODE_RET_BREAK(ret, "ss_mpi_ai_disable failed!\n");
    } while(0);

    return ret;
}

static int audio_in_confchg_sync(int cmd)
{
    int ret = 0;
     do {
        if (cmd & CMD_AUDIO_IN_CFG) {
            encode_audio_set_input_volume(g_cfg_ai->audiocfg.involume);
        }
    } while(0);

    do {
        if (cmd & CMD_AUDIO_IN_ENABLE) {
            ret = encode_audio_in_stop();
            break_if_fail(TD_SUCCESS == ret, ret);

            ret = audio_in_destroy_enc_chn();
            break_if_fail(TD_SUCCESS == ret, ret);

            ret = audio_in_reset_shmbuf_from_conf();
            break_if_fail(TD_SUCCESS == ret, ret);

            ret = audio_in_create_enc_chn();
            break_if_fail(TD_SUCCESS == ret, ret);

            ret = encode_audio_in_start();
            break_if_fail(TD_SUCCESS == ret, ret);
        }
    } while(0);

    return ret;
}

static void reset_audio_in(void *userdata)
{
    encode_audio_in_uninit();
    encode_audio_in_init();
}

static void loop_audio_in(void *ctx)
{
    int ret = 0;
    do {
        int cmd = cmd_get_command((struct cmdstat *)ctx);

        if (cmd & CMD_AI_CFG) {
            ret= audio_in_confchg_sync(cmd);
            if (FAILURE == ret) {
                ERR("audio in sync conf failed\n");
                break;
            }
        }

        if (cmd & CMD_VQEV2_CFG) {
            encode_ai_sync_vqev2_mask(&g_cfg_ai->vqev2cfg);
            encode_ai_trans2negative_vqev2_param(&g_cfg_ai->vqev2cfg);
            ret = encode_audio_start_ai_vqe(AUDIO_VQE_TYPE_TALKV2);
            ENCODE_RET_CHECK(ret, "encode_audio_start_ai_vqe failed!\n");
        }

        if (cmd & CMD_SPEEX_CFG) {
            audio_in_speex_init(&g_cfg_ai->speexcfg);
        }

        if (cmd & CMD_DEV_CFG) {
            js_run_function(sch_fast, reset_audio_in, NULL, 0);
        }
    } while(0);
}

static void diff_cfg2cmd(void *ctx)
{
    struct cmdstat *p_cmd = (struct cmdstat *)ctx;

    if (p_cmd->cmd_stage) {
        if (p_cmd->cmd_stage & CMD_AI_CFG) {
            if (g_cfg_ai->audiocfg.involume != g_raw_ai->audiocfg.involume ||
                g_cfg_ai->audiocfg.ingain != g_raw_ai->audiocfg.ingain) {
                cmd_set_command(p_cmd, CMD_AUDIO_IN_CFG);
            }
            if (g_cfg_ai->audiocfg.codetype != g_raw_ai->audiocfg.codetype ||
                g_cfg_ai->audiocfg.inenable != g_raw_ai->audiocfg.inenable) {
                cmd_set_command(p_cmd, CMD_AUDIO_IN_ENABLE);
            }
            memcpy(&g_cfg_ai->audiocfg, &g_raw_ai->audiocfg, sizeof(g_raw_ai->audiocfg));
            DBG("inamp: %.2f\n", g_cfg_ai->audiocfg.inamp);
        }

        if (p_cmd->cmd_stage & CMD_ALI_CFG) {
            if (g_cfg_ai->devconf.devicebind != g_raw_ai->devconf.devicebind) {
                cmd_set_command(p_cmd, CMD_DEV_CFG);
            }
            memcpy(&g_cfg_ai->devconf, &g_raw_ai->devconf, sizeof(g_raw_ai->devconf));
        }

        if (p_cmd->cmd_stage & CMD_VQEV2_CFG) {
            memcpy(&g_cfg_ai->vqev2cfg, &g_raw_ai->vqev2cfg, sizeof(g_raw_ai->vqev2cfg));
        }

        if (p_cmd->cmd_stage & CMD_SPEEX_CFG) {
            memcpy(&g_cfg_ai->speexcfg, &g_raw_ai->speexcfg, sizeof(g_raw_ai->speexcfg));
        }
    }
}

int encode_audio_in_init(void)
{
    DBG("encode_audio_in_start\n");
    int ret = TD_SUCCESS;

    do {
        static struct cmdstat cmdstat_audio_in;
        struct cmdstat *ctx = &cmdstat_audio_in;
        cmdstat_audio_in.diff_cfg2cmd = diff_cfg2cmd;
        conf_get_audiocfg(&g_cfg_ai->audiocfg);
        get_config(handleDevConf, g_cfg_ai->devconf);
        get_config(handleAiVqeV2Cfg, g_cfg_ai->vqev2cfg);
        get_config(handleAiSpeexCfg, g_cfg_ai->speexcfg);
        g_run_ai->p_ctx = ctx;

        encode_ai_sync_vqev2_mask(&g_cfg_ai->vqev2cfg);
        encode_ai_trans2negative_vqev2_param(&g_cfg_ai->vqev2cfg);

        ret = audio_in_sdk_init();
        ENCODE_RET_BREAK(ret, "audio_in_sdk_init failed!\n");
        
        ret = audio_in_speex_init(&g_cfg_ai->speexcfg);
        ENCODE_RET_BREAK(ret, "audio_in_speex_init failed!\n");

        ret = audio_in_reset_shmbuf_from_conf();
        ENCODE_CONDI_BREAK(ret < 0, ret, "audio_in_reset_shmbuf_from_conf failed\n");

        g_run_ai->g_sch_audio = js_create_scheduler("sch_audioin");
        ENCODE_NULL_RET_BREAK(g_run_ai->g_sch_audio, ret);

        js_create_timer_r(g_run_ai->g_sch_audio, START_DELAY, AUDIO_IN_LOOP_INTERVAL, loop_audio_in, ctx, &g_run_ai->g_hdl_audioin);
        ENCODE_NULL_RET_BREAK(g_run_ai->g_hdl_audioin, ret);

        ret = encode_audio_in_start();
        ENCODE_RET_BREAK(ret, "encode_audio_in_start failed!\n");

        attach_config(JEvent_AudioInCfgChg, cb_audioincfg_sync, (void *)ctx);
        attach_config(JEvent_AiVqeV2CfgChg, cb_aivqev2cfg_sync, (void *)ctx);
        attach_config(JEvent_AiSpeexCfgChg, cb_aispeexcfg_sync, (void *)ctx);
        attach_config(JEvent_DevCfg, cb_devicebind_sync, (void *)ctx);
        DBG("encode_audio_in_start success\n");
    } while(0);

    return ret;
}

int encode_audio_in_uninit(void)
{
    int ret = 0;
    DBG("encode_audio_in_uninit\n");

    detach_config(JEvent_AudioInCfgChg, cb_audioincfg_sync, g_run_ai->p_ctx);
    detach_config(JEvent_AiVqeV2CfgChg, cb_aivqev2cfg_sync, g_run_ai->p_ctx);
    detach_config(JEvent_AiSpeexCfgChg, cb_aispeexcfg_sync, g_run_ai->p_ctx);
    detach_config(JEvent_DevCfg, cb_devicebind_sync, g_run_ai->p_ctx);

    js_delete_timer_r(&g_run_ai->g_hdl_audioin);

    js_delete_reader_r(&g_run_ai->g_rw_audioin);

    js_delete_scheduler(g_run_ai->g_sch_audio);
    g_run_ai->g_sch_audio = NULL;

    audio_in_speex_uninit();

    ret = audio_in_sdk_uninit();

    DBG("encode_audio_in_uninit success\n");
    return ret;
}

static void cb_audio_in_set_aec(void *usr_data)
{
    int aec_enabled = *((int *)usr_data);

    if (g_run_ai->aec_enabled != aec_enabled) {
        g_run_ai->aec_enabled = aec_enabled;

        encode_ai_sync_vqev2_mask(&g_cfg_ai->vqev2cfg);

        int ret = encode_audio_start_ai_vqe(AUDIO_VQE_TYPE_TALKV2);
        ENCODE_RET_CHECK(ret, "encode_audio_start_ai_vqe failed!\n");
    }
}

void encode_audio_in_set_aec(int enable)
{
    if (NULL != g_run_ai->g_sch_audio) {
        js_run_function(g_run_ai->g_sch_audio, cb_audio_in_set_aec, &enable, 1);
    } else {
        cb_audio_in_set_aec(&enable);
    }
}
