#ifndef __SHM_BUF_POOL__H__
#define __SHM_BUF_POOL__H__

#ifdef __cplusplus
extern "C" {
#endif

#include "shm_buf.h"

typedef struct shmbuf_pool{
    shm_buf_t buf;
    int size;
}shmbuf_pool_t;

void init_shm_buf_pool(void);
void uninit_shm_buf_pool(void);
shm_buf_t get_shm_buf_pool(int idx);
void reset_shm_buf_pool(int idx);

#ifdef __cplusplus
}
#endif
#endif
