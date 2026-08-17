/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    :
 * Created Time : 2018-09-11
 * Version      : 1.0
 * Author       : luoshunfa
 * Description  :
 */

#include <errno.h>

#include "g711.h"
#include "libmp4.h"
#include "encodeapi.h"
#include "shm_buf_pool.h"
#include "record_watch.h"
#include "record_file_manage.h"
#include "js_rec.h"
#include "utils.h"
#include "record_base64.h"
#include "system_sch.h"
#include "ot_type.h"
#include "ss_mpi_sys.h"

#define VEDIO_LOOP 30
#define AUDIO_LOOP 50
#define AAC_HEAD_INFO 7  // mp4不能封装AAC的7字节头部信息
static int g_last_I_frame = -1;
static int g_serial = -1;
static int g_aserial = 0;

typedef struct {
    JSScheduler    scheduler;
    JSScheduler    scheduler_stop;  //后台停止录像调度
    int            createflags;

    struct timespec starttime;
    char            startday[16];
    int             timelen;     //录像时长，结束时计算

    JRecParamInfo  recparam;

    char           mp4filename[264];
    CMP4Write*     mp4writer;

    int            notready;  //统计获取不到帧的次数

    JSTCHandle     vhandle;
    shm_buf_t      vbuf;
    int            vserial;
    double         vfirsttimestamp;
    unsigned short lastwidth;
    unsigned short lastheight;

    eShmMediaType  mediatype;
    char           vpsData[256];
    char           spsData[256];
    char           ppsData[256];
    int            vpsDataSize;
    int            spsDataSize;
    int            ppsDataSize;

    JSTCHandle     ahandle;
    shm_buf_t      abuf;
    int            aserial;
    double         afirsttimestamp;
    int            aframenums;

    int            totalsize;
} js_record_t;

/*
 * get_cur_serial() 和 set_cur_serial() 的作用
 *      1. 保证初始化取的帧号和开始录像取的第一帧视频序号相同
 *      2. 保证开始录像的时候帧号不会回跳
 * */
int get_cur_serial()
{
    return g_serial;
}

void set_cur_serial(int serial)
{
    g_serial = serial;
    return;
}

int get_cur_aserial()
{
    return g_aserial;
}

void set_cur_aserial(int serial)
{
    g_aserial = serial;
}

static void js_read_init_mp4(void *userdata, tSBFrame* tFrame)
{
    // 初始化mp4文件,只赋值需要的信息
    tSBFrame* pOutFrm = (tSBFrame*)userdata;

    pOutFrm->error           = tFrame->error;
    pOutFrm->frame_type      = tFrame->frame_type;
    pOutFrm->mediatype       = tFrame->mediatype;
    pOutFrm->v.vpsdata_size = tFrame->v.vpsdata_size;
    pOutFrm->v.spsdata_size = tFrame->v.spsdata_size;
    pOutFrm->v.ppsdata_size = tFrame->v.ppsdata_size;
    pOutFrm->v.width        = tFrame->v.width;
    pOutFrm->v.height       = tFrame->v.height;
    pOutFrm->frame_timestamp = tFrame->frame_timestamp;
    pOutFrm->frame_serial   = tFrame->frame_serial;
    pOutFrm->curframe_serial = tFrame->curframe_serial;

    // I帧时需要拷贝SPSPPSVPS信息
    if (tFrame->frame_type == SHM_FRAME_VIDEO_I) {
        pOutFrm->v.vpsdata = tFrame->v.vpsdata;
        pOutFrm->v.spsdata = tFrame->v.spsdata;
        pOutFrm->v.ppsdata = tFrame->v.ppsdata;
    }

    return;
}

#define MIN_PRE_RECORD_TIME 2 // 最小预录制时间，单位：秒
#define MAX_PRE_RECORD_TIME 6 // 最大预录制时间，单位：秒

/**
 * @brief 处理报警预录制，目前定义范围 [min_time, min_time + gop)，以 min_time = 2 为例，小于 2S 就往前找一个 I 帧，最长就 2+gop
 * 
 * @param shm_buf shm_buf 句柄
 * @param frm_info 帧信息指针，返回帧信息
 * @param time_offset 时间偏移，返回当前帧与最新 utc 时间的差值
 * @return int 成功返回 0 失败 -1
 */
