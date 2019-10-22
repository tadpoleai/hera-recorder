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
                           const std::string& device_data_folder,
                           ConverterHandler* handler) :
    Converter(frame_id, device_name, device_data_folder, handler)
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

bool ImuConverter::convert_one_data(const std::shared_ptr<DeviceRawData>& raw_data)
{
    if (!raw_data) {
        sem_wait(managed_this_->sem_converter);
        managed_this_->data.msg_type = MsgType::Invalid;
        managed_this_->data.msg = nullptr;
        managed_this_->data.timestamp_ns = 0;
        managed_this_->data.topic_name = "";
        sem_post(managed_this_->sem_writer);
        return false;
    }

    auto sensor_data = Imu::do_convert(raw_data);
    DataImu* data_imu_buf = reinterpret_cast<DataImu*>(sensor_data->data_buf);
    auto ros_time_intrinsic = to_ros_time(sensor_data->timestamp_intrinsic_ns);
    // auto ros_time_received = to_ros_time(sensor_data->timestamp_receive_ns);

    auto imu_msg = new sensor_msgs::Imu;
    imu_msg->header.stamp = ros_time_intrinsic;
    imu_msg->header.seq = sensor_data->sequence;
    imu_msg->header.frame_id = frame_id_;
    // Mark orientation as n/a, refer to sensor_msgs::Imu definition
    imu_msg->orientation_covariance[0] = -1;
    imu_msg->linear_acceleration.x = data_imu_buf->linear_acceleration[0];
    imu_msg->linear_acceleration.y = data_imu_buf->linear_acceleration[1];
    imu_msg->linear_acceleration.z = data_imu_buf->linear_acceleration[2];
    imu_msg->angular_velocity.x = data_imu_buf->angular_velocity[0];
    imu_msg->angular_velocity.y = data_imu_buf->angular_velocity[1];
    imu_msg->angular_velocity.z = data_imu_buf->angular_velocity[2];

    sem_wait(managed_this_->sem_converter);
    managed_this_->data.msg_type = MsgType::SensorMsgsImu;
    managed_this_->data.msg = reinterpret_cast<void*>(imu_msg);
    managed_this_->data.timestamp_ns = sensor_data->timestamp_intrinsic_ns;
    managed_this_->data.topic_name = imu_topic_name_;
    sem_post(managed_this_->sem_writer);

    auto magnetic_msg = new sensor_msgs::MagneticField;
    magnetic_msg->header.stamp = ros_time_intrinsic;
    magnetic_msg->header.seq = sensor_data->sequence;
    magnetic_msg->header.frame_id = frame_id_;
    magnetic_msg->magnetic_field.x = data_imu_buf->magnetic_field[0];
    magnetic_msg->magnetic_field.y = data_imu_buf->magnetic_field[1];
    magnetic_msg->magnetic_field.z = data_imu_buf->magnetic_field[2];

    sem_wait(managed_this_->sem_converter);
    managed_this_->data.msg_type = MsgType::SensorMsgsMagneticField;
    managed_this_->data.msg = reinterpret_cast<void*>(magnetic_msg);
    managed_this_->data.timestamp_ns = sensor_data->timestamp_intrinsic_ns;
    managed_this_->data.topic_name = magnetic_topic_name_;
    sem_post(managed_this_->sem_writer);

    return true;
}


}  // namespace tron
}  // namespace wayz