///
/// @file device_types.hpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Forward declaration of type enums for DeviceVendor, DeviceData, SensorData
/// @version 0.1
/// @date 2019-11-06
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include <cstdint>

namespace wayz {
namespace hera {
namespace device {

///
/// @brief Forward declaration of DeviceVendorType
///
/// Category and Vendor name concatenated in PascalCase,
/// e.g., for CameraFlir, the category name is Camera and
/// the vendor-model (commany/manufacturer) name is Flir
/// @note every entry correspond to an derived class of Device
/// @see Device
///
using DeviceVendorType = uint16_t;

///
/// @brief Forward declaration of DeviceDataType
///
/// Type of device data, in PascalCase
/// @note every entry correspond to an derived class of DeviceData
/// @see DeviceData
///
using DeviceDataType = uint16_t;

///
/// @brief Forward declaration of SensorDataType
///
/// Type of sensor data, in PascalCase.
/// @note data types here should have same or compatible structures with
/// corresponding ROS Message, i.e., having sensor data,
/// no extra effort like some device specificated functions
/// are needed to convert to ROS Message
/// @note every entry correspond to an derived class of SensorData
/// @see SensorData
///
enum class SensorDataType : uint16_t;

}  // namespace device
}  // namespace hera
}  // namespace wayz