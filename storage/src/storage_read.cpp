///
/// @file storage_read.cpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief Read related implementation of class Storage
/// @date 2020-06-19
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include <sstream>

#include "common/include/logger/logger.hpp"
#include "storage.hpp"
#include "unistd.h"

namespace wayz {
namespace hera {
namespace storage {

device::data::DeviceDataPtr StorageManager::read()
{
    if (!read_mode_ || !header) {
        return nullptr;
    }

    if (!read_strict_) {
        return device::data::DeviceData::read_from(in_file_);
    } else {
        if (!thread_) {
            thread_running_ = true;
            thread_ = new std::thread(&StorageManager::prefetch_thread_function, this);
        }
    }

    // Read strict
    device::data::DeviceDataPtr data;
    std::unique_lock<std::mutex> lock(mutex_prefetch_);
    cv_prefetch_.wait(lock, [this] { return prefetch_data_ready_; });
    prefetch_data_ready_ = false;

    if (prefetch_ended_) {
        lock.unlock();
        cv_prefetch_.notify_one();
        return nullptr;
    }

    uint64_t earlist_timestamp = UINT64_MAX;
    int32_t earlist_index = -1;
    for (size_t i = 0; i < data_array_prefetch_.size(); ++i) {
        if (!data_array_prefetch_[i].empty()) {
            auto timestamp = data_array_prefetch_[i].front()->get_timestamp_receive_ns();
            if (timestamp < earlist_timestamp) {
                earlist_timestamp = timestamp;
                earlist_index = i;
            }
        }
    }
    if (earlist_index >= 0) {
        data = data_array_prefetch_[earlist_index].front();
        data_array_prefetch_[earlist_index].pop();
        read_header_timestamp_ = earlist_timestamp;
    }

    lock.unlock();
    cv_prefetch_.notify_one();
    return data;
}

void StorageManager::prefetch_thread_function()
{
    data_array_prefetch_.resize(header->device_names.size());
    std::vector<uint64_t> device_latest_timestamp_array;
    uint64_t prefetch_tail_timestamp = 0;

    bool inited = false;
    constexpr auto MaxPrefetchDuration = 3 * time::OneSecond;

    while (thread_running_) {
        std::unique_lock<std::mutex> lock(mutex_prefetch_);

        bool need_fetch = false;
        if (!inited) {
            need_fetch = std::any_of(data_array_prefetch_.cbegin(), data_array_prefetch_.cend(), [](auto&& queue) {
                return queue.empty();
            });
        } else {
            for (size_t i = 0; i < data_array_prefetch_.size(); ++i) {
                if (data_array_prefetch_[i].empty() &&
                    prefetch_tail_timestamp < read_header_timestamp_ + MaxPrefetchDuration) {
                    need_fetch = true;
                    break;
                }
            }
        }

        if (need_fetch) {
            auto data = device::data::DeviceData::read_from(in_file_);
            if (!data) {
                prefetch_ended_ = true;
                thread_running_ = false;
                log::info << "StorageManager::Prefetch ended, since no new data" << log::endl;
            } else {
                auto device_id = data->get_device_id();
                auto timestamp = data->get_timestamp_receive_ns();
                data_array_prefetch_[device_id].emplace(std::move(data));

                if (!inited) {
                    inited = true;
                    device_latest_timestamp_array.resize(header->device_names.size(), timestamp);
                    prefetch_tail_timestamp = timestamp;
                }

                if (prefetch_tail_timestamp < timestamp) {
                    prefetch_tail_timestamp = timestamp;
                }

                device_latest_timestamp_array[device_id] = timestamp;
            }

            prefetch_data_ready_ = true;
            lock.unlock();
            cv_prefetch_.notify_one();

        } else {
            prefetch_data_ready_ = true;
            lock.unlock();
            cv_prefetch_.notify_one();

            std::unique_lock<std::mutex> lock_2(mutex_prefetch_);
            cv_prefetch_.wait(lock_2, [this] { return !prefetch_data_ready_; });
        }
    }
}

}  // namespace storage
}  // namespace hera
}  // namespace wayz
