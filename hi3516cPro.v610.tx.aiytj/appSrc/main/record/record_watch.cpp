/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    :
 * Created Time : 2022-07-28
 * Version      : 1.0
 * Author       : luoshunfa
 * Description  :
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <dirent.h>

#include "recordapi.h"
#include "debug.h"
#include "utils.h"
#include "shm_buf.h"
#include "record_main.h"
#include "encodeapi.h"
#include "record_file_manage.h"
#include "record_watch.h"

#include "record_disk.h"
#include "delay_exec.h"
#include "record_delay.h"
#include "record_email_lib.h"
#include "record_lib.h"
#include "record_alarm_param.h"

#include "jconfig.h"
#include "confapi.h"
#include "libmp4.h"
#include "js_rec.h"
#include "js_scheduler.h"
#include "cmdstat.h"
#include "g_sys.h"
#include "g_stat.h"
#include "g_log.h"
#include "g_run.h"
#include "system_ctrl.h"
#include "encode_videomask.h"

#define SECS_OF_DAY (60*60*24)

/*
 * 1. 生成修复列表
 * 2. 如果某个文件的修复导致 jco_server 异常，在 auto_run.sh fn_reboot() 对该文件进行删除，
 *    防止设备反复重启。
 **/
#define RECLIST_TMP "/var/run/reclist.tmp"

static struct record_cfg g_record_cfg = {{0}};
static struct record_cfg g_record_raw = {{0}};
static struct record_run g_record_run = {.lock = PTHREAD_MUTEX_INITIALIZER};

static int repair_record_one(const char *file,unsigned int *timelen)
{
    int ret = 0;

    if(0 != access(file, F_OK)) {
        DBG("file %s not exist\n", file);
        return -1;
    }

    CMP4Repair *fMP4Repair = new CMP4Repair();
    fMP4Repair->Open(file);
    ret = fMP4Repair->GetMP4FileStatus();
    DBG("ret[%d]: 1 normal, 2 expection, 3 repaired @%s\n", ret, file);

    //tmp 后缀的是否正常结束都修复一次
    if (ret == eMP4File_JCOK || ret == eMP4File_JCOX || ret == eMP4File_JCOR) {
        ret = fMP4Repair->Repair(timelen);
        if (ret < 0) {
            SYSLOG("repair file[%s] failed\n", file);
        }
        DBG("Repair %s ret %d\n", file, ret);
    } else {
        SYSLOG("repair status error: ret[%d]\n", ret);
        ret = -1;
    }

    delete fMP4Repair;

    //ret = tencent_get_mp4_time(file, (int *)timelen);
    DBG("repair_record_one timelen %u \n", *timelen);
    return ret;
}

int scan_record_all_tmp_mp4(void)
{
    int ret = -1, splen = 0;
    char mmcpath[128] = {0};
    char dirPath[256] = {0};
    char datetime[16] = {0};
    char dstName[256] = {0};

    SYSLOG("start scan_record_all_tmp_mp4\n");

    do {
        ret = get_lastest_record_date(datetime, sizeof(datetime));
        if(ret < 0){
            js_log("get_lastest_record_date error!\n");
            break;
        }

        ret = storage_get_mmcpath(mmcpath);
        if(ret < 0){
            break;
        }

        sprintf(dirPath, "%s/%s/%s", mmcpath, "IPCamera", datetime);

        sCache1File *f_mp4s = (sCache1File *)system_malloc(sizeof(sCache1File)*MAX_TMPS_OF_DAY);
        sCache1File *p_mp4s[MAX_TMPS_OF_DAY] = {NULL};

        for (int ii = 0; ii < ARRAY_SIZE(p_mp4s); ii++) {
            p_mp4s[ii] = &f_mp4s[ii];
        }

        int num = 0;
        lookupdir(dirPath, p_mp4s, MAX_TMPS_OF_DAY, &num, REC_FILE_TMP);

        remove(RECLIST_TMP);

        for (int i = 0; num > 0 && i < num; i++) {
            splen = snprintf(dstName, sizeof(dstName), "%s/%s\n", dirPath, p_mp4s[i]->name);
            if(splen < 0 || splen >= (int)sizeof(dstName)) {
                ERR("snprintf failed splen:%d\n", splen);
                continue;
            }
            AppendFile(RECLIST_TMP, dstName);
        }

        if (f_mp4s) {
            free(f_mp4s);
        }
    } while (0);

    return 0;
}

