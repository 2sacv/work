/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : alarm_log.cpp
 * @Created Time : 2014-03-12
 * @Version      : 1.0
 * @Author       : zengy
 * @Description  :
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <pthread.h>
#include <time.h>
#include <fcntl.h>

#include "utils.h"
#include "jconfstruct.h"
#include "g_log.h"
#include "debug.h"

#include "alarm_log.h"
#include "js_scheduler.h"
#include "conf_nand.h"

#define  LOGPATH    "/opt/log/alarmlog"
#define  TMPPATH    "/tmp/alarmlog"
#define  MAX_LINE_BYTE   256
#define  MAX_ALARMLOG_SIZE  50
#define  MAX_DELLOG_COUNT   100
#define  MAX_ADDLOG_COUNT   50
#define  MAX_LOGSYNC_TIME   (60*1000)

static int new_add_num = 0;
static FILE *file = NULL;

static JSScheduler sch_alog = NULL;
static JSTCHandle  hdl_alog = NULL;

static pthread_mutex_t m_mutex;

const char *alarm_type[JALARM_TYPE_END] = {
    "begin",

    "MotionDetect", 
    "MotionVgline", 
    "MotionVgrect", 
    "VideoLoss"   , 
    "DiskFull"  ,

    "DiskError"   ,
    "CableDisconn",
    "IPConflict",
    "IllegalVis"  , 
    "AlarmIn"     ,

    "AlarmExp"  ,
    "Maskalarm", 
    "HumanDetect",
    "CarDetect",
    "PlateDetect",
    "FireDetect",

    "PetDetect",
    "HumanMix",
    "EbikeDetect",
    "ThrowDetect",

    "CigaretteDetect",
    "Passenger",
    "FallDetect",
    "BarcodeDetect",
    "Facesnap",
    "CryDetect",

    "SenceChange",
};

static void alarm_log_lock()
{
    pthread_mutex_lock(&m_mutex);
}
static void alarm_log_unlock()
{
    pthread_mutex_unlock(&m_mutex);
};

static int check_alarm_log_file();
static int copy_file(const char *dst, const char *src);
static int get_file_size();
static int query_and_assemble_msg(QueryInfoS *info, char *type, char *buf, int bufsize, int *retlen);
static int assemble_type_field(JALARM_TYPE type, int channel, char *rst, int size);

static int delete_alarm_log_record();
static void alarm_log_capa_check(void *arg);

int init_alarm_log(void *data)
{
    SYSLOG("init_alarm_log create time engine\n");
    if (check_alarm_log_file() < 0) {
        return -1;
    }

    file = fopen(TMPPATH, "a+");
    if (NULL == file) {
        return -1;
    }
    
    pthread_mutex_init(&m_mutex, NULL);

    //sleep(1);

    if (NULL != (sch_alog = data)) {
        js_create_timer_r(sch_alog, MAX_LOGSYNC_TIME, MAX_LOGSYNC_TIME, alarm_log_capa_check, NULL, &hdl_alog);
    }

    return 0;
}

int uninit_alarm_log()
{
    if (NULL == file) {
        DBG("Didn't init!\n");
        return 0;
    }

    js_delete_timer_r(&hdl_alog);
   
    sch_alog = NULL;

    alarm_log_sync();

    alarm_log_lock();
    fclose(file);
    file = NULL;
    alarm_log_unlock();

    pthread_mutex_destroy(&m_mutex);
    return 0;
}

int alarm_log_sync()
{
    alarm_log_lock();

    if (!access(LOGPATH, F_OK)) {
        remove(LOGPATH);
    }

    if (copy_file(LOGPATH, TMPPATH) < 0) {
        ERR("Synchro bash command [cp] error!\n");
        alarm_log_unlock();
        return -1;
    }

    new_add_num = 0; //reset
    alarm_log_unlock();

    DBG("Synchro alarm log file Success!\n");
    return 0;
}

