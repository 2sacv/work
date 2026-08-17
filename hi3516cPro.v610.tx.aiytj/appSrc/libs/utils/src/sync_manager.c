/* 
 *       Filename:  sync_manager.c
 *    Description:  
 *        Version:  1.0
 *        Created:  03/16/2026 09:35:28 PM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  xiangyp (), 
 *   Organization:  
 */

#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include "debug.h"
#include "sync_manager.h"

// 创建同步管理器
sSyncManager *sync_manager_create(int tasks_total, int secs_timeout)
{
    int ret = E_SYNC_SUCCESS, mutex_inited = FALSE;
    int cond_all_inited = FALSE, cond_new_inited = FALSE;
    sSyncManager *manager = NULL;

    goto_exit_if_fail(tasks_total > 0, exit, ret = E_SYNC_ERR_INVALID_ARG,
                      "tasks must greater than 0\n");

    manager = (sSyncManager *)calloc(1, sizeof(sSyncManager));
    goto_exit_if_fail(NULL != manager, exit, ret = E_SYNC_ERR_MEMORY,
                      "failed to calloc manager\n");

    // 初始化互斥锁和条件变量
    ret = pthread_mutex_init(&manager->mutex, NULL);
    goto_exit_if_fail(0 == ret, exit, ret = E_SYNC_ERR_COND, "failed to init mutex\n");
    mutex_inited = TRUE;

    ret = pthread_cond_init(&manager->cond_all_done, NULL);
    goto_exit_if_fail(0 == ret, exit, ret = E_SYNC_ERR_COND,
                      "failed to init cond all done\n");
    cond_all_inited = TRUE;

    ret = pthread_cond_init(&manager->cond_new_result, NULL);
    goto_exit_if_fail(0 == ret, exit, ret = E_SYNC_ERR_COND,
                      "failed to init cond new result\n");
    cond_new_inited = TRUE;

    // 分配结果数组
    manager->results = (sTaskResult **)calloc(tasks_total, sizeof(sTaskResult *));
    goto_exit_if_fail(NULL != manager->results, exit, ret = E_SYNC_ERR_MEMORY,
                      "failed to calloc results\n");

    manager->result_status = (int *)calloc(tasks_total, sizeof(int));
    goto_exit_if_fail(NULL != manager->result_status, exit, ret = E_SYNC_ERR_MEMORY,
                      "failed to calloc result status\n");

    // 初始化其他字段
    manager->tasks_total = tasks_total;
    manager->tasks_completed = 0;
    manager->producers = 0;
    manager->is_cancelled = false;
    manager->secs_timeout = secs_timeout;

    ret = E_SYNC_SUCCESS;

exit:

    if (ret != E_SYNC_SUCCESS && NULL != manager) {
        if (NULL != manager->result_status) {
            free(manager->result_status);
            manager->result_status = NULL;
        }

        if (NULL != manager->results) {
            free(manager->results);
            manager->results = NULL;
        }

        if (cond_new_inited) {
            pthread_cond_destroy(&manager->cond_new_result);
        }

        if (cond_all_inited) {
            pthread_cond_destroy(&manager->cond_all_done);
        }

        if (mutex_inited) {
            pthread_mutex_destroy(&manager->mutex);
        }

        free(manager);
        manager = NULL;
    }

    return manager;
}

// 销毁同步管理器
int sync_manager_destroy(sSyncManager *manager)
{
    int ret = E_SYNC_SUCCESS;

    goto_exit_if_fail(NULL != manager, exit, ret = E_SYNC_ERR_INVALID_ARG,
                      "arg manager is null\n");

    pthread_mutex_lock(&manager->mutex);

    // 释放所有结果
    for (int idx = 0; idx < manager->tasks_total; idx++) {
        if (NULL != manager->results[idx]) {
            if (manager->results[idx]->data) {
                free(manager->results[idx]->data);
            }

            free(manager->results[idx]);
            manager->results[idx] = NULL;
        }
    }

    if (NULL != manager->results) {
        free(manager->results);
    }

    if (NULL != manager->result_status) {
        free(manager->result_status);
    }

    pthread_mutex_unlock(&manager->mutex);

    // 销毁互斥锁和条件变量
    pthread_mutex_destroy(&manager->mutex);
    pthread_cond_destroy(&manager->cond_all_done);
    pthread_cond_destroy(&manager->cond_new_result);

    free(manager);

    ret = E_SYNC_SUCCESS;

exit:

    return ret;
}