int repair_record_all(void)
{
    FILE *fp = NULL;
    char src[128] = {0};
    char dst[128] = {0};
    char *line = NULL;
    size_t len = 0;
    ssize_t read = 0;
    unsigned int timelen = 0;

    fp = fopen("/var/run/reclist.tmp", "r");
    if (fp == NULL) {
        return -1;
    }

    while ((read = getline(&line, &len, fp)) != -1) {
        sscanf(line, "%s", src);
        timelen = 0;
        if (repair_record_one(src,&timelen)  == 0) {
            if(timelen <= 3) {  //Clear a video with an abnormal duration
                remove(src);
                SYSLOG("______ remove %s\n", src);
                continue;
            }
            strncpy(dst,src,sizeof(dst) - 1);
            char *p = strstr(dst, TMPFILE_SUFFIX);
            if(p != NULL) {
                sprintf(p,"-%04u.mp4",timelen);
                rename(src, dst);
                SYSLOG("______ mv %s %s\n", src, dst);
            }
        } else {
            remove(src);
            SYSLOG("repair record fail remove %s\n", src);
        }
    }

    free(line);
    fclose(fp);
    return 0;
}

int filter_tmp(const struct dirent * ent)
{
    if (ent->d_type != DT_REG)
        return 0;

    return (strncmp(ent->d_name + 8, TMPFILE_SUFFIX, strlen(TMPFILE_SUFFIX)) == 0); //tmp文件格式"S-105317.mp4.tmp"跳过前8位比较
}

int compar_tmp(const struct dirent **first, const struct dirent **second)
{
    return (strncmp((*first)->d_name, (*second)->d_name, sizeof((*first)->d_name)) < 0);//从高到低排序
}

int repair_last_record(const char *path)
{
    int ret = 0, num = 0, splen= 0;
    time_t t;
    struct tm p_tm;
    char dir_ymd[256] = {0};
    char yyyymmdd[64] = {0};
    sCache1File *f_mp4s = NULL;

    do {
        t = time(NULL);
        localtime_r(&t, &p_tm);
        if (strftime(yyyymmdd, sizeof(yyyymmdd), "%Y%m%d", &p_tm) == 0) {
            ret = FAILURE;
            ERR("strftime error!\n");
            break;
        }

        snprintf(dir_ymd, sizeof(dir_ymd) - 1, "%s/%s/%s", path, "IPCamera", yyyymmdd);

        f_mp4s = (sCache1File *)system_malloc(sizeof(sCache1File)*MAX_TMPS_OF_DAY);
        sCache1File *p_mp4s[MAX_TMPS_OF_DAY] = {NULL};

        for (int ii = 0; ii < ARRAY_SIZE(p_mp4s); ii++) {
            p_mp4s[ii] = &f_mp4s[ii];
        }

        lookupdir(dir_ymd, p_mp4s, MAX_TMPS_OF_DAY, &num, REC_FILE_TMP);

        if (num <= 0) {
            ERR("scandir error! n=%d %s\n", num, dir_ymd);
            ret = FAILURE;
            break;
        }

        char src[256] = {0};
        char dst[256] = {0};
        unsigned int timelen = 0;

        splen = snprintf(src, sizeof(src) - 1, "%s/%s", dir_ymd, p_mp4s[num-1]->name);
        if(splen < 0 || splen >= (int)sizeof(src)) {
            ERR("snprintf failed splen:%d\n", splen);
            break;
        }

        if (repair_record_one(src,&timelen)  == 0) {
            if(timelen <= 3) {  //Clear a video with an abnormal duration
                remove(src);
            } else {
                strncpy(dst,src,sizeof(dst) - 1);
                char *p = strstr(dst, TMPFILE_SUFFIX);
                if (p != NULL) {
                    sprintf(p,"-%04u.mp4",timelen);
                    rename(src, dst);
                    SYSLOG("______ mv %s %s\n", src, dst);
                }
            }
        } else {
            remove(src);
            SYSLOG("repair record fail remove %s\n", src);
        }

        ret = SUCCESS;
    } while(0);

    if (f_mp4s) {
        free(f_mp4s);
    }
    return ret;
}

int is_need_delete_old_files(void)
{
    struct record_cfg *p_cfg = &g_record_cfg;

    if(p_cfg->rec_reccfg.diskstrategy == 0)// 0 :stop, 1: delete
        return 0;

    return 1;
}

