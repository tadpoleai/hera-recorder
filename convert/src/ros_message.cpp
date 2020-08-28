///
/// @file ros_message.cpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Implementation of class ROSMessage
/// @date 2019-11-12
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include "ros_message_impl.hpp"

namespace wayz {
namespace hera {
namespace convert {

/// Write to a bag file,
/// with variant write function, by type information
void operator<<(rosbag_direct_write::DirectBag& bag, ROSMessagePtr&& message)
{
    switch (message->type) {

#undef _HERA_CONVERT_ROS_MESSAGE_TYPES_HPP_
#define ROS_MESSAGE_TYPE_TEMPLATE_EXPAND

#undef ROS_MESSAGE_TYPE_DEFINE
#define ROS_MESSAGE_TYPE_DEFINE(msg_category, msg_type)                             \
    case ROSMessageType::msg_type: {                                                \
        auto ros_message = reinterpret_cast<msg_category::msg_type*>(message->ptr); \
        bag.write(message->topic_name, ros_message->header.stamp, *ros_message);    \
        break;                                                                      \
    }

#include "ros_message_types.hpp"

    default:
        break;
    }
}

template<>
ROSMessagePtr ROSMessage::create<ROSMessageType::EndOfFile>()
{
    auto result = std::unique_ptr<ROSMessage>(new ROSMessage(ROSMessageType::EndOfFile));
    result->timestamp_ns = 0;
    result->ptr = nullptr;
    return result;
}

template<>
ROSMessagePtr ROSMessage::create<ROSMessageType::BrokenData>()
{
    auto result = std::unique_ptr<ROSMessage>(new ROSMessage(ROSMessageType::BrokenData));
    result->timestamp_ns = 0;
    result->ptr = nullptr;
    return result;
}

#undef _HERA_CONVERT_ROS_MESSAGE_TYPES_HPP_
#define ROS_MESSAGE_TYPE_TEMPLATE_EXPAND

#undef ROS_MESSAGE_TYPE_DEFINE
#define ROS_MESSAGE_TYPE_DEFINE(msg_category, msg_type)                                      \
    template<>                                                                               \
    ROSMessagePtr ROSMessage::create<ROSMessageType::msg_type>()                             \
    {                                                                                        \
        auto result = std::unique_ptr<ROSMessage>(new ROSMessage(ROSMessageType::msg_type)); \
        result->ptr = new msg_category::msg_type();                                          \
        return result;                                                                       \
    }

#include "ros_message_types.hpp"

/// Destroy with variant deleter, by type information
///
ROSMessage::~ROSMessage()
{
    if (ptr != nullptr) {
        switch (type) {

#undef _HERA_CONVERT_ROS_MESSAGE_TYPES_HPP_
#define ROS_MESSAGE_TYPE_TEMPLATE_EXPAND

#undef ROS_MESSAGE_TYPE_DEFINE
#define ROS_MESSAGE_TYPE_DEFINE(msg_category, msg_type)        \
    case ROSMessageType::msg_type:                             \
        delete reinterpret_cast<msg_category::msg_type*>(ptr); \
        break;

#include "ros_message_types.hpp"

        default:
            log::error << "Fatal: Unknown Message Type" << log::endl;
            break;
        }
    }
}

std::vector<ROSMessagePtr> ROSMessage::convert(device::data::SensorDataPtr& sensor_data,
                                               const std::string& topic_prefix,
                                               const std::string& frame_id,
                                               const common::Remapper* remapper)
{
    if (sensor_data == nullptr) {
        std::vector<ROSMessagePtr> ret;
        ret.emplace_back(ROSMessage::create<ROSMessageType::EndOfFile>());
        return ret;
    } else {
        switch (sensor_data->sensor_data_type) {
        case device::SensorDataType::Broken: {
            std::vector<ROSMessagePtr> ret;
            ret.emplace_back(ROSMessage::create<ROSMessageType::BrokenData>());
            return ret;
        }

#undef _HERA_DEVICE_SENSOR_DATA_TYPES_HPP_
#define SENSOR_DATA_TYPE_TEMPLATE_EXPAND

#undef SENSOR_DATA_TYPE_DEFINE
#define SENSOR_DATA_TYPE_DEFINE(name, value) \
    case device::SensorDataType::name:       \
        return convert<device::SensorDataType::name>(sensor_data, topic_prefix, frame_id, remapper);

#include "device/include/sensor_data_types.hpp"

        default: {
            log::error << "Converter:: Invalid Sensor Data Type" << log::endl;
            std::vector<ROSMessagePtr> ret;
            ret.emplace_back(ROSMessage::create<ROSMessageType::BrokenData>());
            return ret;
        }
        }
    }
}

}  // namespace convert
}  // namespace hera
}  // namespace wayz
