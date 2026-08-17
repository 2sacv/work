/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : jcolog.h
 * @Created Time : 2013-10-15
 * @Version      : 1.0
 * @Author       : zengy
 * @Description  :
 */

#ifndef _JCOLOG_H_
#define _JCOLOG_H_

#include <stdio.h>
#include <string>

#include "mxml.h"
#include "js_scheduler.h"

using namespace std;

class JCOLog
{
public:
    int            log_init();
    int            log_init_client_sync(void *data);

    void           reclaim() {
        delete this;
    };
	
	int 		   sync_log_file();

protected:
    JCOLog(JSScheduler engine, const char *path, const char *tmppath, int maxsize, int maxrecord, int delnum);
    virtual ~JCOLog();

    virtual int   add_record(void *req, void *resp) = 0;
    virtual int   query_log(void *req, void *resp, int resplen, int *extlen) = 0;
    void          log_lock() {
        pthread_mutex_lock(&m_mutex);
    }
    void          log_unlock() {
        pthread_mutex_unlock(&m_mutex);
    }

    int           get_file_fd() {
        if (m_file) {
            return fileno(m_file);
        } else {
            return fileno(stdout);
        }
    };
    int &         get_new_add_num() {
        return m_new_add_num;
    };

private:
    static void   log_client_sync(void *instance);
    static int    do_add_record(void *req, void *buf, int bufsize, int *extlen, void *instance);
    static int    do_sync_log(void *req, void *buf, int bufsize, int *extlen, void *instance);
    static int    do_query_log(void *req, void *buf, int bufsize, int *extlen, void *instance);
    void          log_client_sync1();
    int           check_log_file();
    int           do_log_init();
    int           get_file_size();

    int           copy_file(const char *dst, const char *src);
    int           write_fully(int fd, const void* buf, int nbytes);
    int           read_fully(int fd, void* buf, int nbytes);

    void          delete_log_record();

private:
    JSScheduler 	m_pScheduler;	
    JSTCHandle		m_timing_sync_task;

    string          m_path;
    string          m_tmppath;

    int             m_max_size;     //Mb
    int             m_max_record;
    int             m_del_num;
    int             m_new_add_num;  //Each add 1000 records need to be synchronized

    int             m_del_flag;

    FILE            *m_file;
    pthread_mutex_t m_mutex;
};

#endif

