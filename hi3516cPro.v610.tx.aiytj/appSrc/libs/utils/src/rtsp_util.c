/******************************************************************************
    Copyright (C), 2008-2018, JABSCO ELECTRONIC Tech. Co., Ltd
 
    File Name    : rtsp_util.c
    Version      : 1.0
    Author       : JABSCO Video Server Software Group
    Created      : 2009-12-24
    Description  : 
    History      : 
                        created by lsf. 2009-12-24
******************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "rtsp_util.h"

void gen_random_serial(char *random)
{
    static char smask[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    int i;
        
    for (i=0; i<16; i++)
        random[i] = smask[rand() % strlen(smask)];
    random[16] = 0;
}

int rtsp_parse_url(char* msg, char* url)
{
    char * url_start, * url_end;
    
    url_start = strstr(msg, "rtsp://");
    if (url_start == NULL)
        return -1;
        
    url_end = url_start + 7;
    while (*url_end != ' ' && 
           *url_end != '\t' &&
           *url_end != '\r')
      url_end ++;
    
    if (*url_end == '\r')
        return -1;
    
    memcpy(url, url_start, url_end-url_start);
    url[url_end-url_start] = 0;
    return url_end-url_start;
}

int rtsp_parse_cseq(char* msg)
{
    char * p;
    int cseq;

    cseq = 0;
    // Parse CSeq field
    p = rtsp_parse_param_line(msg, "CSeq");
    if (p != NULL)
        cseq = atoi(p);
    
    return cseq;
}

char* rtsp_parse_param_line(char* msg, char* param_name)
{
    char * line_start, * line_end, * param1, * param2, * param;
    
    line_start = strstr(msg, param_name);
    if (line_start == NULL)
        return NULL;
        
    line_end = strstr(line_start, "\r\n");
    if (line_end == NULL)
        return NULL;
        
    param1 = strstr(line_start, "=");
    param2 = strstr(line_start, ":");
    if ((param1 == NULL || param1 >= line_end) && (param2 == NULL || param2 >= line_end))
        return NULL;
    
    if (param1 == NULL)
        param = param2;
    else if (param2 == NULL)
        param = param1;
    else
        param = param1 < param2 ? param1 : param2;
    
    param += 1;
    while (*param == ' ' || *param == '\t')
        param ++;
    
    if (*param == '\r' || *param == '\n')
        return NULL;
    
    return param;
}

char *rtsp_parse_param_line_within_range(char* msg, int length, char* param_name)
{
    char * line_start, * line_end, * param1, * param2, * param;
    
    line_start = strstr(msg, param_name);
    if (line_start == NULL)
        return NULL;
    if (line_start-msg > length)
        return NULL;
        
    line_end = strstr(line_start, "\r\n");
    if (line_end == NULL)
        return NULL;
    if (line_end-msg > length)
        return NULL;
        
    param1 = strstr(line_start, "=");
    param2 = strstr(line_start, ":");
    if ((param1 == NULL || param1 >= line_end) && (param2 == NULL && param2 >= line_end))
        return NULL;
    
    if (param1 == NULL)
        param = param2;
    else if (param2 == NULL)
        param = param1;
    else
        param = param1 < param2 ? param1 : param2;
    
    param += 1;
    while (*param == ' ' || *param == '\t')
        param ++;
    
    if (*param == '\r' || *param == '\n')
        return NULL;
    
    return param;
}

int rtsp_is_interleaved_msg_complete(char * msg, int length)
{
    unsigned short len;
    
    if (msg[0] != '$')
        return 0;
    if (length < 4)
        return 0;
    
    len = ((unsigned char)msg[2] << 8) + (unsigned char)msg[3]; 
    if (length >= len + 4)
        return 1;
    
    return 0;
}

int rtsp_is_msg_complete(char * msg, int length)
{
    int m, n;
    char * p;
    
    if (msg[0] == '$')
        return rtsp_is_interleaved_msg_complete(msg, length);
    
    // Find the end of header
    p = strstr(msg, "\r\n\r\n");
    if (p == NULL)
        return 0;
    m = p - msg;
    
    // Find "Content-Length" header line
    p = rtsp_parse_param_line_within_range(msg, m, "Content-Length");
    if (p == NULL)  // No content body
    {
        p = rtsp_parse_param_line_within_range(msg, m, "Content-length");
    }
    if (p == NULL)
    {
        return 1;
    }
    n = atoi(p);
    if (n <= 0)
    {
        return 1;
    }
    
    if (length >= m+n+4)
        return 1;
    else
        return 0;
}

int get_rtcp_msg_len(char*buf)
{
    unsigned short len;
    if(buf[0] == '$'){
        len = ((unsigned char)buf[2] << 8) + (unsigned char)buf[3];
        return len+4;
    }
    else
        return 0;
}

char* rtsp_get_body(char* msg)
{
    char * p;
    
    p = strstr(msg, "Content-Length");
    if (p == NULL)
        p = strstr(msg, "Content-length");
    if (p == NULL)
        return NULL;
    
    p = strstr(msg, "\r\n\r\n");
    if (p == NULL)
        return NULL;
        
    p += 4;
        
    if (p-msg >= (int)strlen(msg))
        return NULL;
    
    return p;
}

int rtsp_get_msg(char * buf, int buf_len, char *msg)
{
    int m, n;
    char * p;
    unsigned short len;
    
    if(buf[0] == '$')
    {
        len = ((unsigned char)buf[2] << 8) + (unsigned char)buf[3]; 
        memcpy(msg, buf, len+4);
        return len+4;
    }
    
    // Find the end of header
    p = strstr(buf, "\r\n\r\n");
    if (p == NULL)
        return 0;
    m = p - buf;
    
    // Find "Content-Length" header line
    p = rtsp_parse_param_line_within_range(buf, m, "Content-Length");
    if (p == NULL)  // No content body
    {
        p = rtsp_parse_param_line_within_range(buf, m, "Content-length");
    }
    if (p == NULL)
    {
        n = 0;
    }
    else
    {
        n = atoi(p);
    }
    
    if (buf_len >= m+n+4)
    {
        memset(msg, 0, m+n+5);
        memcpy(msg, buf, m+n+4);
        return m+n+4;
    }
    else
        return 0;
}

