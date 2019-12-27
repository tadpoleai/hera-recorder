///
/// @file replayer.cpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Implementation of class Replayer
/// @version 0.2
/// @date 2019-12-25
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include "replayer.hpp"

#include <mutex>
#include <semaphore.h>

#include "device/include.hpp"

namespace wayz {
namespace hera {
namespace replay {

/// Read the recorded data filename and construct processers,
/// and init the ipc thread
Replayer::Replayer(const std::string& filename, const double replay_rate, const bool showonly) :
    replay_thread_(nullptr),
    replay_rate_(replay_rate),
    running_(false),
    progress_(0),
    total_duration_(0)
{
    storage_ = storage::StorageManager::open(filename, true);
    if (storage_->header != nullptr) {
        if (showonly) {
            std::cout << *storage_->header << std::endl;
        } else {
            log::info << "Replayer: Printing info\n" << *storage_->header << log::endl;

            ipc_queue_ = ipc::IPCQueue<device::data::SensorData>::create();
            ipc_queue_->open(0, ipc::OpenMode::Write);
            total_duration_ = storage_->header->timestamp_end - storage_->header->timestamp_start;
            running_ = true;

            replay_thread_ = new std::thread(&Replayer::replay_thread_function, this);
        }
    } else {
        log::error << "Replayer: Can not open hera record file " << filename << log::endl;
    }
}

/// Wait for thread to join, and close bag file
///
Replayer::~Replayer()
{
    if (replay_thread_) {
        replay_thread_->join();
    }
}

void Replayer::replay_thread_function()
{
    decltype(storage_->read()) data = nullptr;
    auto t_start = time::Timestamp::now();
    uint64_t t_data_start = UINT64_MAX;
    while ((data = storage_->read())) {
        auto t_data = data->get_timestamp_receive_ns();
        if (t_data < t_data_start) {
            t_data_start = t_data;
        }
        auto t_wait = t_start + (t_data - t_data_start) / replay_rate_;
        while (time::Timestamp::now() < t_wait) {
            usleep(1);
            progress_ = (uint64_t(time::Timestamp::now()) - t_start) * replay_rate_;
        };
        auto sensor_data = device::DeviceFactory::convert(data);
        if (sensor_data->sensor_data_type != device::SensorDataType::Broken) {
            ipc_queue_->write(sensor_data);
        }
    }
    running_ = false;
}

}  // namespace replay
}  // namespace hera
}  // namespace wayz