int handle_alarm_pre_record(shm_buf_t shm_buf, tSBFrame *frm_info, time_t *time_offset)
{
    if (frm_info == NULL) {
        ERR("frm_info error\n");
        return FAILURE;
    }
    /* 本函数的前提是底层写视频帧用的是 启动  时间，如果底层用的是utc时间，则函数失效*/
    td_u64 cur_pts = 0;
    ss_mpi_sys_get_cur_pts(&cur_pts);
    double ts_sec = cur_pts/1000000.0;
    unsigned short width = 0, height = 0; 
    int ret = FAILURE;
    int serial = -1;
    do {
        shm_buf_read_frame_add_vspps(shm_buf, serial, js_read_init_mp4, (void*)frm_info);
        if (SHM_ERR_SUCCESS == frm_info->error) {
            if (frm_info->frame_type == SHM_FRAME_VIDEO_I
                    && ts_sec - frm_info->frame_timestamp >= MIN_PRE_RECORD_TIME) {

                if (ts_sec - frm_info->frame_timestamp >= MAX_PRE_RECORD_TIME) {
                    serial = -1;
                    ret = FAILURE;
                    break;
                }
                /*只有前向 I 帧的宽高和当前 I 帧的宽高一致才进行预录制，不一样的话会花屏，不进行预录制*/
                if ((width == 0 && height == 0) // 第一个 I 帧满足条件
                        || (frm_info->v.width == width && frm_info->v.height == height)) {
                    ret = SUCCESS;
                    if (time_offset) {
                        *time_offset = round(ts_sec - frm_info->frame_timestamp);
                    }
                    DBG("pre record success, time_offset:%lf\n", ts_sec - frm_info->frame_timestamp);
                } else {
                    ERR("maybe vencsize change, return failure\n");
                    ret = FAILURE;
                }
                
                break;
            } else {
                if (width == 0 || height == 0) {
                    // 记录第一个 I 帧，也就是最新 I 帧的宽高
                    width = frm_info->v.width;
                    height = frm_info->v.height;
                }
                /*回滚找上一个 I 帧*/
                serial = frm_info->frame_serial - 1;
            }
            continue;
        } else if (SHM_ERR_OVER_WRITE == frm_info->error) {
            /*序号对应的帧小于 buf 最小的帧序号，无法找到满足条件的帧，返回失败*/
            SYSLOG("pre_record serial:%d overwrite\n", serial);
            break;
        } else if (SHM_ERR_NOT_READY == frm_info->error) {
            /*序号对应的帧大于 buf 最大帧序号，回滚一般不会出现这种情况，返回失败*/
            DBG("shm buf read frame:%d not ready\n", serial);
            break;
        } else if (SHM_ERR_SUCCESS != frm_info->error) {
            /*取帧错误，返回失败*/
            ERR("shm buf read frame:%d error:%d\n", serial, frm_info->error);
            break;
        } else {
            break;
        }
    } while (1);

    return ret;
}

static void js_read_init_audio(void *userdata, tSBFrame* tFrame)
{

    // 初始化mp4文件,只赋值需要的信息
    tSBFrame* pOutFrm = (tSBFrame*)userdata;

    pOutFrm->error           = tFrame->error;
    pOutFrm->frame_type      = tFrame->frame_type;
    pOutFrm->mediatype       = tFrame->mediatype;
    pOutFrm->frame_timestamp = tFrame->frame_timestamp;
    pOutFrm->frame_serial   = tFrame->frame_serial;
    pOutFrm->curframe_serial = tFrame->curframe_serial;

    return;
}

/**
 * @brief 通过视频时间戳查找音频帧号
 * 
 * @param abuf shm_buf 句柄
 * @param vframe_timestamp 视频帧时间戳
 * @return int 返回 音频帧
 */
int get_serial_by_vts(shm_buf_t abuf, double          vframe_timestamp)
{
    int serial = 0;
    tSBFrame tFrame = {SHM_ERR_FAILDE,};

    do {
        shm_buf_read_frame_ex(abuf, serial, js_read_init_audio, &tFrame);
        
        if (SHM_ERR_SUCCESS == tFrame.error) {
            
            serial = tFrame.frame_serial;
            if (vframe_timestamp > tFrame.frame_timestamp) {
                DBG("vframe_timestamp:%lf, serial:%d, tFrame.frame_timestamp:%lf\n", vframe_timestamp, serial, tFrame.frame_timestamp);
                break;
            }
            serial --;
            continue;
        } else if (SHM_ERR_OVER_WRITE == tFrame.error) {
            /*序号对应的帧小于 buf 最小的帧序号，无法找到满足条件的帧，返回最近音频帧*/
            SYSLOG("pre_record serial:%d overwrite\n", serial);
            serial = -2;
            break;
        } else if (SHM_ERR_NOT_READY == tFrame.error) {
            /*序号对应的帧大于 buf 最大帧序号，回滚一般不会出现这种情况，返回失败*/
            DBG("shm buf read frame:%d not ready\n", serial);
            serial = 0;
            break;
        } else if (SHM_ERR_SUCCESS != tFrame.error) {
            /*取帧错误，返回失败*/
            ERR("shm buf read frame:%d error:%d\n", serial, tFrame.error);
            serial = 0;
            break;
        } else {
            break;
        }
    } while (1);

    return serial;
}

