/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : logstartup.h
 * @Created Time : 2013-12-11
 * @Version      : 1.0
 * @Author       : zengy
 * @Description  :
 */

#ifndef _LOG_API_H_
#define _LOG_API_H_

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct {
		int type;
		int level;
		int itemindex;
		int itemnum;
		char starttime[64];
		char endtime[64];
	}LogQueryInfoS;
	
	typedef struct {
		int type;
		int subtype;
		int level;
		char module[64];
		char msg[256];
	}LogRecordInfoS;

    void *init_server_log();
	
    void *uninit_server_log();
	
    void* init_client_log_sync(void *data);

	int log_record(int type, int subtype, int level, const char *module, const char *fmt, ...);

	int log_query(LogQueryInfoS *info, char *buf, int buflen);

	int log_sync();

#ifdef __cplusplus
}
#endif
#endif