// 重置同步管理器
int sync_manager_reset(sSyncManager *manager)
{
    int ret = E_SYNC_SUCCESS;

    goto_exit_if_fail(NULL != manager, exit, ret = E_SYNC_ERR_INVALID_ARG,
                      "arg manager is null\n");

    pthread_mutex_lock(&manager->mutex);

    //释放所有结果
    for (int i = 0; i < manager->tasks_total; i++) {
        if (manager->results[i]) {
            if (manager->results[i]->data) {
                free(manager->results[i]->data);
            }
            free(manager->results[i]);
            manager->results[i] = NULL;
        }
        manager->result_status[i] = 0;
    }

    //重置所有状态
    manager->tasks_completed = 0;
    manager->is_cancelled = FALSE;

    pthread_mutex_unlock(&manager->mutex);

    ret = E_SYNC_SUCCESS;

exit:

    return ret;
}

// 生产者注册
int sync_manager_register_producer(sSyncManager *manager)
{
    int ret = E_SYNC_SUCCESS;

    goto_exit_if_fail(NULL != manager, exit, ret = E_SYNC_ERR_INVALID_ARG,
                      "arg manager is null\n");

    pthread_mutex_lock(&manager->mutex);
    manager->producers++;
    pthread_mutex_unlock(&manager->mutex);

    ret = E_SYNC_SUCCESS;

exit:

    return ret;
}

// 生产者注销
int sync_manager_unregister_producer(sSyncManager *manager)
{
    int ret = E_SYNC_SUCCESS, all_done = FALSE;

    goto_exit_if_fail(NULL != manager, exit, ret = E_SYNC_ERR_INVALID_ARG,
                      "arg manager is null\n");

    pthread_mutex_lock(&manager->mutex);

    if (manager->producers > 0) {
        manager->producers--;
    }

    // 检查是否所有生产者都已完成
    if (manager->producers == 0 && manager->tasks_completed >= manager->tasks_total) {
        all_done = TRUE;
    }

    pthread_mutex_unlock(&manager->mutex);

    // 如果所有生产者都已完成，通知消费者
    if (all_done) {
        pthread_cond_signal(&manager->cond_all_done);
    }

    ret = E_SYNC_SUCCESS;

exit:

    return ret;
}

// 生产者提交结果
int sync_manager_submit_result(sSyncManager *manager, int task_id,
                               void *data, size_t data_size, int producer_id,
                               int keep_newest)
{
    int ret = E_SYNC_SUCCESS, unlocked = FALSE;
    sTaskResult *result = NULL;

    goto_exit_if_fail(NULL != manager, exit, ret = E_SYNC_ERR_INVALID_ARG,
                      "arg manager is null\n");
    goto_exit_if_fail(task_id >= 0 || task_id < manager->tasks_total, exit,
                      ret = E_SYNC_ERR_INVALID_ARG, "arg task_id is out of range\n");

    pthread_mutex_lock(&manager->mutex);

    // 检查是否已取消
    if (manager->is_cancelled) {
        WAR("manager cancelled wait\n");
        ret = E_SYNC_ERR_CANCELLED;
        goto unlock_exit;
    }

    // 检查任务是否已完成，且不需要更新最新的数据
    if (manager->result_status[task_id] && !keep_newest) {
        ret = E_SYNC_SUCCESS;  // 任务已提交，忽略重复提交
        goto unlock_exit;
    }

    // 创建结果对象
    result = (sTaskResult *)calloc(1, sizeof(sTaskResult));
    goto_exit_if_fail(NULL != result, unlock_exit, ret = E_SYNC_ERR_MEMORY,
                      "failed to calloc result\n");

    result->task_id = task_id;
    result->producer_id = producer_id;
    result->timestamp = time(NULL);

    // 复制数据
    if (data && data_size > 0) {
        result->data = malloc(data_size);
        goto_exit_if_fail(NULL != result->data, unlock_exit, ret = E_SYNC_ERR_MEMORY,
                          "failed to calloc result data\n");
        memcpy(result->data, data, data_size);
        result->data_size = data_size;
    } else {
        result->data = NULL;
        result->data_size = 0;
    }

    // 之前已经提交过数据，不进行 tasks 累加
    if (!manager->result_status[task_id]) {
        manager->tasks_completed++;
    }
    manager->results[task_id] = result;
    manager->result_status[task_id] = 1;

    pthread_mutex_unlock(&manager->mutex);
    unlocked = TRUE;

    // 通知等待的消费者
    pthread_cond_signal(&manager->cond_new_result);

    // 如果所有任务都已完成，通知等待的消费者
    if (manager->tasks_completed >= manager->tasks_total) {
        pthread_cond_broadcast(&manager->cond_all_done);
    }

    ret = E_SYNC_SUCCESS;

unlock_exit:

    if (E_SYNC_SUCCESS != ret) {
        if (NULL != result) {
            if (NULL != result->data) {
                free(result->data);
            }

            free(result);
        }
    }

    if (!unlocked) {
        pthread_mutex_unlock(&manager->mutex);
        unlocked = TRUE;
    }

exit:

    return ret;
}

