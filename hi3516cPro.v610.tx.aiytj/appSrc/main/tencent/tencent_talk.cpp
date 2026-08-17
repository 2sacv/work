#ifdef PLATFORM_TENCENT
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include "g711.h"
#include "fifo_queue.h"
#include "utils.h"
#include "js_scheduler.h"
#include "confapi.h"
#include "encode_common.h"
#include "tencent_talk.h"
#include "tencent_server.h"
#include "g_run.h"
#include "securec.h"
#include "ot_common_aio.h"

JSScheduler tencent_talk_sch = NULL;
JSTCHandle  tencent_talk_hdl = NULL;
static queue_t *g_talk_queue = NULL;
static aac_dec_info_t aac_dec_info = {0};
static int g_talk_sockfd = -1;

int pcmaudio_16k_to_8k(short *data_16, int len_16, short *data_8)
{
    long i;
    for (i = 0; i < len_16; i++) {
        data_8[i]=data_16[2*i];
    }

    return 0;
}

int tencent_play_talk_process(char *buf, int len)
{
    int read_len = 0;
    int write_len = 0;
    char *pwrite = NULL;

    write_len = len;

    pwrite = buf;
    while (write_len > 0) {
        read_len = write(g_talk_sockfd, pwrite, write_len);
        if (read_len < 0) {
            if (errno != EINTR && errno != EAGAIN) {
                ERR("write error\n");
                break;
            }
            continue;
        }

        write_len -= read_len;
        pwrite += read_len;
    }

    return 0;
}

void talk_set_dec_init_val(int val)
{
    aac_dec_info.dec_init = val;
}

int tencent_aac_decode_init(void)
{
    aac_dec_info.dec_init = 0;
    NeAACDecConfigurationPtr config = {0};

    aac_dec_info.dec_handle = NeAACDecOpen();

    /* Set the default object type and samplerate */
    /* This is useful for RAW AAC files */
    config = NeAACDecGetCurrentConfiguration(aac_dec_info.dec_handle);
    config->defSampleRate = 16000;
    config->defObjectType = LC; // 2
    config->outputFormat = FAAD_FMT_16BIT;
    config->downMatrix = 0;
    config->useOldADTSFormat = 0;
    config->dontUpSampleImplicitSBR = 1;
    NeAACDecSetConfiguration(aac_dec_info.dec_handle, config);

    DBG("tencent_aac_decode_init success\n");

    return 0;
}

