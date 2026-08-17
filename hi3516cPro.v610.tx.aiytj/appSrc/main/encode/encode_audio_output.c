/*
 *       Filename:  encode_audio_output.c
 *    Description:
 *        Version:  1.0
 *        Created:  10/26/2022 08:52:44 AM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  xiangyp (),
 *   Organization:
 */

#include <unistd.h>
#include <netinet/tcp.h>

#include "debug.h"
#include "utils.h"
#include "cmdstat.h"
#include "g_log.h"
#include "jconfig.h"
#include "conf_list.h"
#include "js_listen.h"
#include "jconfstruct.h"
#include "confapi.h"
#include "system_ctrl.h"
#include "ot_common_aio.h"
#include "ot_acodec.h"

#include "ss_mpi_audio.h"
#include "encode_audio_input.h"
#include "encode_audio_output.h"
#include "encode_audio_queue.h"
#include "encode_common.h"
#include "net_qrcode.h"
#include "speex_resample.h"
#include "speex_resampler.h"
#include "encode_audio.h"

#define MAX_NETBUFFER_DROP_TIMES    (12)
#define IP_PORT_LEN                 (32)
#define START_DELAY                 (40)
#define AUDIO_OUT_LOOP_INTERVAL     (80)
#define AUDIO_OUT_TALK_DEVIATION    (5)
#define PCT_VOLUME_TALKING          (65)

enum {
    CMD_AO_CFG           = 1 << 0,
    CMD_AUDIO_OUT_CFG    = 1 << 1,
    CMD_AO_ALARM         = 1 << 2,
    CMD_AO_PORT          = 1 << 3,
    CMD_AO_ALARM_EN      = 1 << 4,
};

typedef struct {
    int                 fd;
    int                 read_bytes;
    int                 samplerate;
    JSScheduler         scheduler;
    JSRWHandle          rdhandle;
    SSL                 *ssl_sock;
    struct sockaddr_in  peer_addr;
    char                *buf_16k;
    char                recv_buf[PCM_SMPL_PER_FRM_16K];
    char                ip_port[IP_PORT_LEN];
} AudioSessionS;

struct audio_out_run{
    int is_spk_port_chg;
    int is_talkback;
    int is_local_audio_play;
    int sr_talking;
    int drop_times;
    int play_offset;
    int64_t audio_start_time;
    AudioDataS local_audio;
    AudioSessionS *curr_session;
    JSScheduler sch;
    JSTCHandle  hdl_loop;
    JSListener *listener_audioout;
    int audio_is_created;
    int gpio_is_closed;
    struct cmdstat *p_ctx;
};

struct audio_out_cfg{
    AudioCfgS audiocfg;
    AudioAlarmS alarmcfg;
    NetPortS net_port;
    ot_aio_attr oattr;
    int devicebind;
};

static struct audio_out_cfg cfg = {{0}};
static struct audio_out_cfg raw = {{0}};
static struct audio_out_run run = {0};
static struct audio_out_cfg *g_cfg_ao = &cfg;
static struct audio_out_cfg *g_raw_ao = &raw;
static struct audio_out_run *g_run_ao = &run;

int get_audioout_status(void)
{
    int value = 0;
    gpio_open_get_value(GPIO_AUDIO_OUT, &value);
    return value;
}

int audio_output_sdk_uninit(void)
{
    int ret = TD_SUCCESS;
    do {
        g_run_ao->audio_is_created = FALSE;

        ret = ss_mpi_ao_clr_chn_buf(AUDIO_OUT_DEV_ID, AUDIO_OUT_CHN_ID);
        ENCODE_RET_BREAK(ret, "ss_mpi_ao_clr_chn_buf failed!\n");

        ret = ss_mpi_ao_disable_chn(AUDIO_OUT_DEV_ID, AUDIO_OUT_CHN_ID);
        ENCODE_RET_BREAK(ret, "ss_mpi_ao_disable_chn failed!\n");

        ret = ss_mpi_ao_disable(AUDIO_OUT_DEV_ID);
        ENCODE_RET_BREAK(ret, "ss_mpi_ao_disable failed!\n");
    }while(0);

    return ret;
}

