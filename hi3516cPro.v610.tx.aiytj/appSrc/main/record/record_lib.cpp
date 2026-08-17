#include <dirent.h>
#include <errno.h>

#include "shm_buf.h"
#include "encode_main.h"
#include "record_disk.h"
#include "record_lib.h"
#include "record_dirent.h"
#include "debug.h"
#include "jconfstruct.h"
#include "utils.h"
#include "confapi.h"
#include "g_sys.h"
#include "record_file_manage.h"
#include "time_config.h"

static sRecTime g_rec_plans[2][7] = {{{0}}};
struct record_cache {
    int      yyyymmdd;   //年月日
    int      num[MAX_SENSOR_NUM];     //数量
    sRec1File reclist[MAX_SENSOR_NUM][MAX_RECS_OF_DAY];
};

static struct record_cache g_record_cache = {0};   //缓存录像列表
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static void sync_cur_recmp4_header(const char *path)
{
    int ret = SUCCESS;
    char recpath[256] = {0};

    do {
        ret = record_get_cur_recfile_path(recpath);
        if (FAILURE == ret) {
            break;
        }

        if (0 != memcmp(recpath, path, strlen(recpath))) {
            break;
        }

        record_sync_cur_mp4info();  // 刷新当前正在录的文件的头部信息
    } while (0);

    return;
}

/*下次录像查询时强制更新列表*/
void record_clr_cache_list(int yyyymmdd)
{
    pthread_mutex_lock(&lock);
    DBG("update record list, cache:%d  cur:%d\n", g_record_cache.yyyymmdd, yyyymmdd);
    if (g_record_cache.yyyymmdd == yyyymmdd) {
        g_record_cache.yyyymmdd = 0;
    }

    pthread_mutex_unlock(&lock);
    return ;
}
CMP4Read *record_open_file(char *path, bool *bAudio, unsigned int *audio_fps, tagMP4AudioInfo *audio_info,
                                    int *h26x, double *video_fps, tagMP4VideoInfo* video_info)
{
    int             ret = 0;
    CMP4Read        *pCMP4Read = NULL;
    tagMP4VideoInfo videoInfo;
    tagMP4AudioInfo audioInfo;

    sync_cur_recmp4_header(path);

    pCMP4Read = new CMP4Read;
    ret = pCMP4Read->Open(path);
    if (ret < 0) {
        ERR("open mp4 file :%s error\n", path);
        delete pCMP4Read;
        pCMP4Read = NULL;
        return NULL;
    }

    pCMP4Read->GetAudioInfo(audioInfo);
    ret = (int)audioInfo.frame_count;
    if (ret == -1) {    //  获取不到frame_count直接不能播放，防止不能播放的mp4导致段错误
        ERR("open mp4 file :%s error, audio count:%u\n", path, audioInfo.frame_count);
        delete pCMP4Read;
        pCMP4Read = NULL;
        return NULL;
    }
    DBG("audio duration: %d; frame_count:%u\n", audioInfo.duration,audioInfo.frame_count);
    if (bAudio) {
        *bAudio = audioInfo.bValid;
        if (audioInfo.frame_count) {
            *audio_fps = audioInfo.duration/audioInfo.frame_count;
        }
    }
    pCMP4Read->GetVideoInfo(videoInfo);
    ret = (int)videoInfo.frame_count;
    if (ret == -1) {
        ERR("open mp4 file :%s error, video count:%u\n", path, videoInfo.frame_count);
        delete pCMP4Read;
        pCMP4Read = NULL;
        return NULL;
    }

    if (videoInfo.video_type == eVideoType_H264) {
        *h26x = VIDEO_H264;
    } else if (videoInfo.video_type == eVideoType_H265) {
        *h26x = VIDEO_H265;
    }
    *video_fps = videoInfo.fps;
    DBG("video: fps:%f; timescale:%u; duration:%u; frame_count:%u\n", videoInfo.fps, videoInfo.timescale, videoInfo.duration,videoInfo.frame_count);

    memcpy(audio_info,&audioInfo,sizeof(tagMP4AudioInfo));
    memcpy(video_info,&videoInfo,sizeof(tagMP4VideoInfo));

    return pCMP4Read;
}

