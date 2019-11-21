//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#pragma once

#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>

using namespace std::chrono_literals;

namespace wayz {
namespace hera {

///
/// @brief Multi-thread-safe queue of shared/unique pointer
///
/// @tparam T underlying data type
/// @tparam bool shared, use shared pointer if true(default), otherwise unique pointer
template<class T, bool shared = true>
class ThreadQueue final {
private:
    ///
    /// @brief shared pointer to T typed data
    ///
    using sharedTPtr = std::shared_ptr<T>;

    ///
    /// @brief unique pointer to T typed data
    ///
    using uniqueTPtr = std::unique_ptr<T>;

    ///
    /// @brief determine whether shared / unique pointer to use
    ///
    using DataType = std::conditional_t<shared, sharedTPtr, uniqueTPtr>;

public:
    ///
    /// @brief Construct a new Thread Queue object
    ///
    /// @param capacity Capacity of queue
    ThreadQueue(size_t capacity = 0, std::chrono::milliseconds wait_duration = 10ms) :
        Capacity_(capacity),
        Wait_Duration_(wait_duration)
    {}

    ThreadQueue(const ThreadQueue&) = delete;
    ThreadQueue& operator=(const ThreadQueue&) = delete;

    ///
    /// @brief Push or emplace a storage data into queue
    ///
    /// @param data data to push
    /// @return true operation succeed
    /// @return false operation failed, due to capacity over
    /// Check the capacity first, and then
    /// push the data into underlying stl queue and notify waiter
    bool push(DataType&& data)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (Capacity_ > 0 && queue_.size() >= Capacity_) {
            return false;
        }
        queue_.emplace(std::forward<DataType>(data));
        lock.unlock();
        cond_.notify_one();
        return true;
    }

    ///
    /// @brief Pop a data from queue
    ///
    /// @note pop a data immediately if queue is not empty,
    /// otherwise, wait for new data until a certain timeout
    /// @return DataType data poped from queue, or nullptr if timed out
    DataType wait_pop()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        const bool fulfilled =
                cond_.wait_for(lock, Wait_Duration_, [this] { return !queue_.empty(); });
        DataType data = nullptr;
        if (fulfilled) {
            data = std::move(queue_.front());
            queue_.pop();
        }
        return data;
    }

private:
    const size_t Capacity_;                          ///< Capacity of queue
    const std::chrono::milliseconds Wait_Duration_;  ///< Duration to wait for pop

    mutable std::mutex mutex_;      ///< mutex for operations to queue
    std::queue<DataType> queue_;    ///< underlying stl queue
    std::condition_variable cond_;  ///< condition Variable for push/pop operations
};

}  // namespace hera
}  // namespace wayz