static void cb_audiocfg_sync(int id, void *p_src, int size, void *ctx)
{
    dbg_audio("---------------audio cfg coming\n");
    CPY2CMDCFG(CMD_AO_CFG, &g_raw_ao->audiocfg, p_src, size);
}

static void cb_audioalarmcfg_sync(int id, void *p_src, int size, void *ctx)
{
    dbg_audio("---------------audioalarm cfg coming\n");
    CPY2CMDCFG(CMD_AO_ALARM, &g_raw_ao->alarmcfg, p_src, size);
}

static void cb_speek_port_chg(int id, void *p_src, int size, void *ctx)
{
    dbg_audio("---------------audioout speekport chg\n");
    CPY2CMDCFG(CMD_AO_PORT, &g_raw_ao->net_port, p_src, size);
    g_run_ao->is_spk_port_chg = TRUE;
}

static void free_audio_data(void)
{
    if (NULL != g_run_ao->local_audio.audio) {
        free(g_run_ao->local_audio.audio);
        g_run_ao->local_audio.audio = NULL;
        g_run_ao->local_audio.len = 0;
    }
}

static ot_aio_attr *fet_ao_attr(ot_aio_attr *oattr)
{
    oattr->sample_rate = OT_AUDIO_SAMPLE_RATE_16000;
    oattr->bit_width = OT_AUDIO_BIT_WIDTH_16;
    oattr->snd_mode = OT_AUDIO_SOUND_MODE_MONO;
    oattr->expand_flag = 0;
    oattr->work_mode = OT_AIO_MODE_I2S_MASTER;
    oattr->frame_num = 5;
    oattr->point_num_per_frame = AUDIO_OUT_PERFRM;
    oattr->clk_share = 1;
    oattr->chn_cnt = 1;
    oattr->i2s_type = OT_AIO_I2STYPE_INNERCODEC;

    dbg_audio("audio out pubattr:\nsamplerate = %d\nbitwidth = %d\nsoundmode = %d\n"
              "frmNum = %d\nnumPerFrm = %d\nchnCnt = %d\n",
              oattr->sample_rate, oattr->bit_width, oattr->snd_mode,
              oattr->frame_num, oattr->point_num_per_frame, oattr->chn_cnt);

    return oattr;
}

int encode_audio_out_get_volume(int            volume_out)
{
    int volume_min = 0, volume_max = 100;
    int volume_acodec_min = -20, volume_acodec_max = 6;

    return (volume_out - volume_min) * (volume_acodec_max - volume_acodec_min) / (volume_max - volume_min) + volume_acodec_min;
}

void encode_audio_out_set_volume_cb(void* userdata)
{
    DBG("%s\n", __func__);

    int ret = 0;
    do {
        if(NULL == userdata) {
            ERR("%s failed\n", __func__);
            break;
        }

        int volume = (int)userdata;
        int out_vol = encode_audio_out_get_volume(volume);
        DBG("out_vol : %d\n", out_vol);
        ret = ss_mpi_ao_set_volume(AUDIO_OUT_DEV_ID, out_vol);
        ENCODE_RET_BREAK(ret, "ss_mpi_ao_enable_chn failed!\n");
    } while(0);
} 

int encode_audio_out_set_volume(void)
{
    int volume = 0;
    if (g_run_ao->is_talkback) {
        volume = g_cfg_ao->audiocfg.talkvolume;
    } else {
        volume = g_cfg_ao->audiocfg.outvolume;
    }

    js_run_function(g_run_ao->sch, encode_audio_out_set_volume_cb, (void*)volume, 1);

    return 0;
}

