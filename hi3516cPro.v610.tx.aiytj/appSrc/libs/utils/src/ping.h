/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    : ping.h
 * Created Time : 2014-03-05
 * Version      : 1.0
 * Author       : tangpengcheng
 * Description  :
 */

#ifndef __PING__H__
#define __PING__H__

#ifdef __cplusplus
extern "C" {
#endif

typedef int (* ping_result_cb)(char *buff, void *data);

typedef struct ping_opt_s 
{
	char *dest;
	int pack_num;
	ping_result_cb handle;
	void* data;
	int timeout;
} ping_opt_t;

typedef void* HANDLE;


int ping_init(HANDLE* ping_obj, ping_opt_t* para);
int ping(HANDLE ping_obj);
int ping_realse(HANDLE* ping_obj);


#ifdef __cplusplus
}
#endif

#endif