static void init_read_audio_cb(void *userdata, tSBFrame* aFrame)
{
    tSBFrame* pOutFrm = (tSBFrame*)userdata;
    pOutFrm->mediatype       = aFrame->mediatype;
    pOutFrm->frame_type      = aFrame->frame_type;
    pOutFrm->frame_timestamp = aFrame->frame_timestamp;
    pOutFrm->frame_size      = aFrame->frame_size;
    pOutFrm->frame_serial    = aFrame->frame_serial;
    pOutFrm->curframe_serial = aFrame->curframe_serial;
    pOutFrm->error           = aFrame->error;

    if (aFrame->error != SHM_ERR_SUCCESS) {
        return;
    }

    pOutFrm->framedata       = aFrame->framedata;
}

int __js_init_mp4rec(js_record_t *jsrec)
{
    int ret = -1;

    int bps = 0;
    int fps = 0;
    eShmMediaType mediatype = SHM_MEDIA_UNKOWN;
    int width = 0;
    int height = 0;
    double time_start = 0;

    char sprop_parameter_sets_str[1024] = {0};

    char *vps_base64 = NULL;
    char *sps_base64 = NULL;
    char *pps_base64 = NULL;

    char *vpsData = NULL;
    char *spsData = NULL;
    char *ppsData = NULL;

    int vpsDataSize = 0;
    int spsDataSize = 0;
    int ppsDataSize = 0;

    eShmMediaType audiotype = SHM_MEDIA_UNKOWN;

    do {
        if(jsrec == NULL) {
            break;
        }

        if(jsrec->recparam.record_chn == 0)
            jsrec->vbuf = get_shm_buf_pool(SHM_BUF_MAIN);
        else if(jsrec->recparam.record_chn == 1)
            jsrec->vbuf = get_shm_buf_pool(SHM_BUF_SUB);

        if(jsrec->vbuf == NULL)
            break;

        int serial = get_cur_serial();
        js_log("init chn:%d mp4 rec serial:%d\n", jsrec->recparam.record_chn, serial);

        tSBFrame tFrame = {SHM_ERR_FAILDE,}, aFrame = {SHM_ERR_FAILDE,};
        bool is_pre_record = false;
        time_t time_offset = 0;

        jsrec->aserial = 0;

        if (jsrec->recparam.record_type == JREC_TYPE_ALARM) { 
            // 单报警录像需要支持预录制，
            ret = handle_alarm_pre_record(jsrec->vbuf, &tFrame, &time_offset);
            if (ret == SUCCESS && serial < tFrame.frame_serial) {
                is_pre_record = true;
                serial = tFrame.frame_serial;
                jsrec->recparam.start_utc -= time_offset; // 录像开始时间要减去预录制的时间
                jsrec->aserial = get_serial_by_vts(get_shm_buf_pool(SHM_BUF_AUDIO_AAC), tFrame.frame_timestamp);
                set_cur_aserial(jsrec->aserial);

                generate_record_filename(JREC_TYPE_ALARM,  // 开始时间变更，重新生成文件名
                                    jsrec->recparam.record_filename, jsrec->recparam.start_utc);
            } else {
                is_pre_record = false; // 预录制失败则走正常的录像逻辑
            }
        }

        do {
            if (is_pre_record) {
                /*预录制成功则不走正常的找 I 帧逻辑*/
                break;
            }

            shm_buf_read_frame_add_vspps(jsrec->vbuf, serial, js_read_init_mp4, (void*)&tFrame);
            if (SHM_ERR_SUCCESS == tFrame.error) {
                serial = tFrame.frame_serial;
                if (tFrame.frame_type == SHM_FRAME_VIDEO_I) {
                    break;
                }
            } else if (SHM_ERR_OVER_WRITE == tFrame.error) {
                SYSLOG("[WAR] record chn:%d serial:%d overwrite\n", jsrec->recparam.record_chn, serial);
                serial = -1;
                continue;
            } else if (SHM_ERR_NOT_READY == tFrame.error) {
                if (serial - tFrame.curframe_serial > 1) { // 防止切换通道的时候一直 not ready
                    DBG("cur_serial:%d dst_serial:%d set -1\n", tFrame.curframe_serial, serial);
                    set_cur_serial(-1);
                    set_cur_aserial(0);
                } else {
                    DBG("shm buf read frame:%d not ready\n", serial);
                }
                return FAILURE;
            } else if (SHM_ERR_SUCCESS != tFrame.error) {
                ERR("shm buf read frame:%d error:%d\n", serial, tFrame.error);
                return FAILURE;
            }

            serial++;
        } while (tFrame.frame_type != SHM_FRAME_VIDEO_I);

        set_cur_serial(serial);

        vpsData     = tFrame.v.vpsdata;
        spsData     = tFrame.v.spsdata;
        ppsData     = tFrame.v.ppsdata;
        vpsDataSize = tFrame.v.vpsdata_size;
        spsDataSize = tFrame.v.spsdata_size;
        ppsDataSize = tFrame.v.ppsdata_size;
        width       = tFrame.v.width;
        height      = tFrame.v.height;
        mediatype   = tFrame.mediatype;

        SYSLOG("init %s w:%d h:%d type:%d serial:%d, stamp: %lf\n",
               jsrec->mp4filename + strlen("/mnt/IPCamera/"), width, height, mediatype,
               serial, tFrame.frame_timestamp);
        js_log("frame vpsDataSize:%d spsDataSize:%d ppsDataSize:%d\n",
               vpsDataSize, spsDataSize, ppsDataSize);

        if(SHM_MEDIA_VIDEO_H265 == mediatype) {
            vps_base64 = recbase64Encode(vpsData+4, vpsDataSize-4);
            sps_base64 = recbase64Encode(spsData+4, spsDataSize-4);
            pps_base64 = recbase64Encode(ppsData+4, ppsDataSize-4);

            if(vps_base64 || sps_base64 || pps_base64) {
                strcpy(sprop_parameter_sets_str, vps_base64);
                sprop_parameter_sets_str[strlen(vps_base64)] = ',';

                strcpy(sprop_parameter_sets_str + strlen(vps_base64) +1, sps_base64);
                sprop_parameter_sets_str[strlen(vps_base64) +1 + strlen(sps_base64)] = ',';

                strcpy(sprop_parameter_sets_str + strlen(vps_base64) +1 + strlen(sps_base64)+1, pps_base64);

                strcpy(sprop_parameter_sets_str + strlen(vps_base64) +1 + strlen(sps_base64)+1 + strlen(pps_base64), ";HEVC");
            }
        } else if(SHM_MEDIA_VIDEO_H264 == mediatype) {
            sps_base64 = recbase64Encode(spsData+4, spsDataSize-4);
            pps_base64 = recbase64Encode(ppsData+4, ppsDataSize-4);

            if(sps_base64 || pps_base64) {
                strcpy(sprop_parameter_sets_str, sps_base64);
                sprop_parameter_sets_str[strlen(sps_base64)] = ',';

                strcpy(sprop_parameter_sets_str + strlen(sps_base64) +1,pps_base64);
            }
        } else {
            js_log("unkown mediatype:%d!\n", mediatype);
            break;
        }
/*
        if(vps_base64)
            js_b64_free((unsigned char *)vps_base64);
        if(sps_base64)
            js_b64_free((unsigned char *)sps_base64);
        if(pps_base64)
            js_b64_free((unsigned char *)pps_base64);
*/
        if(vps_base64) {
            delete[] pps_base64;
        }
        if(sps_base64) {
            delete[] sps_base64;
        }
        if(pps_base64) {
            delete[] vps_base64;
        }
        js_log("base 64 encode success!\n");

        jsrec->mp4writer = new CMP4Write;
        ret = jsrec->mp4writer->Open(jsrec->mp4filename);
        if(ret < 0) {
            js_log("mp4 open %s error:%s\n", jsrec->mp4filename, strerror(errno));
            break;
        }

        ret = jsrec->mp4writer->InitVideo(96, CSize(width, height), sprop_parameter_sets_str);
        if(ret < 0) {
            js_log("mp4 init video :%s error\n", sprop_parameter_sets_str);
            break;
        }

        if (jsrec->recparam.need_audio_record != 0) {
            jsrec->abuf = get_shm_buf_pool(SHM_BUF_AUDIO_AAC);
            shm_buf_get_media_info(jsrec->abuf, &audiotype, &bps, &fps, &width, &height);

            int read_times = 0;
            do {
                read_times++;
                shm_buf_read_frame_add_vspps(jsrec->abuf, get_cur_aserial(), init_read_audio_cb, (void*)&aFrame);
                if (SHM_ERR_SUCCESS != aFrame.error && read_times >= 30) {
                    set_cur_aserial(0);
                    break;
                }

                if (aFrame.frame_timestamp >= tFrame.frame_timestamp) {
                    set_cur_aserial(aFrame.frame_serial);
                    break;
                }

                set_cur_aserial(aFrame.frame_serial + 1);
            } while(1);

            js_log("mp4 init seek audio serial:%d tamp:%lf \n", get_cur_aserial(), aFrame.frame_timestamp);

            if(audiotype == SHM_MEDIA_AUDIO_ALAW) {
                jsrec->mp4writer->InitAudio(0);
            } else if(audiotype == SHM_MEDIA_AUDIO_ULAW) {
                jsrec->mp4writer->InitAudio(0);
            } else if (audiotype == SHM_MEDIA_AUDIO_AAC) {
                jsrec->mp4writer->InitAudio(RTP_TYPE_AAC);
            } else {
                js_log("unkown audiotype:%d!\n", audiotype);
            }
        }

        js_log("[%p]videotype:%d audiotype:%d\n", jsrec, mediatype, audiotype);

        time_start = mono_stamp();
        //must init audio, then write first frame
        jsrec->mediatype = mediatype;
        if(SHM_MEDIA_VIDEO_H265 == mediatype) {
            jsrec->vpsDataSize = vpsDataSize;
            memcpy(jsrec->vpsData, vpsData, vpsDataSize);
            jsrec->spsDataSize = spsDataSize;
            memcpy(jsrec->spsData, spsData, spsDataSize);
            jsrec->ppsDataSize = ppsDataSize;
            memcpy(jsrec->ppsData, ppsData, ppsDataSize);
        } else {
            jsrec->spsDataSize = spsDataSize;
            memcpy(jsrec->spsData, spsData, spsDataSize);
            jsrec->ppsDataSize = ppsDataSize;
            memcpy(jsrec->ppsData, ppsData, ppsDataSize);
        }
        DBG("write frame info timestamp: %lf\n", mono_stamp() - time_start);
        ret = 0;
    } while(0);

    return ret;
}

