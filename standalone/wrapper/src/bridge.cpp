///
/// @file bridge.cpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Implementation of Node to bridge between ROS and Hera
/// @version 0.1
/// @date 2020-02-04
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include "bridge.hpp"

#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#include <hera/common/logger/logger.hpp>
#include <hera/common/utils/time.hpp>

namespace wayz {

Bridge::Bridge() : ipc_queue_(ipc::IPCQueue<device::data::SensorData>::create())
{
    log::debug << "Bridge ipc_queue_ init OK" << log::endl;

    ipc_queue_->open(0, ipc::OpenMode::Read, false, 4UL, (1 << 20));
}

Bridge::~Bridge()
{
    ipc_queue_->close();
}

void Bridge::spin()
{
    while (true) {
        auto data = ipc_queue_->read();

        if (data == nullptr) {
            usleep(500000);
            continue;
        }

        switch (data->sensor_data_type) {
        case device::SensorDataType::Image:
            if (int32_t(data->sensor_id) == camera_sensor_id_ || camera_sensor_id_ < 0) {
                auto data_impl = reinterpret_cast<device::data::Image*>(data.get());
                camera_handler(data_impl);
            }
            break;
        default:
            log::debug << "Interface: Unknown type of data" << log::endl;
            break;
        }
    }
}

void Bridge::camera_handler(const device::data::Image* const data)
{
    uint64_t timestamp = data->timestamp_intrinsic_ns;
    uint32_t cols = data->image_meta.cols;
    uint32_t rows = data->image_meta.rows;

    log::debug << "Bridge: got an image, timestamp = " << time::Timestamp(timestamp) << ", cols = << " << cols
               << ", rows = " << rows << log::endl;

    // cv::Mat grayMat(rows, cols, CV_8UC1);

    // memcpy(grayMat.data, data->image_data, data->image_data_size);
    // std::string fileName = std::to_string(timestamp);
    // fileName += ".jpeg";
    // cv::imwrite(fileName, grayMat);
}

}  // namespace wayz