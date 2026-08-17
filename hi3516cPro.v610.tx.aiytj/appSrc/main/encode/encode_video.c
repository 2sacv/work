/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : encode_video.c
 * @Created Time : 2022-12-5
 * @Version      : 1.0
 * @Author       : tangjx
 * @Description  :
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

#include "securec.h"
#include "ot_common_venc.h"
#include "ss_mpi_venc.h"
#include "ss_mpi_vpss.h"

#include "g_log.h"
#include "g_run.h"
#include "g_sys.h"
#include "g_stat.h"
#include "debug.h"
#include "utils.h"
#include "jevent.h"
#include "cmdstat.h"
#include "encodeapi.h"
#include "js_scheduler.h"
#include "conf_list.h"
#include "jconfig.h"
#include "confapi.h"
#include "system_ctrl.h"
#include "delay_exec.h"
#include "shm_buf.h"
#include "shm_buf_pool.h"
#include "encode_venc.h"
#include "encode_vpss.h"
#include "encode_typedef.h"
#include "encode_common.h"
#include "encode_osd.h"
#include "encode_video.h"
#include "ptz_ctrl.h"
#include "ss_mpi_sys.h"
#include "encode_ivp_aidetect.h"
#define ENCODE_JPEG_CHN 2

typedef struct venc_cfg {
    VideoEncS  venc;
    VeProfileS prof;
    Appvecfg   appve;
}venc_cfg_t;

struct enc_run {
    int venchn[CH_FS_H26X_END];
    int frame_tick[CH_FS_H26X_END];
    int deinitialized;
    JSScheduler sch_stream;
    JSScheduler sch_watch;
    JSTCHandle hdl_loop;
    JSRWHandle hdl_sock[CH_FS_H26X_END];    // main,sub
    JSTCHandle hdl_loop_watch;
    struct cmdstat *p_ctx;
};

/* cfg raw run 定义 */
static struct venc_cfg   cfg = {0};
static struct venc_cfg   raw = {0};
static struct enc_run    run = {
};

struct venc_cfg  *g_cfg_venc = &cfg;
struct venc_cfg  *g_raw_venc = &raw;
struct enc_run   *g_run_enc  = &run;

extern int encode_Body_set_videoimgenc(VideoEncS *ptImageEnc);
extern int encode_osd_Set_delaytime(int delaytime);

static td_s32 encode_jpeg_get_stream(ot_venc_chn venc_chn, char *buf, int *size)
{
    td_s32 ret = TD_SUCCESS;
    ot_venc_chn_status stat = {0};
    ot_venc_stream stream = {0};
    td_u32 i = 0;
    int data_len = 0;

    do {
        ret = ss_mpi_venc_query_status(venc_chn, &stat);
        ENCODE_RET_BREAK(ret, "query_status failed\n");

        if (stat.cur_packs == 0) {
            ERR("NOTE: Current frame is NULL!\n");
            break;
        }

        stream.pack = (ot_venc_pack *)malloc(sizeof(ot_venc_pack) * stat.cur_packs);
        ENCODE_NULL_BREAK(stream.pack);

        stream.pack_cnt = stat.cur_packs;
        ret = ss_mpi_venc_get_stream(venc_chn, &stream, 100);
        ENCODE_RET_BREAK(ret, "ss_mpi_venc_get_stream failed\n");

        for (i = 0; i < stream.pack_cnt; i++) {
            ret = memcpy_s(buf+data_len, *size-data_len, stream.pack[i].addr + stream.pack[i].offset, stream.pack[i].len - stream.pack[i].offset);
            if (ret != S_OK) {
                ERR("memcpy_s failed ret = %d!\n", ret);
                break;
            }
            data_len += (stream.pack[i].len - stream.pack[i].offset);
        }

        // if(0 == ret && *size > data_len && flag) {
        //     encode_jpg_write_single_file(buf, data_len);
        // }

        if(*size > data_len) {
            *size = data_len;
        }

        DBG("JPEG size:%uK\n", data_len/1024);

        ret = ss_mpi_venc_release_stream(venc_chn, &stream);
        ENCODE_RET_BREAK(ret, "ss_mpi_venc_release_stream failed\n");
    }while(0);

    if(stream.pack) {
        free(stream.pack);
        stream.pack = NULL;
    }

    return ret;
}