static void js_close_mp4(js_record_t *jsrec)
{
    char *p = NULL;
    char chtype = 'M';
    int ret = 0;

    if (NULL == jsrec){
        return;
    }

    jsrec->mp4writer->Close();
    delete jsrec->mp4writer;
    jsrec->mp4writer = NULL;

    js_log("record file:%s len:%d\n", jsrec->mp4filename, jsrec->timelen);
    if(jsrec->timelen <= 3 || bytes_of_file(jsrec->mp4filename) < 10 * 1024){
        /* 工厂测试当无帧时，出现很多时长 3 秒大小为 0 的录像，这里删除小于等于三秒，大小小于 10K 的录像*/
        SYSLOG("remove record file:%s\n", jsrec->mp4filename);
        remove(jsrec->mp4filename);
    }else {
        p = strstr(jsrec->recparam.record_filename, TMPFILE_SUFFIX);
        if(p != NULL) {
            sprintf(p,"-%04d.mp4", jsrec->timelen);
        }

        switch(jsrec->recparam.record_type){
            case JREC_TYPE_ALARM:
                chtype = 'S';
                break;
            case JREC_TYPE_SCHEDULE:
                chtype = 'S';
                break;
            case JREC_TYPE_MANUAL:
                chtype = 'S';
                break;
        }

        p = strchr(jsrec->recparam.record_filename, '-');
        if(p != NULL){
            p --;
            *p = chtype;
        }

        js_log("record file:%s rename to :%s\n", jsrec->mp4filename, jsrec->recparam.record_filename);
        ret = rename(jsrec->mp4filename, jsrec->recparam.record_filename);
        if (ret < 0) {
            set_g_stat(record, SD_REC_TMP_REPAIR);
            SYSLOG("rename from %s to %s fail:%s\n", jsrec->mp4filename, jsrec->recparam.record_filename, strerror(errno));
        }
    }
    set_g_stat(record, SD_REC_RENAME);
    DBG("js_close_mp4 success\n");
}

