///
/// @file ros_message.hpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Header of class ROSMessage
/// @version 0.1
/// @date 2019-11-12
///
/// @copyright Copyright (c) 2019
///

#pragma once
#include <cstdint>
#include <memory>

#include <sensor_msgs/CompressedImage.h>
#include <sensor_msgs/Imu.h>
#include <sensor_msgs/MagneticField.h>
#include <sensor_msgs/PointCloud2.h>

#include "direct_bag/direct_bag.h"

namespace wayz {
namespace hera {

///
/// @brief converter and ROS related codes
///
namespace convert {

///
/// @brief ROS message type enum
///
enum class ROSMessageType : int32_t {
    EndOfFile = -2,   ///< End of file, storage reading over
    BrokenData = -1,  ///< Invalid data, or bad data;
    Imu = 0,          ///< ROS sensor_msgs/Imu
    MagneticField,    ///< ROS sensor_msgs/MagneticField
    PointCloud2,      ///< ROS sensor_msgs/PointCloud2
    CompressedImage,  ///< ROS sensor_msgs/CompressedImage
    Image,            ///< ROS sensor_msgs/Image
};

class ROSMessage;

///
/// @brief An unique pointer to ROSMessage
///
using ROSMessagePtr = std::unique_ptr<ROSMessage>;

///
/// @brief A Decorator class of ROS defined messages
///
/// It keeps type trait for various ROS messages,
/// also it keeps the topic name and timstamp.
///
/// We can directly write it to a bag file
///
class ROSMessage {
    ///
    /// @brief Write a ROSMessage to a bag file
    ///
    /// @param bag a ros bag file handler, implemented by DirectBag
    /// @param message pointer to a ROSMessage
    friend void operator<<(rosbag_direct_write::DirectBag& bag, ROSMessagePtr&& message);

public:
    ///
    /// @brief Create a ROSMessage object
    ///
    /// @tparam T type of ROSMessage, from ROSMessageType
    /// @return ROSMessagePtr a unique pointer to created message
    ///
    /// Call templated function by T in ros_message.cpp
    template<ROSMessageType T>
    static ROSMessagePtr create();

    ///
    /// @brief Destroy the ROSMessage object
    ///
    ~ROSMessage();

private:
    ///
    /// @brief Construct a new ROSMessage object
    ///
    /// @param t type of ROSMessage, from ROSMessageType
    ROSMessage(ROSMessageType t) : type(t) {}

public:
    ROSMessageType type;     ///< type of ros message
    void* ptr;               ///< a void-typed pointer to ros message
    int64_t timestamp_ns;    ///< timestamp of message, in UTC, ns
    std::string topic_name;  ///< topic name of message
};

}  // namespace convert
}  // namespace hera
}  // namespace wayz