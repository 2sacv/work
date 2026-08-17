 /*      Filename:  aliyun_mmi_hal.c
 *    Description:  阿里云MMI SDK HAL层实现
 *        Version:  1.0
 *        Created:  07/10/2026 10:01:29 AM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  xiangyp (), 
 *   Organization:  
 */
#include <errno.h>
#include <sys/time.h>

#include "debug.h"
#include "g_log.h"
#include "utils.h"
#include "c_utils.h"
#include "factory_db.h"
#include "hal_util_mutex.h"
#include "aliyun_mmi_server.h"

/* ===== 1. 内存管理 ===== */
void *util_malloc(int32_t size)
{
    void *ptr = NULL;

    goto_exit_if_fail(size > 0, exit, NULL, "invalid malloc size %d\n", size);

    ptr = malloc(size);
    goto_exit_if_fail(NULL != ptr, exit, NULL, "failed to malloc size %d\n", size);

    //pri_mmi(LVL_LOOP, "succ to malloc %d bytes\n", size);

exit:

    return ptr;
}

void util_free(void *ptr)
{
    if (NULL != ptr) {
        free(ptr);
    } else {
        ERR("try to free a null ptr\n");
    }
}

void *util_realloc(void *ptr, int32_t size)
{
    void *new_ptr = NULL;

    if (size > 0) {
        new_ptr = realloc(ptr, size);
        goto_exit_if_fail(NULL != new_ptr, exit, new_ptr = ptr,
                          "failed to malloc size %d\n", size);
    } else if (0 == size) {
        if (NULL != ptr) {
            free(ptr);
        }
    } else {
        ERR("invalid realloc size %d\n", size);
        goto exit;
    }

    //pri_mmi(LVL_LOOP, "succ to malloc %d bytes\n", size);

exit:

    return new_ptr;
}

/* ===== 2. 互斥锁（基于pthread_mutex）===== */
util_mutex_t *util_mutex_create(void)
{
    util_mutex_t *um = NULL;
    pthread_mutex_t *pm = NULL;
    int ret = UTIL_SUCCESS;

    um = (util_mutex_t *)calloc(1, sizeof(util_mutex_t));
    goto_exit_if_fail(NULL != um, exit, ret = UTIL_ERR_NO_MEMORY,
                      "failed to malloc util_mutex_t\n");

    pm = (pthread_mutex_t *)calloc(1, sizeof(pthread_mutex_t));
    goto_exit_if_fail(NULL != pm, exit, ret = UTIL_ERR_NO_MEMORY,
                      "failed to malloc pthread_mutex_t\n");

    ret = pthread_mutex_init(pm, NULL);
    goto_exit_if_fail(0 == ret, exit, ret = UTIL_ERR_INIT_FAIL,
                      "failed to init pthread_mutex\n");

    um->mutex_handle = (void *)pm;

    //pri_mmi(LVL_LOOP, "succ to create util mutex\n");
    ret = UTIL_SUCCESS;

exit:

    if (UTIL_SUCCESS != ret) {
        if (NULL != pm) {
            free(pm);
            pm = NULL;
        }

        if (NULL != um) {
            free(um);
            um = NULL;
        }
    }

    return um;
}

void util_mutex_delete(util_mutex_t *um)
{
    int ret = UTIL_SUCCESS;

    if (NULL == um) {
        goto exit;
    }

    if (NULL != um->mutex_handle) {
        ret = pthread_mutex_destroy((pthread_mutex_t *)(um->mutex_handle));
        if (ret != 0) {
            ERR("failed to destroy mutex\n");
        }

        free(um->mutex_handle);
    }

    free(um);

exit:

    return;
}

