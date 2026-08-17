/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    : record_file_mange.h
 * Created Time : 2012-10-16
 * Version      : 1.0
 * Author       : luoshunfa
 * Description  :
 */


#ifndef _record_file_manage_H_
#define _record_file_manage_H_

#include "record_watch.h"

#ifdef __cplusplus
extern "C" {
#endif
#define TMPFILE_LEN    9999
#define TMPFILE_SUFFIX "-9999.mp4"
	
    int generate_record_dir(const char *inPath);
	int generate_record_filename(eJRecType rectype, char *filename, time_t start_utc);
    int is_removing_files(void);
    void remove_oldest_dir(void *data);
	
	int get_lastest_record_date(char *buf, int bufsize);
	int get_lastest_record_datetime(char *buf, int bufsize);

#ifdef __cplusplus
}
#endif
#endif