int record_close_file(CMP4Read *pCMP4Read)
{
    if (pCMP4Read) {
        pCMP4Read->Close();
        delete pCMP4Read;
        pCMP4Read = NULL;
    }

    return 0;
}

int record_read_vframe(CMP4Read *pCMP4Read, int *serial, char *buf, size_t buf_sz, int *frame_sz, int *keyframe, size_t *duration, int h26x)
{
    int ret = 0;
    int cbRead;
    int frame_count = 0;
    DWORD32 duration_v = 0;

    if (buf == NULL || frame_sz == NULL || pCMP4Read == NULL || duration == NULL) {
        DBG("p NULL in %p %p %p @dura %p\n", buf,  frame_sz, pCMP4Read, duration);
        return -1;
    }

    *duration = 0;
    
    while (1) {
        ret = pCMP4Read->ReadVideoFrame(LPBYTE(buf + 4), buf_sz - 4, cbRead, duration_v);
        if (ret < 0 || cbRead <= 0) {
            DBG("ret: %d cbRead: %d\n", ret, cbRead);
            return -1;
        }

        buf[0] = 0;
        buf[1] = 0;
        buf[2] = 0;
        buf[3] = 1;
        frame_count++;

        if(((7 == (buf[4]&0x1f) || 5 == (buf[4]&0x1f)) && VIDEO_H264 == h26x) || (0x26 == buf[4] && VIDEO_H265 == h26x)) {
            *keyframe = SHM_FRAME_VIDEO_I;
        } else {
            *keyframe = SHM_FRAME_VIDEO_P;
        }

        *duration += duration_v;

        if (*serial == -1 && *keyframe != SHM_FRAME_VIDEO_I) {
            continue;
        }

        /* 腾讯播放器遇到 vps-sps-pps 会有播放异常，提示参数，so 跳过 */
        if (*keyframe == SHM_FRAME_VIDEO_I && cbRead < 50) {
            DBG("size :%d\n", cbRead);
            continue;
        }

        break;
    }

    *frame_sz = cbRead+5; //2019.08.20 fixed: the data is not complete.
    return frame_count;
}

int record_read_aframe(CMP4Read *pCMP4Read, char *buf, size_t buf_sz, int *frame_sz, size_t *duration)
{
    int         cbRead;
    DWORD32     duration_a;
    int         ret = 0;
    
    if (buf == NULL || frame_sz == NULL || pCMP4Read == NULL) {
        return -1;
    }

    while (1) {
        ret = pCMP4Read->ReadAudioFrame(LPBYTE(buf), buf_sz, cbRead, duration_a);
        if(ret < 0 || cbRead <= 0){
            return -1;
        }

        if(duration){
            *duration = duration_a;
        }

        break;
    }

    *frame_sz = cbRead;
    return 0;
}

int record_seek_file(CMP4Read *pCMP4Read, int offset_time)
{
    int frameno = 0;
    tagMP4VideoInfo videoInfo;

    if (pCMP4Read == NULL) {
        return -1;
    }

    if (offset_time < 0) {
        return -1;
    }
    pCMP4Read->GetVideoInfo(videoInfo);
    pCMP4Read->SeekToDuration(offset_time * videoInfo.timescale);
    pCMP4Read->SeekToIFrame();

    frameno = pCMP4Read->GetCurVideoFrame();
    if (frameno < 0) {
        // mp4 库查询帧号会返回下一帧要取的帧号减一，当下一帧是第 0 帧时会返回 -1，需要置为 0
        frameno = 0;
    }

    DBG("seek frameno:%d  offset_time:%d\n", frameno, offset_time);
    return frameno;
}

int record_get_curfarme(CMP4Read *pCMP4Read)
{
    if (pCMP4Read == NULL) {
        return -1;
    }

    return pCMP4Read->GetCurVideoFrame();
}

