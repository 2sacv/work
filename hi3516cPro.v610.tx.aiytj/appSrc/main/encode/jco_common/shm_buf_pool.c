#include "jconfstruct.h"
#include "conf_list.h"
#include "encode_main.h"
#include "encode_common.h"
#include "shm_buf_pool.h"
#include "debug.h"

static shmbuf_pool_t g_shmbuf_pool[SHM_BUF_END] = {
    { .buf=NULL, .size=(1.2 * 1024 * 1024) },           \
    { .buf=NULL, .size=(256 * 1024) },                  \
    { .buf=NULL, .size=(96 * 1024) },                   \
    { .buf=NULL, .size=(96 * 1024) },                   \
};

void init_shm_buf_pool(void)
{
    struct shmbuf_pool *pool = NULL;

    for (int idx = 0; idx < SHM_BUF_END; idx++) {
        pool = &g_shmbuf_pool[idx];
        pool->buf = shm_buf_new(pool->size);
    }
}

void uninit_shm_buf_pool(void)
{
    struct shmbuf_pool *pool = NULL;

    for (int idx = 0; idx < SHM_BUF_END; idx++) {
        pool = &g_shmbuf_pool[idx];
        if (pool->buf) {
            shm_buf_del(pool->buf);
            pool->buf = NULL;
        }
        SYSLOG("uninit shm buf pool %d\n", idx);
    }
}

shm_buf_t get_shm_buf_pool(int idx)
{
    if (idx < SHM_BUF_MAIN || idx >= SHM_BUF_END) {
        ERR("E_SHM_BUF_IDX idx:%d error\n", idx);
        return NULL;
    }

    struct shmbuf_pool *pool = &g_shmbuf_pool[idx];

    return pool->buf;
}

void reset_shm_buf_pool(int idx)
{
    do {
        if (idx < SHM_BUF_MAIN || idx >= SHM_BUF_END) {
            ERR("E_SHM_BUF_IDX idx:%d error\n", idx);
            break;
        }

        struct shmbuf_pool *pool = &g_shmbuf_pool[idx];

        if (!pool->buf) {
            ERR("buf is null, ixd:%d", idx);
            break;
        }

        shm_buf_reset(pool->buf);
    } while(0);
}