// 消费者等待所有结果
int sync_manager_wait_all(sSyncManager *manager, sTaskResult ***results,
                          int *completed_count)
{
    struct timespec ts = {0};
    int ret = E_SYNC_SUCCESS, wait_result = 0, unlocked = FALSE;

    goto_exit_if_fail(NULL != manager, exit, ret = E_SYNC_ERR_INVALID_ARG,
                      "arg manager is null\n");
    goto_exit_if_fail(NULL != results, exit, ret = E_SYNC_ERR_INVALID_ARG,
                      "arg results is null\n");
    goto_exit_if_fail(NULL != completed_count, exit, ret = E_SYNC_ERR_INVALID_ARG,
                      "arg completed_count is null\n");

    pthread_mutex_lock(&manager->mutex);

    // 如果已经有超时设置，计算超时时间
    if (manager->secs_timeout > 0) {
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += manager->secs_timeout;
    }

    // 等待所有任务完成或超时
    while (manager->tasks_completed < manager->tasks_total &&
           !manager->is_cancelled && manager->producers > 0) {

        if (manager->secs_timeout > 0) {
            wait_result = pthread_cond_timedwait(&manager->cond_all_done,
                                                 &manager->mutex, &ts);
            goto_exit_if_fail(ETIMEDOUT != wait_result, unlock_exit,
                              ret = E_SYNC_ERR_TIMEOUT, "wait all task done timeout\n");
        } else {
            pthread_cond_wait(&manager->cond_all_done, &manager->mutex);
        }
    }

    // 检查是否被取消
    if (manager->is_cancelled) {
        WAR("manager cancelled wait\n");
        ret = E_SYNC_ERR_CANCELLED;
        goto unlock_exit;
    }

    // 返回结果
    *results = manager->results;
    *completed_count = manager->tasks_completed;

    pthread_mutex_unlock(&manager->mutex);
    unlocked = TRUE;

    ret = E_SYNC_SUCCESS;

unlock_exit:

    if (E_SYNC_SUCCESS != ret) {
        *results = NULL;
        *completed_count = manager->tasks_completed;
    }

    if (!unlocked) {
        pthread_mutex_unlock(&manager->mutex);
    }

exit:

    return ret;
}

