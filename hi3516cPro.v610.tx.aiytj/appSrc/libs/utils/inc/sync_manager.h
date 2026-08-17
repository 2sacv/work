/*
 *       Filename:  sync_manager.h
 *    Description:  
 *        Version:  1.0
 *        Created:  03/16/2026 09:36:26 PM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  xiangyp (), 
 *   Organization:  
 */

/*
 * 本API提供线程安全的机制，用于协调多个生产者线程与单个消费者线程之间的同步。
 * 适用于多个生产者独立生成结果，单个消费者需要等待所有（或任意）结果可用的场景。
 * 
 * ## 典型使用场景：
 * 
 * 1. **并行数据处理**：多个工作线程处理数据集的不同部分，主线程等待所有结果。
 *    例如：图像处理、科学计算、数据分析等并行计算任务。
 *    
 * 2. **任务分发与收集**：主线程将任务分发给工作线程，并收集它们的结果。
 *    例如：Web服务器请求处理、批量文件处理、分布式计算等。
 *    
 * 3. **异步作业系统**：多个作业并发运行，协调器等待所有作业完成。
 *    例如：后台任务处理、定时任务调度、异步消息处理等。
 *    
 * 4. **实时数据聚合**：多个数据源向单个处理线程提供数据。
 *    例如：传感器数据收集、日志聚合、实时监控系统等。
 *    
 * 5. **批处理流水线**：带有同步点的并行处理阶段。
 *    例如：ETL流程、数据处理流水线、多阶段计算任务等。
 * 
 * ## 主要特性：
 * 
 * - 使用pthread互斥锁和条件变量实现线程安全操作
 * - 支持等待所有结果完成 (sync_manager_wait_all) 和等待任意结果到达 (sync_manager_wait_any)
 * - 等待操作支持超时机制
 * - 任务取消能力
 * - 进度跟踪和统计信息
 * - 结果数据的内存管理
 * 
 * ## 内存管理说明：
 * 
 * - sSyncManager拥有所有sTaskResult对象并管理其内存
 * - sTaskResult中的数据会被复制，调用者保留原始数据的所有权
 * - 使用sync_manager_destroy()正确清理所有资源
 * 
 * ## 线程安全性：
 * 
 * 所有函数都是线程安全的，可以从多个线程并发调用，除非明确说明例外情况。
 */

