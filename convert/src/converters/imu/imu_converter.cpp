//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include "imu_converter.hpp"

#include <devices/src/imu/imu.hpp>
#include <sensor_msgs/Imu.h>
#include <sensor_msgs/MagneticField.h>

namespace wayz {
namespace tron {

ImuConverter::ImuConverter(const std::string& frame_id,
                           const std::string& device_name,
                           const std::string& device_data_folder) :
    Converter(frame_id, device_name, device_data_folder)
{
    imu_topic_name_ = topic_name_prefix_ + "imu";
    magnetic_topic_name_ = topic_name_prefix_ + "magnetic_field";
}

ImuConverter::~ImuConverter()
{
    if (thread_) {
        thread_->join();
    }
}

bool ImuConverter::convert_and_write_one_data(const std::shared_ptr<DeviceRawData>& rawdata)
{
    auto sensor_data = Imu::do_convert(rawdata);
    DataImu* data_imu_buf = reinterpret_cast<DataImu*>(sensor_data->data_buf);

    auto ros_time_intrinsic = to_ros_time(sensor_data->timestamp_intrinsic_ns);
    // auto ros_time_received = to_ros_time(sensor_data->timestamp_receive_ns);

    sensor_msgs::Imu imu_msg;
    imu_msg.header.stamp = ros_time_intrinsic;
    imu_msg.header.seq = sensor_data->sequence;
    imu_msg.header.frame_id = frame_id_;
    // Mark orientation as n/a, refer to sensor_msgs::Imu definition
    imu_msg.orientation_covariance[0] = -1;
    imu_msg.linear_acceleration.x = data_imu_buf->linear_acceleration[0];
    imu_msg.linear_acceleration.y = data_imu_buf->linear_acceleration[1];
    imu_msg.linear_acceleration.z = data_imu_buf->linear_acceleration[2];
    imu_msg.angular_velocity.x = data_imu_buf->angular_velocity[0];
    imu_msg.angular_velocity.y = data_imu_buf->angular_velocity[1];
    imu_msg.angular_velocity.z = data_imu_buf->angular_velocity[2];

    sensor_msgs::MagneticField magnetic_msg;
    magnetic_msg.header.stamp = ros_time_intrinsic;
    magnetic_msg.header.seq = sensor_data->sequence;
    magnetic_msg.header.frame_id = frame_id_;
    magnetic_msg.magnetic_field.x = data_imu_buf->magnetic_field[0];
    magnetic_msg.magnetic_field.y = data_imu_buf->magnetic_field[1];
    magnetic_msg.magnetic_field.z = data_imu_buf->magnetic_field[2];

    bag_write_mutex_.lock();
    bag_->write(imu_topic_name_, ros_time_intrinsic, imu_msg);
    bag_->write(magnetic_topic_name_, ros_time_intrinsic, magnetic_msg);
    bag_write_mutex_.unlock();

    return true;
}


}  // namespace tron
}  // namespace wayz