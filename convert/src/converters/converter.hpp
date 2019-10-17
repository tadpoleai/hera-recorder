//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#pragma once

#include <cstdint>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <common/data_def/device_types.hpp>
#include <common/tron_errno.h>
#include <common/utils/threadsafe_queue.hpp>
#include <devices/all_devices.hpp>
#include <ros/ros.h>
#include <rosbag/bag.h>

#include "../utils/utils.hpp"

namespace wayz {
namespace tron {
class Converter {
public:
    Converter(const std::string& device_data_fpath);
    Converter(const Device&) = delete;
    Converter& operator=(const Converter&) = delete;

    static bool open_bag(const std::string& bag_filepath);
    static bool close_bag();

    int64_t get_converted_size();

protected:
    void convert_thread_function();

    bool open_device_data_file();
    std::unique_ptr<DeviceRawData> read_one_data();
    template<class T>
    virtual std::shared_ptr<T const> convert_one_data(const std::unique_ptr<DeviceRawData>&) = 0;
    template<class T>
    void write_one_data(std::shared_ptr<T const> const& ros_msg);

    static rosbag::Bag* bag_ = nullptr;
    static std::mutex bag_write_mutex_;

    int64_t converted_size;
    std::string frame_id_;
    std::string topic_id_prefix_;
    std::string device_data_fpath_;
    std::ifstream device_data_file_;
    std::thread* thread_;
};

}  // namespace tron
}  // namespace wayz