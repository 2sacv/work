/**
 * Copyright (C) by Jabsco Company
 * 
 * @File Name    : thread_pool.cpp
 * @Created Time : 2014-03-11
 * @Version      : 1.0
 * @Author       : zengy
 * @Description  : From OpenSource
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <assert.h>
#include <sys/types.h>
#include <linux/unistd.h>
#include <sys/syscall.h>  

#include "debug.h"
#include "thread_pool.h"
#include "utils.h"

#define  gettid()  syscall(__NR_gettid)  

typedef struct worker
{
    void           *(*process)(void *arg);
    void           *arg;
    
    struct worker  *next;
}CThread_worker;

typedef struct
{
    pthread_mutex_t  queue_lock;
    pthread_cond_t   queue_ready;

    CThread_worker*  queue_head;

    int              shutdown;
    int              max_thread_num;
    int              cur_queue_size;
    pthread_t*       threadid;  
}CThread_pool;

static CThread_pool *pool = NULL;

static void *thread_routine(void *arg);

int thread_pool_init(int max_thread_num)
{
    pool = (CThread_pool *)malloc(sizeof(CThread_pool));
    if(!pool)
    {
        ERR("pool malloc error\n");
        return -1;
    }

    pthread_mutex_init(&(pool->queue_lock), NULL);
    pthread_cond_init(&(pool->queue_ready), NULL);

    pool->queue_head = NULL;
    pool->max_thread_num = max_thread_num;
    pool->cur_queue_size = 0;
    pool->shutdown = 0;

    pool->threadid = (pthread_t *)malloc(max_thread_num * sizeof(pthread_t));
    if(!pool->threadid)
    {
        ERR("pool->threadid malloc error\n");
        free(pool);
        return -1;
    }

    for(int i = 0; i < max_thread_num; i++)
    {
        if (0 != pthread_create(&(pool->threadid[i]), NULL, thread_routine, NULL))
        {
            pool->threadid[i] = 0;
        }
    }

    return 0;
}

int pool_add_worker (void *(*process) (void *arg), void *arg)
{
    CThread_worker *newworker = (CThread_worker *)malloc(sizeof(CThread_worker));
    if(!newworker)
    {
        ERR("newworker malloc error\n");
        return -1;
    }
    
    newworker->process = process;
    newworker->arg = arg;
    newworker->next = NULL;

    pthread_mutex_lock(&(pool->queue_lock));
    
    CThread_worker *member = pool->queue_head;
    if(member != NULL)
    {
        while(member->next != NULL)
        {
            member = member->next;
        }
        
        member->next = newworker;
    }
    else
    {
        pool->queue_head = newworker;
    }
    
    pool->cur_queue_size++;
    pthread_mutex_unlock(&(pool->queue_lock));

    pthread_cond_signal(&(pool->queue_ready));
    
    return 0;
}

int pool_destroy()
{
    if(NULL == pool)
    {
        DBG("pool is NULL!\n");
        return 0;
    }

    if(pool->shutdown)
    {
        return -1;
    }
    
    pool->shutdown = 1;

    pthread_cond_broadcast(&(pool->queue_ready));

    for(int i = 0; i < pool->max_thread_num; i++)
    {
        if (0 != pool->threadid[i])
        {
            pthread_join(pool->threadid[i], NULL);
            pool->threadid[i] = 0;
        }
    }
    
    free(pool->threadid);

    CThread_worker *head = NULL;
    while(pool->queue_head != NULL)
    {
        head = pool->queue_head;
        pool->queue_head = pool->queue_head->next;
        free (head);
    }
    
    pthread_mutex_destroy(&(pool->queue_lock));
    pthread_cond_destroy(&(pool->queue_ready));
    
    free(pool);
    pool = NULL;
    
    return 0;
}

void *thread_routine(void *arg)
{
    SYSLOG("thread: thread_routine, ppid: %ld, pid: %d\n", syscall(SYS_gettid), (int)getpid());
    while(1)
    {
        pthread_mutex_lock(&(pool->queue_lock));
        /*
        pthread_cond_wait() shall be called with 'mutex' locked by the calling thread;
        this function atomically release 'mutex' and cause the calling thread to block
        on the condition variable 'cond'; when wakeup 'mutex' will be locked by it.
        */
        while(pool->cur_queue_size == 0 && !pool->shutdown)
        {
            //DBG("thread %d is waiting\n", (int)gettid());
            pthread_cond_wait(&(pool->queue_ready), &(pool->queue_lock));
        }

        if(pool->shutdown)
        {
            pthread_mutex_unlock(&(pool->queue_lock));

            DBG("thread %d will exit\n", (int)gettid());
            pthread_exit(NULL);
        }
        //DBG("thread 0x%x is starting to work\n", pthread_self());

        pool->cur_queue_size--;
        CThread_worker *worker = pool->queue_head;
        pool->queue_head = worker->next;
        pthread_mutex_unlock(&(pool->queue_lock));

        (*(worker->process))(worker->arg);
        
        free(worker);
        worker = NULL;
    }

    pthread_exit(NULL);
}

