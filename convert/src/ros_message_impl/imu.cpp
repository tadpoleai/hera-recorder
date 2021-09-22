///
/// @file imu.cpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Implementation of class ROSMessage for imu
/// @date 2019-12-27
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include "ros_message_impl.hpp"

namespace wayz {
namespace hera {
namespace convert {

template<>
std::vector<ROSMessagePtr> ROSMessage::convert<device::SensorDataType::ImuComposed>(
        device::data::SensorDataPtr& sensor_data,
        const std::string& topic_prefix,
        const std::string& frame_id,
        const common::Remapper* remapper)
{
    return {};
}

template<>
std::vector<ROSMessagePtr> ROSMessage::convert<device::SensorDataType::ImuMagneticField>(
        device::data::SensorDataPtr& sensor_data,
        const std::string& topic_prefix,
        const std::string& frame_id,
        const common::Remapper* remapper)
{
    auto data_impl = reinterpret_cast<device::data::ImuMagneticField*>(sensor_data.get());
    std::vector<ROSMessagePtr> ret;

    {
        auto message = ROSMessage::create<ROSMessageType::Imu>();
        auto ros_message = reinterpret_cast<sensor_msgs::Imu*>(message->ptr);

        message->topic_name = remapper->remap(topic_prefix + "imu");
        message->timestamp_ns = sensor_data->timestamp_intrinsic_ns;
        ros_message->header.seq = sensor_data->sequence;
        ros_message->header.stamp = to_ros_time(sensor_data->timestamp_intrinsic_ns);
        ros_message->header.frame_id = frame_id;


        ros_message->orientation_covariance[0] = -1;
        ros_message->linear_acceleration.x = data_impl->linear_acceleration[0];
        ros_message->linear_acceleration.y = data_impl->linear_acceleration[1];
        ros_message->linear_acceleration.z = data_impl->linear_acceleration[2];
        ros_message->angular_velocity.x = data_impl->angular_velocity[0];
        ros_message->angular_velocity.y = data_impl->angular_velocity[1];
        ros_message->angular_velocity.z = data_impl->angular_velocity[2];

        ret.emplace_back(std::move(message));
    }

    {
        auto message = ROSMessage::create<ROSMessageType::MagneticField>();
        auto ros_message = reinterpret_cast<sensor_msgs::MagneticField*>(message->ptr);

        message->topic_name = remapper->remap(topic_prefix + "magnetic_field");
        message->timestamp_ns = sensor_data->timestamp_intrinsic_ns;
        ros_message->header.seq = sensor_data->sequence;
        ros_message->header.stamp = to_ros_time(sensor_data->timestamp_intrinsic_ns);
        ros_message->header.frame_id = frame_id;

        ros_message->magnetic_field.x = data_impl->magnetic_field[0];
        ros_message->magnetic_field.y = data_impl->magnetic_field[1];
        ros_message->magnetic_field.z = data_impl->magnetic_field[2];

        ret.emplace_back(std::move(message));
    }

    return ret;
}

}  // namespace convert
}  // namespace hera
}  // namespace wayz
