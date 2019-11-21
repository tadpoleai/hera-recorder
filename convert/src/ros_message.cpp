///
/// @file ros_message.cpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Implementation of class ROSMessage
/// @date 2019-11-12
///
/// @copyright Copyright (c) 2019
///

#include "ros_message.hpp"

namespace wayz {
namespace hera {
namespace convert {

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

template<>
ROSMessagePtr ROSMessage::create<ROSMessageType::Imu>()
{
    auto result = std::unique_ptr<ROSMessage>(new ROSMessage(ROSMessageType::Imu));
    result->ptr = new sensor_msgs::Imu();
    return result;
}

template<>
ROSMessagePtr ROSMessage::create<ROSMessageType::MagneticField>()
{
    auto result = std::unique_ptr<ROSMessage>(new ROSMessage(ROSMessageType::MagneticField));
    result->ptr = new sensor_msgs::MagneticField();
    return result;
}

template<>
ROSMessagePtr ROSMessage::create<ROSMessageType::PointCloud2>()
{
    auto result = std::unique_ptr<ROSMessage>(new ROSMessage(ROSMessageType::PointCloud2));
    result->ptr = new sensor_msgs::PointCloud2();
    return result;
}

template<>
ROSMessagePtr ROSMessage::create<ROSMessageType::CompressedImage>()
{
    auto result = std::unique_ptr<ROSMessage>(new ROSMessage(ROSMessageType::CompressedImage));
    result->ptr = new sensor_msgs::CompressedImage();
    return result;
}

/// Destroy with variant deleter, by type information
///
ROSMessage::~ROSMessage()
{
    if (ptr != nullptr) {
        switch (type) {
        case ROSMessageType::Imu:
            delete reinterpret_cast<sensor_msgs::Imu*>(ptr);
            break;
        case ROSMessageType::MagneticField:
            delete reinterpret_cast<sensor_msgs::MagneticField*>(ptr);
            break;
        case ROSMessageType::PointCloud2:
            delete reinterpret_cast<sensor_msgs::PointCloud2*>(ptr);
            break;
        case ROSMessageType::CompressedImage:
            delete reinterpret_cast<sensor_msgs::CompressedImage*>(ptr);
            break;
        default:
            std::cout << "Fatal: Unknown Message Type" << std::endl;
            break;
        }
    }
}

/// Write to a bag file,
/// with variant write function, by type information
void operator<<(rosbag_direct_write::DirectBag& bag, ROSMessagePtr&& message)
{
    switch (message->type) {
    case ROSMessageType::Imu: {
        auto ros_message = reinterpret_cast<sensor_msgs::Imu*>(message->ptr);
        bag.write(message->topic_name, ros_message->header.stamp, *ros_message);
        break;
    }
    case ROSMessageType::MagneticField: {
        auto ros_message = reinterpret_cast<sensor_msgs::MagneticField*>(message->ptr);
        bag.write(message->topic_name, ros_message->header.stamp, *ros_message);
        break;
    }
    case ROSMessageType::PointCloud2: {
        auto ros_message = reinterpret_cast<sensor_msgs::PointCloud2*>(message->ptr);
        bag.write(message->topic_name, ros_message->header.stamp, *ros_message);
        break;
    }
    case ROSMessageType::CompressedImage: {
        auto ros_message = reinterpret_cast<sensor_msgs::CompressedImage*>(message->ptr);
        bag.write(message->topic_name, ros_message->header.stamp, *ros_message);
        break;
    }
    default:
        break;
    }
}

}  // namespace convert
}  // namespace hera
}  // namespace wayz
