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
    camera_sensor_id_ = -1;
    serial_sensor_id_ = -1;
    odometry_sensor_id_ = -1;
    ipc_queue_->open(0, ipc::OpenMode::Read, false);
    stop_ = false;
}

Bridge::~Bridge()
{
    ipc_queue_->close();
}

void Bridge::spin()
{
    while (!stop_) {
        auto data = ipc_queue_->read();

        if (data == nullptr) {
            usleep(1000);
            continue;
        }

        switch (data->sensor_data_type) {
        case device::SensorDataType::Image:
            if (int32_t(data->sensor_id) == camera_sensor_id_ || camera_sensor_id_ < 0) {
                auto data_impl = reinterpret_cast<device::data::Image*>(data.get());
                camera_handler(data_impl);
            }
            break;
        case device::SensorDataType::NavSatFix:
            if (int32_t(data->sensor_id) == serial_sensor_id_ || serial_sensor_id_ < 0) {
                auto data_impl = reinterpret_cast<device::data::NavSatFix*>(data.get());
                gnss_handler(data_impl);
            }
            break;
        case device::SensorDataType::OdometryFrontWheelSpeed:
            if (int32_t(data->sensor_id) == odometry_sensor_id_ || odometry_sensor_id_ < 0) {
                auto data_impl = reinterpret_cast<device::data::FrontWheelSpeed*>(data.get());
                front_wheel_speed_handler(data_impl);
            }
            break;
        case device::SensorDataType::OdometryRearWheelSpeed:
            if (int32_t(data->sensor_id) == odometry_sensor_id_ || odometry_sensor_id_ < 0) {
                auto data_impl = reinterpret_cast<device::data::RearWheelSpeed*>(data.get());
                rear_wheel_speed_handler(data_impl);
            }
            break;
        case device::SensorDataType::OdometrySteeringAngle:
            if (int32_t(data->sensor_id) == odometry_sensor_id_ || odometry_sensor_id_ < 0) {
                auto data_impl = reinterpret_cast<device::data::SteeringAngle*>(data.get());
                steering_angle_handler(data_impl);
            }
            break;
        default:
            log::warn << "data->sensor_data_type unkown type" << log::endl;
            break;
        }
    }
}

void Bridge::stop()
{
    stop_ = true;
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

void Bridge::gnss_handler(const device::data::NavSatFix* const data)
{
    uint64_t timestamp = data->timestamp_intrinsic_ns;
    double latitude = data->latitude;
    double longitude = data->longitude;
    double altitude = data->altitude;

    log::debug << "Bridge: got an image, timestamp = " << time::Timestamp(timestamp) << ", latitude = " << latitude
               << ", longitude = " << longitude << ", altitude = " << altitude << log::endl;
}

void Bridge::front_wheel_speed_handler(const device::data::FrontWheelSpeed* const data)
{
    uint64_t timestamp = data->timestamp_intrinsic_ns;

    log::debug << "Bridge: got front_wheel_speed, timestamp = " << time::Timestamp(timestamp)
               << " front_wheel_speed = " << data->right << log::endl;
}

void Bridge::rear_wheel_speed_handler(const device::data::RearWheelSpeed* const data)
{
    uint64_t timestamp = data->timestamp_intrinsic_ns;

    log::debug << "Bridge: got rear_wheel_speed, timestamp = " << time::Timestamp(timestamp)
               << " rear_wheel_speed = " << data->right << log::endl;
}

void Bridge::steering_angle_handler(const device::data::SteeringAngle* const data)
{
    uint64_t timestamp = data->timestamp_intrinsic_ns;

    log::debug << "Bridge: got steering_angle_handler, timestamp = " << time::Timestamp(timestamp)
               << " steering_angle = " << data->steering_angle << log::endl;
}
}  // namespace wayz