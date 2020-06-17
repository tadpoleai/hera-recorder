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
    case ROSMessageType::Vector3Stamped: {
        auto ros_message = reinterpret_cast<geometry_msgs::Vector3Stamped*>(message->ptr);
        bag.write(message->topic_name, ros_message->header.stamp, *ros_message);
        break;
    }
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
    case ROSMessageType::Image: {
        auto ros_message = reinterpret_cast<sensor_msgs::Image*>(message->ptr);
        bag.write(message->topic_name, ros_message->header.stamp, *ros_message);
        break;
    }
    case ROSMessageType::NavSatFix: {
        auto ros_message = reinterpret_cast<sensor_msgs::NavSatFix*>(message->ptr);
        bag.write(message->topic_name, ros_message->header.stamp, *ros_message);
        break;
    }
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

template<>
ROSMessagePtr ROSMessage::create<ROSMessageType::Vector3Stamped>()
{
    auto result = std::unique_ptr<ROSMessage>(new ROSMessage(ROSMessageType::Vector3Stamped));
    result->ptr = new geometry_msgs::Vector3Stamped();
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

template<>
ROSMessagePtr ROSMessage::create<ROSMessageType::Image>()
{
    auto result = std::unique_ptr<ROSMessage>(new ROSMessage(ROSMessageType::Image));
    result->ptr = new sensor_msgs::Image();
    return result;
}

template<>
ROSMessagePtr ROSMessage::create<ROSMessageType::NavSatFix>()
{
    auto result = std::unique_ptr<ROSMessage>(new ROSMessage(ROSMessageType::NavSatFix));
    result->ptr = new sensor_msgs::NavSatFix();
    return result;
}

/// Destroy with variant deleter, by type information
///
ROSMessage::~ROSMessage()
{
    if (ptr != nullptr) {
        switch (type) {
        case ROSMessageType::Vector3Stamped:
            delete reinterpret_cast<geometry_msgs::Vector3Stamped*>(ptr);
            break;
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
        case ROSMessageType::Image:
            delete reinterpret_cast<sensor_msgs::Image*>(ptr);
            break;
        case ROSMessageType::NavSatFix:
            delete reinterpret_cast<sensor_msgs::NavSatFix*>(ptr);
            break;
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
        ret.emplace_back(std::move(ROSMessage::create<ROSMessageType::EndOfFile>()));
        return ret;
    } else {
        switch (sensor_data->sensor_data_type) {
        case device::SensorDataType::Broken: {
            std::vector<ROSMessagePtr> ret;
            ret.emplace_back(std::move(ROSMessage::create<ROSMessageType::BrokenData>()));
            return ret;
        }
        case device::SensorDataType::Dummy:
            return convert<device::SensorDataType::Dummy>(sensor_data, topic_prefix, frame_id, remapper);
        case device::SensorDataType::ImuMagneticField:
            return convert<device::SensorDataType::ImuMagneticField>(sensor_data, topic_prefix, frame_id, remapper);
        case device::SensorDataType::PointsXYZI:
            return convert<device::SensorDataType::PointsXYZI>(sensor_data, topic_prefix, frame_id, remapper);
        case device::SensorDataType::CompressedImage:
            return convert<device::SensorDataType::CompressedImage>(sensor_data, topic_prefix, frame_id, remapper);
        case device::SensorDataType::Image:
            return convert<device::SensorDataType::Image>(sensor_data, topic_prefix, frame_id, remapper);
        case device::SensorDataType::NavSatFix:
            return convert<device::SensorDataType::NavSatFix>(sensor_data, topic_prefix, frame_id, remapper);
        case device::SensorDataType::InsBestPosition:
            return convert<device::SensorDataType::InsBestPosition>(sensor_data, topic_prefix, frame_id, remapper);
        case device::SensorDataType::OdometryOrientation:
            return convert<device::SensorDataType::OdometryOrientation>(sensor_data, topic_prefix, frame_id, remapper);
        default: {
            log::error << "Converter:: Invalid Sensor Data Type" << log::endl;
            std::vector<ROSMessagePtr> ret;
            ret.emplace_back(std::move(ROSMessage::create<ROSMessageType::BrokenData>()));
            return ret;
        }
        }
    }
}

}  // namespace convert
}  // namespace hera
}  // namespace wayz
