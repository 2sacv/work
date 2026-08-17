/**
 * Copyright (C) by Jabsco Company
 *
 * @File Name    : jcpService.h
 * @Created Time : 2013-12-24
 * @Modified     : 2014-03-26
 * @Version      : 2.0
 * @Author       : zengy zhangjian
 * @Description  :
 */
#ifndef _JCPSERVICE_H
#define _JCPSERVICE_H
#ifdef __cplusplus
extern "C" {
#endif

#define JCP_MAX_INPUT   1024
#define JCP_MAX_LEN 4096

int  jcpcmd_devbatch(char *respbuf, int buflen, int argc, char **argv);
int  jcpcmd_sendrecv(const char *cmdline, char *respbuf, int buflen);
int  jcpcmd_sendrecv2(char *respbuf, int buflen, const char *format, ...);

int  init_server_jcpcmd(void *data);
void set_jcp_authorization();
void uninit_server_jcpcmd();

#ifdef __cplusplus
}
#endif
#endif