int record_get_timestamp_ms(CMP4Read *pCMP4Read)
{
    tagMP4VideoInfo videoInfo;
    pCMP4Read->GetVideoInfo(videoInfo);
    int record_timestamp = 0;

    int i = pCMP4Read->GetCurVideoFrame();
    if (videoInfo.fps) {
        record_timestamp = i*1000/videoInfo.fps;
    }

    return record_timestamp;
}

int record_get_pps_sps(CMP4Read *pCMP4Read, sSpsPpsInfo *info)
{
    if (pCMP4Read == NULL) {
        return -1;
    }

    if (pCMP4Read->GetExtData(eExtData_sps, (LPBYTE)info->sps_buf, sizeof(info->sps_buf), info->sps_size) < 0) {
        ERR("GetExtData sps fail\n");
        return -1;
    }

    if (pCMP4Read->GetExtData(eExtData_pps, (LPBYTE)info->pps_buf, sizeof(info->pps_buf), info->pps_size) < 0) {
        ERR("GetExtData pps fail\n");
        return -1;
    }

    memmove(info->sps_buf+4, info->sps_buf, info->sps_size);
    info->sps_buf[0] = 0;
    info->sps_buf[1] = 0;
    info->sps_buf[2] = 0;
    info->sps_buf[3] = 1;
    info->sps_size += 4;
    memmove(info->pps_buf+4, info->pps_buf, info->pps_size);
    info->pps_buf[0] = 0;
    info->pps_buf[1] = 0;
    info->pps_buf[2] = 0;
    info->pps_buf[3] = 1;
    info->pps_size += 4;

    return 0;
}

int record_get_vps_pps_sps(CMP4Read *pCMP4Read, sVpsSpsPpsInfo *info)
{
    if (pCMP4Read == NULL) {
        return -1;
    }

    if (pCMP4Read->GetExtData(eExtData_vps, (LPBYTE)info->vps_buf, sizeof(info->vps_buf), info->vps_size) < 0) {
        ERR("GetExtData pps fail\n");
        return -1;
    }

    if (pCMP4Read->GetExtData(eExtData_sps, (LPBYTE)info->sps_buf, sizeof(info->sps_buf), info->sps_size) < 0) {
        ERR("GetExtData sps fail\n");
        return -1;
    }

    if (pCMP4Read->GetExtData(eExtData_pps, (LPBYTE)info->pps_buf, sizeof(info->pps_buf), info->pps_size) < 0) {
        ERR("GetExtData pps fail\n");
        return -1;
    }

    memmove(info->vps_buf+4, info->vps_buf, info->vps_size);
    info->vps_buf[0] = 0;
    info->vps_buf[1] = 0;
    info->vps_buf[2] = 0;
    info->vps_buf[3] = 1;
    info->vps_size+= 4;
    memmove(info->sps_buf+4, info->sps_buf, info->sps_size);
    info->sps_buf[0] = 0;
    info->sps_buf[1] = 0;
    info->sps_buf[2] = 0;
    info->sps_buf[3] = 1;
    info->sps_size += 4;
    memmove(info->pps_buf+4, info->pps_buf, info->pps_size);
    info->pps_buf[0] = 0;
    info->pps_buf[1] = 0;
    info->pps_buf[2] = 0;
    info->pps_buf[3] = 1;
    info->pps_size += 4;


    DBG("vps size:%d, sps size :%d, pps size :%d\n", info->vps_size, info->sps_size, info->pps_size);
    return 0;
}

int record_get_mp4_secs(const char *path, uint32_t *secs)
{
    int ret = 0;
    tagMP4VideoInfo videoInfo;
    float times = 0;

    CMP4Read* CMP4 = new CMP4Read;
    sync_cur_recmp4_header(path);
    ret = CMP4->Open(path);
    if(ret < 0){
        ERR("open mp4 file :%s error\n", path);
        delete CMP4;
        CMP4 = NULL;
        return -1;
    }

    CMP4->GetVideoInfo(videoInfo);
    if(videoInfo.timescale){
        times = videoInfo.duration / videoInfo.timescale;
    }

    CMP4->Close();
    delete CMP4;
    CMP4 = NULL;
    
    *secs = (uint32_t)times;

    return 0;
}