// 消费者等待任一结果
int sync_manager_wait_any(sSyncManager *manager, sTaskResult **result,
                          int *task_id, int timeout_ms)
{
    struct timespec ts = {0};
    int ret = E_SYNC_SUCCESS, wait_result = 0;

    goto_exit_if_fail(NULL != manager, exit, ret = E_SYNC_ERR_INVALID_ARG,
                      "arg manager is null\n");
    goto_exit_if_fail(NULL != result, exit, ret = E_SYNC_ERR_INVALID_ARG,
                      "arg result is null\n");
    goto_exit_if_fail(NULL != task_id, exit, ret = E_SYNC_ERR_INVALID_ARG,
                      "arg task_id is null\n");

    pthread_mutex_lock(&manager->mutex);

    // 计算超时时间
    if (timeout_ms > 0) {
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += timeout_ms / 1000;
        ts.tv_nsec += (timeout_ms % 1000) * 1000000;
        if (ts.tv_nsec >= 1000000000) {
            ts.tv_sec += 1;
            ts.tv_nsec -= 1000000000;
        }
    }
    // 等待新结果或超时
    while (manager->tasks_completed == 0 &&
           !manager->is_cancelled && manager->producers > 0) {

        if (timeout_ms > 0) {
            wait_result = pthread_cond_timedwait(&manager->cond_new_result,
                                                 &manager->mutex, &ts);
            goto_exit_if_fail(ETIMEDOUT != wait_result, result_null_exit,
                              ret = E_SYNC_ERR_TIMEOUT, "wait all task done timeout\n");
        } else {
            pthread_cond_wait(&manager->cond_new_result, &manager->mutex);
        }
    }

    // 检查是否被取消
    if (manager->is_cancelled) {
        WAR("manager cancelled wait\n");
        ret = E_SYNC_ERR_CANCELLED;
        goto result_null_exit;
    }

    // 找到第一个未处理的结果
    for (int i = 0; i < manager->tasks_total; i++) {
        if (manager->result_status[i] == 1) {
            *result = manager->results[i];
            *task_id = i;
            ret = E_SYNC_SUCCESS;
            goto unlock_exit;
        }
    }

result_null_exit:

    *result = NULL;
    *task_id = -1;

unlock_exit:

    pthread_mutex_unlock(&manager->mutex);

exit:

    return ret;
}

// 取消所有等待
void sync_manager_cancel(sSyncManager *manager)
{
    goto_exit_if_fail(NULL != manager, exit, NULL, "arg manager is null\n");

    pthread_mutex_lock(&manager->mutex);
    manager->is_cancelled = true;
    pthread_mutex_unlock(&manager->mutex);

    // 唤醒所有等待的线程
    pthread_cond_broadcast(&manager->cond_all_done);
    pthread_cond_broadcast(&manager->cond_new_result);

exit:

    return;
}

// 获取进度信息
int sync_manager_get_progress(sSyncManager *manager,
                              int *completed, int *total, int *active)
{
    int ret = E_SYNC_SUCCESS;

    goto_exit_if_fail(NULL != manager, exit, ret = E_SYNC_ERR_INVALID_ARG,
                      "arg manager is null\n");
    goto_exit_if_fail(NULL != completed, exit, ret = E_SYNC_ERR_INVALID_ARG,
                      "arg completed is null\n");
    goto_exit_if_fail(NULL != total, exit, ret = E_SYNC_ERR_INVALID_ARG,
                      "arg total is null\n");
    goto_exit_if_fail(NULL != active, exit, ret = E_SYNC_ERR_INVALID_ARG,
                      "arg active is null\n");

    pthread_mutex_lock(&manager->mutex);
    *completed = manager->tasks_completed;
    *total = manager->tasks_total;
    *active = manager->producers;
    pthread_mutex_unlock(&manager->mutex);

    ret = E_SYNC_SUCCESS;

exit:

    return ret;
}

// 获取指定任务的结果
sTaskResult *sync_manager_get_result(sSyncManager *manager, int task_id)
{
    sTaskResult *result = NULL;

    goto_exit_if_fail(NULL != manager, exit, result = NULL, "arg manager is null\n");
    goto_exit_if_fail(task_id >= 0 && task_id < manager->tasks_total, exit,
                      result = NULL, "arg task_id is out of range\n");

    pthread_mutex_lock(&manager->mutex);
    if (manager->result_status[task_id] == 1) {
        result = manager->results[task_id];
    }
    pthread_mutex_unlock(&manager->mutex);

exit:

    return result;
}

// 释放结果数组
void sync_manager_free_results(sTaskResult **results, int count)
{
    goto_exit_if_fail(NULL != results, exit, NULL, "arg results is null\n");

    // 注意：这里不释放results数组本身，因为它由sync_manager管理
    for (int i = 0; i < count; i++) {
        if (results[i]) {
            if (results[i]->data) {
                free(results[i]->data);
            }
            free(results[i]);
        }
    }

exit:

    return;
}

