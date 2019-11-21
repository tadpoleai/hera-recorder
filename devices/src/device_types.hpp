///
/// @file device_types.hpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Type enums for DeviceVendor, StorageData, SensorData and DeviceParameter
/// @version 0.1
/// @date 2019-11-06
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include <cstdint>

#include "common/third_party/enum.hpp"

namespace wayz {
namespace hera {

///
/// @brief Device vendor type
///
/// Category and Vendor name concatenated in PascalCase,
/// e.g., for CameraFlir, the category name is Camera and
/// the vendor-model (commany/manufacturer) name is Flir
/// @note every entry correspond to an derived class of Device
/// @see Device
///
enum class DeviceVendorType : uint16_t {
    DummyFoobar = 0x0101,    ///< A dummy category for testing, vendor is Wayz
    ImuAceinna = 0x0201,     ///< An 9-axis Imu, vendor is Aceinna, embedded in Wayz Tron Sync Board
    GpsHuace = 0x0301,       ///< RTK-GPS, outputs Na`vSatFix, vendor is Huace, not realized yet
    GpsZed = 0x0302,         ///< RTK-GPS, vendor is ZED, not realized yet
    CameraFlir = 0x0401,     ///< Camera, outputs RawImage or CompressedImage, vendor is FLIR
    LidarVelodyne = 0x0501,  ///< Lidar, outputs PointsXYZI, vendor is Velodyne
};

///
/// @brief Storage data type
///
/// Type of storage data, in PascalCase
/// @note every entry correspond to an derived class of StorageData
/// @see StorageData
///
enum class StorageDataType : uint16_t {
    Dummy = 0x0101,                 ///< A dummy device's storage data
    ImuAceinnaEmbedded = 0x0201,    ///< For Wayz Tron Sync Board's serial output
    GpsGprmcString = 0x0301,        ///< For Raw GPRMC string
    CameraFlirCompressed = 0x0401,  ///< For Flir's camera's compressed image
    CameraFlirRaw = 0x0402,         ///< For Flir's camera's raw image
    LidarVelodynePacket = 0x0501,   ///< For Velodyne's raw UDP packet
};

///
/// @brief Sensor data type
///
/// Type of sensor data, in PascalCase.
/// @note data types here should have same or compatible structures with
/// corresponding ROS Message, i.e., having sensor data,
/// no extra effort like some device specificated functions
/// are needed to convert to ROS Message
/// @note every entry correspond to an derived class of SensorData
/// @see SensorData
///
enum class SensorDataType : uint16_t {
    Broken = 0xFFFF,            ///< Mark for a broken data
    Dummy = 0x0101,             ///< A dummy message, no correspond ROS Message
    ImuMagneticField = 0x0201,  ///< ROS Imu and MagneticField
    NavSatFix = 0x0301,         ///< ROS NavSatFix
    CompressedImage = 0x0401,   ///< ROS CompressedImage
    RawImage = 0x0402,          ///< ROS Image, some minor format conversion needed
    PointsXYZI = 0x0501,        ///< ROS PointCloud2, some PCL function needed
};

///
/// @brief Device parameter type
///
/// used by Device::define_parameter() and Device::adjust_parameter();
/// @see Device
///
BETTER_ENUM(DeviceParameterType,
            uint32_t,
            DummyRate = 0,
            DummyValue,
            DummyString,
            IpAddress,
            DataPort,
            TelemetryPort,
            Kernel,
            BaudRate,
            SerialMsgType,
            Exposure,
            WhiteBalanceRed,
            WhiteBalanceBlue,
            FrameRate)

}  // namespace hera
}  // namespace wayz