static int cb_compmi(const void *m1, const void *m2)
{
    /*文件校验*/
    sRec1File *i1 = (sRec1File *) m1;
    sRec1File *i2 = (sRec1File *) m2;

    if (i1->start_time >= i2->start_time && i1->start_time <= i2->stop_time) {
       return 0;
    } else if (i1->start_time < i2->start_time) {
       return -1;
    } else {
       return 1;
    }
}

int record_set_len_of_tmpfile(sRec1File *respinfo)
{
    if (respinfo == NULL) {
        return -1;
    }

    char *p = strstr(respinfo->file_name, TMPFILE_SUFFIX);
    if (p != NULL) {
        DBG("file_secs = %d\n", respinfo->file_secs);
        sprintf(p,"-%04d.mp4", respinfo->file_secs);
    }

    return 0;
}

// add because: new api and struct
static int rebuild_newlist(int yyyymmdd, struct record_cache *reclist_info) //20210617,偏移时间，保存的结构体，576
{
    int total_num = 0;
    struct tm start = {0};
    start.tm_mday = yyyymmdd%100;
    start.tm_mon  = (yyyymmdd%10000)/100 - 1;
    start.tm_year = yyyymmdd/10000 - 1900;   // 从1900年开始计算的年份
    uint32_t secs_ymd = mktime(&start); //秒数;
                                        //
    sCache1File *f_mp4s = (sCache1File *)system_malloc(sizeof(sCache1File)*MAX_RECS_OF_DAY);

    for (int chn = 0; chn < MAX_SENSOR_NUM; chn++) {
        int inc = 0, num = 0;
        char path[128] = {0};
        record_get_path_of_ymd(yyyymmdd, path, sizeof(path), chn);

        /*获取全部录像列表*/
        sCache1File *p_mp4s[MAX_RECS_OF_DAY] = {NULL};
        DBG("\n");
        for (int ii = 0; ii < ARRAY_SIZE(p_mp4s); ii++) {
            p_mp4s[ii] = &f_mp4s[ii];
        }

        DBG("\n");
        int ret = lookupdir(path, p_mp4s, MAX_RECS_OF_DAY, &num, 0);
        if (ret == -1 || num <= 0) {
            perror("get filelist failed");
            continue;
        }

        /*
         * j 指向当前要填充的，返回的，去重的数组。
         * i 指向 scandir 的原生数组。
         **/
        for (int i = 0,j = 0; i < num;i++) {
            sRec1File *p = &(reclist_info->reclist[chn][j]);
            sCache1File *f = p_mp4s[i];

            // reclist_info 是全局变量，为节省一个 memset，需要手动清0
            p->is_tmp_file = FALSE;

            // 如果不是15分钟内开始的未修复的文件(即不是最后一个 9999 文件)，不处理
            if (f->file_secs == TMPFILE_LEN && i != (num-1)) {
                continue;
            }

            if (f->file_secs == TMPFILE_LEN) {
                char filepath[256] = {0};
                snprintf(filepath, sizeof(filepath) - 1, "%s%s", path, f->name);
                ret = record_get_mp4_secs(filepath, &f->file_secs);
                if (ret < 0) {
                    DBG("get mp4 time fail\n");
                    continue;
                }
                if (get_g_run(record, RUN_RECORD_DENTRY)) DBG("got_tmp %s\n", filepath);
                p->is_tmp_file = TRUE;
            }

            strncpy(p->file_name, f->name, sizeof(p->file_name)-1);

            start.tm_min = (f->start_hms%10000)/100;
            start.tm_hour = f->start_hms/10000;

            p->file_secs = f->file_secs;
            p->start_time = secs_ymd + start.tm_hour*3600 + start.tm_min*60 + (f->start_hms%100);
            p->stop_time = p->start_time + f->file_secs;

            /*
             * 包含处理：因为是升序存储，必是 old 包含 p
             **/
            if (j) {
                sRec1File *o = p-1;
                if (o->start_time + o->file_secs < p->start_time) {
                    // different
                } else if (o->start_time + o->file_secs >=  p->start_time && o->start_time + o->file_secs < p->start_time + p->file_secs) {
                    // cross
                    o->file_secs = p->start_time - o->start_time - 1;
                } else {
                    // include
                    --j;
                    inc++;
                }
            }
            j++;
        }

        reclist_info->num[chn] = (num - inc);
        reclist_info->yyyymmdd = yyyymmdd;
        total_num += reclist_info->num[chn];
    }

    if (f_mp4s) {
        free(f_mp4s);
    }

    return total_num;
}