#ifndef _SYNC_MANAGER_H
#define _SYNC_MANAGER_H
#ifdef __cplusplus 
extern "C" {
#endif

#include <time.h>
#include <pthread.h>

/**
 * @brief 任务结果数据结构
 * 
 * 表示生产者线程生成的单个任务结果。
 * 包含任务的元数据和实际的结果数据。
 */
typedef struct {
    int task_id;           /**< 任务的唯一标识符 */
    void *data;            /**< 指向任务结果数据的指针 */
    size_t data_size;      /**< 数据大小（字节） */
    int producer_id;       /**< 生产者线程的标识符 */
    time_t timestamp;      /**< 结果提交的时间戳 */
} sTaskResult;

/**
 * @brief 同步管理器结构体
 * 
 * 协调生产者和消费者的主同步管理器。
 */
typedef struct {
    pthread_mutex_t mutex;           // 互斥锁
    pthread_cond_t  cond_all_done;   // 全部完成条件变量
    pthread_cond_t  cond_new_result; // 新结果到达条件变量
    sTaskResult **  results;         // 结果数组
    int *result_status;              // 结果状态: 0-未完成, 1-完成
    int  tasks_total;                // 总任务数
    int  tasks_completed;            // 已完成任务数
    int  producers;                  // 活跃生产者数
    int  is_cancelled;               // 是否被取消
    int  secs_timeout;               // 超时时间(秒)，0表示无限等待
} sSyncManager;

/**
 * @brief 错误码定义
 */
typedef enum {
    E_SYNC_ERR_CANCELLED   = -6,   // 操作被取消
    E_SYNC_ERR_TIMEOUT     = -5,   // 操作超时
    E_SYNC_ERR_MEMORY      = -4,   // 内存分配失败
    E_SYNC_ERR_COND        = -3,   // 条件变量操作失败
    E_SYNC_ERR_MUTEX       = -2,   // 互斥锁操作失败
    E_SYNC_ERR_INVALID_ARG = -1,   // 参数无效
    E_SYNC_SUCCESS         = 0,    // 操作成功
} eSyncError;

/**
 * @brief 创建同步管理器
 * 
 * 创建并初始化一个新的同步管理器实例。
 * 
 * @param tasks_total 总任务数量，必须大于0
 * @param secs_timeout 等待超时时间（秒），0表示无限等待
 * @return 成功返回sSyncManager指针，失败返回NULL
 * 
 * @note 调用者负责使用sync_manager_destroy()释放资源
 */
sSyncManager *sync_manager_create(int tasks_total, int secs_timeout);

/**
 * @brief 销毁同步管理器
 * 
 * 释放同步管理器及其所有相关资源。
 * 
 * @param manager 要销毁的同步管理器
 * @return 成功返回SYNC_SUCCESS，失败返回错误码
 */
int sync_manager_destroy(sSyncManager *manager);

/**
 * @brief 重置同步管理器
 * 
 * 重置管理器状态，需要在消费者消费完之后主动重置
 * 
 * @param manager 要重置的同步管理器
 * @return 成功返回SYNC_SUCCESS，失败返回错误码
 */
int sync_manager_reset(sSyncManager *manager);

/**
 * @brief 生产者注册
 * 
 * 注册一个生产者线程，应在生产者线程开始工作前调用。
 * 
 * @param manager 同步管理器
 * @return 成功返回SYNC_SUCCESS，失败返回错误码
 */
int sync_manager_register_producer(sSyncManager *manager);

/**
 * @brief 生产者注销
 * 
 * 注销一个生产者线程。应在生产者线程完成工作后调用。
 * 
 * @param manager 同步管理器
 * @return 成功返回SYNC_SUCCESS，失败返回错误码
 */
int sync_manager_unregister_producer(sSyncManager *manager);

/**
 * @brief 生产者提交结果
 * 
 * 生产者线程提交一个任务的结果。
 * 
 * @param manager 同步管理器
 * @param task_id 任务ID，必须在[0, tasks_total-1]范围内
 * @param data 指向结果数据的指针
 * @param data_size 数据大小（字节）
 * @param producer_id 生产者标识符
 * @param keep_newest 生产者重复提交，是否保持更新到最新的数据
 * @return 成功返回SYNC_SUCCESS，失败返回错误码
 * 
 * @note 数据会被复制，调用者保留原始数据的所有权
 */
int sync_manager_submit_result(sSyncManager *manager, int task_id, 
                               void *data, size_t data_size, int producer_id,
                               int keep_newest);

/**
 * @brief 消费者等待所有结果
 * 
 * 消费者线程等待所有任务完成。
 * 
 * @param manager 同步管理器
 * @param results 用于接收结果数组指针的输出参数
 * @param completed_count 用于接收完成任务数量的输出参数
 * @return 成功返回SYNC_SUCCESS，超时返回SYNC_ERR_TIMEOUT，取消返回SYNC_ERR_CANCELLED
 * 
 * @note 返回的结果数组由同步管理器管理，不要手动释放
 */
int sync_manager_wait_all(sSyncManager *manager, sTaskResult ***results, 
                          int *completed_count);

/**
 * @brief 消费者等待任意结果
 * 
 * 消费者线程等待任意一个任务完成。
 * 
 * @param manager 同步管理器
 * @param result 用于接收结果指针的输出参数
 * @param task_id 用于接收任务ID的输出参数
 * @param timeout_ms 等待超时时间（毫秒），0表示无限等待
 * @return 成功返回SYNC_SUCCESS，超时返回SYNC_ERR_TIMEOUT，取消返回SYNC_ERR_CANCELLED
 */
int sync_manager_wait_any(sSyncManager *manager, sTaskResult **result, 
                          int *task_id, int timeout_ms);

/**
 * @brief 取消所有等待
 * 
 * 取消所有正在进行的等待操作，并唤醒所有等待的线程。
 * 
 * @param manager 同步管理器
 */
void sync_manager_cancel(sSyncManager *manager);

/**
 * @brief 获取进度信息
 * 
 * 获取当前的任务完成进度和活跃生产者数量。
 * 
 * @param manager 同步管理器
 * @param completed 用于接收已完成任务数量的输出参数
 * @param total 用于接收总任务数量的输出参数
 * @param active 用于接收活跃生产者数量的输出参数
 * @return 成功返回SYNC_SUCCESS，失败返回错误码
 */
int sync_manager_get_progress(sSyncManager *manager, 
                              int *completed, int *total, int *active);

/**
 * @brief 获取指定任务的结果
 * 
 * 获取指定任务ID的结果。
 * 
 * @param manager 同步管理器
 * @param task_id 任务ID
 * @return 成功返回sTaskResult指针，失败或结果不存在返回NULL
 */
sTaskResult *sync_manager_get_result(sSyncManager *manager, int task_id);

/**
 * @brief 释放结果数组
 * 
 * 释放由sync_manager_wait_all返回的结果数组。
 * 
 * @param results 结果数组
 * @param count 结果数量
 * 
 * @note 仅释放结果对象本身，不释放数组指针
 */
void sync_manager_free_results(sTaskResult **results, int count);

/**
 * @brief 打印统计信息
 * 
 * 打印同步管理器的当前统计信息到标准输出。
 * 
 * @param manager 同步管理器
 */
void sync_manager_print_stats(sSyncManager *manager);

#ifdef __cplusplus
}
#endif
#endif
