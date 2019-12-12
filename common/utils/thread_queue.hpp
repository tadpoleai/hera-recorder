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
    ThreadQueue(size_t capacity = 0, size_t history_depth = 0, std::chrono::milliseconds wait_duration = 10ms) :
        Capacity_(capacity),
        History_Depth_(history_depth),
        Wait_Duration_(wait_duration)
    {}

    ThreadQueue(const ThreadQueue&) = delete;
    ThreadQueue& operator=(const ThreadQueue&) = delete;

    ///
    /// @brief Emplace a storage data into queue (for unique pointer)
    ///
    /// @param data data to push
    /// @return true operation succeed
    /// @return false operation failed, due to capacity over
    /// Check the capacity first, and then
    /// push the data into underlying stl queue and notify waiter
    bool emplace(DataType&& data, typename std::enable_if<!shared, bool>* _ = 0)
    {
        if (data == nullptr) {
            return false;
        }

        std::unique_lock<std::mutex> lock(mutex_);
        if (Capacity_ > 0 && queue_.size() >= Capacity_) {
            return false;
        }
        queue_.emplace(std::move(data));
        lock.unlock();
        cond_.notify_one();
        return true;
    }

    ///
    /// @brief Push a storage data into queue (for shared pointer)
    ///
    /// @param data data to push
    /// @param only_history only push to history
    /// @return true operation succeed
    /// @return false operation failed, due to capacity over
    /// Check the capacity first, and then
    /// push the data into underlying stl queue and notify waiter
    bool push(DataType&& data, const bool only_history = false, typename std::enable_if<shared, bool>* _ = 0)
    {
        if (data == nullptr) {
            return false;
        }

        std::unique_lock<std::mutex> lock(mutex_);
        if (History_Depth_ > 0) {
            history_.push_front(data);
            if (history_.size() > History_Depth_) {
                history_.pop_back();
            }
        }

        if (!only_history) {
            if (Capacity_ > 0 && queue_.size() >= Capacity_) {
                return false;
            }
            queue_.emplace(std::forward<DataType>(data));
            lock.unlock();
            cond_.notify_one();
            return true;
        }
        return false;
    }

    ///
    /// @brief Pop a data from queue
    ///
    /// @note pop a data immediately if queue is not empty,
    /// otherwise, return nullptr immediately
    /// @return DataType data poped from queue, or nullptr if empty
    DataType pop()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            return nullptr;
        } else {
            auto data = std::move(queue_.front());
            queue_.pop();
            return data;
        }
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
        const bool fulfilled = cond_.wait_for(lock, Wait_Duration_, [this] { return !queue_.empty(); });
        DataType data = nullptr;
        if (fulfilled) {
            data = std::move(queue_.front());
            queue_.pop();
        }
        return data;
    }

    ///
    /// @brief Get latest history data immediately (in shared mode)
    ///
    /// @return std::vector<DataType> (in shared mode) a vector containing history data
    ///
    std::vector<DataType> history(typename std::enable_if<shared, bool>* _ = 0)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        std::vector<DataType> datas;
        datas.reserve(History_Depth_);
        for (const auto& data : history_) {
            datas.emplace_back(data);
        }
        return datas;
    };

private:
    const size_t Capacity_;                          ///< Capacity of queue
    const size_t History_Depth_;                     ///< Depth of History data fifo (only shared)
    const std::chrono::milliseconds Wait_Duration_;  ///< Duration to wait for pop

    mutable std::mutex mutex_;                                       ///< mutex for operations to queue
    std::queue<DataType> queue_;                                     ///< underlying stl queue
    std::conditional_t<shared, std::deque<DataType>, int> history_;  ///< history data
    std::condition_variable cond_;                                   ///< condition Variable for push/pop operations
};

}  // namespace hera
}  // namespace wayz