int record_get_ymd_of_epoch(int64_t start_utc)
{
        /*获取录像日期*/
    struct tm start = {0};
    time_t start_lol;
    int yyyymmdd_in = 0;

    start_lol = start_utc;

    localtime_r(&start_lol, &start);

    dbg_record("________list.start: YYMMDD %04d-%02d-%02d HHMMSS %02d:%02d:%02d\n",
        start.tm_year + 1900, start.tm_mon +1, start.tm_mday, 
        start.tm_hour, start.tm_min, start.tm_sec);

    yyyymmdd_in = (start.tm_year + 1900)*10000 + (start.tm_mon +1)*100 + start.tm_mday;

    dbg_record("%d\n", yyyymmdd_in);

    return yyyymmdd_in;
}

int record_get_path_of_ymd(int yyyymmdd_in, char path[], int len, int channel)
{
    /*获取录像路径*/
    if (yyyymmdd_in < 19720101) {
        return -1;
    }

    char mmcpath[128] = {0};
    if (-1 == storage_get_mmcpath(mmcpath)) {
        DBG("------------------ not exist disc -------------\n");
        return -1;
    }

    if (MAX_SENSOR_NUM > 1) {
        //snprintf(path, sizeof(path), "rec/%d", yyyymmdd);
        if (channel == DEV_DIRECT_RECORD) {
            snprintf(path, len, "%s/IPCamera/%d/%s/", mmcpath, yyyymmdd_in, "direct");
        } else if (channel == DEV_DOME_RECORD) {
            snprintf(path, len, "%s/IPCamera/%d/%s/", mmcpath, yyyymmdd_in, "dome");
        } else if (channel == DEV_DOME_DIRECT_RECORD) {
            snprintf(path, len, "%s/IPCamera/%d/", mmcpath, yyyymmdd_in);
        }
    } else {
        snprintf(path, len, "%s/IPCamera/%d/", mmcpath, yyyymmdd_in);
    }


    DBG("%s\n", path);
    return 0;
}

