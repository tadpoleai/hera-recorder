///
/// @file device_types.hpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Definition of SensorDataType
/// @version 0.1
/// @date 2020-06-18
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#ifndef _HERA_DEVICE_SENSOR_DATA_TYPES_HPP_
#define _HERA_DEVICE_SENSOR_DATA_TYPES_HPP_

#ifndef SENSOR_DATA_TYPE_TEMPLATE_EXPAND
#define SENSOR_DATA_TYPE_DEFINE(name, value) name = value,

#include "types.hpp"

namespace wayz {
namespace hera {
namespace device {

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
#endif

    SENSOR_DATA_TYPE_DEFINE(Dummy, 0x0101)  ///< A dummy message, no correspond ROS Message
    SENSOR_DATA_TYPE_DEFINE(DummyImage,
                            0x0102)  ///< A dummy message, no correspond ROS Message, only to debug image show
    SENSOR_DATA_TYPE_DEFINE(ImuMagneticField, 0x0201)         ///< ROS Imu and MagneticField
    SENSOR_DATA_TYPE_DEFINE(NavSatFix, 0x0301)                ///< ROS NavSatFix
    SENSOR_DATA_TYPE_DEFINE(InsBestPosition, 0x0381)          ///< Ins Best Position -> ROS NavSatFix
    SENSOR_DATA_TYPE_DEFINE(InsCorrectedImu, 0x0382)          ///< Ins CorrectImu -> ROS Imu
    SENSOR_DATA_TYPE_DEFINE(InsInsPosition, 0x0383)           ///< Ins Position -> ROS NavSatFix
    SENSOR_DATA_TYPE_DEFINE(CompressedImage, 0x0401)          ///< ROS CompressedImage
    SENSOR_DATA_TYPE_DEFINE(Image, 0x0402)                    ///< ROS Image, some minor format conversion needed
    SENSOR_DATA_TYPE_DEFINE(PointsXYZI, 0x0501)               ///< ROS PointCloud2, some PCL function needed
    SENSOR_DATA_TYPE_DEFINE(LaserScan, 0x0502)                ///< ROS LaserScan
    SENSOR_DATA_TYPE_DEFINE(OdometryFrontWheelSpeed, 0x0601)  ///< For Odometry
    SENSOR_DATA_TYPE_DEFINE(OdometryRearWheelSpeed, 0x0602)   ///< For Odometry
    SENSOR_DATA_TYPE_DEFINE(OdometrySteeringAngle, 0x0603)    ///< For Odometry
    SENSOR_DATA_TYPE_DEFINE(OdometryOrientation, 0x0611)      ///< For Odometry, Single Axis Orientation > ROS Imu
    SENSOR_DATA_TYPE_DEFINE(OdometryLocalizationResult,
                            0x0620)  ///< LocalizationResult, feedback by Localization Service to Hera

#ifndef SENSOR_DATA_TYPE_TEMPLATE_EXPAND

    SENSOR_DATA_TYPE_DEFINE(EndOfFile, 0xFFFE)  ///< Mark as end of file
    SENSOR_DATA_TYPE_DEFINE(Broken, 0xFFFF)     ///< Mark for a broken data

};  // enum

}  // namespace device
}  // namespace hera
}  // namespace wayz

#endif

#endif