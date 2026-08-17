/*
 * Copyright (C) by Jabsco Company
 *
 * File Name    : fifo_queue.c
 * Created Time : 2012-09-21
 * Version      : 1.0
 * Author       : luoshunfa
 * Description  :
 */

#include <stdlib.h>

#include "fifo_queue.h"
#include <sys/time.h>

queue_t *create_fifo_queue(void)
{
    queue_t* pQueue = NULL;

    pQueue = (queue_t*)malloc(sizeof(queue_t));

    pQueue->first_elmt = NULL;
    pQueue->last_elmt = NULL;

    pthread_mutex_init(&pQueue->lock, NULL);
    pthread_cond_init(&pQueue->signal, NULL);

    return pQueue;
}

void release_fifo_queue(queue_t* pQueue)
{
    if(pQueue == NULL)
        return;

    free(pQueue);
}

int fifo_queue_push(queue_t* pQueue, void *data)
{
    int ret = 0;
    queue_elmt_t *pElem = NULL;

    if(pQueue== NULL || data == NULL)
        return 0;

    pthread_mutex_lock(&pQueue->lock);

    pElem = (queue_elmt_t *)malloc(sizeof(queue_elmt_t));
    if(pElem == NULL)
    {
        ret = -1;
        goto exit;
    }

    pElem->data = data;
    pElem->next = NULL;

    if(!pQueue->first_elmt)
        pQueue->first_elmt = pElem;
    else
        pQueue->last_elmt->next = pElem;
    pQueue->last_elmt = pElem;

    pthread_cond_signal(&pQueue->signal);

exit:
    pthread_mutex_unlock(&pQueue->lock);

    return ret;
}

int fifo_queue_push_head(queue_t* pQueue, void *data)
{
    int ret = 0;
    queue_elmt_t *pElem = NULL;

    if(pQueue== NULL || data == NULL)
        return 0;

    pthread_mutex_lock(&pQueue->lock);

    pElem = (queue_elmt_t *)malloc(sizeof(queue_elmt_t));
    if(pElem == NULL)
    {
        ret = -1;
        goto exit;
    }

    pElem->data = data;
    pElem->next = NULL;

    if(!pQueue->first_elmt) {
        pQueue->last_elmt = pElem;
    } else {
        pElem->next = pQueue->first_elmt;
    }
    pQueue->first_elmt = pElem;

    pthread_cond_signal(&pQueue->signal);

exit:
    pthread_mutex_unlock(&pQueue->lock);

    return ret;
}

void *fifo_queue_pop(queue_t* pQueue)
{
    queue_elmt_t *pElem = NULL;
    void *pData = NULL;

    if(pQueue == NULL)
        return NULL;

    pthread_mutex_lock(&pQueue->lock);

    while(!pQueue->first_elmt)
    {
        pthread_cond_wait(&pQueue->signal, &pQueue->lock);
    }

    pElem = pQueue->first_elmt;
    pQueue->first_elmt = pElem->next;
    if(!pElem->next)
        pQueue->last_elmt = NULL;

    pData = pElem->data;
    free(pElem);

    pthread_mutex_unlock(&pQueue->lock);

    return pData;
}


void *fifo_queue_pop_unblock(queue_t* pQueue)
{
    queue_elmt_t *pElem = NULL;
    void *pData = NULL;

    if(pQueue == NULL)
        return NULL;

    pthread_mutex_lock(&pQueue->lock);

    if(!pQueue->first_elmt)
    {
        goto exit;
    }

    pElem = pQueue->first_elmt;
    pQueue->first_elmt = pElem->next;
    if(!pElem->next)
        pQueue->last_elmt = NULL;

    pData = pElem->data;
    if (pElem)
        free(pElem);

exit:
    pthread_mutex_unlock(&pQueue->lock);

    return pData;
}

void clear_fifo_queue(queue_t* pQueue)
{
    if(pQueue == NULL){
        return;
    }
    
    queue_elmt_t *pElem = NULL;
    queue_elmt_t *nElem = NULL;
    pElem = pQueue->first_elmt;

    if(!pQueue->first_elmt)
    {
        return;
    }

    while(pElem){
        nElem = pElem->next;
        free(pElem);
        pElem = nElem;
    }

    pQueue->first_elmt = pQueue->last_elmt = NULL;

    return;
}