static void cb_close_mp4(void *date)
{
    js_record_t *jsrec_bac = (js_record_t *)date;

    if(jsrec_bac == NULL) return;

    js_close_mp4(jsrec_bac);

    free(jsrec_bac);
}

static void __js_uninit_mp4rec(js_record_t *jsrec)
{
    do {
        if(jsrec == NULL)
            break;

        js_delete_timer_r(&jsrec->vhandle);
        js_delete_timer_r(&jsrec->ahandle);

        if(jsrec->mp4writer == NULL) {
            break;
        }

        jsrec->timelen = sec_since_previous(&jsrec->starttime);
        set_cur_serial(jsrec->vserial);
        set_cur_aserial(jsrec->aserial);

        if(jsrec->scheduler_stop != NULL) {
            //后台关闭录像，因为偶尔录像关闭耗时会比较长，会影响下一段录像开始
            DBG("[%d] async close record serial: %d\n", jsrec->recparam.record_chn, get_cur_serial());
            js_record_t *jsrec_bac = (js_record_t *)malloc(sizeof(js_record_t));
            memcpy(jsrec_bac, jsrec, sizeof(js_record_t));
            js_run_function(jsrec->scheduler_stop, cb_close_mp4, jsrec_bac, 0);
        } else {
            js_close_mp4(jsrec);
        }

        jsrec->mp4writer = NULL;
        jsrec->vserial = -1;
        jsrec->aserial = 0;
        jsrec->aframenums = 0;
        jsrec->totalsize = 0;
    } while(0);
}

