/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    : fifo_queue.h
 * Created Time : 2012-09-21
 * Version      : 1.0
 * Author       : luoshunfa
 * Description  :
 */

#ifndef _fifo_queue_h_
#define _fifo_queue_h_
#ifdef __cplusplus
extern "C" {
#endif

#include "pthread.h"

typedef struct _queue_elmt_t
{
	void *  data;
	struct _queue_elmt_t * next;
} queue_elmt_t;

typedef struct
{
	queue_elmt_t *first_elmt;
	queue_elmt_t *last_elmt;

	pthread_mutex_t lock;
	pthread_cond_t  signal;
} queue_t;


queue_t *   create_fifo_queue(void);
void        release_fifo_queue(queue_t* pQueue);

int         fifo_queue_push(queue_t* pQueue, void *data);
int         fifo_queue_push_head(queue_t* pQueue, void *data);
void *      fifo_queue_pop(queue_t* pQueue);
void *      fifo_queue_pop_unblock(queue_t* pQueue);	
void        clear_fifo_queue(queue_t* pQueue);

#ifdef __cplusplus
}
#endif
#endif

