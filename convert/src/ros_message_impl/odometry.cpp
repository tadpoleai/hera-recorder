///
/// @file dummy.cpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Implementation of class ROSMessage for dummy
/// @date 2019-12-26
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include <tf2_geometry_msgs/tf2_geometry_msgs.h>

#include "ros_message_impl.hpp"

namespace wayz {
namespace hera {
namespace convert {

template<>
std::vector<ROSMessagePtr> ROSMessage::convert<device::SensorDataType::OdometryFrontWheelSpeed>(
        device::data::SensorDataPtr& sensor_data,
        const std::string& topic_prefix,
        const std::string& frame_id,
        const common::Remapper* remapper)
{
    auto data_impl = reinterpret_cast<device::data::FrontWheelSpeed*>(sensor_data.get());
    std::vector<ROSMessagePtr> ret;

    auto message = ROSMessage::create<ROSMessageType::Vector3Stamped>();
    auto ros_message = reinterpret_cast<geometry_msgs::Vector3Stamped*>(message->ptr);

    message->topic_name = remapper->remap(topic_prefix + "front_wheel_speed");
    message->timestamp_ns = sensor_data->timestamp_intrinsic_ns;
    ros_message->header.seq = sensor_data->sequence;
    ros_message->header.stamp = to_ros_time(sensor_data->timestamp_intrinsic_ns);
    ros_message->header.frame_id = frame_id;

    ros_message->vector.x = data_impl->left;
    ros_message->vector.y = data_impl->right;
    ros_message->vector.z = 0;

    ret.emplace_back(std::move(message));
    return ret;
}

template<>
std::vector<ROSMessagePtr> ROSMessage::convert<device::SensorDataType::OdometryRearWheelSpeed>(
        device::data::SensorDataPtr& sensor_data,
        const std::string& topic_prefix,
        const std::string& frame_id,
        const common::Remapper* remapper)
{
    auto data_impl = reinterpret_cast<device::data::RearWheelSpeed*>(sensor_data.get());
    std::vector<ROSMessagePtr> ret;

    auto message = ROSMessage::create<ROSMessageType::Vector3Stamped>();
    auto ros_message = reinterpret_cast<geometry_msgs::Vector3Stamped*>(message->ptr);

    message->topic_name = remapper->remap(topic_prefix + "rear_wheel_speed");
    message->timestamp_ns = sensor_data->timestamp_intrinsic_ns;
    ros_message->header.seq = sensor_data->sequence;
    ros_message->header.stamp = to_ros_time(sensor_data->timestamp_intrinsic_ns);
    ros_message->header.frame_id = frame_id;

    ros_message->vector.x = data_impl->left;
    ros_message->vector.y = data_impl->right;
    ros_message->vector.z = 0;

    ret.emplace_back(std::move(message));
    return ret;
}

template<>
std::vector<ROSMessagePtr> ROSMessage::convert<device::SensorDataType::OdometrySteeringAngle>(
        device::data::SensorDataPtr& sensor_data,
        const std::string& topic_prefix,
        const std::string& frame_id,
        const common::Remapper* remapper)
{
    auto data_impl = reinterpret_cast<device::data::SteeringAngle*>(sensor_data.get());
    std::vector<ROSMessagePtr> ret;

    auto message = ROSMessage::create<ROSMessageType::Vector3Stamped>();
    auto ros_message = reinterpret_cast<geometry_msgs::Vector3Stamped*>(message->ptr);

    message->topic_name = remapper->remap(topic_prefix + "steering_angle");
    message->timestamp_ns = sensor_data->timestamp_intrinsic_ns;
    ros_message->header.seq = sensor_data->sequence;
    ros_message->header.stamp = to_ros_time(sensor_data->timestamp_intrinsic_ns);
    ros_message->header.frame_id = frame_id;

    ros_message->vector.x = data_impl->steering_angle;
    ros_message->vector.y = 0;
    ros_message->vector.z = 0;

    ret.emplace_back(std::move(message));
    return ret;
}

template<>
std::vector<ROSMessagePtr> ROSMessage::convert<device::SensorDataType::OdometryOrientation>(
        device::data::SensorDataPtr& sensor_data,
        const std::string& topic_prefix,
        const std::string& frame_id,
        const common::Remapper* remapper)
{
    auto data_impl = reinterpret_cast<device::data::Orientation*>(sensor_data.get());
    std::vector<ROSMessagePtr> ret;

    auto message = ROSMessage::create<ROSMessageType::Imu>();
    auto ros_message = reinterpret_cast<sensor_msgs::Imu*>(message->ptr);

    message->topic_name = remapper->remap(topic_prefix + "imu");
    message->timestamp_ns = sensor_data->timestamp_intrinsic_ns;
    ros_message->header.seq = sensor_data->sequence;
    ros_message->header.stamp = to_ros_time(sensor_data->timestamp_intrinsic_ns);
    ros_message->header.frame_id = frame_id;

    tf2::Quaternion quat_tf;
    quat_tf.setRPY(data_impl->orientation, 0, 0);
    ros_message->orientation = tf2::toMsg(quat_tf);

    ros_message->angular_velocity_covariance[0] = -1;
    ros_message->linear_acceleration_covariance[0] = -1;
    ret.emplace_back(std::move(message));
    return ret;
}

template<>
std::vector<ROSMessagePtr> ROSMessage::convert<device::SensorDataType::OdometryLocalizationResult>(
        device::data::SensorDataPtr& sensor_data,
        const std::string& topic_prefix,
        const std::string& frame_id,
        const common::Remapper* remapper)
{
    return {};
}

}  // namespace convert
}  // namespace hera
}  // namespace wayz