// 打印统计信息
void sync_manager_print_stats(sSyncManager *manager)
{
    int completed = 0, total = 0, active = 0;

    goto_exit_if_fail(NULL != manager, exit, NULL, "arg manager is null\n");

    sync_manager_get_progress(manager, &completed, &total, &active);

    COLOR_G("========== sync manager statistics start ==========\n");
    COLOR_G("task total: %d\n", total);
    COLOR_G("task complete: %d\n", completed);
    COLOR_G("producers online: %d\n", active);
    COLOR_G("task progress: %.1f%%\n", total > 0 ? (completed * 100.0 / total) : 0.0);
    COLOR_G("=========== sync manager statistics end ===========\n");

exit:

    return;
}

// 测试函数
void test_sync_api()
{
    DBG("start testing sync manager API...\n");

    // 创建同步管理器
    sSyncManager *manager = sync_manager_create(10, 5); // 10个任务，5秒超时
    if (!manager) {
        ERR("failed to create sync manager\n");
        return;
    }

    DBG("1. basic function test...\n");

    // 模拟生产者线程提交结果
    sync_manager_register_producer(manager);

    char data[64] = {0};

    for (int i = 0; i < 5; i++) {

        snprintf(data, sizeof(data), "task%d data is from producer1", i);

        sync_manager_submit_result(manager, i, data, strlen(data) + 1, 1, TRUE);

        DBG("producer1 submit task%d\n", i);
        usleep(100000);         // 模拟处理时间
    }

    sync_manager_unregister_producer(manager);

    // 注册第二个生产者
    sync_manager_register_producer(manager);

    for (int i = 5; i < 10; i++) {
        snprintf(data, sizeof(data), "task%d data is from producer2", i);

        sync_manager_submit_result(manager, i, data, strlen(data) + 1, 2, TRUE);

        DBG("producer2 submit task%d\n", i);
        usleep(150000);         // 模拟处理时间
    }

    sync_manager_unregister_producer(manager);

    // 消费者等待所有结果
    sTaskResult **results = NULL;
    int completed_count = 0;

    int ret = sync_manager_wait_all(manager, &results, &completed_count);
    if (ret == E_SYNC_SUCCESS) {
        DBG("\nall task is completed! total %d tasks:\n", completed_count);
        for (int i = 0; i < completed_count; i++) {
            if (results[i]) {
                DBG("task%d: producer%d, data: %s\n", results[i]->task_id,
                     results[i]->producer_id, (char *)results[i]->data);
            }
        }
    } else if (ret == E_SYNC_ERR_TIMEOUT) {
        DBG("wait timeout，only completed %d tasks\n", completed_count);
    } else if (ret == E_SYNC_ERR_CANCELLED) {
        DBG("operation cancelled\n");
    }

    sync_manager_print_stats(manager);

    // 测试获取单个结果
    DBG("\n2. test get single result...\n");
    sTaskResult *single_result = sync_manager_get_result(manager, 3);
    if (single_result) {
        DBG("task3 result: %s\n", (char *)single_result->data);
    }

    // 测试等待任一结果
    DBG("\n3. test wait any result completed...\n");
    sTaskResult *any_result = NULL;
    int task_id = -1;

    void *cb_test_submit_aync(void *arg) {
        sSyncManager *m = (sSyncManager *)arg;

        usleep(500000);         // 延迟0.5秒
        char data[] = "submit data async";

        sync_manager_submit_result(m, 0, data, sizeof(data), 99, TRUE);

        sync_manager_unregister_producer(m);

        return NULL;
    }

    // 创建新管理器进行测试
    sSyncManager *manager2 = sync_manager_create(5, 3);

    if (manager2) {
        sync_manager_register_producer(manager2);

        // 启动一个线程模拟异步提交
        pthread_t thread;

        pthread_create(&thread, NULL, cb_test_submit_aync, manager2);

        ret = sync_manager_wait_any(manager2, &any_result, &task_id, 2000);
        if (ret == E_SYNC_SUCCESS && any_result) {
            DBG("got result: task%d, data: %s\n", task_id,
                   (char *)any_result->data);
        } else if (ret == E_SYNC_ERR_TIMEOUT) {
            DBG("wait single result timeout\n");
        }

        pthread_join(thread, NULL);

        sync_manager_destroy(manager2);
    }

    // 清理
    sync_manager_destroy(manager);

    DBG("\ntest complete!\n");
}