/*Note:this fuction must be called in recsess's scheduler*/
static int stop_recording(int need_I_frame)
{
    int ret = 0;
    time_t time_start = 0, time_end = 0;
    time_start = mono_sec();
    struct record_cfg *p_cfg = &g_record_cfg;
    struct record_run *p_run = &g_record_run;

    if (p_run->currec_handle == NULL) {
        return 0;
    }

    if(need_I_frame) {
        if(p_cfg->rec_encodecfg.enc[0].enable && p_cfg->rec_reccfg.rec_type == 0) {
            encode_immediate_iframe(CH_FS_MAIN0);
        }
        else if(p_cfg->rec_encodecfg.enc[1].enable && p_cfg->rec_reccfg.rec_type == 1){
            encode_immediate_iframe(CH_FS_SUB0);
        }
    }

    js_stop_record_mp4(p_run->currec_handle);
    p_run->currec_handle = NULL;

    memset(&p_run->rec_paraminfo, 0, sizeof(p_run->rec_paraminfo));
    time_end = mono_sec();
    if(time_end - time_start > 1) {
        DBG("stop record time:%lld\n", time_end - time_start);
    }
    return ret;
}


/*Note:this fuction must be called in recsess's scheduler*/
static int create_new_recording(eJRecType rectype)
{
    int ret = 0;
    struct tm curTm;
    struct record_run *p_run = &g_record_run;
    struct record_cfg *p_cfg = &g_record_cfg;

    memset(&p_run->rec_paraminfo, 0, sizeof(p_run->rec_paraminfo));
    if(p_cfg->rec_encodecfg.enc[0].enable && p_cfg->rec_reccfg.rec_type == 0) {
        p_run->rec_paraminfo.record_chn = 0;
        p_run->rec_paraminfo.I_frame_interval = p_cfg->rec_encodecfg.enc[0].gop / p_cfg->rec_encodecfg.enc[0].fps;
    }
    else if(p_cfg->rec_encodecfg.enc[1].enable && p_cfg->rec_reccfg.rec_type == 1){
        p_run->rec_paraminfo.record_chn = 1;
        p_run->rec_paraminfo.I_frame_interval = p_cfg->rec_encodecfg.enc[1].gop / p_cfg->rec_encodecfg.enc[1].fps;
    }

    p_run->rec_paraminfo.record_time = p_cfg->rec_reccfg.schedminutes * 60;
    p_run->rec_paraminfo.need_audio_record= p_cfg->rec_audiocfg.inenable;
    p_run->rec_paraminfo.record_type = rectype;

    if(rectype == JREC_TYPE_ALARM) {
        p_run->rec_paraminfo.record_time = p_cfg->rec_reccfg.alarmseconds; //fix time for alarm record
        set_cur_serial(-1); // 报警录像取最新的 i 帧
        set_cur_aserial(0);
    } else if(rectype == JREC_TYPE_MANUAL) {
        p_run->rec_paraminfo.record_time = DEF_RECORD_TIME; // fix max record time
    }

    if(p_run->rec_paraminfo.record_time <=0 || p_run->rec_paraminfo.record_time > DEF_RECORD_TIME) {
        p_run->rec_paraminfo.record_time = DEF_RECORD_TIME;
    }

    p_run->rec_paraminfo.start_utc = time(NULL);
    //在跨天的前 10s 之内停下的，拉长录像时间给一个一定会跨天的持续时间
    localtime_r(&p_run->rec_paraminfo.start_utc, &curTm);
    int record_sec = curTm.tm_hour * 60*60 + curTm.tm_min *60 + curTm.tm_sec + ALARM_RECORD_SEC;
    if((record_sec < SECS_OF_DAY) && (SECS_OF_DAY - record_sec <= 10)) {
        p_run->rec_paraminfo.record_time += 10;
    }

    do{
        ret = generate_record_filename(rectype, p_run->rec_paraminfo.record_filename, p_run->rec_paraminfo.start_utc);
        if(ret < 0) {
            //js_log( "generate record filename ERR!\n");
            break;
        }

        js_log("new file:%s\n",  p_run->rec_paraminfo.record_filename);
        p_run->currec_handle = js_start_record_mp4(p_run->sch_rec, p_run->sch_stop, &p_run->rec_paraminfo);
        if(p_run->currec_handle == NULL){
            js_log("js_start_record_mp4 error!\n");
            ret = -1;
            break;
        }
    }while(0);

    return ret;
}