/*从子码流ivps层取一帧jpeg编码*/
int encode_video_get_jpeg(char *buf, int *size)
{
    td_s32 ret = 0;
    struct timeval timeout_val;
    fd_set read_fds;
    td_s32 venc_fd = 0;
    ot_video_frame_info  video_frame= {0};
    int vpss_grp = 0, vpss_chn = 2, vpss_frame = 0;

    do {
        ret = ss_mpi_vpss_get_chn_frame(vpss_grp, vpss_chn, &video_frame, 3000);
        if (ret != 0) {
            WAR("get frame failed, try again once");
            ret = ss_mpi_vpss_get_chn_frame(vpss_grp, vpss_chn, &video_frame, 3000);
            ENCODE_RET_BREAK(ret, "ss_mpi_vpss_get_chn_frame failed\n");
        }

        vpss_frame = 1;

        ret = ss_mpi_venc_send_frame(ENCODE_JPEG_CHN, &video_frame, 2000);
        ENCODE_RET_BREAK(ret, "ss_mpi_venc_send_frame failed\n");

        venc_fd = ss_mpi_venc_get_fd(ENCODE_JPEG_CHN);
        if (venc_fd < 0) {
            ERR("venc_get_fd faild with%#x!\n", venc_fd);
            break;
        }

        FD_ZERO(&read_fds);
        FD_SET(venc_fd, &read_fds);
        timeout_val.tv_sec = 4; // 2 seconds
        timeout_val.tv_usec = 0;
        ret = select(venc_fd + 1, &read_fds, NULL, NULL, &timeout_val);
        if (ret < 0) {
            ERR("snap select failed!\n");
            ret = TD_FAILURE;
            break;
        } else if (ret == 0) {
            ERR("snap time out!\n");
            ret = TD_FAILURE;
            break;
        } else {
            if (FD_ISSET(venc_fd, &read_fds)) {
                ret = encode_jpeg_get_stream(ENCODE_JPEG_CHN, buf, size);
                ENCODE_RET_CHECK(ret, "encode_jpeg_get_stream failed\n");
            }
        }
    }while(0);

    if(vpss_frame) {
        ret = ss_mpi_vpss_release_chn_frame(vpss_grp, vpss_chn, &video_frame);
        ENCODE_RET_CHECK(ret, "ss_mpi_vpss_release_chn_frame failed\n");
    }

    return ret;
}

static int dyn_write_h26x(void *buf, int len, int i_am_iframe)
{
    static int i = 0, got_iframe = FALSE;
    static FILE *fp = NULL;
    const char *filepath = "/mnt/nake.h26x";
    int start = get_g_run(venc, RUN_VENC_AFTER_SAVE);

    if (start) {
        // start from I frame
        if (!got_iframe) {
            //  DBG("! i_am_iframe %d\n", i_am_iframe);
            if (i_am_iframe) {
                got_iframe = TRUE;
            } else {
                DBG("! i_am_iframe\n");
                return 0;
            }
        }

        if (!fp) {
            fp = fopen(filepath, "w+");
        }
        int wr = fwrite(buf, len, 1, fp);
        DBG("___i[%d]_ len:%d wr:%d\n", i++, len, wr);
    } else {
        if (fp) {
            fsync(fileno(fp));
            fclose(fp);
            fp = NULL;
            i = 0;
            SYSLOG("___ write %s over\n", filepath);
        }
        got_iframe = FALSE;
    }

    return 0;
}

static void _encode_immediate_iframe(void* eChannel)
{
    int ret = S_OK;

    int channel = (int)eChannel;

    DBG("encode_immediate_iframe channel:%d\n", channel);
    switch (channel) {
        case CH_FS_MAIN0:{
            ret = ss_mpi_venc_request_idr(CH_FS_MAIN0, TD_TRUE);
            ENCODE_RET_JUDGE(ret);
            break;
        }
        case CH_FS_SUB0:{
            ret = ss_mpi_venc_request_idr(CH_FS_SUB0, TD_TRUE);
            ENCODE_RET_JUDGE(ret);
            break;
        }
        case CH_FS_ALL:{
            ret = ss_mpi_venc_request_idr(CH_FS_MAIN0, TD_TRUE);
            ENCODE_RET_JUDGE(ret);

            ret = ss_mpi_venc_request_idr(CH_FS_SUB0, TD_TRUE);
            ENCODE_RET_JUDGE(ret);
            break;
        }
        default:
            ERR("ImmeIframe eChannel:%d is ERROR \n",channel);
            break;
    }
}

