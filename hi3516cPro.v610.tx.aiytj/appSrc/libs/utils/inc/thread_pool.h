/**
 * Copyright (C) by Jabsco Company
 * 
 * @File Name    : thread_pool.h
 * @Created Time : 2014-03-11
 * @Version      : 1.0
 * @Author       : zengy
 * @Description  : From OpenSource
 */

#ifndef _RECORD_THREAD_POOL_H_
#define _RECORD_THREAD_POOL_H_
#ifdef __cplusplus
extern "C" {
#endif

int thread_pool_init(int max_thread_num);

int pool_add_worker(void *(*process)(void *arg), void *arg);

int pool_destroy();

#ifdef __cplusplus
}
#endif
#endif