int alarm_add_event_log(JALARM_TYPE type, int channel, char *desc)
{
    if(type <= JALARM_TYPE_BEGIN || type >= JALARM_TYPE_END) {
        ERR("Paramter error!\n");
        return -1;
    }

	dbg_alarm("alarm_add_event_log desc:%s\n", desc);

    int nwrite = 0;
    int addnum = 0;
    char szTime[20] = {0};
    char text[128] = {0};
    char record[256] = {0};
    char type_desc[16] = {0};
    char tmp[32] = {0};
    time_t now;

    time(&now);
    struct tm tmnow ;
    localtime_r(&now, &tmnow);
    strftime(szTime, 20, "%F %H:%M:%S", &tmnow);

    if (NULL != desc) {
        snprintf(text, sizeof(text) - 1, "%s", desc);
    }
    
    if (type == JALARM_TYPE_EXP) {
        snprintf(tmp, sizeof(tmp), "%s%d", alarm_type[type], channel+1);
        sscanf(tmp, "%13s", type_desc);
    } else {
        sscanf(alarm_type[type], "%13s", type_desc);
    }

    alarm_log_lock();
    nwrite  =snprintf(record, sizeof(record), "%-20s|%-13s|%s\r\n", szTime, type_desc, text);
	
	dbg_alarm("alarm_add_event_log record:%s\n", record);
    Writefully(fileno(file), record, nwrite);
    fflush(file);
    addnum = ++new_add_num;
    alarm_log_unlock();

    if(addnum >= MAX_ADDLOG_COUNT) {
        DBG("add num >= %d, need sync\n", MAX_ADDLOG_COUNT);
        alarm_log_sync();
    }

    return 0;
}

int alarm_event_query(QueryInfoS *info, char *buf, int bufsize, int *retlen)
{
    if((info->type) >= JALARM_TYPE_END) {
        ERR("Parameter error!\n");
        return -1;
    }

    /*time format : 2014-03-13 16:11:43 */
    int ret = 0;

    char *field = NULL;
    char type_field[32] = {0};

    if(info->type != JALARM_TYPE_BEGIN) {
        ret = assemble_type_field(info->type, info->channel, type_field, sizeof(type_field));
        if(0 > ret) {
            return ret;
        }

        field = type_field;
    }

    ret = query_and_assemble_msg(info, field, buf, bufsize, retlen);

    return ret;
}

int query_and_assemble_msg(QueryInfoS *info, char *type, char *buf, int bufsize, int *retlen)
{	
	char typebuf[32] = {0};
	if (type != NULL) {
		strcpy(typebuf, type);
	}
	
    FILE *rd = NULL;
    char *line = NULL;
    size_t length = 0;
    ssize_t read = 0;
    int len = 0;
    char stime[64] = {0};
    char stype[128] = {0};

    char *p = NULL;
    char tempbuf[128] = {0};
    int templen = 0;

    int line_num = 0;
    int realnum = 0;
    alarm_log_lock();
	int str_len = 0;

    rd = fopen(TMPPATH, "r");
    if(0 > rd) {
        ERR("open [%s] failed!\n", TMPPATH);
        alarm_log_unlock();
        return -1;
    }

    while((read = getline(&line, &length, rd)) != -1) {
        p = NULL;
		str_len = strlen(line);
		if (str_len < 40 || str_len > 50) {
			continue;
		}

        bzero(stime, sizeof(stime));
        bzero(stype, sizeof(stype));
        sscanf(line, "%[^|]|%[^|]", stime, stype);
        if ((strcmp(stime, info->etime) > 0) ||
            (strcmp(stime, info->stime) < 0)) {
            continue;
        }

        if (NULL != type) {
            if ((p = strstr(stype, " ")) != NULL) {
                *p = '\0';
            }

            if(strcmp(typebuf, stype) != 0) {
                continue;
            }
        }
        p = strstr(line, "\r\n");
        if (NULL == p) {
            continue;
        }
        *p = '\0';

        line_num++;
        if ((line_num >= (info->itemindex)) &&
            line_num < (info->itemindex+ info->itemnum)) {
            if ((len + MAX_LINE_BYTE) > bufsize) {
                ERR("(len + MAX_LINE_BYTE):%d > bufsize : %d\n",
                    (len + MAX_LINE_BYTE), bufsize);

                break;
            }
            len += sprintf(buf+len, "%s#", line);
            realnum++;
        }

    }
    templen = sprintf(tempbuf, "itemtotal=%d;itemnum=%d;itemlist=",
                      line_num, realnum);
    memmove(buf+templen, buf, len);
    memcpy(buf, tempbuf, templen);
    strcat(buf, ";");
    *retlen = strlen(buf);
    fclose(rd);
    alarm_log_unlock();
	//DBG("buf = %s\n", buf);
    if (line) {
        free(line);
    }

    return 0;
}