int encode_immediate_iframe(         CH_FS_E eChannel)
{
    int ret = S_OK;

    if (!g_run_enc->sch_stream) { return S_OK; }
    js_run_function(g_run_enc->sch_stream, _encode_immediate_iframe, (void *)eChannel, 0);

    return ret;
}

/*
 * 1. 统计总帧数 g_run_enc->frame_tick[ch]，以报 no frame count
 * 2. fps gok imax 等信息
 **/
int encode_video_frame_stati(int ch, int is_iframe, int len, double ts_sec)
{
    static int gok[CH_FS_H26X_END] = {0};
    static int imax[CH_FS_H26X_END] = {0};
    static int max_f_b = MAX_FRAME_BYTES;

    static struct vdata {
        int cnt;
        double prev;
        double curr;
    } stati[CH_FS_H26X_END];

    g_run_enc->frame_tick[ch]++;
    stati[ch].cnt++;
    gok[ch] += (len);

    if (!is_iframe) {
        return 0;
    }

    imax[ch] = (len > imax[ch]) ? len : imax[ch];

    if (len > max_f_b) {
        max_f_b = len;
        SYSLOG("got super I frame: %d > %d(KB)\n", len, MAX_FRAME_BYTES/1024);
        DBG("got super I frame: %d > %d(KB)\n", len, MAX_FRAME_BYTES/1024);
    }

    if (fet_g_run(venc)) {
        stati[ch].curr = ts_sec;
        if (get_g_run(venc, RUN_VENC0<<ch)) {
             printf("ch[%d] gok:%6.1fK iFr:%6.1fK iMax:%6.1fK fps:%4.1f itv:%.1f bsp:%6.1f\n",
                 ch, gok[ch]/1024.0, len/1024.0, imax[ch]/1024.0,
                 stati[ch].cnt/(stati[ch].curr - stati[ch].prev), (stati[ch].curr - stati[ch].prev),
                 gok[ch]/1024.0/(stati[ch].curr - stati[ch].prev)
                 );
        }
        stati[ch].prev = stati[ch].curr;
        stati[ch].cnt = 0;
        gok[ch] = 0;
    }
    return 0;
}
/*
 * 将 VPS(仅H265)、SPS、PPS 参数集原子写入 shm buf。
 * 通过缓存指针避免向后索引 pack 导致的越界访问风险。
 */
static void write_vps_sps_pps(shm_buf_t sbuf, int index, int is_h265,
    ot_venc_pack *p_vps, ot_venc_pack *p_sps, ot_venc_pack *p_pps, double ts_sec)
{
    char *vaddr;
    uint32_t len;

    if (is_h265 && p_vps) {
        vaddr = (char *)p_vps->addr + p_vps->offset;
        len = p_vps->len - p_vps->offset;
        shm_buf_write_frame(sbuf, vaddr, len, SHM_FRAME_H265_VPS, ts_sec);
        if (0 == index) {
            dyn_write_h26x(vaddr, len, 1);
        }
    }
    if (p_sps) {
        vaddr = (char *)p_sps->addr + p_sps->offset;
        len = p_sps->len - p_sps->offset;
        shm_buf_write_frame(sbuf, vaddr, len,
            is_h265 ? SHM_FRAME_H265_SPS : SHM_FRAME_H264_SPS, ts_sec);
        if (0 == index) {
            dyn_write_h26x(vaddr, len, 1);
        }
    }
    if (p_pps) {
        vaddr = (char *)p_pps->addr + p_pps->offset;
        len = p_pps->len - p_pps->offset;
        shm_buf_write_frame(sbuf, vaddr, len,
            is_h265 ? SHM_FRAME_H265_PPS : SHM_FRAME_H264_PPS, ts_sec);
        if (0 == index) {
            dyn_write_h26x(vaddr, len, 1);
        }
    }
}

