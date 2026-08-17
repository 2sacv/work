/*
 * File Name    :
 * Created Time : 2024-01-15
 * Version      : 1.0
 * Author       : luoshunfa
 * Description  :
 */

#ifndef _SS_THREAD_H_
#define _SS_THREAD_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*ss_once_cb)(void);
typedef void (*ss_thread_cb)(void* arg);


#if  defined(_WIN32)

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#define SS_ONCE_INIT        INIT_ONCE_STATIC_INIT

typedef INIT_ONCE           ss_once_t;
typedef HANDLE              ss_thread_t;
typedef CRITICAL_SECTION    ss_mutex_t;
typedef SRWLOCK             ss_rwlock_t;
typedef CONDITION_VARIABLE  ss_cond_t;
typedef DWORD               ss_key_t;

#elif defined (__linux__) || defined(__APPLE__)

#include <stdint.h>
#include <pthread.h>

#define SS_ONCE_INIT        PTHREAD_ONCE_INIT

typedef pthread_once_t      ss_once_t;
typedef pthread_t           ss_thread_t;
typedef pthread_mutex_t     ss_mutex_t;
typedef pthread_rwlock_t    ss_rwlock_t;
typedef pthread_cond_t      ss_cond_t;
typedef pthread_key_t       ss_key_t;

#else
#warning "unkown system!"
#endif


void ss_once(ss_once_t* guard, ss_once_cb callback);

int ss_thread_create(ss_thread_t* tid, ss_thread_cb entry, void* arg);
int ss_thread_join(ss_thread_t *tid);

int ss_mutex_init(ss_mutex_t* handle);
int ss_mutex_init_recursive(ss_mutex_t* handle);
void ss_mutex_destroy(ss_mutex_t* handle);
void ss_mutex_lock(ss_mutex_t* handle);
int ss_mutex_trylock(ss_mutex_t* handle);
void ss_mutex_unlock(ss_mutex_t* handle);

int ss_rwlock_init(ss_rwlock_t* rwlock);
void ss_rwlock_destroy(ss_rwlock_t* rwlock);
void ss_rwlock_rdlock(ss_rwlock_t* rwlock);
int ss_rwlock_tryrdlock(ss_rwlock_t* rwlock);
void ss_rwlock_rdunlock(ss_rwlock_t* rwlock);
void ss_rwlock_wrlock(ss_rwlock_t* rwlock);
int ss_rwlock_trywrlock(ss_rwlock_t* rwlock);
void ss_rwlock_wrunlock(ss_rwlock_t* rwlock);

int ss_cond_init(ss_cond_t* cond);
void ss_cond_destroy(ss_cond_t* cond);
void ss_cond_signal(ss_cond_t* cond);
void ss_cond_broadcast(ss_cond_t* cond);
void ss_cond_wait(ss_cond_t* cond, ss_mutex_t* mutex);
int ss_cond_timedwait(ss_cond_t* cond, ss_mutex_t* mutex, int timeoutms);

int ss_key_create(ss_key_t* key);
void ss_key_delete(ss_key_t* key);
void* ss_key_get(ss_key_t* key);
void ss_key_set(ss_key_t* key, void* value);

#ifdef __cplusplus
}
#endif

#endif


