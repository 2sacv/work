/** Copyright (C) by Jabsco Company
*
* @File Name	: circular_queue.h
* @Created Time : 2024-12-12
* @Version		: 1.0
* @Author		: 
* @Description	: 循环队列，使用 c++ 原子变量，非线程安全，只能单线程读单线程写
*/
#ifndef CIRCULAR_QUEUE_H__
#define CIRCULAR_QUEUE_H__

#include <atomic>
#include <cstddef>

template <typename T, std::size_t Size>
class CircularQueue {
public:
    CircularQueue() : read_(0), write_(0) {
        static_assert(Size > 0 && (Size & (Size - 1)) == 0, "Size must be a power of 2");
        // Ensure the queue size is a power of 2 for easier modulo calculation
    }

    bool EnqueueFront(const T& item) { // 头部插入队列
        std::size_t read = read_.load(std::memory_order_relaxed);
        std::size_t prev_read = (read - 1) & (Size - 1);
        if (prev_read == write_.load(std::memory_order_acquire)) {
            // Queue is full
            return false;
        }
        buffer_[prev_read] = item;
        read_.store(prev_read, std::memory_order_release);
        return true;
    }

    bool Enqueue(const T& item) { // 入队
        std::size_t write = write_.load(std::memory_order_relaxed);
        std::size_t next_write = (write + 1) & (Size - 1);
        if (next_write == read_.load(std::memory_order_acquire)) {
            // Queue is full
            return false;
        }
        buffer_[write] = item;
        write_.store(next_write, std::memory_order_release);
        return true;
    }

    bool Dequeue(T& item) { // 出队
        std::size_t read = read_.load(std::memory_order_relaxed);
        if (read == write_.load(std::memory_order_acquire)) {
            // Queue is empty
            return false;
        }
        item = buffer_[read];
        read_.store((read + 1) & (Size - 1), std::memory_order_release);
        return true;
    }
    
    bool IsEmpty() const { // 判断队列是否为空
        return read_.load(std::memory_order_acquire) == write_.load(std::memory_order_acquire);
    }

    void ClearQueue() { // 清空队列
        read_.store(write_.load(std::memory_order_relaxed), std::memory_order_release);
        return ;
    }
private:
    T buffer_[Size];
    std::atomic<std::size_t> read_; // 读指针
    std::atomic<std::size_t> write_; // 写指针
};

#endif