int audio_output_sdk_init(void)
{
    int ret = 0;
    DevConfS devconf = {0};
    ot_ao_vqe_cfg  ao_vqe = {0};

    do {
        get_config(handleDevConf, devconf);
        g_cfg_ao->devicebind = devconf.devicebind;

        ret = ss_mpi_ao_set_pub_attr(AUDIO_OUT_DEV_ID, fet_ao_attr(&g_cfg_ao->oattr));
        ENCODE_RET_BREAK(ret, "ss_mpi_ao_set_pub_attr failed!\n");

        ret = ss_mpi_ao_enable(AUDIO_OUT_DEV_ID);
        ENCODE_RET_BREAK(ret, "ss_mpi_ao_enable failed!\n");

        ret = ss_mpi_ao_enable_chn(AUDIO_OUT_DEV_ID, AUDIO_OUT_CHN_ID);
        ENCODE_RET_BREAK(ret, "ss_mpi_ao_enable_chn failed!\n");

        ao_vqe.open_mask = OT_AO_VQE_MASK_EQ | OT_AO_VQE_MASK_AGC;
        ao_vqe.frame_sample = AUDIO_OUT_PERFRM;
        ao_vqe.work_sample_rate = g_cfg_ao->oattr.sample_rate;
        ao_vqe.work_state = OT_VQE_WORK_STATE_COMMON;
        ao_vqe.eq_cfg.gain_db[6] = 6;
        ao_vqe.eq_cfg.gain_db[7] = 6;
        ao_vqe.eq_cfg.gain_db[8] = 6;
        
        ao_vqe.agc_cfg.usr_mode = 1;
        ao_vqe.agc_cfg.target_level = -1; // 最大-1dB
        ao_vqe.agc_cfg.noise_floor = -60;
        ao_vqe.agc_cfg.max_gain = 2; // 不做增强
        ao_vqe.agc_cfg.adjust_speed = 10;
        ao_vqe.agc_cfg.improve_snr = 2;
        ao_vqe.agc_cfg.use_hpf = 0;
        ao_vqe.agc_cfg.output_mode = 0;
        ao_vqe.agc_cfg.noise_suppress_switch = 1;

        ret = ss_mpi_ao_set_vqe_attr(AUDIO_OUT_DEV_ID, AUDIO_OUT_CHN_ID, &ao_vqe);
        ENCODE_RET_BREAK(ret, "ss_mpi_ao_set_vqe_attr failed!\n");
        
        ret = ss_mpi_ao_enable_vqe(AUDIO_OUT_DEV_ID, AUDIO_OUT_CHN_ID);
        ENCODE_RET_BREAK(ret, "ss_mpi_ao_enable_vqe failed!\n");

        int out_vol = encode_audio_out_get_volume(g_cfg_ao->audiocfg.outvolume);
        ret = ss_mpi_ao_set_volume(AUDIO_OUT_DEV_ID, out_vol);
        break_if_fail(TD_SUCCESS == ret, ret);

        g_run_ao->audio_is_created = TRUE;
    } while(0);

    return ret;
}

static int audio_out_conf_init(void)
{
    int ret = TD_SUCCESS;

    do {
        ret = audio_output_sdk_init();
        break_if_fail(TD_SUCCESS == ret, ret);
    } while(0);

    return ret;
}

void audio_session_close(AudioSessionS *session)
{
    return_if_fail(NULL != session);

    js_delete_reader_r(&session->rdhandle);

    if (session->fd > 0) {
        close(session->fd);
        session->fd = -1;
    }

    if (NULL != session->buf_16k && session->buf_16k != session->recv_buf) {
        free(session->buf_16k);
        session->buf_16k = NULL;
    }

    free(session);
    session = NULL;

    g_run_ao->curr_session = NULL;
    g_run_ao->is_talkback = FALSE;
    g_run_ao->gpio_is_closed = FALSE;
    
    encode_audio_out_set_volume();

    dbg_audio("close session\n");
}

int encode_audio_stop_playing(void)
{
    int ret = 0;

    dbg_audio("audio clean buffer\n");

    g_run_ao->play_offset = 0;
    g_run_ao->local_audio.duration = 0;
    g_run_ao->audio_start_time = 0;

    ret = ss_mpi_ao_clr_chn_buf(AUDIO_OUT_DEV_ID, AUDIO_OUT_CHN_ID);
    ENCODE_RET_CHECK(ret, "ss_mpi_ao_clr_chn_buf failed!\n");

    return ret;
}

