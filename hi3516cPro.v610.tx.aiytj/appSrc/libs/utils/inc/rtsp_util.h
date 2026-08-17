/******************************************************************************
    Copyright (C), 2008-2018, JABSCO ELECTRONIC Tech. Co., Ltd
 
    File Name    : rtsp_util.h
    Version      : 1.0
    Author       : JABSCO Video Server Software Group
    Created      : 2009-12-24
    Description  : 
    History      : 
                        created by lsf. 2009-12-24
******************************************************************************/
#ifndef _H_RTSP_UTIL_
#define _H_RTSP_UTIL_
#ifdef __cplusplus
extern "C"{
#endif

void gen_random_serial(char *random);

int rtsp_parse_url(char* msg, char* url);
int rtsp_parse_cseq(char* msg);

char* rtsp_parse_param_line(char* msg, char* param_name);
char *rtsp_parse_param_line_within_range(char* msg, int length, char* param_name);

int rtsp_is_interleaved_msg_complete(char * msg, int length);
int rtsp_is_msg_complete(char * msg, int length);
int get_rtcp_msg_len(char*buf);
char* rtsp_get_body(char* msg);
int rtsp_get_msg(char * buf, int buf_len, char *msg);

#ifdef __cplusplus
}
#endif
#endif