/*
    *param:
        @start_utc : 开始 utc 时间
        @query_n   : 查询数量
        @olist     : 查询结果
    *return
        return the unmber of file queried, or -1 if an error occurred.
*/
int record_query_list(int64_t start_utc, int query_n, sRec1File *olist, int dev)//start_time,offtime,day_max,save_record
{
    /**************
    描述:
        1.查询缓存一天的录像
        2.阿里只有在初次建立录像拉流时会调用这个函数查询录像，后面 seek 都是使用缓存的录像列表
        
    录像列表查询逻辑：
        1.除了查询当天的录像之外，有缓存优先使用缓存s
        2.查当天，两次查询之间超过30秒，强制更新列表
    **************/
    dbg_record("start aliyun_query_record_list start_utc:%llu query_n:%d\n", start_utc, query_n);

    static time_t last_utc = 0; //上一次查询时间 
    time_t cur_utc = 0;
    cur_utc = time(NULL);

    int ret = 0;
    int num = 0;
    int chn = 0;
    int get_num = 0;
    int yyyymmdd_search = 0;
    int is_today = 0;  //是否查当天
    int need_rebuild = 0; //需要重构列表
    sRec1File *reclist0 = NULL;
    sRec1File *got[MAX_SENSOR_NUM] = {0};
    sRec1File *p = NULL;
    sRec1File key;

    int sec_east = get_tz_seceast(); //UTC + 偏移量才能知道是否为同一天
    is_today = (cur_utc + sec_east) / SECS_OF_DAY == (start_utc + sec_east) / SECS_OF_DAY;

    yyyymmdd_search = record_get_ymd_of_epoch(start_utc);  //utc转搜索日期字符串

    pthread_mutex_lock(&lock);

    if (yyyymmdd_search == g_record_cache.yyyymmdd) {
        if (is_today && (pop_g_stat(record, SD_REC_RENAME) || abs(cur_utc - last_utc) > 30))  {
            //查当天，只有rename后刷新列表
            DBG("Today refresh-build diff@%llus\n", cur_utc - last_utc);
            need_rebuild = 1;
        }
    } else {
        need_rebuild = 1;
    }

    do {
        if (need_rebuild) {
            DropCache(__func__);
            last_utc = cur_utc;
            WAR("____ goto rebuild list %d\n", yyyymmdd_search);
            g_record_cache.yyyymmdd = 0;
            g_record_cache.num[0] = 0;
            if (get_g_run(jcp, RUN_JCP_PRI_OUTPUT)) {
                struct timespec clock = {0};
                ms_clock_reset(&clock);
                ret = rebuild_newlist(yyyymmdd_search, &g_record_cache);
                printf("tid %d rebuild_newlist spend %lldms\n\n\n", gettid(), ms_since_previous(&clock));
            } else {
                ret = rebuild_newlist(yyyymmdd_search, &g_record_cache);
            }
            if (ret <= 0) { 
                g_record_cache.yyyymmdd = 0;
                DBG("not exist or empty, num: %d\n", ret);
                break;
            } 
            DropCache(__func__);
            CompactMemo(__func__);
        }
        
        for (chn = 0; chn < MAX_SENSOR_NUM; chn++) {
            num = g_record_cache.num[chn];
            reclist0 = g_record_cache.reclist[chn];
            key.start_time = (int)start_utc;
            key.stop_time = 0;
            got[chn] = (sRec1File *)bsearch(&key, reclist0, num, sizeof(sRec1File), cb_compmi);//二分查找
        }            

        for (chn = 0; chn < MAX_SENSOR_NUM; chn++) {
            if (dev != DEV_DOME_DIRECT_RECORD && chn != dev) {
                continue;
            }

            num = g_record_cache.num[chn];
            reclist0 = g_record_cache.reclist[chn];
            if(got[chn] == NULL){  /* 防止录像时间非全天 or 中间有中断 */
                for (int i = 0; i < num; i++){
                    if(start_utc <= reclist0[i].start_time){
                        DBG("got interrupt @num[%d]\n", i);
                        got[chn] = &reclist0[i];
                        break;
                    }
                }

                if(got[chn] == NULL){
                    DBG("WARNNING, not found\n");
                    ret = -1;
                    continue;
                }
            }

            p = got[chn];
            num = num - (p - reclist0); //计算当前列表中满足条件的录像数量
            if (get_num + num > query_n) {
                num = query_n - get_num;
            }

            for(int i = get_num; i < num + get_num;  p++, i++) {
                olist[i].start_time = p->start_time;
                olist[i].stop_time = p->stop_time;
                olist[i].file_secs = p->file_secs;
                //olist[i].record_type = REC_TYPE_PLAN;
                olist[i].is_tmp_file = p->is_tmp_file;
                strncpy(olist[i].file_name, p->file_name, sizeof(olist[i].file_name));
            }
            get_num += num;
        }

        ret = get_num;
    } while(0);
    
    pthread_mutex_unlock(&lock);
    return ret;
}

