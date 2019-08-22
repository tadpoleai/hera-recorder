//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#ifndef __utils_threadsafe_queue_hpp__
#define __utils_threadsafe_queue_hpp__
#include <condition_variable>
#include <mutex>
#include <queue>

namespace wayz {

template<typename T>
class ThreadsafeQueue {
public:
    ThreadsafeQueue() {}
    ThreadsafeQueue(const ThreadsafeQueue&) = delete;
    ThreadsafeQueue& operator=(const ThreadsafeQueue&) = delete;

    void push(T value)
    {
        std::lock_guard<std::mutex> _(data_mutex_);
        data_queue_.push(value);
        data_cond_.notify_one();
    }
    T wait_and_pop()
    {
        std::unique_lock<std::mutex> _(data_mutex_);
        data_cond_.wait(_, [this] { return !data_queue_.empty(); });
        T value = data_queue_.front();
        data_queue_.pop();
        return value;
    }
    bool empty() const
    {
        std::lock_guard<std::mutex> _(data_mutex_);
        return data_queue_.empty();
    }

private:
    mutable std::mutex data_mutex_;
    std::queue<T> data_queue_;
    std::condition_variable data_cond_;
};

}  // namespace wayz

#endif