static void diff_cfg2cmd(void *ctx)
{
    struct cmdstat *p_cmd = (struct cmdstat *)ctx;

    if (p_cmd->cmd_stage) {
        if (p_cmd->cmd_stage & CMD_AO_CFG) {
            if (g_raw_ao->audiocfg.outvolume != g_cfg_ao->audiocfg.outvolume ||
                g_raw_ao->audiocfg.outgain != g_cfg_ao->audiocfg.outgain) {
                cmd_set_command(p_cmd, CMD_AUDIO_OUT_CFG);
            }
            memcpy(&g_cfg_ao->audiocfg, &g_raw_ao->audiocfg, sizeof(g_raw_ao->audiocfg));
            DBG("talkamp: %.2f, outamp: %.2f, talk_volume: %d\n",
                g_cfg_ao->audiocfg.talkamp, g_cfg_ao->audiocfg.outamp,
                g_cfg_ao->audiocfg.talkvolume);
        }

        if (p_cmd->cmd_stage & CMD_AO_ALARM) {
            if (g_raw_ao->alarmcfg.enable != g_cfg_ao->alarmcfg.enable) {
                cmd_set_command(p_cmd, CMD_AO_ALARM_EN);
            }
            memcpy(&g_cfg_ao->alarmcfg, &g_raw_ao->alarmcfg, sizeof(g_raw_ao->alarmcfg));
        }

        if (p_cmd->cmd_stage & CMD_AO_PORT) {
            memcpy(&g_cfg_ao->net_port, &g_raw_ao->net_port, sizeof(g_raw_ao->net_port));
        }
    }
}

static int encode_audio_clean_all(void)
{
    int ret = 0;

    encode_audio_queue_clean();

    ret = encode_audio_stop_playing();

    return ret;
}

static int reset_conf_for_new_connection(AudioSessionS *session)
{
    return_val_if_fail(NULL != session, FAILURE);

    dbg_audio("reset conf for new connection\n");

    int ret = 0;

    encode_audio_in_set_aec(TRUE);

    if (NULL != g_run_ao->curr_session) {
        dbg_audio("close prev connection\n");
        audio_session_close(g_run_ao->curr_session);
    }

    g_run_ao->is_talkback = TRUE;
    g_run_ao->curr_session = session;
    g_run_ao->gpio_is_closed = TRUE;

    ret = encode_audio_clean_all();
    if (FAILURE == ret) {
        ERR("audio clean buffer and queue failed\n");
    }

    ret = gpio_open_set_value(GPIO_AUDIO_OUT, GPIO_AUDIO_ON);
    if (FAILURE == ret) {
        ERR("audio out gpio open failed\n");
    }

    encode_audio_out_set_volume();

    return ret;
}

