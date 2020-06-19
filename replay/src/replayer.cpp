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

#include "common/include/third_party/json.hpp"
#include "device/include/include.hpp"

using nlohmann::json;

namespace wayz {
namespace hera {
namespace replay {

/// Read the recorded data filename and construct processers,
/// and init the ipc thread
Replayer::Replayer(const std::string& filename,
                   const double replay_rate,
                   const std::string& config_filename,
                   const uint64_t start_time,
                   const bool strict_aligned) :
    replay_thread_(nullptr),
    replay_rate_(replay_rate),
    running_(false),
    progress_(0),
    param_start_time_(start_time),
    total_duration_(0)
{
    storage_ = storage::StorageManager::open(filename, true, false, false, strict_aligned);
    if (storage_->header == nullptr) {
        log::error << "Replayer: Can not open hera record file " << filename << log::endl;
        return;
    }

    log::info << "Replayer: Printing info\n" << *storage_->header << log::endl;

    if (config_filename.empty()) {
        auto ipc_queue = ipc::IPCQueue<device::data::SensorData>::create();
        ipc_queue->open(0, ipc::OpenMode::Write);
        for (const auto& device_name : storage_->header->device_names) {
            log::info << "Replayer: Have Device " << device_name << log::endl;
            ipc_ptrs_.emplace_back(ipc_queue.get());
        }
        ipc_queues_.emplace_back(std::move(ipc_queue));
    } else {
        log::info << "Replayer: Config file specified, reading" << log::endl;

        json profile_json;
        try {
            std::ifstream ifs;
            ifs.open(config_filename, std::ios::in);
            ifs >> profile_json;
            ifs.close();
        } catch (std::exception& e) {
            log::error << "Replayer: Can not open config file" << config_filename << ", since " << e.what()
                       << log::endl;
            log::flush();
            exit(-1);
        }

        try {
            for (const auto& ipc : profile_json["ipcs"]) {
                auto ipc_queue = ipc::IPCQueue<device::data::SensorData>::create();
                ipc_queue->open(ipc["key"], ipc::OpenMode::Write, false, ipc["length"], ipc["size"]);
                log::info << "Create IPC, ipc key: " << ipc["key"] << " ipc length: " << ipc["length"]
                          << " ipc size: " << ipc["size"] << log::endl;
                ipc_queues_.emplace_back(std::move(ipc_queue));
            }
        } catch (std::exception& e) {
            log::error << "Replayer: Can read ipcs in config file, since " << e.what() << log::endl;
            log::flush();
            exit(-1);
        }

        uint32_t id = 0;
        for (const auto& device : profile_json["devices"]) {
            std::string type = device["type"];
            std::string name = device["name"];
            int32_t forward_index = device["forward"];
            ipc::IPCQueue<device::data::SensorData>* ipc_ptr = nullptr;

            if (id >= storage_->header->device_names.size()) {
                log::error << "Replayer: Config file mismatch recorded file, since id = " << id
                           << " larger than .hera record file" << log::endl;
                log::flush();
                exit(-1);
            }
            std::string device_name;
            device_name = type + "/" + name;
            if (storage_->header->device_names[id] != device_name) {
                log::error << "Replayer: Config file mismatch recorded file since\n"
                           << "\tdevice name = " << device_name
                           << " in config file is different with device name = " << storage_->header->device_names[id]
                           << ", in .hera record file" << log::endl;
                log::flush();
                exit(-1);
            }
            log::info << "Replayer: Have Device " << type << "/" << name << ", indexed " << id << log::endl;

            if (forward_index >= 0 && forward_index < (int32_t)ipc_queues_.size()) {
                ipc_ptr = ipc_queues_[forward_index].get();
                log::info << "Replayer: Registering ipc " << forward_index << " to device " << type << "/" << name
                          << log::endl;
            } else {
                log::info << "Replayer: Muting ipc output for device " << type << "/" << name << log::endl;
            }
            ipc_ptrs_.emplace_back(ipc_ptr);

            id++;
        }
    }

    running_ = true;
    total_duration_ = storage_->header->timestamp_end - storage_->header->timestamp_start;
    replay_thread_ = new std::thread(&Replayer::replay_thread_function, this);
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
    auto t = t_start;
    auto t_start_inited = false;
    int64_t pause_waited = 0;
    uint64_t t_data_start = UINT64_MAX;

    while (running_ && (data = storage_->read())) {
        auto t_data = data->get_timestamp_receive_ns();
        if (t_data < t_data_start) {
            t_data_start = t_data;
        }

        if (!t_start_inited) {
            if (t_data - t_data_start < param_start_time_) {
                progress_ = t_data - t_data_start;
                continue;
            } else {
                t_start_inited = true;
                t_start = time::Timestamp::now();
                t_data_start = t_data;
            }
        }

        auto t_wait = t_start + pause_waited + (t_data - t_data_start) / replay_rate_;

        while (true) {
            auto now = time::Timestamp::now();
            auto duration = now - t;
            t = now;
            if (paused_) {
                pause_waited += duration;
            } else {
                progress_ = (now - t_start - pause_waited) * replay_rate_ + param_start_time_;
                if (now >= t_wait) {
                    break;
                }
            }
            usleep(1);
        };
        progress_ = t_data - t_data_start + param_start_time_;

        auto sensor_data = device::DeviceFactory::convert(data);
        if (sensor_data) {
            if (sensor_data->sensor_data_type != device::SensorDataType::Broken) {
                if (data->get_device_id() < ipc_ptrs_.size()) {
                    ipc_ptrs_[data->get_device_id()]->write(sensor_data);
                }
            }
        } else {
            log::error << "FATAL: Replay: convert() could never return nullptr, return SensorData::broken() instead!"
                       << log::endl;
        }
    }
    running_ = false;
}  // namespace replay

}  // namespace replay
}  // namespace hera
}  // namespace wayz