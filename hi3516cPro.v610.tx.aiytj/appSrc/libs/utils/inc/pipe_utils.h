/**
 * Copyright (C) by Jabsco Company
 * 
 * @File Name    : pipe_utils.h
 * @Created Time : 2014-06-30
 * @Version      : 1.0
 * @Author       : zengy
 * @Description  : 
 */

#ifndef _PIPE_UTILS_H_
#define _PIPE_UTILS_H_
#ifdef __cplusplus
extern "C" {
#endif

#define  MSG_MAGIC_STR   "V01"

typedef struct notify_msg_s {
    char msg_magic[4];
    int  msg_type;
    int  msg_len;
    char msg_buf[0];
} notify_msg_t;

int create_pipe(int fd[2]);

int close_pipe(int fd[2]);

int write_pipe(int fd, const void* buf, int nbytes);

int read_pipe(int fd, void *buf, int bufSize);

int set_pipe_nonblock(int fd[2]);

#ifdef __cplusplus
}
#endif
#endif