static int dyn_write_ao_recv_pcm(void *buf, int len)
{
    static FILE *fp = NULL;

    const char *filepath = is_okey("/opt/long") ? "/mnt/ao_recv.pcm" : "/tmp/ao_recv.pcm";
    int start = get_g_run(audio, RUN_AUDIO_SAVE);

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

static int audio_session_recv_data(AudioSessionS *session)
{
    return_val_if_fail(NULL != session, FAILURE);

    int read_bytes = 0, ret = 0, all_read = 0, repeat_times = 0;

    //总共有多少数据可以读出
    ioctl(session->fd, FIONREAD, &all_read);

    //分几次能读完
    if (0 == all_read % sizeof(session->recv_buf)) {
        repeat_times = all_read / sizeof(session->recv_buf);
    } else {
        repeat_times = all_read / sizeof(session->recv_buf) + 1;
    }

    //如果正在播放本地音频，此时来对讲，需丢弃前面一段 net 音频保证音频同步性
    if (TRUE == g_run_ao->is_local_audio_play) {
        g_run_ao->is_local_audio_play = FALSE;
        g_run_ao->drop_times = MAX_NETBUFFER_DROP_TIMES;
    }

    do {
        repeat_times--;
        memset(session->recv_buf, 0, sizeof(session->recv_buf));

        do {
            read_bytes = recv(session->fd, session->recv_buf, sizeof(session->recv_buf), 0);
        } while(FAILURE == read_bytes && EINTR == errno);

        if (FAILURE == read_bytes) {
            if (EAGAIN != errno) {
                ERR("recv fail:%s\n", strerror(errno));
                ret = FAILURE;
                g_run_ao->drop_times = 0;
            }
            break;
        } else if (0 == read_bytes) {
            dbg_audio("connection break\n");
            ret = FAILURE;
            g_run_ao->drop_times = 0;
            break;
        }

        if (g_run_ao->drop_times > 0) {
            g_run_ao->drop_times--;
            break;
        }

        //采样率在 audio_session_create 时默认 8K，如果外部有设置，采用外部的设置
        if (g_run_ao->sr_talking > 0) {
            session->samplerate = g_run_ao->sr_talking;
            g_run_ao->sr_talking = 0;
        }

        if (OT_AUDIO_SAMPLE_RATE_16000 == session->samplerate) {
            session->buf_16k = session->recv_buf;
            session->read_bytes = read_bytes;
        } else {
            if (NULL == session->buf_16k) {
                session->read_bytes = sizeof(session->recv_buf) * 2;
                session->buf_16k = (char *)malloc(session->read_bytes);
                if (NULL == session->buf_16k) {
                    ERR("failed to malloc ao 16k buf\n");
                    continue;
                }
            }

            session->read_bytes = sizeof(session->recv_buf) * 2;
            speex_resample_8k_to_16k(session->buf_16k, (size_t *)&session->read_bytes,
                                     session->recv_buf, read_bytes, NULL);
        }

        ot_audio_frame audio_frame = {0};

        pri_audio(LVL_LOOP, "all push %d bytes\n", read_bytes);
        audio_frame.bit_width = OT_AUDIO_BIT_WIDTH_16;
        audio_frame.len = session->read_bytes;
        audio_frame.snd_mode = OT_AUDIO_SOUND_MODE_MONO;
        audio_frame.virt_addr[0] = (td_u8 *)session->buf_16k;

        amplify_pcm_volume(audio_frame.virt_addr[0], audio_frame.len,
                           audio_frame.bit_width, g_cfg_ao->audiocfg.talkamp);

        dyn_write_ao_recv_pcm(audio_frame.virt_addr[0], audio_frame.len);
        ret = ss_mpi_ao_send_frame(AUDIO_OUT_DEV_ID, AUDIO_OUT_CHN_ID, &audio_frame, -1);
        ENCODE_RET_BREAK(ret, "ss_mpi_ao_send_frame failed!\n");
    } while(repeat_times > 0);

    return ret;
}

static void audio_on_listening_event(int fd, int events, void *userdata)
{
    return_if_fail(NULL != userdata);

    int ret = 0;
    AudioSessionS *session = (AudioSessionS *)userdata;

    if (events & JS_READABLE) {
        ret = audio_session_recv_data(session);
        if (ret < 0) {
            WAR("audio [fd%d:%s] conenction closed\n", session->fd, session->ip_port);
            audio_session_close(session);
            encode_audio_in_set_aec(FALSE);
            return;
        }
    }
}

void audio_session_create(void *userdata, int acceptsock, SSL * ssl_sock)
{
    return_if_fail(NULL != userdata && acceptsock > 0);

    int ret = 0, flag = 1;
    AudioSessionS *session = NULL;

    JSListener *listener = (JSListener *)userdata;

    do {
        break_if_fail(NULL != listener->scheduler, FAILURE);

        session = (AudioSessionS *)malloc(sizeof(AudioSessionS));
        break_if_fail(NULL != session, FAILURE);
        memset(session, 0, sizeof(AudioSessionS));

        memcpy(&session->peer_addr, &listener->peer_addr, sizeof(struct sockaddr_in));
        session->scheduler = listener->scheduler;       // 必须和 accept 同线程，防止阻塞卡死
        session->fd = acceptsock;
        session->ssl_sock = ssl_sock;
        session->samplerate = OT_AUDIO_SAMPLE_RATE_8000;

        ret = setsockopt(acceptsock, IPPROTO_TCP, TCP_NODELAY,
                         (char *)&flag, sizeof(flag));
        if (ret < 0) {
            ERR("setsockopt error:%s\n", strerror(errno));
            break;
        }

        snprintf(session->ip_port, sizeof(session->ip_port) - 1, "%s:%d",
                 inet_ntoa(session->peer_addr.sin_addr), ntohs(session->peer_addr.sin_port));
        WAR("establish connection from %s\n", session->ip_port);

        ret = reset_conf_for_new_connection(session);
        break_if_fail(SUCCESS == ret, FAILURE);

        js_create_reader_r(session->scheduler, session->fd, JS_READABLE,audio_on_listening_event, session, &session->rdhandle);
    } while(0);
}

static int audio_out_listener_restart(void)
{
    if (NULL != g_run_ao->listener_audioout) {
        js_listen_delete(g_run_ao->listener_audioout);
        g_run_ao->listener_audioout = NULL;
    }

    g_run_ao->listener_audioout = js_listen_create(g_run_ao->sch,
                g_cfg_ao->net_port.audioport, audio_session_create, NULL, NULL);
    if (NULL != g_run_ao->listener_audioout) {
        return TD_SUCCESS;
    } else {
        return TD_FAILURE;
    }
}

static int audio_out_confchg_sync(int cmd)
{
    int ret = 0;

    do {
        if (cmd & CMD_AO_ALARM_EN) {
            ret = encode_audio_clean_all();
            break_if_fail(TD_SUCCESS == ret, ret);
        }
    } while(0);

    do {
        if (cmd & CMD_AUDIO_OUT_CFG) {
            int out_volume = 0;
            if(g_run_ao->is_talkback) {
                out_volume = g_cfg_ao->audiocfg.outvolume - AUDIO_OUT_TALK_DEVIATION;
                if(out_volume < 0) {
                    out_volume  = 1;
                }
            } else {
                out_volume = g_cfg_ao->audiocfg.outvolume;
            } 
            int out_vol = encode_audio_out_get_volume(g_cfg_ao->audiocfg.outvolume);
            DBG("out_vol:%d\n", out_vol);
            ret = ss_mpi_ao_set_volume(AUDIO_OUT_DEV_ID, out_vol);
            break_if_fail(TD_SUCCESS == ret, ret);
        }
    } while(0);

    do {
        if (TRUE == g_run_ao->is_spk_port_chg) {
            ret = audio_out_listener_restart();
            break_if_fail(TD_SUCCESS == ret, ret);
            g_run_ao->is_spk_port_chg = FALSE;
        }
    } while(0);

    return ret;
}

static int audio_out_play_local(void)
{
    return_val_if_fail(NULL !=g_run_ao->local_audio.audio, FAILURE);

    int ret = 0, audioout_len = 0;

    //刚开始播放时，play_offset 为 0，记录当前播放开始时间点
    if (0 == g_run_ao->play_offset) {
        //打开音频 gpio 口
        ret = gpio_open_set_value(GPIO_AUDIO_OUT, GPIO_AUDIO_ON);
        ret = ss_mpi_ao_clr_chn_buf(AUDIO_OUT_DEV_ID, AUDIO_OUT_CHN_ID);
        ENCODE_RET_CHECK(ret, "ss_mpi_ao_clr_chn_buf failed!\n");

        g_run_ao->audio_start_time = mono_msec();
        dbg_audio("start time:%lld, audio_play_duration:%d\n",
               g_run_ao->audio_start_time, g_run_ao->local_audio.duration);
    }

    do {
        if (g_run_ao->play_offset + AUDIO_OUT_NUM_PERFRM < g_run_ao->local_audio.len) {
            audioout_len = AUDIO_OUT_NUM_PERFRM;
        } else {
            audioout_len = (g_run_ao->local_audio.len - g_run_ao->play_offset);
        }

        ot_audio_frame audio_frame = {0};

        audio_frame.bit_width = OT_AUDIO_BIT_WIDTH_16;
        audio_frame.len = audioout_len;
        audio_frame.snd_mode = OT_AUDIO_SOUND_MODE_MONO;
        audio_frame.virt_addr[0] = (td_u8 *)(g_run_ao->local_audio.audio+g_run_ao->play_offset);

        amplify_pcm_volume(audio_frame.virt_addr[0], audio_frame.len,
                           audio_frame.bit_width, g_cfg_ao->audiocfg.outamp);

        ret = ss_mpi_ao_send_frame(AUDIO_OUT_DEV_ID, AUDIO_OUT_CHN_ID, &audio_frame, -1);
        ENCODE_RET_BREAK(ret, "ss_mpi_ao_send_frame failed!\n");
        g_run_ao->play_offset += audioout_len;
    } while (g_run_ao->play_offset < g_run_ao->local_audio.len);

    g_run_ao->play_offset = 0;

    return ret;
}

static int resample_audio_to_16k(void)
{
    int ret = TD_SUCCESS;
    size_t bytes_16k_aud = g_run_ao->local_audio.len * 2;

    uint8_t *aud_16k = (uint8_t *)malloc(bytes_16k_aud);
    goto_exit_if_fail(NULL != aud_16k, exit, ret = TD_FAILURE,
                      "failed to malloc 16k audio\n");

    ret = speex_resample_8k_to_16k(aud_16k, &bytes_16k_aud, g_run_ao->local_audio.audio,
                                   g_run_ao->local_audio.len, NULL);
    goto_exit_if_fail(RESAMPLER_ERR_SUCCESS == ret, exit, ret = TD_FAILURE,
                      "failed to resample 16k audio\n");

exit:
    if (RESAMPLER_ERR_SUCCESS == ret) {
        g_run_ao->local_audio.audio = aud_16k;
        dbg_audio("success resample audio to 16k, org len: %d, resampled len: %d\n",
                  g_run_ao->local_audio.len, bytes_16k_aud);
        g_run_ao->local_audio.len = bytes_16k_aud;
    } else {
        if (NULL != aud_16k) {
            free(aud_16k);
        }
    }

    return ret;
}

static int audio_out_process(void)
{
    int ret = 0;
    int64_t interval = 0;
    ot_ao_chn_state ao_status = {0};

    do {
        //正在语音对讲，break
        if (TRUE == g_run_ao->is_talkback) {
            break;
        }

        //如果之前的数据已经发送且播放完，重新取数据
        //获取下一个音频数据
        if (0 == g_run_ao->play_offset) {
            interval = mono_msec() - g_run_ao->audio_start_time;

            //未播放完，时间间隔小于音频时长
            if (interval < g_run_ao->local_audio.duration) {
                break;
            }

           	g_run_ao->audio_start_time = 0;
            g_run_ao->local_audio.duration = 0;

            //用完音频数据之后释放
            free_audio_data();

            //队列未取到数据，break
            ret = encode_audio_queue_get_amr(&g_run_ao->local_audio);
            if (NO_AUDIO_DATA == ret) {
                ret = ss_mpi_ao_query_chn_status(AUDIO_OUT_DEV_ID, AUDIO_OUT_CHN_ID, &ao_status);
                ENCODE_RET_BREAK(ret, "ss_mpi_ao_query_chn_status failed!\n");
                if ((FALSE == g_run_ao->gpio_is_closed) && (ao_status.chn_busy_num == 0)) {
                    ret = gpio_open_set_value(GPIO_AUDIO_OUT, GPIO_AUDIO_OFF);
                    break_if_fail(SUCCESS == ret, ret);
                    g_run_ao->gpio_is_closed = TRUE;
                    dbg_audio("gpio_is_closed:%d\n", g_run_ao->gpio_is_closed);
                }
                ret = 0;
                break;
            } else if (FAILURE == ret) {
                ERR("get amr failed\n");
                break;
            } else {
                g_run_ao->gpio_is_closed = FALSE;
                resample_audio_to_16k();
            }
            g_run_ao->local_audio.duration += 500;
            dbg_audio("audio len:%d, audio_duration:%d\n",
                   g_run_ao->local_audio.len, g_run_ao->local_audio.duration);
        }

        ret = audio_out_play_local();
        break_if_fail(0 <= ret, ret);
    } while(0);

    return ret;
}

static void loop_audio_out(void *ctx)
{
    int ret = 0;

    do {
        if (FALSE == g_run_ao->audio_is_created) {
            break;
        }

        int cmd = cmd_get_command((struct cmdstat *)ctx);

        if ((cmd & CMD_AO_ALARM) || (cmd & CMD_AO_CFG)) {
            ret = audio_out_confchg_sync(cmd);
            ENCODE_RET_BREAK(ret, "audio_out_confchg_sync failed!\n");
        }

        ret = audio_out_process();
        if (FAILURE == ret) {
            ERR("audio out running error\n");
            break;
        }
    } while(0);
}

int encode_audio_out_init(void)
{
    int ret = 0;
    DBG("encode audio out start\n");

    do {
        static struct cmdstat cmdstat_audio_out;
        struct cmdstat *ctx = &cmdstat_audio_out;
        cmdstat_audio_out.diff_cfg2cmd = diff_cfg2cmd;
        g_run_ao->p_ctx = ctx;

        conf_get_audiocfg(&g_cfg_ao->audiocfg);
        conf_get_audioalarm_cfg(&g_cfg_ao->alarmcfg);
        conf_get_netportcfg(&g_cfg_ao->net_port);

        ret = gpio_open_export(GPIO_AUDIO_OUT);
        break_if_fail(TD_SUCCESS == ret, ret);

        ret = gpio_open_set_direction(GPIO_AUDIO_OUT, "out");
        break_if_fail(TD_SUCCESS == ret, ret);

        ret = gpio_open_set_value(GPIO_AUDIO_OUT, GPIO_AUDIO_OFF);
        break_if_fail(TD_SUCCESS == ret, ret);

        attach_config(JEvent_AudioInCfgChg, cb_audiocfg_sync, (void *)ctx);
        attach_config(JEvent_AudioAlarmCfg, cb_audioalarmcfg_sync, (void *)ctx);
        attach_config(JEvent_SpeekPortCfgChg, cb_speek_port_chg, (void *)ctx);

        g_run_ao->sch = js_create_scheduler("sch_audioout");
        break_if_fail(NULL != g_run_ao->sch, TD_FAILURE);

        ret = audio_out_conf_init();
        break_if_fail(TD_SUCCESS == ret, ret);

        g_run_ao->listener_audioout = js_listen_create(g_run_ao->sch, g_cfg_ao->net_port.audioport,
                                               audio_session_create, NULL, NULL);
        break_if_fail(NULL != g_run_ao->listener_audioout, TD_FAILURE);

        ret = js_create_timer_r(g_run_ao->sch, START_DELAY,
                                         AUDIO_OUT_LOOP_INTERVAL, loop_audio_out, ctx, &g_run_ao->hdl_loop);
        break_if_fail(TD_SUCCESS == ret, TD_FAILURE);
    } while(0);

    return ret;
}

void encode_audio_out_uninit(void)
{
    int ret = 0;

    DBG("encode_audio_out_uninit\n");

    js_delete_timer_r(&g_run_ao->hdl_loop);

    js_listen_delete(g_run_ao->listener_audioout);

    encode_audio_in_set_aec(FALSE);

    js_delete_scheduler(g_run_ao->sch);
    g_run_ao->sch = NULL;

    detach_config(JEvent_AudioInCfgChg, cb_audiocfg_sync, (void *)g_run_ao->p_ctx);
    detach_config(JEvent_AudioAlarmCfg, cb_audioalarmcfg_sync, (void *)g_run_ao->p_ctx);
    detach_config(JEvent_SpeekPortCfgChg, cb_speek_port_chg, (void *)g_run_ao->p_ctx);

    ret = gpio_open_set_value(GPIO_AUDIO_OUT, GPIO_AUDIO_OFF);
    ENCODE_RET_CHECK(ret, "gpio_open_set_value failed!\n");

    ret = audio_output_sdk_uninit();
    ENCODE_RET_CHECK(ret, "audio_output_sdk_uninit failed!\n");

    destroy_speex_resampler_8k_to_16k();
}

void encode_ao_set_talking_samplerate(int samplerate)
{
    DBG("set talking samplerate: %d\n", samplerate);
    g_run_ao->sr_talking = samplerate;
}
