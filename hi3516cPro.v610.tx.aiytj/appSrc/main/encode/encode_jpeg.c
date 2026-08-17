/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : encode_jpeg.c
 * @Created Time : 2021-04-21
 * @Version      : 1.0
 * @Author       : cheby
 * @Description  :
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "jconfstruct.h"
#include "debug.h"
#include "js_scheduler.h"
#include "shm_buf_pool.h"
#include "cmdstat.h"
#include "encode_jpeg.h"
#include "conf_list.h"
#include "jconfig.h"
#include "encode_jpeg.h"
#include "encode_video.h"
#include "encode_common.h"
#include "system_ctrl.h"
// #include "jpeglib.h"
#include "record_email_lib.h"
#include "alarm_paramcfg.h"
#include "alarm_service.h"
#include "confapi.h"
#include "system_sch.h"

#define RECORD_FILEPATH_MAX  (256)           //录像文件路径最大长度
#define MS_STEP_JPEG        400

typedef struct snapshot {
    char *pbuf;
    int   bufsize;
    int   result;
    int   needsave;
} JpegSnapshotS;

struct jpeg_cfg{
    CaptureS  capture;
    VideoEncS venc;
    JpegAlarm alarminfo;
    int tick;
};

static struct jpeg_cfg cfg = {{0}};
static struct jpeg_cfg raw = {{0}};
static struct jpeg_run run = {0};
static struct jpeg_cfg *g_cfg_jpeg = &cfg;
static struct jpeg_cfg *g_raw_jpeg = &raw;
static struct jpeg_run *g_run_jpeg = &run;

int alarmmb_handle_filepath(JALARM_TYPE event, char *filePath, char *filename)
{
    char *p = NULL;
    struct tm curTm_s;
    struct tm *curTm = &curTm_s;
    time_t curTime;

    curTime = time(NULL);
    curTime = (curTime%900<=3) ? curTime-(curTime%900) : curTime;
    localtime_r(&curTime, &curTm_s);
    sprintf(filePath, "/tmp/%d%02d%02d-%d-%02d%02d%02d.jpg",
        curTm->tm_year + 1900, curTm->tm_mon + 1, curTm->tm_mday,
        event, curTm->tm_hour, curTm->tm_min, curTm->tm_sec);

    if ((p = strrchr(filePath, '/'))) {
        p += 1;
    } else {
        p = filePath;
    }

    sprintf(filename, "%s", p);

    return SUCCESS;
}

int capture_make_file(int iFramesize, char *filePath, char *pBuf)
{
    int fd = -1;
    int ret = 0;
    int  result = 0;

    if(pBuf == NULL) {
        return -1;
    }

    DBG("%s\n", filePath);
    fd = open(filePath, O_CREAT | O_WRONLY, 0755);
    if (-1 == (fd)) {
        DBG("%s open fail\n", __FUNCTION__);
        SYSLOG_RECORD("%s open fail\n", __FUNCTION__);
        return -1;
    }

    ret = write(fd, pBuf, iFramesize);
    if(ret < 0) {
        DBG("write file error:%s\n", strerror(errno));
        SYSLOG_RECORD("write file error:%s\n", strerror(errno));
        close(fd);
        return -1;
    }

    ret = close(fd);
    if(ret < 0) {
        DBG("close file error:%s\n", strerror(errno));
        SYSLOG_RECORD("close file error:%s\n", strerror(errno));
        return -1;
    }

    if(strstr(filePath, "samba") == NULL) {
        struct timeval times[2] = {{0}};
        times[0].tv_sec = times[1].tv_sec = time(NULL);
        result = utimes(filePath, times);
        if(result < 0) {
            DBG("filename:%s utime RDBG%d):%s\n", filePath, errno, strerror(errno));
            SYSLOG_RECORD("filename:%s utime RDBG%d):%s\n", filePath, errno, strerror(errno));
            return -1;
        }
    }

    return 0;
}

