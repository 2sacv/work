/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : jcolog.cpp
 * @Created Time : 2013-10-15
 * @Version      : 1.0
 * @Author       : zengy
 * @Description  :
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <time.h>
#include <fcntl.h>

#include "debug.h"
#include "jcolog.h"
#include "utils.h"

#define TMPREALLOG  "/opt/log/temp_applog"

#define MAX_CHECK_LOG_TIME  (60*1000)
int JCOLog::log_init()
{
    if(check_log_file() < 0) {
        return -1;
    }

    if(do_log_init() < 0) {
        return -1;
    }

    return 0;
}

int JCOLog::log_init_client_sync(void *data)
{
    m_pScheduler = (JSScheduler)data;
	js_create_timer_r(m_pScheduler, MAX_CHECK_LOG_TIME, MAX_CHECK_LOG_TIME, log_client_sync, this, &m_timing_sync_task);

    return 0;
}

JCOLog::JCOLog(JSScheduler engine, const char* path, const char* tmppath, int maxsize,
               int maxrecord, int delnum)
    : m_pScheduler(engine), m_timing_sync_task(NULL),
      m_path(path), m_tmppath(tmppath),
      m_max_size(maxsize), m_max_record(maxrecord), m_del_num(delnum),
      m_new_add_num(0), m_del_flag(0), m_file(NULL)
{
    pthread_mutex_init(&m_mutex, NULL);
}

JCOLog::~JCOLog()
{
    js_delete_timer_r(&m_timing_sync_task);

    log_lock();
    fclose(m_file);
    m_file = NULL;
    log_unlock();

    m_pScheduler = NULL;
}

int JCOLog::check_log_file()
{
    do {
        if(!access(m_path.c_str(), F_OK))
            break;

        FILE *file = fopen(m_path.c_str(), "w+");
        if(file == NULL)
            return -1;

        if(fclose(file) != 0)
            return -1;
    } while(0);

    if(!access(m_tmppath.c_str(), F_OK)) {
        remove(m_tmppath.c_str());
    }

    if(copy_file(m_tmppath.c_str(), m_path.c_str()) < 0) {
        return -1;
    }

    return 0;
}

int JCOLog::do_log_init()
{
    m_file = fopen(m_tmppath.c_str(), "a+");
    if(NULL == m_file)
        return -1;
    
    return 0;
}

int JCOLog::do_query_log(void *req, void *buf, int bufsize, int *extlen, void *instance)
{
    JCOLog *log = (JCOLog *)instance;
	
    return log->query_log(req, buf, bufsize, extlen);
}

int JCOLog::do_add_record(void *req, void *buf, int bufsize, int *extlen, void *instance)
{
    JCOLog *log = (JCOLog *)instance;

    return log->add_record(req, buf);
}

int JCOLog::do_sync_log(void *req, void *buf, int bufsize, int *extlen, void *instance)
{
    JCOLog *log = (JCOLog *)instance;

    return log->sync_log_file();
}

void JCOLog::log_client_sync(void *instance)
{
    //DBG("log sync start\n");
    JCOLog *log = (JCOLog*)instance;
	
    log->log_client_sync1();
    //DBG("log sync end\n");
}

void JCOLog::log_client_sync1()
{
    int size = get_file_size();

    if(size >= m_max_size*1024) {
        delete_log_record();
    }

    if (is_okey("/tmp/messages.0")) {
        // 保存开始时的80行
        if (!is_okey("/tmp/messages.head")) {
            UtilSystemCmd((char *)"head -80 /tmp/messages.0 > /tmp/messages.head");
        }
        UtilSystemCmd((char *)"tar -zcf /tmp/messages.0.tgz /tmp/messages.0");
        remove("/tmp/messages.0");
    }

    const char *msg_dot = "/tmp/messages.dot";
    if (bytes_of_file(msg_dot) > 512*1024) {
        // 解压时命令: cd /tmp/; tar -zxvf messages.dot.tgz
        SYSLOG("roll %s @512KB\n", msg_dot);
        UtilSystemCmd2("test -f %s.tgz && mv %s.tgz %s.0.tgz", msg_dot, msg_dot, msg_dot);
        UtilSystemCmd2("tar -zcf %s.tgz %s", msg_dot, msg_dot);
        truncate(msg_dot, 0);
    }

    log_lock();
    int flag = m_del_flag;
    log_unlock();

    if(flag) {
        DBG("Have delete sysnc...\n");
        sync_log_file();
    }
}