static void __js_do_read_video(void *userdata, tSBFrame* tFrame)
{
    js_record_t *jsrec = (js_record_t *)userdata;
    int cur_record_time = 0;
    int ret;

    if(jsrec == NULL)
        return;


    if(tFrame->mediatype == SHM_MEDIA_UNKOWN ||
       (jsrec->vserial > 0 && tFrame->curframe_serial + 2 < jsrec->vserial)) {
        // the buf has been reset
        js_log("[%p]the vbuf has been reset! tFrame->mediatype:%d tFrame->curframe_serial:%d vserial:%d\n",
               jsrec, tFrame->mediatype, tFrame->curframe_serial, jsrec->vserial);
        goto err;
    }

    switch(tFrame->error) {
        case SHM_ERR_SUCCESS:
            jsrec->totalsize += tFrame->frame_size;
            jsrec->vserial = tFrame->frame_serial;
            jsrec->vserial ++;

            if(jsrec->vfirsttimestamp == 0) {
                jsrec->notready++;
                if (jsrec->notready * VEDIO_LOOP > 2 * 1000 ) {
                    //录像时间小于三秒丢弃，两秒取不到第一帧，停止这段录像
                    js_log("No first frame is read for %d ms\n", jsrec->notready * VEDIO_LOOP);
                    goto err;
                }

                if(tFrame->frame_type != SHM_FRAME_VIDEO_I) {
                    //js_log("wait for first I frame\n");
                    //jsrec->vserial = -1;
                    break;
                }
                if(tFrame->frame_serial == g_last_I_frame) {
                   //js_log("wait for latest I frame\n");
                   //jsrec->vserial = -1;
                   break;
                }

                //获取到第一个 I 帧后写入帧头信息
                if (SHM_MEDIA_VIDEO_H265 == jsrec->mediatype) {
                    jsrec->mp4writer->WriteFrame( 1, (LPBYTE)jsrec->vpsData, jsrec->vpsDataSize, 10);
                    jsrec->mp4writer->WriteFrame( 1, (LPBYTE)jsrec->spsData, jsrec->spsDataSize, 20);
                    jsrec->mp4writer->WriteFrame( 1, (LPBYTE)jsrec->ppsData, jsrec->ppsDataSize, 30);
                } else {
                    jsrec->mp4writer->WriteFrame( 1, (LPBYTE)jsrec->spsData, jsrec->spsDataSize, 10);
                    jsrec->mp4writer->WriteFrame( 1, (LPBYTE)jsrec->ppsData, jsrec->ppsDataSize, 20);
                }
                jsrec->vfirsttimestamp = tFrame->frame_timestamp;
                jsrec->lastwidth       = tFrame->v.width;
                jsrec->lastheight      = tFrame->v.height;
            }

            if (tFrame->frame_type == SHM_FRAME_VIDEO_I) {
                if (tFrame->v.width != jsrec->lastwidth || tFrame->v.height != jsrec->lastheight) {
                    jsrec->vserial--;
                    js_log("cut resolution %d to %d vserial:%d, need stop\n",
                            jsrec->lastwidth, tFrame->v.width, jsrec->vserial);
                    goto stop;
                }

                cur_record_time = sec_since_previous(&jsrec->starttime);
                if(cur_record_time + jsrec->recparam.I_frame_interval > jsrec->recparam.record_time) { //收到最后一个gop内的I帧，结束录像
                    jsrec->vserial--;
                    js_log("Timeout close recording\n");
                    goto stop;
                }

                if(jsrec->totalsize >= 512*1024*1024) {
                    jsrec->vserial--;
                    js_log("[%p]mp4 size big than 512M, so goto close!\n", jsrec);
                    goto err;
                }

                g_last_I_frame = tFrame->frame_serial;  //update last I frame
            }

            //js_log("[%p]vserial:%d frame_size:%d type:%d time:%f!\n",
            //          jsrec, jsrec->vserial, tFrame->frame_size, tFrame->frame_type, tFrame->frame_timestamp);
            ret = jsrec->mp4writer->WriteFrame( 1, (LPBYTE)tFrame->framedata, tFrame->frame_size,
                                                (tFrame->frame_timestamp - jsrec->vfirsttimestamp)*90000);
            if(ret < 0) {
                get_g_stat(record, SD_ERR_WRITE);
                js_log("[%p]mp4 write v frame err!\n", jsrec);
                goto err;
            }

            jsrec->notready = 0;
            break;

        case SHM_ERR_NOT_READY:
            jsrec->notready++;
            if (jsrec->notready * VEDIO_LOOP > 5 * 1000 ) {
                //检测 5 秒获取不到帧，停止录像
                js_log("No frame is read for %d ms\n", jsrec->notready * VEDIO_LOOP);
                goto err;
            }
            break ;

        case SHM_ERR_OVER_WRITE:
            js_log("[%p]read vframe serial:%d over write!\n", jsrec, jsrec->vserial);
            jsrec->vserial = -1;
            break ;

        default:
            js_log("read vframe data error:%d!\n", tFrame->error);
            break;
    }
    return ;
stop:
err:
    js_log("[%p]close mp4 write!\n", jsrec);
    __js_uninit_mp4rec(jsrec);
    return;
}

static void __js_read_video(void *userdata)
{
    js_record_t *jsrec = (js_record_t *)userdata;

    if(jsrec == NULL)
        return;

    shm_buf_read_frame_ex(jsrec->vbuf, jsrec->vserial, __js_do_read_video, jsrec);
}

