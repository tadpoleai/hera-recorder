///
/// @file dummy.cpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Implementation of class ROSMessage for dummy
/// @date 2019-12-26
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include "ros_message_impl.hpp"

namespace wayz {
namespace hera {
namespace convert {

template<>
std::vector<ROSMessagePtr> ROSMessage::convert<device::SensorDataType::Dummy>(device::data::SensorDataPtr& sensor_data,
                                                                              const std::string& topic_prefix,
                                                                              const std::string& frame_id,
                                                                              const common::Remapper* remapper)
{
    auto data_impl = reinterpret_cast<device::data::Dummy*>(sensor_data.get());
    auto message = ROSMessage::create<ROSMessageType::Vector3Stamped>();
    auto ros_message = reinterpret_cast<geometry_msgs::Vector3Stamped*>(message->ptr);
    std::vector<ROSMessagePtr> ret;

    message->topic_name = remapper->remap(topic_prefix + "vector3_stamped");
    message->timestamp_ns = sensor_data->timestamp_intrinsic_ns;
    ros_message->header.seq = sensor_data->sequence;
    ros_message->header.stamp = to_ros_time(sensor_data->timestamp_intrinsic_ns);
    ros_message->header.frame_id = frame_id;

    ros_message->vector.x = data_impl->int_value;
    ros_message->vector.y = data_impl->int_value;
    ros_message->vector.z = data_impl->int_value;


    ret.emplace_back(std::move(message));
    return ret;
}

}  // namespace convert
}  // namespace hera
}  // namespace wayz
