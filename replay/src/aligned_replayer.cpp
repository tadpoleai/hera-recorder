///
/// @file aligned_replayer.cpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Implementation of class AlignedReplayer
/// @version 0.1
/// @date 2019-12-19
///
/// @copyright Copyright (c) 2019
///

#include "aligned_replayer.hpp"

namespace wayz {
namespace hera {
namespace replay {

/// Scan the recorded data folder and construct processers.
/// Open the bag file for writing, and init the writing thread
AlignedReplayer::AlignedReplayer(const std::string& folder, const double replay_rate) :
    write_thread_(nullptr),
    replay_rate_(replay_rate),
    running_(false),
    total_size_(0)
{
    uint32_t device_index = 0;
    auto types = get_folder_content(folder);
    if (!types.opened) {
        log::error << "Can not open folder " << folder << log::endl;
    }
    for (const auto& type : types.folders) {
        auto vendors = get_folder_content(type.fullname);
        if (!vendors.opened) {
            continue;
        }

        for (const auto& vendor : vendors.folders) {
            auto devices = get_folder_content(vendor.fullname);
            if (!devices.opened) {
                continue;
            }

            for (const auto& device : devices.folders) {
                auto processer =
                        Processer::create(device_index, type.basename, vendor.basename, device.basename, folder);
                if (processer->is_open()) {
                    processers_.emplace_back(std::move(processer));
                    auto ipc_queue = ipc::IPCQueue<SensorData>::create();
                    ipc_queue->open(device_index, ipc::OpenMode::Write);
                    ipc_queues_.emplace_back(std::move(ipc_queue));
                    auto device_size = get_folder_content(device.fullname).total_size;
                    total_size_ = total_size_ + device_size;
                    log::debug << "Add device "
                               << "index = " << device_index << " type = " << type.basename << "/" << vendor.basename
                               << "/" << device.basename << ", sized " << device_size << log::endl;
                    device_index++;
                }
            }
        }
    }

    if (processers_.size() > 0) {
        running_ = true;
        write_thread_ = new std::thread(&AlignedReplayer::write_thread_function, this);
    } else {
        running_ = false;
        log::error << "No hera record data in " << folder << log::endl;
    }
}  // namespace replay

/// Wait for thread to join, and close bag file
///
AlignedReplayer::~AlignedReplayer()
{
    if (write_thread_) {
        write_thread_->join();
    }
}


FileSize AlignedReplayer::converted_size() const
{
    uint64_t result = 0;
    for (const auto& processer : processers_) {
        result += processer->processed_size();
    }
    return result;
}

FileSize AlignedReplayer::total_size() const noexcept
{
    return total_size_;
}


bool AlignedReplayer::running() const noexcept
{
    return running_;
}

void AlignedReplayer::write_thread_function()
{
    thread_local Processer* earliest_processer = nullptr;
    thread_local const auto start_time_local = Timestamp::now();
    thread_local uint64_t start_time_data = 0;

    /// In a loop.
    while (true) {
        if (earliest_processer == nullptr) {
            /// Firstly,
            /// wait for all processer for publishing their first message.
            for (auto& processer : processers_) {
                sem_wait(processer->receive_sem);
            }
        } else {
            /// Or, wait for last processer for a new message.
            /// Here, the last processer is the processer that published a message
            /// with earliest timestamp in last round.
            sem_wait(earliest_processer->receive_sem);
        }

        uint64_t earliest_timestamp = UINT64_MAX;
        earliest_processer = nullptr;
        /// Search among all processers to determine earliest message,
        /// and get its handler, until all processers published EndOfFile.
        uint32_t index = 0;
        uint32_t earliest_index = 0;
        for (auto& processer : processers_) {
            if (processer->message->sensor_data_type != SensorDataType::EndOfFile) {
                if (earliest_timestamp > processer->message->timestamp_intrinsic_ns) {
                    earliest_timestamp = processer->message->timestamp_intrinsic_ns;
                    earliest_processer = processer.get();
                    earliest_index = index;
                    if (start_time_data == 0) {
                        start_time_data = earliest_timestamp;
                    }
                }
            }
            index++;
        }

        /// If all processers published EndOfFile, break the loop
        if (!earliest_processer) {
            break;
        }

        uint64_t replay_time = 1.0 / replay_rate_ * (earliest_timestamp - start_time_data) + start_time_local;
        while (true) {
            auto ts_now = Timestamp::now();
            if (replay_time < ts_now) {
                break;
            }
            usleep((replay_time - ts_now) / 1000);
        }

        /// Write the message into bag file
        log::debug << "Replayer: Id =  " << earliest_index << ", Ts_data = " << earliest_timestamp << log::endl;
        ipc_queues_[earliest_index]->write(earliest_processer->message);

        /// Notify the last processer the publish next message.
        sem_post(earliest_processer->publish_sem);
    }
    running_ = false;
}

}  // namespace replay
}  // namespace hera
}  // namespace wayz