static void __js_do_read_audio(void *userdata, tSBFrame* tFrame)
{
    js_record_t *jsrec = (js_record_t *)userdata;
    int ret = 0;
    unsigned int  i;
    unsigned char framebufTmp[2048];

    if(jsrec == NULL)
        return;

    if(tFrame->mediatype == SHM_MEDIA_UNKOWN ||
       (jsrec->aserial> 0 && tFrame->curframe_serial + 10 < jsrec->aserial)) {
        // the buf has been reset
        js_log("[%p]the abuf has been reset! tFrame->mediatype:%d tFrame->curframe_serial:%d aserial:%d\n",
               jsrec, tFrame->mediatype, tFrame->curframe_serial, jsrec->aserial);
        goto err;
    }

    switch(tFrame->error) {
        case SHM_ERR_SUCCESS:
            //js_log("[%p]aserial:%d frame_size:%d type:%d time:%f framenums:%d!\n",
            //      jsrec, jsrec->aserial, tFrame->frame_size, tFrame->frame_type, tFrame->frame_timestamp, jsrec->aframenums);
            //js_log("[%p]audio serail:%d type:%d %02x %02x %02x %02x %02x %02x\n", jsrec, jsrec->aserial, tFrame->mediatype,
            //  (unsigned char)tFrame->framedata[0], (unsigned char)tFrame->framedata[1],
            //  (unsigned char)tFrame->framedata[2], (unsigned char)tFrame->framedata[3],
            //  (unsigned char)tFrame->framedata[4], (unsigned char)tFrame->framedata[5]);

            if(jsrec->vfirsttimestamp == 0)
                break;

            jsrec->totalsize += tFrame->frame_size;
            if(jsrec->aserial <= 0)
                jsrec->aserial = tFrame->frame_serial;
            jsrec->aserial ++;

            if(jsrec->afirsttimestamp == 0)
                jsrec->afirsttimestamp = tFrame->frame_timestamp;

            if(tFrame->mediatype == SHM_MEDIA_AUDIO_ALAW) {
                for(i=0; i<tFrame->frame_size; i++) {
                    framebufTmp[i] = jco_alaw2ulaw(tFrame->framedata[i]);
                }
                //ret = jsrec->mp4writer->WriteFrameEx(0, (LPBYTE)framebufTmp, tFrame->frame_size, (DWORD32)((tFrame->frame_timestamp - jsrec->afirsttimestamp) * 8000));
                ret = jsrec->mp4writer->WriteFrameEx(0, (LPBYTE)framebufTmp, tFrame->frame_size, (DWORD32)(jsrec->aframenums * tFrame->frame_size));
            } else if(tFrame->mediatype == SHM_MEDIA_AUDIO_ULAW) {
                //ret = jsrec->mp4writer->WriteFrameEx(0, (LPBYTE)tFrame->framedata, tFrame->frame_size, (DWORD32)((tFrame->frame_timestamp - jsrec->afirsttimestamp) * 8000));
                ret = jsrec->mp4writer->WriteFrameEx(0, (LPBYTE)tFrame->framedata, tFrame->frame_size, (DWORD32)(jsrec->aframenums * tFrame->frame_size));
            }  else if(tFrame->mediatype == SHM_MEDIA_AUDIO_AAC) {
                //ret = jsrec->mp4writer->WriteFrameEx(0, (LPBYTE)tFrame->framedata, tFrame->frame_size, (DWORD32)((tFrame->frame_timestamp - jsrec->afirsttimestamp) * 8000));
                ret = jsrec->mp4writer->WriteFrameEx(0, (LPBYTE)tFrame->framedata + AAC_HEAD_INFO, tFrame->frame_size - AAC_HEAD_INFO, (DWORD32)(tFrame->frame_timestamp));
            }

            if(ret < 0) {
                js_log("[%p]mp4 write a frame err!\n", jsrec);
            }

            jsrec->aframenums ++;
            break;

        case SHM_ERR_NOT_READY:
            break;

        case SHM_ERR_OVER_WRITE:
            js_log("[%p]read aframe serial:%d over write!\n", jsrec, jsrec->aserial);
            jsrec->aserial = 0;
            break ;

        default:
            js_log("[%p]read aframe data error:%d!\n", jsrec, tFrame->error);
            break ;
    }
    return ;
err:
    js_log("[%p]close mp4 write!\n", jsrec);
    __js_uninit_mp4rec(jsrec);
    return;
}

static void __js_read_audio(void *userdata)
{
    js_record_t *jsrec = (js_record_t *)userdata;

    if(jsrec == NULL)
        return;

    shm_buf_read_frame_ex(jsrec->abuf, jsrec->aserial, __js_do_read_audio, jsrec);
}

