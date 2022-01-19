///
/// @file ros_message_types.inc
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief Define ROS Messages
/// @date 2020-07-10
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#ifndef _HERA_CONVERT_ROS_MESSAGE_TYPES_HPP_
#define _HERA_CONVERT_ROS_MESSAGE_TYPES_HPP_

#ifndef ROS_MESSAGE_TYPE_TEMPLATE_EXPAND
#define ROS_MESSAGE_TYPE_DEFINE(msg_category, msg_type) msg_type,

#include <cstdint>

namespace wayz {
namespace hera {
namespace convert {

enum class ROSMessageType : int32_t {
    EndOfFile = -2,
    BrokenData = -1,
#endif
    // clang-format off

    ROS_MESSAGE_TYPE_DEFINE(geometry_msgs, Vector3Stamped)
    ROS_MESSAGE_TYPE_DEFINE(sensor_msgs, Imu)
    ROS_MESSAGE_TYPE_DEFINE(sensor_msgs, MagneticField)
    ROS_MESSAGE_TYPE_DEFINE(sensor_msgs, PointCloud2)
    ROS_MESSAGE_TYPE_DEFINE(sensor_msgs, LaserScan)
    ROS_MESSAGE_TYPE_DEFINE(sensor_msgs, CompressedImage)
    ROS_MESSAGE_TYPE_DEFINE(sensor_msgs, FluidPressure)
    ROS_MESSAGE_TYPE_DEFINE(sensor_msgs, Temperature)
    ROS_MESSAGE_TYPE_DEFINE(sensor_msgs, Image)
    ROS_MESSAGE_TYPE_DEFINE(sensor_msgs, NavSatFix)

// clang-format on
#ifndef ROS_MESSAGE_TYPE_TEMPLATE_EXPAND
};  // enum

}  // namespace convert
}  // namespace hera
}  // namespace wayz

#endif

#endif