int send_alarm_email(JSEventType alarm_type, char *pBuf, int iFramesize)
{
    int ret = 0;
    char filename[RECORD_FILEPATH_MAX] = {0};
    char filePath[RECORD_FILEPATH_MAX] = {0};
    char szAlarmType[32] = {0};
    char *p = NULL, *pp = NULL;
    EmailS  sEmailCfg;
    MailInfoS mus;

    memset(szAlarmType, 0, sizeof(szAlarmType));
    memset(&sEmailCfg, 0, sizeof(sEmailCfg));
    memset(&mus, 0, sizeof(mus));
    ret = get_config(handleEmailCfg, sEmailCfg);
    RET_JUDGE(ret);

    //创建 jpeg 文件
    ret = alarmmb_handle_filepath(alarm_type, filePath, filename);
    RET_JUDGE(ret);

    //将图片存入文件
    ret = capture_make_file(iFramesize,filePath,pBuf);
    RET_JUDGE(ret);

    dbg_alarm("alarm_type:%d\n", alarm_type);
    if (JEvent_AlarmAI0 == alarm_type) {
        sprintf(szAlarmType, "Alarm input");
    } else if (JEvent_AlarmMD == alarm_type) {
        sprintf(szAlarmType, "Motion detect");
    } else if (JEvent_AlarmVgline == alarm_type) {
        sprintf(szAlarmType, "Motion vgline");
    } else if (JEvent_AlarmVgrect == alarm_type) {
        sprintf(szAlarmType, "Motion vgrect");
    } else if (JEvent_AlarmVL == alarm_type) {
        sprintf(szAlarmType, "Video loss");
    } else if (JEvent_Alarmhumadetect == alarm_type) {
        sprintf(szAlarmType, "Human detect");
    } else if (JEvent_AlarmFace == alarm_type) {
        sprintf(szAlarmType, "Face detect");
    } else if (JEvent_AlarmCar == alarm_type) {
        sprintf(szAlarmType, "Vehicle detect");
    } else if (JEvent_AlarmPet == alarm_type) {
        sprintf(szAlarmType, "Pet detect");
    } else if (JEvent_AlarmCry == alarm_type) {
        sprintf(szAlarmType, "Cry detect");
    } else {
        sprintf(szAlarmType, "Alarm event");
    }
    dbg_alarm("szAlarmType:%s\n", szAlarmType);

    pp = strchr(sEmailCfg.user, '@');
    if(pp == NULL && sEmailCfg.smtpserver != NULL) {
        p = strchr(sEmailCfg.smtpserver, '.');
        if(p) {
            p++;
            sprintf(mus.user, "%s@%s", sEmailCfg.user, p);
        }
        dbg_alarm("p:%s, mus.user:%s\n", p, mus.user);
    } else if(pp != NULL) {
        sprintf(mus.user, "%s", sEmailCfg.user);
    }

    sprintf(mus.passwd, "%s", sEmailCfg.password);
    sprintf(mus.dstUser, "%s", sEmailCfg.sendto);
    sprintf(mus.svrAddr, "%s", sEmailCfg.smtpserver);
    sprintf(mus.alarmType, "%s", szAlarmType);

    mailSSLInit();
    ret = mailUpData(filePath, NULL, 0, filename, mus,5);
    RET_JUDGE(ret);

    if (filePath != NULL) {
        ret = remove(filePath);
        RET_JUDGE(ret);
    }

    return ret;
}

static void diff_cfg2cmd(void *ctx)
{
    struct cmdstat *p_cmd = (struct cmdstat *)ctx;

    if (p_cmd->cmd_stage) {
        memcpy(&g_cfg_jpeg, &g_raw_jpeg, sizeof(g_cfg_jpeg));
    }
}

void loop_jpeg(void *ctx)
{
    int cmd = cmd_get_command((struct cmdstat *)ctx);
    static struct timespec email_interv_time = {0};
    int ret = 0;

    if (cmd) {
        if (g_cfg_jpeg->tick > 0) {
            g_cfg_jpeg->tick--;
            return;
        }

        //报警邮件送图
        if (cmd & CMD_JPEG_ALARM) {
            alarm_link_t alarm_link = {0};
            if (get_alarm_link_cfg(g_cfg_jpeg->alarminfo.event_type, (void*)&alarm_link, g_cfg_jpeg->alarminfo.chn) < 0) {
                ERR("get_alarm_link_cfg fail\n");
                return;
            }

            if (alarm_link.email && ms_clock_is_timeup(&email_interv_time, alarm_link.interval*1000)) {
                int len = LEN_JPEG;
                if (NULL == g_run_jpeg->buf) {
                    g_run_jpeg->buf = system_malloc(LEN_JPEG);
                }
                ret = encode_video_get_jpeg(g_run_jpeg->buf, &len);
                if (0 != ret) {
                    ERR("encode_video_get_jpeg fail ret:%d\n", ret);
                    goto __exit;
                }
                DBG("send email %d\n", g_cfg_jpeg->alarminfo.event_type);
                ret = send_alarm_email(g_cfg_jpeg->alarminfo.event_type, g_run_jpeg->buf, len);
                g_cfg_jpeg->alarminfo.event_type = 0;
                if (0 != ret) {
                    ERR("send_alarm_email fail ret:%d\n", ret);
                    goto __exit;
                }
            }
        }
    }

__exit:
    if (g_run_jpeg->buf) {
        system_free(g_run_jpeg->buf);
        g_run_jpeg->buf = NULL;
    }

    return;
}