JSRecHandle* js_start_record_mp4(JSScheduler scheduler, JSScheduler scheduler_stop, JRecParamInfo *info)
{
    js_record_t *jsrec = NULL;
    time_t curTime;
    struct tm curTm_s;
    struct tm *tmTime = &curTm_s;

    do {
        if(info == NULL)
            break;

        jsrec = (js_record_t *)malloc(sizeof(js_record_t));
        if(jsrec == NULL)
            break;

        memset(jsrec, 0, sizeof(js_record_t));

        jsrec->scheduler = scheduler;
        if(jsrec->scheduler == NULL) {
            jsrec->createflags = 1;
            jsrec->scheduler = js_create_scheduler((char *)"mp4 record");
        }
        jsrec->scheduler_stop = scheduler_stop;

        memcpy(&jsrec->recparam, info ,sizeof(jsrec->recparam));
        snprintf(jsrec->mp4filename, sizeof(jsrec->mp4filename) - 1, "%s", info->record_filename);

        //异步删除特殊情况下会出现，start 创建文件被异步 remove 导致丢一段录像
        if (is_okey(jsrec->mp4filename)) {
            free(jsrec);
            jsrec = NULL;
            break;
        }

        if(__js_init_mp4rec(jsrec) < 0) {
            __js_uninit_mp4rec(jsrec);
            free(jsrec);
            jsrec = NULL;
            break;
        }
        curTime = jsrec->recparam.start_utc;
        localtime_r(&curTime, &curTm_s);
        sprintf(jsrec->startday, "%4d%02d%02d", tmTime->tm_year + 1900, tmTime->tm_mon + 1, tmTime->tm_mday);

        ms_clock_reset(&jsrec->starttime);
        js_log("start rec file:%s startday:%s starttime:%lld recordtime:%d\n",
            jsrec->mp4filename, jsrec->startday, jsrec->starttime.tv_sec, jsrec->recparam.record_time);

        jsrec->notready = 0;
        jsrec->vserial = get_cur_serial();
        jsrec->aserial = get_cur_aserial();
        jsrec->vfirsttimestamp = 0;
        jsrec->afirsttimestamp = 0;
        jsrec->aframenums = 0;
        js_create_timer_r(jsrec->scheduler, 40, VEDIO_LOOP, __js_read_video, jsrec, &jsrec->vhandle);
        if(jsrec->abuf != NULL)
            js_create_timer_r(jsrec->scheduler, 50, AUDIO_LOOP, __js_read_audio, jsrec, &jsrec->ahandle);
    } while(0);

    return (JSRecHandle *)jsrec;
}

void js_stop_record_mp4(void *recsess)
{
    js_record_t *jsrec = (js_record_t *)recsess;

    do {
        if(jsrec == NULL)
            break;

        __js_uninit_mp4rec(jsrec);

        if(jsrec->createflags) {
            js_delete_scheduler(jsrec->scheduler);
            jsrec->scheduler = NULL;
        }

        free(jsrec);
    } while(0);

    return;
}

JRecParamInfo* js_get_record_recparaminfo(JSRecHandle *recsess)
{
    js_record_t *jsrec = (js_record_t *)recsess;
    JRecParamInfo *paraminfo = NULL;

    do {
        if(jsrec == NULL)
            break;

        paraminfo = &jsrec->recparam;

    } while(0);

    return paraminfo;
}

int js_get_record_pasttime(JSRecHandle *recsess)
{
    js_record_t *jsrec = (js_record_t *)recsess;
    int pasttimes = 0;

    do {
        if(jsrec == NULL) {
           break;
        }
        pasttimes = sec_since_previous(&jsrec->starttime);
    } while(0);

    return pasttimes;
}


int js_is_record_corssday(JSRecHandle *recsess)
{
    js_record_t *jsrec = (js_record_t *)recsess;

    time_t curTime;
    struct tm curTm_s;
    struct tm *tmTime = &curTm_s;

    char curday[16] = {0,};
    int ret = 0;

    do {
        if(jsrec == NULL)
            break;

        curTime = time(NULL);
        localtime_r(&curTime, &curTm_s);
        sprintf(curday, "%4d%02d%02d", tmTime->tm_year + 1900, tmTime->tm_mon + 1, tmTime->tm_mday);

        if(strcmp(curday, jsrec->startday) != 0){
            js_log("startday:%s, curday:%s\n", jsrec->startday, curday);
            ret = 1;
        }

    } while(0);

    return ret;
}

int js_is_record_stoped(JSRecHandle *recsess)
{
    js_record_t *jsrec = (js_record_t *)recsess;
    int stoped = 0;

    do {
        if(jsrec == NULL)
            break;

        if(jsrec->mp4writer == NULL){
            js_log("record:%p file:%s stoped!\n", jsrec, jsrec->mp4filename);
            stoped = 1;
        }

    } while(0);

    return stoped;
}

int js_clear_scheduler_stop(JSRecHandle *recsess)
{
    js_record_t *jsrec = (js_record_t *)recsess;
    if (jsrec != NULL) {
        jsrec->scheduler_stop = NULL;
    }

    return 0;
}

struct timespec *js_get_record_start_time(JSRecHandle *recsess)
{
    js_record_t *jsrec = (js_record_t *)recsess;
    struct timespec *start_time = NULL;

    do {
        if(jsrec == NULL)
            break;
        start_time = &jsrec->starttime;
    } while(0);
    return start_time;
}

void js_sync_cur_mp4info(void *data)
{
    js_record_t *jsrec = (js_record_t *)data;

    do {
        if (jsrec == NULL || jsrec->mp4writer == NULL) {
            break;
        }

        jsrec->mp4writer->Sync();
    } while (0);

    return;
}