static void encode_video_stream_process(int fd, int ev, void *instances)
{
    int ret;
    int errno;
    ot_venc_stream stream;
    ot_venc_chn_status stat;
    int index = *(int *)instances;
    double ts_sec = 0.0;

    if (index >= CH_FS_H26X_END){
        ERR("venchn err\r\n");
        return;
    }

    if (memset_s(&stream, sizeof(stream), 0, sizeof(stream)) != 0) {
        ERR("call memset_s error\n");
        return;
    }

    do {
        /* step 2.1 : query how many packs in one-frame stream. */
        ret = ss_mpi_venc_query_status(index, &stat);
        if (ret != TD_SUCCESS) {
            ERR("ss_mpi_venc_query_status chn[%d] failed with %#x!\n", index, ret);
            break;
        }

        if (stat.cur_packs == 0) {
            DBG("NOTE: current  frame is TD_NULL!\n");
            break;
        }

        /* step 2.3 : malloc corresponding number of pack nodes. */
        stream.pack = (ot_venc_pack *)malloc(sizeof(ot_venc_pack) * stat.cur_packs);
        if (stream.pack == TD_NULL) {
            ERR("malloc stream pack failed!\n");
            break;
        }

        /* step 2.4 : call mpi to get one-frame stream */
        stream.pack_cnt = stat.cur_packs;
        ret = ss_mpi_venc_get_stream(index, &stream, TD_TRUE);
        if (ret != TD_SUCCESS) {
            ERR("ss_mpi_venc_get_stream failed with %#x!\n", ret);
            break;
        }

        /* step 2.5 : save frame to shm buf */
        shm_buf_t sbuf = get_shm_buf_pool(index);
        ot_venc_pack *p_vps = NULL;   /* H265 vps 缓存 */
        ot_venc_pack *p_sps = NULL;   /* sps 缓存 */
        int is_h265 = 0;
        for (int i = 0; i < stream.pack_cnt; i++) {
            ot_venc_pack ATTRIBUTE *p_pack = &stream.pack[i];
            ts_sec = p_pack->pts/1000000.0;
            td_u64 cur_pts = 0;
            ss_mpi_sys_get_cur_pts(&cur_pts);
            char *vaddr = (char *)p_pack->addr + p_pack->offset;
            uint32_t len = p_pack->len - p_pack->offset;
            if (len <= 0) {
                ERR("frame error, dataType:%d len:%d\n", p_pack->data_type.h264_type, len);
            continue;
            }

            switch ((int)(p_pack->data_type.h264_type)) {
            case OT_VENC_H265_NALU_VPS:
                p_vps = p_pack;
                is_h265 = 1;
                break;
            case OT_VENC_H264_NALU_SPS:
            case OT_VENC_H265_NALU_SPS:
                p_sps = p_pack;
                break;
            case OT_VENC_H264_NALU_PPS:
            case OT_VENC_H265_NALU_PPS:
                write_vps_sps_pps(sbuf, index, is_h265, p_vps, p_sps, p_pack, ts_sec);
                p_vps = NULL;
                p_sps = NULL;
                is_h265 = 0;
                break;
            case OT_VENC_H264_NALU_SEI:
            case OT_VENC_H265_NALU_SEI:
                break;
            case OT_VENC_H264_NALU_IDR_SLICE:
            case OT_VENC_H265_NALU_IDR_SLICE:
                shm_buf_write_frame(sbuf, vaddr, MIN(len, MAX_FRAME_BYTES),/*防花屏*/ SHM_FRAME_VIDEO_I, ts_sec);
                if (0 == index){
                    dyn_write_h26x(vaddr, len, 1);
                }
                encode_video_frame_stati(index, TRUE, len, ts_sec);
                encode_osd_add_stream(index, 1, len);
                break;
            case OT_VENC_H264_NALU_P_SLICE:
                shm_buf_write_frame(sbuf, vaddr, len, SHM_FRAME_VIDEO_P, ts_sec);
                if (0 == index){
                    dyn_write_h26x(vaddr, len, 0);
                }
                encode_video_frame_stati(index, FALSE, len, ts_sec);
                encode_osd_add_stream(index, 1, len);
                break;
            default:
                ERR("dataType:%d \n", p_pack->data_type.h264_type);
                break;
            }
        }

        /* step 2.6 : release stream */
        ret = ss_mpi_venc_release_stream(index, &stream);
        if (ret != TD_SUCCESS) {
            ERR("ss_mpi_venc_release_stream failed!\n");
            break;
        }
    }while (0);

    /* step 2.7 : free pack nodes */
    if (stream.pack){
        free(stream.pack);
        stream.pack = TD_NULL;
    }

    return;
}

