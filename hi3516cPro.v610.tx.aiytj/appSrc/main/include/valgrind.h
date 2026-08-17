/*
 *       Filename:  valgrind.h
 *    Description:  
 *        Version:  1.0
 *        Created:  05/05/2018 03:04:43 PM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  zhangjian (), 
 *   Organization:  
 */


#ifndef _VALGRIND_H
#define _VALGRIND_H
#ifdef __cplusplus 
extern "C" {
#endif

#include <pthread.h>

/* macro */
#ifndef MIN
#define MIN(x,y)    ((x)<(y)?(x):(y))
#endif
#ifndef MAX
#define MAX(x,y)    ((x)>(y)?(x):(y))
#endif

/* declaration */
void vm_statistic();

void vm_set_dbg(int dbg);
void vm_set_tracer_p(int p);
void vm_clr_tracer_p();
void vm_set_xhold(int xhold);

void vm_wrap_start();
void vm_wrap_stop();
void vm_wrap_restart();
void vm_pri_tracer_p(void *_ra, FILE *fp, const char *act, void *ptr);
int  vm_backtrace(void ** array, int size);
int  vm_bigfreekb(int i_bud /* 8 512, 9 1M, 10 4M, 11 16M */);

void * __wrap_malloc(int size);
void __wrap_free(void *ptr);
void *__wrap_strdup(const char *s);
void *__wrap_strndup(const char *s);
int  __wrap_pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg);
void vm_pri_thread_caller();
void vm_pri_thread_running();
void vm_pri_thread_leaking();

#ifdef __cplusplus
}
#endif
#endif
