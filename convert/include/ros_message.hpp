///
/// @file ros_message.hpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Header of class ROSMessage
/// @version 0.1
/// @date 2019-11-12
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include <cstdint>
#include <memory>

#include "common/include/utils/remapper.hpp"
#include "device/include/include.hpp"
#include "direct_bag/direct_bag.h"
#include "ros_message_types.hpp"

namespace wayz {
namespace hera {

///
/// @brief converter and ROS related codes
///
namespace convert {

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
    /// @brief Destroy the ROSMessage object
    ///
    ~ROSMessage();

    ///
    /// @brief Create a ROSMessage object
    ///
    /// @tparam T type of ROSMessage, from ROSMessageType
    /// @return ROSMessagePtr a unique pointer to created message
    ///
    /// Calls templated function by T in ros_message.cpp
    template<ROSMessageType T>
    static ROSMessagePtr create();

    ///
    /// @brief Create a ROSMessage object from sensor data object
    ///
    /// @param sensor_data SensorData
    /// @param topic_prefix Topic prefix of sensor, i.e. "/lidar/top/"
    /// @param frame_id Frame ID of sensor, already remapped
    /// @param remapper Global remapper
    /// @return std::vector<ROSMessagePtr> vector of unique pointer to created messages
    ///
    //// Calls template<device::SensorDataType T> convert()
    static std::vector<ROSMessagePtr> convert(device::data::SensorDataPtr& sensor_data,
                                              const std::string& topic_prefix,
                                              const std::string& frame_id,
                                              const common::Remapper* remapper);

private:
    ///
    /// @brief Construct a new ROSMessage object
    ///
    /// @param t type of ROSMessage, from ROSMessageType
    ROSMessage(ROSMessageType t) : type(t) {}

    ///
    /// @brief Create a ROSMessage object from sensor data object
    ///
    /// @param sensor_data SensorData
    /// @param topic_prefix Topic prefix of sensor, i.e. "/lidar/top/"
    /// @param frame_id Frame ID of sensor, already remapped
    /// @param remapper Global remapper
    /// @return std::vector<ROSMessagePtr> vector of unique pointer to created messages
    ///
    //// Calls create()
    template<device::SensorDataType T>
    static std::vector<ROSMessagePtr> convert(device::data::SensorDataPtr& sensor_data,
                                              const std::string& topic_prefix,
                                              const std::string& frame_id,
                                              const common::Remapper* remapper);

    ///
    /// @brief Cast a time::Timestamp to ros header
    ///
    /// @param ts time::Timestamp
    /// @return ros::Time
    static ros::Time to_ros_time(time::Timestamp ts)
    {
        return ros::Time(ts.tv_sec, ts.tv_nsec);
    }

public:
    ROSMessageType type;     ///< type of ros message
    void* ptr;               ///< pointer to ros message
    int64_t timestamp_ns;    ///< timestamp of message, in UTC, ns
    std::string topic_name;  ///< topic name of ros message
};

}  // namespace convert
}  // namespace hera
}  // namespace wayz