static int dyn_write_ao_talk_pcm(void *buf, int len)
{
    static FILE *fp = NULL;

    const char *filepath = is_okey("/opt/long") ? "/mnt/ao_talk.pcm" : "/tmp/ao_talk.pcm";
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

int tencent_aac_to_pcm_and_play(char* aac_data, int aac_size)
{
    int ret = 0;
    int i = 0,j = 0;
    NeAACDecFrameInfo frame_info = {0};
    unsigned long m_samplerate = 16000;
    unsigned char m_channels = 1;
    unsigned char* pcm_data = NULL;
    unsigned char frame_mono[3*1024] = {0};
    static int audio_len = 0;
    static unsigned char audio_buf[PCM_SMPL_PER_FRM_16K * 4] = {0};

    if (aac_dec_info.dec_init == 0) {
        tencent_aac_decode_uninit();
        tencent_aac_decode_init();
        //初始化时，NeAACDecInit需要通过音频数据来判断数据格式是ADTS还是ADIF
        ret = NeAACDecInit(aac_dec_info.dec_handle, (unsigned char *) aac_data, aac_size, &m_samplerate, &m_channels);
        if (ret < 0) {
            ERR("NeAACDecInit false\n");
            return -1;
        }
        aac_dec_info.dec_init = 1;
        DBG("NeAACDecInit success\n");
    }

    unsigned char *p = (unsigned char *)aac_data;
    int bytes_send = PCM_SMPL_PER_FRM_16K * 2;

    do {
        pcm_data = (unsigned char*)NeAACDecDecode (aac_dec_info.dec_handle, &frame_info, p, aac_size);

        dbg_audio("aac_size = %d, chn = %d, samples = %lu, samplerate = %lu, bytesconsumed = %lu\n", \
          aac_size, frame_info.channels, frame_info.samples, frame_info.samplerate
          , frame_info.bytesconsumed);

        if ((frame_info.error == 0) && (frame_info.samples > 0)) {
            p += frame_info.bytesconsumed;
            aac_size -= frame_info.bytesconsumed;

            //从双声道的数据中提取单通道
            for (i=0,j=0; i<4096 && j<2048; i+=4, j+=2) {
                frame_mono[j]=pcm_data[i];
                frame_mono[j+1]=pcm_data[i+1];
            }

            memcpy_s(audio_buf + audio_len, sizeof(audio_buf) - audio_len,
                     frame_mono, frame_info.samples);
            pri_audio(LVL_LOOP, "copy %ld bytes to audio_buf, audio_len: %d\n",
                      MIN(frame_info.samples, sizeof(audio_buf) - audio_len), audio_len);
            audio_len += frame_info.samples;

            if (audio_len >= bytes_send) {
                dyn_write_ao_talk_pcm(audio_buf, bytes_send);
                tencent_play_talk_process((char *)audio_buf, bytes_send);
                memmove_s(audio_buf, sizeof(audio_buf), audio_buf + bytes_send,
                          sizeof(audio_buf) - bytes_send);
                audio_len -= bytes_send;
            }
        } else if (frame_info.error != 0) {
            ret = -1;
            ERR("NeAACDecDecode error=%d\n", frame_info.error);
            break;
        }
    } while (aac_size > 0);

    return ret;
}

int create_speaker_socket_fd(void)
{
    int ret = 0;
    int audio_port = 0;
    int sockfd = -1;
    struct sockaddr_in addr = {0};

    conf_get_speekportcfg(&audio_port);
    if (audio_port <= 0){
        ERR("get speekport fail\n");
        return -1;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(audio_port);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd <= 0){
        ERR("socket creat fail:%d\n", sockfd);
        return -1;
    }

    ret = connect(sockfd, (struct sockaddr*)&addr, sizeof(struct sockaddr));
    if(ret < 0) {
        ERR("socket connect fail\n");
        close(sockfd);
        return -1;
    }

    DBG("create speaker sockfd:%d success\n", sockfd);
    g_talk_sockfd = sockfd;

    encode_ao_set_talking_samplerate(OT_AUDIO_SAMPLE_RATE_16000);

    return 0;
}

static void tencent_talk_play_cb(void *data)
{
    int ret = 0;
    play_audio_info_t *handle= NULL;

    if (NULL == g_talk_queue) {
        ERR("talk server is not ready\n");
        return;
    }

    handle = (play_audio_info_t *)fifo_queue_pop_unblock(g_talk_queue);
    if (handle == NULL) {
        return;
    }

    if (get_g_run(audio, RUN_AUDIO_AAC)) {
        DBG("aac len: %d\n", handle->nBuf);
    }

    ret = tencent_aac_to_pcm_and_play(handle->pBuf, handle->nBuf);
    if (ret < 0) {
        ERR("tencent_aac_to_pcm error\n");
        goto __exit;
    }

__exit:

    if (handle->pBuf) {
        free(handle->pBuf);
        handle->pBuf = NULL;
    }

    free(handle);
    handle = NULL;
    return;
}

int talk_create_speaker_socket(void)
{
    if (g_talk_sockfd < 0) {
        int ret = create_speaker_socket_fd();
        if (ret < 0) {
            ERR("tencent_start_talk_connect fail\n");
            return FAILURE;
        }
    }

    return SUCCESS;
}

void release_queue_audio_data(void)
{
    unsigned int get_frames = 0;
    play_audio_info_t* p_aodata = NULL;

    do {
        p_aodata = (play_audio_info_t*)fifo_queue_pop_unblock(g_talk_queue);
        if (NULL == p_aodata) {
            break;
        }

        if (p_aodata->pBuf) {
            free(p_aodata->pBuf);
        }
        free(p_aodata);

        get_frames++;
        if (get_frames%20 == 0) {
            usleep(10*1000);
        }
    } while(1);

    clear_fifo_queue(g_talk_queue);

    DBG("clear talk queue node:%u\n", get_frames);
    return;
}

int tencent_talk_stop(void)
{
    release_queue_audio_data();

    destroy_speaker_socket_fd();

    DBG("tencent talk stop succ\n");

    return SUCCESS;
}

int destroy_speaker_socket_fd(void)
{
    if (g_talk_sockfd > 0) {
        int ret = close(g_talk_sockfd);
        if (ret < 0) {
            ERR("close speaker socket fail\n");
            return -1;
        }
    }
    g_talk_sockfd = -1;

    if (g_talk_queue) {
        clear_fifo_queue(g_talk_queue);
    }

    DBG("destroy speaker sockfd success\n");

    return 0;
}

/*
 *对讲时小程序下发的音频数据通过此接口存入队列
*/
int tencent_push_audio_data(char *buf, int size)
{
    play_audio_info_t *handle= NULL;

    if (NULL == g_talk_queue) {
        return -1;
    }

    handle = (play_audio_info_t *)malloc(sizeof(play_audio_info_t));
    if (NULL == handle) {
        ERR("malloc fail\n");
        return -1;
    }

    handle->pBuf = (char *)malloc(size);
    handle->nBuf = size;

    memcpy(handle->pBuf, buf, size);
    fifo_queue_push(g_talk_queue, (void*)handle);

    return 0;
}

int tencent_aac_decode_uninit(void)
{
    if (aac_dec_info.dec_handle != NULL) {
        NeAACDecClose(aac_dec_info.dec_handle);
    }
    aac_dec_info.dec_handle = NULL;
    aac_dec_info.dec_init = 0;

    return 0;
}

int tencent_talk_init(void)
{
    g_talk_queue = create_fifo_queue();
    if(NULL == g_talk_queue) {
        ERR("create_fifo_queue talk fail\n");
        return FAILURE;
    }

    tencent_talk_sch = js_create_scheduler((char*)"p2p_talk_server");
    js_create_timer_r(tencent_talk_sch, 5*1000, 50, tencent_talk_play_cb, NULL, &tencent_talk_hdl);

    return SUCCESS;
}

int tencent_talk_uninit(void)
{

    js_delete_timer_r(&tencent_talk_hdl);

    if(tencent_talk_sch) {
        js_delete_scheduler(tencent_talk_sch);
        tencent_talk_sch = NULL;
    }

    tencent_aac_decode_uninit();
    release_queue_audio_data();
    release_fifo_queue(g_talk_queue);
    g_talk_queue = NULL;

    return 0;
}
#endif //PLATFORM_TENCENT