static void cb_capturecfg(int id, void *p_src, int size, void *ctx)
{
    CPY2CMDCFG(CMD_JPEG_CAPTURE, &g_raw_jpeg->capture, p_src, size);
}
static void cb_videocfg(int id, void *p_src, int size, void *ctx)
{
    g_raw_jpeg->tick = (6*1000)/MS_STEP_JPEG;
    CPY2CMDCFG(CMD_JPEG_VIDEO, &g_raw_jpeg->venc, p_src, size);
}

static void cb_alarm_event(int id, void *p_src, int size, void *ctx)
{
    int chn = 0;
    if (p_src) chn = *(int *)p_src;
    JpegAlarm alarminfo = {chn, id};
    CPY2CMDCFG(CMD_JPEG_ALARM, &g_raw_jpeg->alarminfo, &alarminfo, sizeof(JpegAlarm));
}

int encode_jpg_write_single_file(char *pBuf,int len)
{
    char filename[256];
    FILE* fp;
    memset(filename,0,sizeof(filename));
    time_t curTime = time(NULL);
    sprintf(filename,"/tmp/%lld.jpg", curTime);
    fp = fopen(filename, "a+");
    if(fp==NULL) {
        EERR("Open File %s is ERROR \n",filename);
        return -1;
    }

    fwrite(pBuf, 1, len, fp);
    fclose(fp);
    sync();
    DBG("[%s][%d] \n",filename,len);
    return 0;
}

static int encode_save_jpeg_pic(char *pFileName, char*pPostfix, char *pBuf,int len)
{
    char filename[256] = {0};
    sprintf(filename,"/tmp/%s.%s", pFileName, pPostfix);

    FILE *fp = fopen(filename, "a+");
    if (fp == NULL) {
        ERR("Open File %s is ERROR \n",filename);
        return -1;
    }

    fwrite(pBuf, 1, len, fp);
    fclose(fp);
    DBG("[%s][%d] \n",filename,len);
    return 0;
}

void cb_encode_snapshot(void* userdata)
{
    JpegSnapshotS* pSanp = (JpegSnapshotS*)userdata;

    int ret = encode_video_get_jpeg(pSanp->pbuf, &pSanp->bufsize);
    if (ret == SUCCESS && get_g_run(dbg, RUN_SAVE_TMPJPEG)) {
        char picname[64] = {0};
        time_t now = time(NULL);
        struct tm *pt = localtime(&now);
        sprintf(picname, "test_%04d%02d%02d-%02d%02d%02d",
                pt->tm_year + 1900,
                pt->tm_mon + 1,
                pt->tm_mday,
                pt->tm_hour,
                pt->tm_min,
                pt->tm_sec);

        encode_save_jpeg_pic(picname, "jpeg", pSanp->pbuf, pSanp->bufsize);
        DBG("save jpeg tmp file:%s\n", picname);
    }

    pSanp->result = ret;
    return;
}

int encode_snapshot_ex(char *jpegbuf, int *bufsize)
{
    if (NULL == jpegbuf || NULL == bufsize) {
        ERR("param error\n");
        return -1;
    }

    JpegSnapshotS Snap = {0};
    Snap.pbuf     = jpegbuf;
    Snap.bufsize  = *bufsize;
    Snap.result   = FAILURE;

    js_run_function(g_run_jpeg->sch, cb_encode_snapshot, (void*)&Snap, 1);

    if (Snap.bufsize > 0) {
        *bufsize = Snap.bufsize;
    }

    return Snap.result;
}