int32_t util_mutex_lock(util_mutex_t *um, int32_t timeout)
{
    struct timespec ts = {0};
    pthread_mutex_t *pm = NULL;
    int ret = UTIL_SUCCESS;

    goto_exit_if_fail(NULL != um, exit, ret = UTIL_ERR_INVALID_PARAM,
                      "param um is null\n");
    goto_exit_if_fail(NULL != um->mutex_handle, exit, ret = UTIL_ERR_INVALID_PARAM,
                      "param um->mutex_handle is null\n");

    pm = (pthread_mutex_t *)um->mutex_handle;

    if (MUTEX_WAIT_FOREVER == timeout) {
        ret = pthread_mutex_lock(pm);
        goto_exit_if_fail(0 == ret, exit, ret = UTIL_ERR_FAIL,
                          "failed to lock mutex\n");
    } else {
        clock_gettime(CLOCK_MONOTONIC, &ts);
        ts.tv_sec += (timeout / 1000);
        ts.tv_nsec += (timeout % 1000) * 1000000;

        ret = pthread_mutex_timedlock(pm, &ts);
        if (ETIMEDOUT == ret) {
            ERR("lock mutex timeout\n");
            ret = UTIL_ERR_TIMEOUT;
            goto exit;
        } else if (0 != ret) {
            ERR("failed to lock mutex\n");
            ret = UTIL_ERR_FAIL;
            goto exit;
        }
    }

    //pri_mmi(LVL_LOOP, "succ to lock mutex\n");    
    ret = UTIL_SUCCESS;

exit:
    
    return ret;
}

int32_t util_mutex_unlock(util_mutex_t *um)
{
    int ret = UTIL_SUCCESS;

    goto_exit_if_fail(NULL != um, exit, ret = UTIL_ERR_INVALID_PARAM,
                      "param um is null\n");
    goto_exit_if_fail(NULL != um->mutex_handle, exit, ret = UTIL_ERR_INVALID_PARAM,
                      "param um->mutex_handle is null\n");

    ret = pthread_mutex_unlock((pthread_mutex_t *)um->mutex_handle);
    if (0 == ret) {
        //pri_mmi(LVL_LOOP, "succ to unlock mutex\n");
        ret = UTIL_SUCCESS;
    } else {
        ERR("failed to unlock mutex\n");
        ret = UTIL_ERR_FAIL;
    }

exit:

    return ret;
}

/* ===== 3. 存储（文件系统持久化设备证书）===== */
int32_t util_storage_erase(void)
{
    int ret = remove(F_MMI_CERT);
    if (0 == ret) {
        //pri_mmi(LVL_LOOP, "succ to erase %s\n", F_MMI_CERT);
    } else {
        ERR("failed to erase %s: %s\n", F_MMI_CERT, strerror(errno));
    }

    return ret;
}

int32_t util_storage_storage(uint8_t *data, uint32_t size)
{
    int ret = UTIL_SUCCESS;

    goto_exit_if_fail(NULL != data, exit, ret = UTIL_ERR_INVALID_PARAM,
                      "param data is null\n");
    goto_exit_if_fail(size >= 0, exit, ret = UTIL_ERR_INVALID_PARAM,
                      "param size < 0\n");

    ret = Write2File(F_MMI_CERT, (const char *)data, size);
    if (SUCCESS == ret) {
        //pri_mmi(LVL_LOOP, "succ to storage %s\n", F_MMI_CERT);
        ret = UTIL_SUCCESS;
    } else {
        ERR("failed to storage %s: %s\n", F_MMI_CERT, strerror(errno));
        ret = UTIL_ERR_FAIL;
    }

exit:

    return ret;
}

int32_t util_storage_load(uint8_t *data, uint32_t size)
{
    int ret = UTIL_SUCCESS;

    goto_exit_if_fail(NULL != data, exit, ret = UTIL_ERR_INVALID_PARAM,
                      "param data is null\n");
    goto_exit_if_fail(size > 0, exit, ret = UTIL_ERR_INVALID_PARAM,
                      "param size <= 0\n");

    ret = LoadFile(F_MMI_CERT, (char *)data, size);
    if (ret > 0) {
        //pri_mmi(LVL_LOOP, "succ to load %s\n", F_MMI_CERT);
        ret = UTIL_SUCCESS;
    } else {
        ERR("failed to load %s: %s\n", F_MMI_CERT, strerror(errno));
        ret = UTIL_ERR_FAIL;
    }

exit:

    return ret;
}

/* ===== 4. 时间（基于gettimeofday）===== */
int64_t util_now_ms(void)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);

    return (int64_t)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

int64_t util_get_timestamp(void)
{
    return util_now_ms();
}

void util_msleep(uint32_t ms)
{
    ms_sleep(ms);
}

uint8_t util_timestamp_inited(void)
{
    return aliyun_mmi_tz_synced();
}

/* ===== 5. 随机数（基于标准C库）===== */
int32_t util_random_init(uint32_t seed)
{
    srand(seed);

    //pri_mmi(LVL_LOOP, "random init with seed: %u\n", seed);

    return UTIL_SUCCESS;
}

uint32_t util_random(void)
{
    return (uint32_t)rand();
}
