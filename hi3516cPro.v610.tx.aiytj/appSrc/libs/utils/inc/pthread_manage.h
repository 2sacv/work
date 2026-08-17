/**
 * Copyright (C) by Jabsco Company
 * 
 * @File Name    : servicePthreadManage.h
 * @Created Time : 2013-12-11
 * @Version      : 1.0
 * @Author       : zengy
 * @Description  : 
 */

#ifndef __SERVICEPTHREADMANAGE_H__
#define __SERVICEPTHREADMANAGE_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>

typedef void *(*pthread_func)(void *);

pthread_t pthread_namecreate(const char *name, pthread_func func, void *arg);

pthread_t create_pthread(const char *name, pthread_func func, pthread_attr_t *attr, void *arg);

void      join_pthread(pthread_t pid);

void      quit_all_pthread();

void      print_all_pthread_info();

#ifdef __cplusplus
}
#endif
#endif