/*Note:this fuction must be called in recsess's scheduler*/
static int start_recording(eJRecType rectype)
{
    int ret = 0;
    struct tm curTm = {0};
    JRecParamInfo * recparaminfo = NULL;
    //int pasttimes   = 0;
    int record_time = 0;
    struct record_run *p_run = &g_record_run;
    struct record_cfg *p_cfg = &g_record_cfg;

    do{
        if(p_run->currec_handle == NULL){
            // current not in recording, just start new rec, and return
            ret = create_new_recording(rectype);
            break;
        }

        recparaminfo = js_get_record_recparaminfo(p_run->currec_handle);
        if(rectype == recparaminfo->record_type){
            if(rectype == JREC_TYPE_ALARM) {
                struct timespec *start_time = js_get_record_start_time(p_run->currec_handle);
                if ((record_time = sec_since_previous(start_time)) < 0) {
                    record_time = 0;
                }
                recparaminfo->record_time = record_time + ALARM_RECORD_SEC; // every alarm record extend 120s
                if(recparaminfo->record_time > p_cfg->rec_reccfg.schedminutes * 60) {
                    recparaminfo->record_time = p_cfg->rec_reccfg.schedminutes * 60; //every alarm record no more than max record time
                }
                time_t time_now = time(NULL);
                //在跨天的前 10s 之内停下的，拉长录像时间给一个一定会跨天的持续时间
                localtime_r(&time_now, &curTm);
                int record_sec = curTm.tm_hour * 60*60 + curTm.tm_min *60 + curTm.tm_sec + ALARM_RECORD_SEC;
                if((record_sec < SECS_OF_DAY) && (SECS_OF_DAY - record_sec <= 10)) {
                    recparaminfo->record_time += 10;
                }

                p_run->rec_paraminfo.record_time = recparaminfo->record_time;
            }
            break;
        }
    }while(0);

    return ret;
}

static void diff_cfg2cmd(void *ctx)
{
    struct cmdstat *p_cmd = (struct cmdstat *)ctx;
    struct record_cfg *p_cfg = &g_record_cfg;
    struct record_cfg *p_raw = &g_record_raw;

    if (p_cmd->cmd_stage == 0) {
        return ;
    }

    if (p_cmd->cmd_stage & CMD_RECORD_CFG) {
        if (p_cfg->rec_reccfg.rec_type != p_raw->rec_reccfg.rec_type)
            cmd_set_command(p_cmd, CMD_NEED_STOP);

        if(record_get_currec_status() == RECORD_STATUS_SCHEDULE && TimeJudge( (unsigned int *)&p_raw->rec_reccfg.timestrategy) == 0)
            cmd_set_command(p_cmd, CMD_NEED_STOP);

        memcpy(&p_cfg->rec_reccfg, &p_raw->rec_reccfg, sizeof(p_raw->rec_reccfg));
    }

    if (p_cmd->cmd_stage & CMD_VIDEO_CFG) {
        memcpy(&p_cfg->rec_encodecfg, &p_raw->rec_encodecfg, sizeof(p_raw->rec_encodecfg));
    }

    if (p_cmd->cmd_stage & CMD_AUDIO_CFG) {
        memcpy(&p_cfg->rec_audiocfg, &p_raw->rec_audiocfg, sizeof(p_raw->rec_audiocfg));
    }

    if (p_cmd->cmd_stage & CMD_TIME_CHANGE) {
        cmd_set_command(p_cmd, CMD_NEED_STOP);
    }

    if (p_cmd->cmd_stage & CMD_VIDMASK_CHANGE) {
        int cfg_enable = p_cfg->vid_maskcfg.mask[ID_VIDEO_MASK].enable;
        int raw_enable = p_raw->vid_maskcfg.mask[ID_VIDEO_MASK].enable;
        if (raw_enable && cfg_enable != raw_enable) {
            COLOR_G("mask enable stop recording\n");
            cmd_set_command(p_cmd, CMD_NEED_STOP);
        }
        memcpy(&p_cfg->vid_maskcfg, &p_raw->vid_maskcfg, sizeof(p_raw->vid_maskcfg));
    }

    if (p_cmd->cmd_stage & CMD_ENCODE_CHANGE) {
        cmd_set_command(p_cmd, CMD_NEED_STOP);
    }
}