int encode_loop_jpeg_start(void)
{
    DBG("start jpeg\n");
    int i = 0;
    static struct cmdstat cmdstat_video;
    struct cmdstat *ctx = &cmdstat_video;
    cmdstat_video.diff_cfg2cmd = diff_cfg2cmd;

/* STEP 1 */
    if(g_run_jpeg->sch == NULL){
        g_run_jpeg->sch = js_create_scheduler("sch_jpeg");
    }

    get_config(handleCaptureCfg, g_cfg_jpeg->capture);
    conf_get_videocfg(&g_cfg_jpeg->venc);

    /* STEP 2 */
    g_run_jpeg->p_ctx = ctx;
    attach_config(JEvent_CaptureCfgChg   , cb_capturecfg , (void *)ctx);
    attach_config(JEvent_VideoCfgChg     , cb_videocfg   , (void *)ctx);

    attach_event(JEvent_Alarmhumadetect  , cb_alarm_event , (void *)ctx);
    attach_event(JEvent_AlarmMD          , cb_alarm_event, (void *)ctx);
    attach_event(JEvent_AlarmVgline      , cb_alarm_event, (void *)ctx);
    attach_event(JEvent_AlarmVgrect      , cb_alarm_event, (void *)ctx);
    attach_event(JEvent_AlarmVL          , cb_alarm_event, (void *)ctx);
    attach_event(JEvent_AlarmCabDis      , cb_alarm_event, (void *)ctx);
    attach_event(JEvent_AlarmIpConflict  , cb_alarm_event, (void *)ctx);
    attach_event(JEvent_AlarmIllegalVisit, cb_alarm_event, (void *)ctx);
    for(i = JEvent_AlarmAI0; i <= JEvent_AlarmAI3; i++) {
        attach_event(i,  cb_alarm_event, (void *)ctx);
    }

    for(i = JEvent_AlarmExpand0; i <= JEvent_AlarmExpand15; i++) {
        attach_event(i,  cb_alarm_event, (void *)ctx);
    }

    attach_event(JEvent_AlarmVMask      , cb_alarm_event, (void *)ctx);
    attach_event(JEvent_AlarmCableNormal, cb_alarm_event, (void *)ctx);
    attach_event(JEvent_AlarmFacesnap   , cb_alarm_event, (void *)ctx);
    attach_event(JEvent_AlarmFace       , cb_alarm_event, (void *)ctx);
    attach_event(JEvent_AlarmCar        , cb_alarm_event, (void *)ctx);
    attach_event(JEvent_AlarmPet        , cb_alarm_event, (void *)ctx);
    attach_event(JEvent_AlarmCry        , cb_alarm_event, (void *)ctx);

    /* STEP 3 */
    // 与 encode_snapshot_ex() 会同步调用 IMP_Encoder_StartRecvPic(jpg_chn);
    // email 发送内容，包括事件类型，不能禁用
    js_create_timer_r(g_run_jpeg->sch, 3000, MS_STEP_JPEG, loop_jpeg, ctx, &g_run_jpeg->hdl_loop);

    return 0;
}

int encode_loop_jpeg_stop(void)
{
    DBG("stop jpeg\n");

    int i = 1;
    /* STEP 1 */
    detach_config(JEvent_CaptureCfgChg   , cb_capturecfg , g_run_jpeg->p_ctx);
    detach_config(JEvent_VideoCfgChg     , cb_videocfg   , g_run_jpeg->p_ctx);

    detach_event(JEvent_Alarmhumadetect  , cb_alarm_event, g_run_jpeg->p_ctx);
    detach_event(JEvent_AlarmMD          , cb_alarm_event, g_run_jpeg->p_ctx);
    detach_event(JEvent_AlarmVgline      , cb_alarm_event, g_run_jpeg->p_ctx);
    detach_event(JEvent_AlarmVgrect      , cb_alarm_event, g_run_jpeg->p_ctx);
    detach_event(JEvent_AlarmVL          , cb_alarm_event, g_run_jpeg->p_ctx);
    detach_event(JEvent_AlarmCabDis      , cb_alarm_event, g_run_jpeg->p_ctx);
    detach_event(JEvent_AlarmIpConflict  , cb_alarm_event, g_run_jpeg->p_ctx);
    detach_event(JEvent_AlarmIllegalVisit, cb_alarm_event, g_run_jpeg->p_ctx);
    for(i = JEvent_AlarmAI0; i <= JEvent_AlarmAI3; i++) {
        detach_event(i,  cb_alarm_event, g_run_jpeg->p_ctx);
    }
    for(i = JEvent_AlarmExpand0; i <= JEvent_AlarmExpand15; i++) {
        detach_event(i,  cb_alarm_event, g_run_jpeg->p_ctx);
    }

    detach_event(JEvent_AlarmVMask      , cb_alarm_event, g_run_jpeg->p_ctx);
    detach_event(JEvent_AlarmCableNormal, cb_alarm_event, g_run_jpeg->p_ctx);
    detach_event(JEvent_AlarmFacesnap   , cb_alarm_event, g_run_jpeg->p_ctx);
    detach_event(JEvent_AlarmFace       , cb_alarm_event, g_run_jpeg->p_ctx);
    detach_event(JEvent_AlarmCar        , cb_alarm_event, g_run_jpeg->p_ctx);
    detach_event(JEvent_AlarmPet        , cb_alarm_event, g_run_jpeg->p_ctx);
    detach_event(JEvent_AlarmCry        , cb_alarm_event, g_run_jpeg->p_ctx);

    /* STEP 2 */
    if(g_run_jpeg->hdl_loop != NULL){
        js_delete_timer_r(&g_run_jpeg->hdl_loop);
    }

    if(g_run_jpeg->sch != NULL){
        js_delete_scheduler(g_run_jpeg->sch);
        g_run_jpeg->sch = NULL;
    }

    return 0;
}

