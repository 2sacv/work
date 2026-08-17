/**
 * Copyright (C) by Jabsco Company
 * 
 * @File Name    : pipe_utils.cpp
 * @Created Time : 2014-06-30
 * @Version      : 1.0
 * @Author       : zengy
 * @Description  : 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "debug.h"
#include "pipe_utils.h"

int create_pipe(int fd[2])
{
	int ret = -1;

	ret = pipe(fd);
	if(ret > -1)
		return ret;

	ret = pipe(fd);
	if(ret <= -1) {
		ERR("create_pipe failed!\n");
	}

	return ret;
}

int close_pipe(int fd[2])
{
	int ret = 0;

	for(int i=0; i < 2; i++) {
		int j = -1;
		
		do {
			ret = close(fd[i]);
			if(ret > -1) {
				fd[i] = -1;
				break;
			}

			if(errno == EAGAIN || errno == EINTR) {
				j++;
				if(j < 3) continue;
			}

			return ret;
		}
		while(1);
	}

	return ret;
}

int write_pipe(int fd, const void *buf, int nbytes)
{
    int nwritten = 0;
	
    while (nwritten < nbytes) {
        int r = write(fd, (char*)buf + nwritten, nbytes - nwritten);
        if (0 > r) {
            if (errno == EINTR || errno == EAGAIN) {
                usleep(5000);
                continue;
            } else {
                DBG("error:%s %d\n", strerror(errno), fd);
                return r;
            }
        } else if (0 == r) {
            break;
        }
        nwritten += r;
    }

    return nwritten;	
}

int read_pipe(int fd, void *buf, int bufSize)
{
	int num = 0, i = -1;
	
	do {
		num = read(fd, buf, bufSize);
		if(num > -1)
			break;

		if(errno == EINTR || errno == EAGAIN) {
			i++;
			if(i < 3) continue;
		}

		break;
	}
	while(1);
	
	return num;
}

int set_pipe_nonblock(int fd[2])
{
	int flags = fcntl(fd[0], F_GETFL, 0);
	
	fcntl(fd[0], F_SETFL, flags|O_NONBLOCK);
	fcntl(fd[1], F_SETFL, flags|O_NONBLOCK);

	return 0;
}