int record_request_rec(eJRecType itype)
{
    struct record_run *p_run = &g_record_run;

    switch(itype)
    {
        case JREC_TYPE_ALARM:
            p_run->need_alarmrec = 1;
            break;

        case JREC_TYPE_MANUAL:
            p_run->need_manualrec = 1;
            break;
        default:
            break;
    }
    return 0;
}

int record_request_stop(void)
{
    struct record_run *p_run = &g_record_run;

    p_run->need_stoprec = 1;
    return 0;
}

RecStatusE record_get_currec_status(void)
{
    struct record_run *p_run = &g_record_run;
    RecStatusE status = RECORD_STATUS_STOP;

    if(p_run->currec_handle != NULL){
        switch(p_run->rec_paraminfo.record_type){
            case JREC_TYPE_ALARM:
                status = RECORD_STATUS_ALARM;
            break;

            case JREC_TYPE_SCHEDULE:
                status = RECORD_STATUS_SCHEDULE;
            break;

            case JREC_TYPE_MANUAL:
                status = RECORD_STATUS_MANUAL;
            break;
            default:
            break;
        }
    }

    return status;
}

void email_send_alarm_text(void *data)
{
    int type = (int)data;

    EmailS  sEmailCfg = {0};
    MailInfoS mus = {0};
    char szAlarmType[64] = {0};
    char *p = NULL, *pp = NULL;
    time_t now;
    struct tm tml;
    int ret = 0;

    do{
        if(type < JALARM_TYPE_BEGIN || type >= JALARM_TYPE_END)
            break;

        conf_get_emailcfg(&sEmailCfg);
        now = time(NULL);
        localtime_r(&now, &tml);

        if (JALARM_TYPE_DISK_ERR == type) {
            sprintf(szAlarmType, "Disk Error %d%02d%02d", tml.tm_year + 1900, tml.tm_mon + 1, tml.tm_mday);
        } else if (JALARM_TYPE_MD == type) {
            sprintf(szAlarmType, "Motion detect %d%02d%02d", tml.tm_year + 1900, tml.tm_mon + 1, tml.tm_mday);
        } else if (JALARM_TYPE_VGLINE == type) {
            sprintf(szAlarmType, "Motion vgline %d%02d%02d", tml.tm_year + 1900, tml.tm_mon + 1, tml.tm_mday);
        } else if (JALARM_TYPE_VGRECT == type) {
            sprintf(szAlarmType, "Motion vgrect %d%02d%02d", tml.tm_year + 1900, tml.tm_mon + 1, tml.tm_mday);
        } else if (JALARM_TYPE_VL == type) {
            sprintf(szAlarmType, "Video loss %d%02d%02d", tml.tm_year + 1900, tml.tm_mon + 1, tml.tm_mday);
        } else if (JALARM_TYPE_MASK == type) {
            sprintf(szAlarmType, "Video Mask %d%02d%02d", tml.tm_year + 1900, tml.tm_mon + 1, tml.tm_mday);
        } else if (JALARM_TYPE_HUMAN_DETECT == type) {
            sprintf(szAlarmType, "Human detect %d%02d%02d", tml.tm_year + 1900, tml.tm_mon + 1, tml.tm_mday);
        } else if (0 == type) {
            sprintf(szAlarmType, "Mail testing %d%02d%02d", tml.tm_year + 1900, tml.tm_mon + 1, tml.tm_mday);
        }else{
            break; //not email
        }

        DBG("szAlarmType:%s from:%s to:%s\n", szAlarmType, sEmailCfg.user, sEmailCfg.sendto);

        pp = strchr(sEmailCfg.user, '@');
        if(pp == NULL && sEmailCfg.smtpserver != NULL) {
            p = strchr(sEmailCfg.smtpserver, '.');
            if(p) {
                p++;
                sprintf(mus.user, "%s@%s", sEmailCfg.user, p);
            }
            DBG("p:%s, mus.user:%s\n", p, mus.user);
        } else if(pp != NULL) {
            sprintf(mus.user, "%s", sEmailCfg.user);
        }

        sprintf(mus.passwd, "%s", sEmailCfg.password);
        sprintf(mus.dstUser, "%s", sEmailCfg.sendto);
        sprintf(mus.svrAddr, "%s", sEmailCfg.smtpserver);
        sprintf(mus.alarmType, "%s", szAlarmType);

        mailSSLInit();
        ret = mailUpData(NULL, NULL, 0, NULL, mus,5);
        if (ret < 0) {
            DBG("mailUpData err:%d\n", ret);
        }

    }while(0);
    return;
}