int JCOLog::get_file_size()
{
    struct stat  st;
    char filePath[128] = {0};

    bzero(&st, sizeof(st));
    sprintf(filePath, "%s", m_tmppath.c_str());

    stat(filePath, &st);

    return st.st_size;
}

int JCOLog::copy_file(const char *dst, const char *src)
{
    int fdsrc = -1;
    int fddst = -1;
    char buf[2048];
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
        len = read_fully(fdsrc, buf, sizeof(buf));
        if(0 > len) {
            ERR("read_fully fail\n");
            ret = -1;
            goto cleanup;
        } else if(0 == len) {
            //end of file
            break;
        }

        ret = write_fully(fddst, buf, len);
        if(0 > ret || len != ret) {
            ERR("write_fully fail, %d %d\n", len, ret);
            ret = -1;
            goto cleanup;
        }
    } while(0 < len);

    ret = fsync(fddst);
    if (ret < 0) {
        return ret;
    }

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

int JCOLog::write_fully(int fd, const void *buf, int nbytes)
{
    int nwritten = 0;

    while(nwritten < nbytes) {
        int r;

        r = write(fd, (char*)buf + nwritten, nbytes - nwritten);
        if(0 > r) {
            if(errno == EINTR || errno == EAGAIN) {
                sleep(1);
                continue;
            } else {
                ERR("error:%s %d\n", strerror(errno), fd);
                return r;
            }
        } else if(0 == r) {
            break;
        }

        nwritten += r;
    }

    return nwritten;
}

int JCOLog::read_fully(int fd, void *buf, int nbytes)
{
    int nread = 0;

    while(nread < nbytes) {
        int r;

        r = read(fd, (char*) buf + nread, nbytes - nread);
        if(0 > r) {
            if(errno == EINTR || errno == EAGAIN) {
                sleep(1);
                continue;
            } else {
                return r;
            }
        } else if(0 == r) {
            break;
        }

        nread += r;
    }

    return nread;
}

void JCOLog::delete_log_record()
{
    int fd = -1, delnum = 0;
    FILE *rd  = NULL;
    struct stat sb;

    log_lock();

    rd = fopen(m_tmppath.c_str(), "r");
    fd = fileno(rd);
    fstat(fd, &sb);

    char *p = (char *)malloc(sb.st_size);
    if(NULL == p) {
        ERR("malloc error!\n");
        fclose(rd);
        log_unlock();
        return ;
    }

    int len = fread(p, 1, sb.st_size, rd);
    DBG("st_size : %d len : %d\n", (int)sb.st_size, len);
    if(len != sb.st_size) {
        ERR("read error!\n");
        fclose(rd);
        if (p) 
			free(p);
        log_unlock();
        return ;
    }
    fclose(rd);

    int mfd = fileno(m_file);
    ftruncate(mfd, 0);

    char *pp = p;
    for(delnum=m_del_num; delnum > 0 && (p + sb.st_size - pp > 0); delnum--) {
        pp = strstr(pp, "\n");
        if (pp != NULL) {
            pp += strlen("\n");
		} else {
            ERR("del applog error pp == NULL\n");
            break;
        }
    }

	if (pp != NULL) {
		len = fwrite(pp, 1, p + sb.st_size - pp, m_file);
		if(len == p + sb.st_size - pp)
			DBG("Delete %d records success!\n", m_del_num);
	} else {
		len = fwrite("", 1, 1, m_file);
		if(len == 1)
			DBG("Delete %d records success!\n", m_del_num);		 
	}

    m_del_flag = 1;

	if (p) 
		free(p);

    log_unlock();
}

int JCOLog::sync_log_file()
{
    int ret = 0;

    log_lock();

    if(!access(TMPREALLOG, F_OK)) {
        remove(TMPREALLOG);
    }

    if(copy_file(TMPREALLOG, m_tmppath.c_str()) < 0) {
        ERR("Synchro bash command [cp] error!\n");
        ret = -1;
        log_unlock();
        return ret;
    }

    rename(TMPREALLOG, m_path.c_str());

    m_new_add_num = 0; //reset
    m_del_flag = 0;
    log_unlock();

    DBG("Synchro log file Success!\n");
    return ret;
}

