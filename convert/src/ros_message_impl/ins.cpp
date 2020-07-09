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
std::vector<ROSMessagePtr> ROSMessage::convert<device::SensorDataType::InsBestPosition>(
        device::data::SensorDataPtr& sensor_data,
        const std::string& topic_prefix,
        const std::string& frame_id,
        const common::Remapper* remapper)
{
    static constexpr auto MaxFixedDeviationMeter = 0.02;

    auto data_impl = reinterpret_cast<device::data::BestPosition*>(sensor_data.get());
    std::vector<ROSMessagePtr> ret;

    auto message = ROSMessage::create<ROSMessageType::NavSatFix>();
    auto ros_message = reinterpret_cast<sensor_msgs::NavSatFix*>(message->ptr);

    message->topic_name = remapper->remap(topic_prefix + "best_position");
    message->timestamp_ns = sensor_data->timestamp_intrinsic_ns;
    ros_message->header.seq = sensor_data->sequence;
    ros_message->header.stamp = to_ros_time(sensor_data->timestamp_intrinsic_ns);
    ros_message->header.frame_id = frame_id;

    if (data_impl->solution_status != +device::data::SolutionStatus::SOL_COMPUTED) {
        ros_message->status.status = sensor_msgs::NavSatStatus::STATUS_NO_FIX;
        ros_message->status.service = sensor_msgs::NavSatStatus::SERVICE_GPS;

        ros_message->latitude = data_impl->latitude;
        ros_message->longitude = data_impl->longitude;
        ros_message->altitude = data_impl->altitude;

        ros_message->position_covariance_type = sensor_msgs::NavSatFix::COVARIANCE_TYPE_UNKNOWN;
        ros_message->position_covariance[0] = -1;
        for (size_t i = 1; i < 9; ++i) {
            ros_message->position_covariance[i] = 0;
        }
    } else {
        ros_message->status.status = sensor_msgs::NavSatStatus::STATUS_NO_FIX;
        if (data_impl->latitude_deviation < MaxFixedDeviationMeter &&
            data_impl->longitude_deviation < MaxFixedDeviationMeter) {
            ros_message->status.status = sensor_msgs::NavSatStatus::STATUS_FIX;
        }
        ros_message->status.service = sensor_msgs::NavSatStatus::SERVICE_GPS;

        ros_message->latitude = data_impl->latitude;
        ros_message->longitude = data_impl->longitude;
        ros_message->altitude = data_impl->altitude;

        ros_message->position_covariance_type = sensor_msgs::NavSatFix::COVARIANCE_TYPE_DIAGONAL_KNOWN;
        for (size_t i = 0; i < 9; ++i) {
            ros_message->position_covariance[i] = 0;
        }
        ros_message->position_covariance[0] = data_impl->longitude_deviation * data_impl->longitude_deviation;  // E
        ros_message->position_covariance[4] = data_impl->latitude_deviation * data_impl->latitude_deviation;    // N
        ros_message->position_covariance[8] = data_impl->altitude_deviation * data_impl->altitude_deviation;    // U
    }

    ret.emplace_back(std::move(message));
    return ret;
}

template<>
std::vector<ROSMessagePtr> ROSMessage::convert<device::SensorDataType::InsInsPosition>(
        device::data::SensorDataPtr& sensor_data,
        const std::string& topic_prefix,
        const std::string& frame_id,
        const common::Remapper* remapper)
{
    static constexpr auto MaxFixedDeviationMeter = 0.02;

    auto data_impl = reinterpret_cast<device::data::InsPosition*>(sensor_data.get());
    std::vector<ROSMessagePtr> ret;

    auto message = ROSMessage::create<ROSMessageType::NavSatFix>();
    auto ros_message = reinterpret_cast<sensor_msgs::NavSatFix*>(message->ptr);

    message->topic_name = remapper->remap(topic_prefix + "ins_position");
    message->timestamp_ns = sensor_data->timestamp_intrinsic_ns;
    ros_message->header.seq = sensor_data->sequence;
    ros_message->header.stamp = to_ros_time(sensor_data->timestamp_intrinsic_ns);
    ros_message->header.frame_id = frame_id;

    ros_message->status.status = sensor_msgs::NavSatStatus::STATUS_NO_FIX;
    if (data_impl->latitude_deviation < MaxFixedDeviationMeter &&
        data_impl->longitude_deviation < MaxFixedDeviationMeter) {
        ros_message->status.status = sensor_msgs::NavSatStatus::STATUS_FIX;
    }
    ros_message->status.service = sensor_msgs::NavSatStatus::SERVICE_GPS;

    ros_message->latitude = data_impl->latitude;
    ros_message->longitude = data_impl->longitude;
    ros_message->altitude = data_impl->altitude;

    if (data_impl->longitude_deviation < 0) {
        ros_message->position_covariance_type = sensor_msgs::NavSatFix::COVARIANCE_TYPE_UNKNOWN;
    } else {
        for (size_t i = 0; i < 9; ++i) {
            ros_message->position_covariance[i] = 0;
        }
        ros_message->position_covariance[0] = data_impl->longitude_deviation * data_impl->longitude_deviation;  // E
        ros_message->position_covariance[4] = data_impl->latitude_deviation * data_impl->latitude_deviation;    // N
        ros_message->position_covariance[8] = data_impl->altitude_deviation * data_impl->altitude_deviation;    // U
    }

    ret.emplace_back(std::move(message));
    return ret;
}

template<>
std::vector<ROSMessagePtr> ROSMessage::convert<device::SensorDataType::InsCorrectedImu>(
        device::data::SensorDataPtr& sensor_data,
        const std::string& topic_prefix,
        const std::string& frame_id,
        const common::Remapper* remapper)
{
    auto data_impl = reinterpret_cast<device::data::CorrectedImu*>(sensor_data.get());
    std::vector<ROSMessagePtr> ret;

    auto message = ROSMessage::create<ROSMessageType::Imu>();
    auto ros_message = reinterpret_cast<sensor_msgs::Imu*>(message->ptr);

    message->topic_name = remapper->remap(topic_prefix + "corrected_imu");
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
    return ret;
}

}  // namespace convert
}  // namespace hera
}  // namespace wayz
