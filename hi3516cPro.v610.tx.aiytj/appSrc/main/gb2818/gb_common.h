/** Copyright (C) by Jabsco Company
*
* @File Name    : gb_common.h
* @Created Time : 2024-05-06
* @Version      : 1.0
* @Author       :
* @Description  : 国标公共接口文件
*/

#ifndef GB_COMMON_H_
#define GB_COMMON_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <stdint.h>
#include <assert.h>

#include <vector>
#include <memory>
#include <atomic>
#include <cstddef>

#include "debug.h"
#include "utils.h"
#include "g_log.h"

#define GB_DBG   DBG
#define GB_ERR   ERR
#define GB_INFO  dbg_gb28181

// #define GB28181_2022  // gb2022 开关, 移到 cc.rule 中

#ifdef CUST_SNAPSHOT
#define MAX_PIC_LEN 160*1024
#else
#define MAX_PIC_LEN 0
#endif
#define MAX_XML_LEN MAX_PIC_LEN+4096

template <typename T, std::size_t Size>
class SeqQueue {
public:
    SeqQueue() : head_(0), tail_(0) {
        static_assert(Size > 0 && (Size & (Size - 1)) == 0, "Size must be a power of 2");
        // Ensure the queue size is a power of 2 for easier modulo calculation
    }

    bool Enqueue(const T& item) {
        std::size_t tail = tail_.load(std::memory_order_relaxed);
        std::size_t next_tail = (tail + 1) & (Size - 1);
        if (next_tail == head_.load(std::memory_order_acquire)) {
            // Queue is full
            return false;
        }
        buffer_[tail] = item;
        tail_.store(next_tail, std::memory_order_release);
        return true;
    }

    bool Dequeue(T& item) {
        std::size_t head = head_.load(std::memory_order_relaxed);
        if (head == tail_.load(std::memory_order_acquire)) {
            // Queue is empty
            return false;
        }
        item = buffer_[head];
        head_.store((head + 1) & (Size - 1), std::memory_order_release);
        return true;
    }

private:
    T buffer_[Size];
    std::atomic<std::size_t> head_;
    std::atomic<std::size_t> tail_;
};


class Timer {
public:
    Timer() : target_time_(0) {}
    ~Timer() {};

    /*  开始定时器
     *
     *  @param target_time_s 预期时间，从当前时间往后多少秒 ex:30
     *  @return
     */
    void Start(time_t target_time_s) {
        target_time_ = mono_stamp() + target_time_s;
    }

    /* 判断定时是否到达，时间到了之后需要调用 Start 更新时间，否则条件会一直为真
    *
    *  @param current_time_s 当前时间，不给这个参数的话会调用函数自动获取，
    *                       如果外部有统一获取，直接传进来，少一次函数调用
    *  @return true: 定时时间到达
    *          false:定时时间未到
    */
    bool IsTimerExpired(time_t current_time_s = 0) {
        if (current_time_s == 0) {
            current_time_s = mono_stamp();
        }

        return target_time_ < current_time_s ? true : false;
    }

    /*  停止定时器，停止之后 IsTimerExpired 会一直为真
     *
     *  @param
     *  @return
     */
    void Stop() {
        target_time_ = 0;
    }

    /*  获取定时器使能
     *
     *  @param
     *  @return true: 使能  false:未使能
     */
    bool IsEnable() {
        return target_time_ != 0;
    }
private:
    time_t target_time_;
};

/*用于 run function 带参数使用*/
template <typename T1, typename T2>
class SchedulerParam {
public:
    /*sync == true 代表同步执行，sync == false 代表异步执行*/
    /*type 可选，不用填 0 就行*/
    SchedulerParam(const std::shared_ptr<T1> &this_ptr, bool sync, int type)
        : sync_(sync), result_(0), type_(type), this_ptr_(this_ptr), data2_(NULL) {

        bzero(&data1_, sizeof(T2));
    };

    void set_result(int result) {
        result_ = result;
    }
    int get_result() const {
        return result_;
    }

    void set_type(int type) {
        type_ = type;
    }
    int get_type() const {
        return type_;
    }

    std::shared_ptr<T1> &get_this_ptr() {
        return this_ptr_;
    }

    void set_data1(const T2 &data1) {
        data1_ = data1; // 这个是浅拷贝，如果 T2 里面有指针不要保留，其它地方一旦释放，就成野指针了
    }
    T2 &get_data1() {
        return data1_;
    }
    T2 *get_data1_ptr() {
        return &data1_;
    }

    void set_data2(void *data2) {
        data2_ = data2;
    }
    void *get_data2() const {
        return data2_;
    }


    void set_sync(bool sync) {
        sync_ = sync;
    }
    bool get_sync() const {
        return sync_;
    }

    bool IsSync() const {
        return sync_;
    }
private:
    bool sync_;  // 同步标识
    int result_; // 执行结果，同步执行才有效 sync = true
    int type_;   // 事件类型，选用

    std::shared_ptr<T1> this_ptr_;  // 类指针 this
    T2 data1_; // 自定义数据 1
    void *data2_; // 自定义数据 2
};
#endif