static void watch_is_noframe(void *data)
{
    static int frame_prev[2] = {0};
    static int frame_loss[2] = {0};

    for (int chn = 0; chn < ARRAY_SIZE(frame_prev); chn++) {
        if (frame_prev[chn] == g_run_enc->frame_tick[chn]) {
            frame_loss[chn]++;
        } else {
            frame_loss[chn] = 0;
        }

        frame_prev[chn] = g_run_enc->frame_tick[chn];

        // 单球机 sensor 出流时，不检测枪机出帧情况
        if (frame_loss[chn] >= 4) {
            SYSLOG("reboot for no frame:%d %d\n", frame_loss[0], frame_loss[1]);
            frame_loss[0] = frame_loss[1] = -4;
            UtilSystemCmd("/ipc/bin/lzbox dbg_no_frame | tee -a /tmp/messages");
            if (!is_test_ver()) {
                DELAY_REBOOT_LINUX();
            }
        }
    }

    return ;
}

int encode_video_get_chn_start(int vechn)
{
    int venc_fd = ss_mpi_venc_get_fd(vechn);

    if (venc_fd < 0) {
        ERR("ss_mpi_venc_get_fd failed with %#x!\n", venc_fd);
        return -1;
    }

    g_run_enc->venchn[vechn] = vechn;

    js_create_reader_r(g_run_enc->sch_stream, venc_fd, JS_READABLE,encode_video_stream_process, &g_run_enc->venchn[vechn], &g_run_enc->hdl_sock[vechn]);

    return 0;
}

int encode_video_get_chn_stop(int vechn)
{
    js_delete_reader_r(&g_run_enc->hdl_sock[vechn]);

    return 0;
}

int encode_video_get_stream_start(void)
{
    for (int vechn = 0; vechn < CH_FS_H26X_END; vechn++){
        VideoEnc0 *h26x = &g_cfg_venc->venc.enc[vechn];
        int width = 0, height = 0;
        encode_vencsize_to_resolution(h26x->vencsize, &width, &height);

        reset_shm_buf_pool(vechn);
        shm_buf_set_media_info(get_shm_buf_pool(vechn),
                            (h26x->codec == VENC_FORMAT_H265) ? SHM_MEDIA_VIDEO_H265 : SHM_MEDIA_VIDEO_H264,
                            h26x->bps,
                            h26x->fps,
                            width,
                            height);

        encode_video_get_chn_start(vechn);
    }

    return 0;
}

int encode_video_get_stream_stop(void)
{
    for (int vechn = 0; vechn < CH_FS_H26X_END; vechn++){
        encode_video_get_chn_stop(vechn);
    }

    return 0;
}

