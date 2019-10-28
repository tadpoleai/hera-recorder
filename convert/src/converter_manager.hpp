//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#pragma once

#include <cstdint>
#include <fstream>
#include <mutex>
#include <semaphore.h>
#include <string>
#include <thread>
#include <vector>

#include <common/data_def/device_types.hpp>
#include <common/tron_errno.h>
#include <common/utils/get_folder_content.hpp>
#include <devices/all_devices.hpp>
#include <ros/ros.h>
#include <rosbag/bag.h>
#include <rosbag_direct_write/direct_bag.h>
#include <sensor_msgs/CompressedImage.h>
#include <sensor_msgs/Imu.h>
#include <sensor_msgs/MagneticField.h>
#include <sensor_msgs/PointCloud2.h>

#include "all_converters.hpp"

namespace wayz {
namespace tron {

class ConverterManager final {
public:
    ConverterManager(const std::string& bag_filepath, const std::string& device_data_folder);
    ConverterManager(const Device&) = delete;
    ConverterManager& operator=(const Converter&) = delete;
    virtual ~ConverterManager();

    FileSize report_progress();
    inline FileSize total_size() const
    {
        return total_size_;
    }
    bool running() const;


private:
    bool open_bag(const std::string& bag_filepath);
    bool close_bag();
    void write_thread_function();
    void write_and_free_one_converted_msg(const ConvertedData* data);

    rosbag_direct_write::DirectBag* bag_;
    std::vector<std::shared_ptr<ConverterHandler>> converter_handlers_;
    FileSize total_size_;
    std::thread* thread_;
    bool running_;
    std::string device_data_folder_;
};

}  // namespace tron
}  // namespace wayz