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

    void push(const T& value)
    {
        std::lock_guard<std::mutex> _(mutex_);
        queue_.push(value);
        cond_.notify_one();
    }
    T wait_and_pop()
    {
        std::unique_lock<std::mutex> _(mutex_);
        cond_.wait(_, [this] { return !queue_.empty(); });
        T value = queue_.front();
        queue_.pop();
        return std::move(value);
    }
    bool empty() const
    {
        std::lock_guard<std::mutex> _(mutex_);
        return queue_.empty();
    }

private:
    mutable std::mutex mutex_;
    std::queue<T> queue_;
    std::condition_variable cond_;
};

}  // namespace wayz

#endif