static int encode_video_pipe_reset(int vechn, int is_codec_change)
{
    int ret = 0;
    ot_vpss_grp vpss_grp = 0;
    int width = 0, height = 0;

    do {
        if(vechn < CH_FS_MAIN0 || vechn > CH_FS_SUB0) {
            ERR("vechn errro %d\n", vechn);
            break;
        }

        VideoEnc0 *h26x = &g_cfg_venc->venc.enc[vechn];
        encode_vencsize_to_resolution(h26x->vencsize, &width, &height);

        ret = encode_osd_group_stop(vechn);
        ENCODE_RET_BREAK(ret, "encode_osd_group_stop failed chn:%d\n", vechn);

        ret = encode_video_get_chn_stop(vechn);
        ENCODE_RET_BREAK(ret, "encode_video_get_chn_stop failed chn:%d\n", vechn);

        ret = ss_mpi_venc_stop_chn(vechn);
        ENCODE_RET_BREAK(ret, "ss_mpi_venc_stop_chn(%d) failed\n", vechn);

        ret = encode_vpss_set_chn_param(vpss_grp, vechn, width, height);
        ENCODE_RET_BREAK(ret, "encode_vpss_set_chn_param failed chn:%d\n", vechn);

        ret = encode_venc_chn_start(vechn, &g_cfg_venc->venc);
        ENCODE_RET_BREAK(ret, "encode_venc_chn_start failed chn:%d\n", vechn);

        ret = encode_venc_chn_frame_strategy(vechn);
        ENCODE_RET_CHECK(ret, "encode_venc_chn_frame_strategy failed chn:%d\n", vechn);

        if (is_codec_change) {
            // onvif 上切换编码方式不出图, 需要 shm_buf_reset, 录像会跳秒
            reset_shm_buf_pool(vechn);
        }

        shm_buf_set_media_info(get_shm_buf_pool(vechn),
                                (h26x->codec == VENC_FORMAT_H265) ? SHM_MEDIA_VIDEO_H265 : SHM_MEDIA_VIDEO_H264,
                                h26x->bps,
                                h26x->fps,
                                width,
                                height);

        ret = encode_osd_group_start(vechn);
        ENCODE_RET_BREAK(ret, "encode_osd_group_start failed chn:%d\n", vechn);

        ret = dzoom_show_osd();
        ENCODE_RET_CHECK(ret, "dzoom_show_osd failed\n");

        ret = encode_video_get_chn_start(vechn);
        ENCODE_RET_BREAK(ret, "encode_video_get_chn_start failed chn:%d\n", vechn);
        DBG("encode_video_pipe_reset chn:%d success\n", vechn);
        send_event_chn(JEvent_DevVideoReport, vechn);
    } while(0);

    return 0;
}

static int encode_video_pipe_adjust(int vechn)
{
    int ret = 0;
    do {
        ret = encode_venc_set_chn_param(vechn, &g_cfg_venc->venc);
        ENCODE_RET_BREAK(ret, "encode_venc_set_chn_param failed chn:%d\n", vechn);

        ret = encode_venc_chn_frame_strategy(vechn);
        ENCODE_RET_CHECK(ret, "encode_venc_chn_frame_strategy failed chn:%d\n", vechn);
    } while(0);

    return ret;
}

static void loop_video_stream(void *ctx)
{
    int cmd = cmd_get_command((struct cmdstat *)ctx);

    if (cmd & CMD_CODEC_RESET0) {
        encode_video_pipe_reset(E_MAIN_CHN, cmd & CMD_CEDEC_CHANGE);
    } else if (cmd & CMD_CODEC_QUICKSET0) {
        encode_video_pipe_adjust(E_MAIN_CHN);
    }

    if (cmd & CMD_CODEC_RESET1) {
        encode_video_pipe_reset(E_SUB_CHN, cmd & CMD_CEDEC_CHANGE);
    } else if (cmd & CMD_CODEC_QUICKSET1) {
        encode_video_pipe_adjust(E_SUB_CHN);
    }

    static struct timespec alarm_tm_pre = {0};
    if (ms_clock_is_timeup(&alarm_tm_pre, SCENE_CHANGE_INTER)) {
        send_alarm_for_hour(); // 这里一个小时起来一次
    }
}

static void cb_appvecfg(int id, void *p_src, int size, void *ctx)
{
    CPY2CMDCFG(CMD_VENC_APPVE, &g_raw_venc->appve, p_src, size);
}

static void cb_devvecfg(int id, void *p_src, int size, void *ctx)
{
    CPY2CMDCFG(CMD_VENC_ENC, &g_raw_venc->venc, p_src, size);
}

void cb_veprofilecfg(int id, void *p_src, int size, void *ctx)
{
    CPY2CMDCFG(CMD_VENC_PROF, &g_raw_venc->prof, p_src, size);
}