/*
 *返回值list：表示32位的数字，从低位到高位每一比特代表月份的第几天是否有录像；例如：8320（0010000010000000）表示8号和14号有录像
*/
int record_get_daybits_of_month(int month)
{
    int list = 0;
    int Length = 0;
    FILE* stream;
    char path[128] = {0};
    char cmd[1024] = {0};
    char FileBuf[512] = {0};

    if (0 == is_okey("/opt/media/mmcblk0p1")) {
        ERR("monthis NULL or sdcard is not exit!!!\n");
        return -1;
    }

    for (int i = 1; i<=31; i++) {
        memset(path, 0, sizeof(path));
        snprintf(path, sizeof(path)-1, "/opt/media/mmcblk0p1/IPCamera/%d%02d", month, i);

        if (0 == access(path, F_OK)) {
            memset(cmd, 0, sizeof(cmd));
            snprintf(cmd, sizeof(cmd)-1, "find %s -name [ASM]-*.mp4", path);

            stream = vpopen(cmd, "r");
            if (!stream) {
                continue;
            }

            //查找20221214等目录下是否有录像
            Length = fread(FileBuf, 1,sizeof(FileBuf), stream);
            vpclose(stream);

            // 如果读不到数据
            if (0 >= Length) {
                continue;
            }
            list += 1 << (i-1);
        }
    }
    DBG("month %d, list: %d\n", month, list);
    return list;
}

static int struct_to_str_time(char *starttime, char *endtime, sRecTime *rectime)
{
    if (starttime == NULL || rectime == NULL || endtime == NULL) {
        return -1;
    }

    sprintf(starttime, "%02d:%02d:00", rectime->begin_hour, rectime->begin_min);
    sprintf(endtime, "%02d:%02d:00", rectime->end_hour, rectime->end_min);

    return 0;
}

static int str_time_to_struct(char *starttime, char *endtime, sRecTime *rectime)
{
    if(starttime == NULL || rectime == NULL || endtime == NULL ) {
        return -1;
    }

    sscanf(starttime, "%d:%d:", &rectime->begin_hour, &rectime->begin_min);
    sscanf(endtime, "%d:%d:", &rectime->end_hour, &rectime->end_min);

    return 0;
}

/*
 * load g_rec_plans from 24bit hours
 **/
static int load_record_time()
{
    int j_hour = 0;
    int i_week = 0;

    RecordCtrlS record = {0};
    conf_get_recordcfg(&record);

    for (i_week = 0; i_week < 7; i_week++) {
        if (0 == record.timestrategy[i_week]) {
            continue;
        }
        unsigned time24b = record.timestrategy[i_week] & 0xFFFFFF;

        if ((time24b&((1<<23)+(1<<0))) == ((1<<23)+(1<<0)) && time24b != 0xFFFFFF) { /* 跨天 */
            if (i_week == 0){
                DBG("__ crossing %d__\n", i_week);
            }

            g_rec_plans[0][i_week].record_en = 1;
            for (j_hour = 23; j_hour >= 0; j_hour--) {
                if ((record.timestrategy[i_week] & (1 << j_hour)) == 0) {
                    g_rec_plans[0][i_week].begin_hour = j_hour+1;
                    g_rec_plans[0][i_week].begin_min = 0;
                    break;
                }
            }

            g_rec_plans[1][i_week].record_en = 1;
            for (j_hour = 0; j_hour < 24; j_hour++) {
                if ((record.timestrategy[i_week] & (1 << j_hour)) == 0) {
                    g_rec_plans[1][i_week].end_hour = j_hour-1;
                    g_rec_plans[1][i_week].end_min = 59;
                    break;
                }
            }
        } else {
            if (i_week == 0) {
                DBG("__ straight __ %d \n", i_week);
            }
            memset(g_rec_plans[1], 0, sizeof(g_rec_plans[1]));
            g_rec_plans[0][i_week].record_en = 1;

            for (j_hour = 0; j_hour < 24; j_hour++) {
                if (record.timestrategy[i_week] & (1 << j_hour)) {
                    g_rec_plans[0][i_week].begin_hour = j_hour;
                    g_rec_plans[0][i_week].begin_min = 0;
                    break;
                }
            }

            for (j_hour = 23; j_hour >= 0; j_hour--) {
                if (record.timestrategy[i_week] & (1 << j_hour)) {
                    g_rec_plans[0][i_week].end_hour = j_hour;
                    g_rec_plans[0][i_week].end_min = 59;
                    break;
                }
            }
        }
    }

    return 0;
}

