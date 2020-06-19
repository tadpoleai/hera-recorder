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
    }

    // Read strict
    device::data::DeviceDataPtr data;

    while (!data && !prefetch_ended_) {
        mutex_prefetch_.lock();
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
        }
        mutex_prefetch_.unlock();

        usleep(10);
    }

    return data;
}

void StorageManager::prefetch_thread_function()
{
    data_array_prefetch_.resize(header->device_names.size());
    std::vector<uint64_t> device_latest_timestamp_array;
    uint64_t global_latest_timestamp = 0;

    bool inited = false;

    while (thread_running_) {
        mutex_prefetch_.lock();
        bool any_empty = std::any_of(data_array_prefetch_.cbegin(), data_array_prefetch_.cend(), [](auto&& queue) {
            return queue.empty();
        });

        if (!any_empty) {
            mutex_prefetch_.unlock();
            usleep(10);
            continue;
        }

        auto data = device::data::DeviceData::read_from(in_file_);
        if (!data) {
            mutex_prefetch_.unlock();
            prefetch_ended_ = true;
            log::info << "StorageManager::Prefetch ended, since no new data" << log::endl;
            break;
        }

        auto device_id = data->get_device_id();
        auto timestamp = data->get_timestamp_receive_ns();
        data_array_prefetch_[device_id].emplace(std::move(data));
        mutex_prefetch_.unlock();

        if (!inited) {
            inited = true;
            device_latest_timestamp_array.resize(header->device_names.size(), timestamp);
            global_latest_timestamp = timestamp;
        }

        if (global_latest_timestamp < timestamp) {
            global_latest_timestamp = timestamp;
        }

        device_latest_timestamp_array[device_id] = timestamp;
    }
}

}  // namespace storage
}  // namespace hera
}  // namespace wayz