static void diff_cfg2cmd(void *ctx)
{
    struct cmdstat *p_cmd = (struct cmdstat *)ctx;

    if (p_cmd->cmd_stage & CMD_VENC_ENC) {
        DBG("cmd_stat:%d\n", p_cmd->cmd_stage);
        for (int chn = 0; chn < CH_FS_H26X_END; chn++) {
            VideoEnc0 *h26x = &g_cfg_venc->venc.enc[chn];
            VideoEnc0 *h26x_r  = &g_raw_venc->venc.enc[chn];

            DBG("vencsize[ch:%d] %d --> %d\n", chn, h26x->vencsize, h26x_r->vencsize);

            if (0 == memcmp(h26x, h26x_r, sizeof(VideoEnc0))) {
                DBG("chn[%d] is kept\n", chn);
                continue;
            }

            if ((h26x->vencsize != h26x_r->vencsize) || // 修改尺寸
                (h26x->codec != h26x_r->codec) ||       // 修改 H26X
                (h26x->fixbps != h26x_r->fixbps)) {     // 修改码率控制
                if(h26x->codec != h26x_r->codec) {
                    send_event_chn(JEvent_DevVideoCodec, chn);
                }

                if (h26x->codec != h26x_r->codec) {
                    cmd_set_command(p_cmd, CMD_CEDEC_CHANGE);
                }
                cmd_set_command(p_cmd, (chn == 0) ? CMD_CODEC_RESET0 : CMD_CODEC_RESET1);
            } else {
                cmd_set_command(p_cmd, (chn == 0) ? CMD_CODEC_QUICKSET0: CMD_CODEC_QUICKSET1);
            }
        }

        memcpy(&g_cfg_venc->venc, &g_raw_venc->venc, sizeof(g_raw_venc->venc));
    }

    /* QP */
    if (p_cmd->cmd_stage & CMD_VENC_APPVE) {
        cmd_set_command(p_cmd, CMD_CODEC_RESET0 | CMD_CODEC_RESET1);
        memcpy(&g_cfg_venc->appve, &g_raw_venc->appve, sizeof(g_raw_venc->appve));
    }

    if (p_cmd->cmd_stage & CMD_VENC_PROF) {
        int chn = 0;
        for (chn = 0; chn < CH_FS_H26X_END; chn++) {
            int vencsize = g_cfg_venc->venc.enc[chn].vencsize;
            if (g_cfg_venc->prof.ps[vencsize].profile != g_raw_venc->prof.ps[vencsize].profile) {
                cmd_set_command(p_cmd, (chn == 0) ? CMD_CODEC_RESET0 : CMD_CODEC_RESET1);
            }
        }
        memcpy(&g_cfg_venc->prof, &g_raw_venc->prof, sizeof(g_raw_venc->prof));
    }

    return;
}


/*
 * 视频初始化
 */
int encode_video_init()
{
    static struct cmdstat cmdstat_video;
    struct cmdstat *ctx = &cmdstat_video;
    cmdstat_video.diff_cfg2cmd = diff_cfg2cmd;
    g_run_enc->p_ctx = ctx;

    g_run_enc->sch_watch = js_create_scheduler((char *) "sch_video_watch");
    g_run_enc->sch_stream = js_create_scheduler("sch_video_stream");

    conf_get_videocfg(&g_cfg_venc->venc);
    conf_get_profilecfg(&g_cfg_venc->prof);
    conf_get_appve_cfg(&g_cfg_venc->appve);

    //注册
    attach_config(JEvent_VideoCfgChg   , cb_devvecfg      , (void *)ctx);
    attach_config(JEvent_AppveCfgChg   , cb_appvecfg      , (void *)ctx);
    attach_config(JEvent_ProfileCfgChg , cb_veprofilecfg  , (void *)ctx);

    encode_video_get_stream_start();

    js_create_timer_r(g_run_enc->sch_stream, 50, 50, loop_video_stream, ctx, &g_run_enc->hdl_loop);
    js_create_timer_r(g_run_enc->sch_watch, 60 * 1000, 60 * 1000, watch_is_noframe, NULL, &g_run_enc->hdl_loop_watch);

    DBG("init_ivps_venc end\n");
    return 0;
}

int encode_video_uninit()
{
    detach_config(JEvent_VideoCfgChg   , cb_devvecfg      , g_run_enc->p_ctx);
    detach_config(JEvent_AppveCfgChg   , cb_appvecfg      , g_run_enc->p_ctx);
    detach_config(JEvent_ProfileCfgChg , cb_veprofilecfg  , g_run_enc->p_ctx);

    encode_video_get_stream_stop();

    js_delete_timer_r(&g_run_enc->hdl_loop_watch);
    js_delete_timer_r(&g_run_enc->hdl_loop);

    js_delete_scheduler(g_run_enc->sch_watch);
    js_delete_scheduler(g_run_enc->sch_stream);

    DBG("uninit_venc_ivps end\n");
    return 0;
}