int record_get_plan(sRecPlan *record_info, int *plan_num)
{
    int i;
    int idx = 0;
    int enable;
    int week_count = 0;

    memset(g_rec_plans, 0, sizeof(g_rec_plans));
    DBG("record_get_plan\n");
    load_record_time();

    for (i = 0, enable = 0; i < 7; i++) {
        DBG("0: %d\n", g_rec_plans[0][i].record_en);
        enable |= g_rec_plans[0][i].record_en;
    }
    *plan_num =  (enable ? 1 : 0);

    for (i = 0, enable = 0; i < 7; i++) {
        DBG("1: %d\n", g_rec_plans[1][i].record_en);
        enable |= g_rec_plans[1][i].record_en;
    }
    *plan_num += (enable ? 1 : 0);

    for (idx = 0; idx < *plan_num; idx++, record_info++) {
        for (i = 0, week_count = 0; i < 7; i++) {
            if (g_rec_plans[idx][i].record_en == 0) {
                continue;
            }

            struct_to_str_time(record_info->start_time, record_info->end_time, &g_rec_plans[idx][i]);
            record_info->record_no = idx+1;
            record_info->status = 1;
            record_info->week[week_count] = i ? i : 7;
            week_count++;
        }

        record_info->week_count = week_count;
        DBG("____no.%d ___sta:%s ___end:%s\n", idx, record_info->start_time, record_info->end_time);
    }

    return 0;
}

int record_set_plan(sRecPlan *info)
{
    DBG("record_set_plan\n");
    /*设置录像计划*/
    RecordCtrlS record_info = {0};
    conf_get_recordcfg(&record_info);

    int i = (info->record_no == 0) ? 0 : info->record_no-1;
    int j = !i;
    int del = FALSE;

    sRecTime rec_time = { .record_en = (int)info->status};

    str_time_to_struct(info->start_time, info->end_time, &rec_time);

    if (info->status) {     //add | mod
        memset(&g_rec_plans[i], 0, sizeof(g_rec_plans[i]));
        unsigned int k;
        for (k = 0; k < info->week_count; k++) {
            DBG("Do week[%d]: %d\n", k, info->week[k]);
            memcpy(&g_rec_plans[i][info->week[k]%7], &rec_time, sizeof(sRecTime));
        }
    } else {            // del
        memset(&g_rec_plans[i], 0, sizeof(g_rec_plans[i]));
        i = !i;
        j = !i;
        del = TRUE;
    }

    DBG("i %d, j %d\n", i, j);

    /* record_time_t -> timestrategy[day] */
    int day = 0;
    for (day = 0; day < 7; day++) {
        int timevalue = 0;

        if (g_rec_plans[i][day].record_en) {
            int k = 0;
            int hr_sta = g_rec_plans[i][day].begin_hour;
            int hr_end = g_rec_plans[i][day].end_hour;

            if (hr_sta != hr_end && g_rec_plans[i][day].end_min == 0) {
                hr_end -= 1;
            }

            for (k = hr_sta; k <= hr_end; k++) {
                timevalue = 1<<(k)|timevalue;
            }
            timevalue = timevalue|(1 << 31);

            if (day == 0) {
                DBG("_____1 sta %d ~ end %d\n", hr_sta, hr_end);
            }

            if (!del && g_rec_plans[j][day].record_en) {
                k = 0;
                hr_sta = g_rec_plans[j][day].begin_hour;
                hr_end = g_rec_plans[j][day].end_hour;

                if (hr_sta != hr_end && g_rec_plans[j][day].end_min == 0) {
                    hr_end -= 1;
                }

                for (k = hr_sta; k <= hr_end; k++) {
                    timevalue = 1<<(k)|timevalue;
                }
                timevalue = timevalue|(1 << 31);

                if (day == 0) {
                    DBG("_____2 sta %d ~ end %d\n", hr_sta, hr_end);
                }
            }
        }

        record_info.timestrategy[day] = timevalue;

        if (day == 0) {
            DBG("%u\n", timevalue);
        }
    }

    return conf_set_recordcfg(record_info);
}

