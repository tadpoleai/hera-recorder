///
/// @file gnss.cpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Implementation of class ROSMessage for gnss
/// @date 2019-12-27
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include "../ros_message_impl.hpp"

namespace wayz {
namespace hera {
namespace convert {

template<>
std::vector<ROSMessagePtr> ROSMessage::convert<device::SensorDataType::NavSatFix>(
        device::data::SensorDataPtr& sensor_data,
        const std::string& topic_prefix,
        const std::string& frame_id,
        const common::Remapper* remapper)
{
    auto data_impl = reinterpret_cast<device::data::NavSatFix*>(sensor_data.get());
    auto message = ROSMessage::create<ROSMessageType::NavSatFix>();
    auto ros_message = reinterpret_cast<sensor_msgs::NavSatFix*>(message->ptr);
    std::vector<ROSMessagePtr> ret;

    message->topic_name = remapper->remap(topic_prefix + "nav_sat_fix");
    message->timestamp_ns = sensor_data->timestamp_intrinsic_ns;
    ros_message->header.seq = sensor_data->sequence;
    ros_message->header.stamp = to_ros_time(sensor_data->timestamp_intrinsic_ns);
    ros_message->header.frame_id = frame_id;

    ros_message->status.status = static_cast<decltype(ros_message->status.status)>(data_impl->status.status);
    ros_message->status.service = static_cast<decltype(ros_message->status.service)>(data_impl->status.service);

    ros_message->latitude = data_impl->latitude;
    ros_message->longitude = data_impl->longitude;
    ros_message->altitude = data_impl->altitude;

    ros_message->position_covariance_type =
            static_cast<decltype(ros_message->position_covariance_type)>(data_impl->position_covariance_type);

    for (size_t i = 0; i < 9; ++i) {
        ros_message->position_covariance[i] = data_impl->position_covariance[i];
    }

    ret.emplace_back(std::move(message));
    return ret;
}

}  // namespace convert
}  // namespace hera
}  // namespace wayz