int record_email_text(JALARM_TYPE type)
{
    if(type >= JALARM_TYPE_BEGIN && type < JALARM_TYPE_END)
        return delay_handle_add(email_send_alarm_text, (void *)type, NULL);
    return 0;
}

void loop_record(void *ctx)
{
    static int gwatchcount = 0;

    struct record_cfg *p_cfg = &g_record_cfg;
    struct record_run *p_run = &g_record_run;

    int needStopCurRec = 0;
    int needStopCurRecNoI = 0;

    int needAlarmRec = 0;
    int needScheduleRec = 0;
    int needManualRec = 0;

    char  devPath[128] = {0,};
    int isFull = 0;
    int isNeedDelete = 0;
    int ret;

    int cmd = cmd_get_command((struct cmdstat *)ctx);

    if (cmd & CMD_NEED_STOP) {
        needStopCurRec = true;
    }

    if(get_g_stat(record, SD_ERR_STOP)) {
        return;
    }

    needStopCurRec += js_is_record_corssday(p_run->currec_handle);
    needStopCurRec += p_run->need_stoprec;

    needStopCurRecNoI += js_is_record_stoped(p_run->currec_handle); //录像模块关闭录像，不需要立即I帧

    p_run->need_stoprec = 0;  //clear flag

    if(gwatchcount % 30 == 0){ // every 15 secends to check disk full
        ret = storage_get_mmcpath(devPath);
        if(ret == 0){
            isFull = is_storage_devpath_really_full(devPath);
            isNeedDelete = is_need_delete_old_files();
        }
        if(isFull && isNeedDelete == 0 )
            needStopCurRec = 1;
    }
    gwatchcount ++;

    if(needStopCurRec){
        DBG("need stop record\n");
        stop_recording(1);
    } else if(needStopCurRecNoI) {
        DBG("need stop record no I frame\n");
        stop_recording(0);
    }

    needScheduleRec = TimeJudge((unsigned int *)&p_cfg->rec_reccfg.timestrategy);

    set_recalarm_flag(p_run->need_alarmrec, needScheduleRec);
    if(p_run->need_alarmrec){
        p_run->need_alarmrec = 0;
        if(FALSE == needScheduleRec) {
            needAlarmRec = 1;
        }
    }

    if(p_run->need_manualrec){
        p_run->need_manualrec = 0;
        needManualRec = 1;
    }

    if (videomask_enabled()) {
        needAlarmRec = needScheduleRec = needManualRec = 0;
        return;
    }

    pri_record(LVL_LOOP, "needAlarmRec: %d\n", needAlarmRec);
    pri_record(LVL_LOOP, "needScheduleRec: %d\n", needScheduleRec);
    pri_record(LVL_LOOP, "needManualRec: %d\n", needManualRec);

    do {
        if(isFull && isNeedDelete == 0)
            break;

        if(needAlarmRec){
            start_recording(JREC_TYPE_ALARM);
        } else if(needScheduleRec){
            start_recording(JREC_TYPE_SCHEDULE);
        } else if(needManualRec){
            start_recording(JREC_TYPE_MANUAL);
        }
    } while(0);
}

static void cb_recordcfg(int id, void *p_src, int size, void *ctx)
{
    CPY2CMDCFG(CMD_RECORD_CFG, &g_record_raw.rec_reccfg, p_src, size);
}

static void cb_audiocfg(int id, void *p_src, int size, void *ctx)
{
    CPY2CMDCFG(CMD_AUDIO_CFG, &g_record_raw.rec_audiocfg, p_src, size);
}

static void cb_videocfg(int id, void *p_src, int size, void *ctx)
{
    CPY2CMDCFG(CMD_VIDEO_CFG, &g_record_raw.rec_encodecfg, p_src, size);
}

static void cb_timechange(int id, void *p_src, int size, void *ctx)
{
    CPY2CMD(CMD_TIME_CHANGE);
}

static void cb_vidmaskchange(int id, void *p_src, int size, void *ctx)
{
    CPY2CMDCFG(CMD_VIDMASK_CHANGE, &g_record_raw.vid_maskcfg, p_src, size);
}

static void cb_encodechange(int id, void *p_src, int size, void *ctx)
{
    CPY2CMD(CMD_ENCODE_CHANGE);
}