int assemble_type_field(JALARM_TYPE type, int channel, char *rst, int size)
{
   if(JALARM_TYPE_EXP == type) {
        if(channel < 0 || channel > (AEXPAND_MAX_CHN - 1)) {
            ERR("EXP channel error!\n");
            return -1;
        }
    }

    (JALARM_TYPE_EXP == type) ?
    snprintf(rst, size - 1, "%s%d", alarm_type[type], channel+1) :
    snprintf(rst, size - 1, "%s", alarm_type[type]);

    return 0;
}

int check_alarm_log_file()
{
    do {
        if (!access(LOGPATH, F_OK)) {
            break;
        }
        
        FILE *file = fopen(LOGPATH, "w+");
        if (file == NULL) {
            return -1;
        }
        
        if (fclose(file) != 0) {
            return -1;
        }
    } while(0);

    if(!access(TMPPATH, F_OK)) {
        remove(TMPPATH);
    }

    if(copy_file(TMPPATH, LOGPATH) < 0) {
        return -1;
    }

    return 0;
}


void alarm_log_capa_check(void *arg)
{
    int size = get_file_size();
    int maxheight = 1080;
	int max_size = MAX_ALARMLOG_SIZE;
    maxheight = get_maxheight_2MTo3M();
	if (1296 == maxheight) {
		max_size = 40;
	}
	
    if (size >= max_size*1024) {
        if (SUCCESS == delete_alarm_log_record()) {
            DBG("Have delete sysnc...\n");
            alarm_log_sync();
        }
    }
}


int get_file_size()
{
    struct stat  st;

    bzero(&st, sizeof(st));

    stat(TMPPATH, &st);

    return st.st_size;
}

int delete_alarm_log_record()
{
    int fd = -1, delnum = 0;
    FILE *rd  = NULL;
    struct stat sb;

    alarm_log_lock();

    rd = fopen(TMPPATH, "r");
    fd = fileno(rd);
    fstat(fd, &sb);

    char *p = (char *)malloc(sb.st_size+1);
    if (NULL == p) {
        ERR("malloc error!\n");
        fclose(rd);
        alarm_log_unlock();
        return FAILURE;
    }

    int len = fread(p, 1, sb.st_size, rd);
    DBG("st_size : %d len : %d\n", (int)sb.st_size, len);
    if (len != sb.st_size) {
        ERR("read error!\n");
        fclose(rd);
        free(p);
        alarm_log_unlock();
        return FAILURE;
    }
    fclose(rd);

    int mfd = fileno(file);
    ftruncate(mfd, 0);

    char *pp = p;
    for (delnum=MAX_DELLOG_COUNT; delnum > 0 && (p + sb.st_size - pp > 0); delnum--) {
        pp = strstr(pp, "\r\n");
        if (pp == NULL) {
            break;
        }
        pp += strlen("\r\n");
    }
    if (pp != NULL) {
        len = fwrite(pp, 1, p + sb.st_size - pp, file);
        if (len == p + sb.st_size - pp) {
            DBG("Delete %d records success!\n", MAX_DELLOG_COUNT);
        }
    }

    free(p);

    alarm_log_unlock();
    return SUCCESS;
}

int copy_file(const char *dst, const char *src)
{
    int fdsrc = -1;
    int fddst = -1;
    char buf[2048] = {0};
    int ret = -1;
    int len = 0;

    fdsrc = open(src, O_RDONLY);
    if(-1 == fdsrc) {
        ERR("open src file %s fail\n", src);
        ret = -1;
        goto cleanup;
    }

    //fddst = open(dstFile, O_WRONLY | O_CREAT | O_TRUNC, 0755);
    fddst = creat(dst, 0755);
    if(-1 == fddst) {
        ERR("open dst file %s fail %s\n", dst, strerror(errno));
        ret = -1;
        goto cleanup;
    }

    do {
        len = Readfully(fdsrc, buf, sizeof(buf));
        if(0 > len) {
            ERR("read_fully fail\n");
            ret = -1;
            goto cleanup;
        } else if(0 == len) {
            //end of file
            break;
        }

        ret = Writefully(fddst, buf, len);
        if(0 > ret || len != ret) {
            ERR("write_fully fail, %d %d\n", len, ret);
            ret = -1;
            goto cleanup;
        }
    } while(0 < len);

    ret = 0;

cleanup:
    if(0 < fdsrc) {
        close(fdsrc);
        fdsrc = -1;
    }

    if(0 < fddst) {
        close(fddst);
        fddst = -1;
    }

    return ret;
}