int init_record_watch(void)
{
    static struct cmdstat cmdstat_record;
    struct cmdstat *ctx = &cmdstat_record;

    struct record_cfg *p_cfg = &g_record_cfg;
    struct record_cfg *p_raw = &g_record_raw;
    struct record_run *p_run = &g_record_run;
    p_run->p_ctx = ctx;

    int ret = 0;

    cmdstat_record.diff_cfg2cmd = diff_cfg2cmd;

    /* STEP 1 */
    if (get_g_sys(factest) || !system_get_security()) {
        SYSLOG("\n\n\n___ Warning: skip record bcz FACTORY_DB_FILE ___\n\n\n");
        return -1;
    }
    pthread_mutex_lock(&p_run->lock);
    do {
        if (p_run->record_init) {
            DBG("record moudle already init\n");
            ret = -1;
            break;
        }

        /* STEP 2 init, load */
        p_run->sch_rec = js_create_scheduler((char *)"recording");
        p_run->sch_stop = js_create_scheduler((char *)"record stop"); //有时候录像关闭时间会比较久，为了不影响下一段录像，单独用一个线程来关闭录像

        /* STEP 3 */
        conf_get_recordcfg(&p_cfg->rec_reccfg);
        conf_get_videocfg(&p_cfg->rec_encodecfg);
        conf_get_audiocfg(&p_cfg->rec_audiocfg);
        conf_get_videomaskcfg(&p_raw->vid_maskcfg);
        CPY2CMD(CMD_VIDMASK_CHANGE);

        attach_config(JEvent_RecordCfgChg,    cb_recordcfg,     ctx);
        attach_config(JEvent_AudioInCfgChg,   cb_audiocfg,      ctx);
        attach_config(JEvent_VideoCfgChg,     cb_videocfg,      ctx);
        attach_config(JEvent_TimeChange,      cb_timechange,    ctx);
        attach_config(JEvent_TimeZoneCfgChg,  cb_timechange,    ctx);
        attach_config(JEvent_VideoMaskCfgChg, cb_vidmaskchange, ctx);
        attach_event(JEvent_DevVideoReport,   cb_encodechange,  ctx);
        /* STEP 4 */
        js_create_timer_r(p_run->sch_rec, 10*1000, 500, loop_record, (void *)ctx, &p_run->hdl_loop);

        p_run->record_init = true;
    } while (0);
    pthread_mutex_unlock(&p_run->lock);

    return ret;
}

int uninit_record_watch(void)
{
    struct record_run *p_run = &g_record_run;
    int ret = 0;
    pthread_mutex_lock(&p_run->lock);
    do {
        if(p_run->record_init == false) {
            ret = -1;
            break;
        }

        detach_config(JEvent_RecordCfgChg,    cb_recordcfg,     p_run->p_ctx);
        detach_config(JEvent_AudioInCfgChg,   cb_audiocfg,      p_run->p_ctx);
        detach_config(JEvent_VideoCfgChg,     cb_videocfg,      p_run->p_ctx);
        detach_config(JEvent_TimeChange ,     cb_timechange,    p_run->p_ctx);
        detach_config(JEvent_TimeZoneCfgChg,  cb_timechange,    p_run->p_ctx);
        detach_config(JEvent_VideoMaskCfgChg, cb_vidmaskchange, p_run->p_ctx);
        detach_event(JEvent_DevVideoReport,   cb_encodechange,  p_run->p_ctx);

        js_delete_timer_r(&p_run->hdl_loop);
        js_delete_scheduler(p_run->sch_stop);
        p_run->sch_stop = NULL;
        js_clear_scheduler_stop(p_run->currec_handle);

        uinit_recalarm_flag();
        stop_recording(0);

        js_delete_scheduler(p_run->sch_rec);
        p_run->sch_rec = NULL;
        p_run->record_init = false;
        DBG("uninit record watch\n");
    } while (0);
    pthread_mutex_unlock(&p_run->lock);

    return ret;
}

void record_sync_cur_mp4info()
{
    struct record_run *p_run = &g_record_run;
    js_run_function(p_run->sch_rec, js_sync_cur_mp4info, (void *)p_run->currec_handle, 1);

    return;
}

int record_get_cur_recfile_path(char *path)
{
    struct record_run *p_run = &g_record_run;
    if (p_run->rec_paraminfo.record_filename == NULL) {
        return FAILURE;
    }

    memcpy(path, p_run->rec_paraminfo.record_filename, sizeof(p_run->rec_paraminfo.record_filename));
    return SUCCESS;
}
