///
/// @file device_types.hpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Type enums for DeviceVendor, DeviceData, SensorData and DeviceParameter
/// @version 0.1
/// @date 2019-11-06
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include <cstdint>

#ifdef HERA_COMPILE_IN_REPO
#include "common/include/third_party/enum.hpp"
#else
#include <hera/common/third_party/enum.hpp>
#endif

namespace wayz {
namespace hera {
namespace device {

///
/// @brief Device vendor type
///
/// Category and Vendor name concatenated in PascalCase,
/// e.g., for CameraFlir, the category name is Camera and
/// the vendor-model (commany/manufacturer) name is Flir
/// @note every entry correspond to an derived class of Device
/// @see Device
///
/// IMU = Inertial Measurement Unit
/// AHRS = Attitude and Heading Reference System
/// GNSS = Global Navigation Satellite System
/// INS = Integrated Navigation System
///
/// @note Use term 'GNSS' rather than 'GPS', 'GPS' is one of 'GNSS'
///
enum class DeviceVendorType : uint16_t {
    DummyFoobar = 0x0101,        ///< A dummy category for testing, vendor is Wayz
    DummyImage = 0x0102,         ///< A dummy category for testing, outputs dummy image
    ImuAceinna = 0x0201,         ///< An 9-axis Imu, vendor is Aceinna, embedded in Wayz Tron Sync Board
    ImuS32VSal = 0x0202,         ///< IMU, driver is provided by S32VSal
    GnssSerialsync = 0x0301,     ///< RTK-GNSS, outputs NavSatFix, vendor is any that outpus NMEA
    GnssSerial = 0x0302,         ///< For Nmea Sentence with it's UTC time already synced to system time
    GnssS32VSal = 0x0303,        ///< GNSS, driver is provided by S32VSal
    InsNovatelSpan = 0x0381,     ///< INS vendored by Novatel, product name is SPAN, connected by usb-serial
    CameraFlir = 0x0401,         ///< Camera, outputs RawImage or CompressedImage, vendor is FLIR
    CameraS32VSal = 0x0402,      ///< Camera, outputs RawImage, only for S32V, by mipi-csi, library provided by S32VSal
    LidarVelodyne = 0x0501,      ///< Lidar, outputs PointsXYZI, vendor is Velodyne
    OdometryS32VGeely = 0x0601,  ///< Odometry, only for S32V, by CAN-bus, only for Car vendorGeely
};

///
/// @brief Device data type
///
/// Type of device data, in PascalCase
/// @note every entry correspond to an derived class of DeviceData
/// @see DeviceData
///
enum class DeviceDataType : uint16_t {
    DummyFoobarData = 0x0101,               ///< A dummy device's device data
    DummyImageData = 0x0102,                ///< A dummy image device's data
    ImuAceinnaData = 0x0201,                ///< For Wayz Tron Sync Board's serial output
    ImuS32VSalData = 0x0202,                ///< IMU Data from S32VSal
    GnssSerialsyncNmea = 0x0301,            ///< For Nmea Sentence
    GnssSerialNmea = 0x0302,                ///< For Nmea Sentence with time sync
    GnssS32VSalData = 0x0303,               ///< GNSS Data from S32VSal
    InsNovatelSpanBinaryData = 0x381,       ///< Binary Data of InsNovatelSpan
    CameraFlirCompressedImage = 0x0401,     ///< For Flir's camera's compressed image
    CameraFlirRawImage = 0x0402,            ///< For Flir's camera's raw image
    CameraS32VSalRawImage = 0x0403,         ///< For S32vMipi's camera's raw image
    CameraS32VSalCompressedImage = 0x0404,  ///< For S32vMipi's camera's compressed image
    LidarVelodynePacket = 0x0501,           ///< For Velodyne's raw UDP packet
    OdometryS32VGeelyCANFrame = 0x0601,     ///< For OdometryS32V Geely
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
    EndOfFile = 0xFFFE,                   ///< Mark as end of file
    Broken = 0xFFFF,                      ///< Mark for a broken data
    Dummy = 0x0101,                       ///< A dummy message, no correspond ROS Message
    DummyImage = 0x0102,                  ///< A dummy message, no correspond ROS Message, only to debug image show
    ImuMagneticField = 0x0201,            ///< ROS Imu and MagneticField
    NavSatFix = 0x0301,                   ///< ROS NavSatFix
    InsBestPosition = 0x381,              ///< Ins Best Position -> ROS NavSatFix
    InsCorrectedImu = 0x382,              ///< Ins CorrectImu -> ROS Imu
    InsInsPosition = 0x383,               ///< Ins Position -> ROS NavSatFix
    CompressedImage = 0x0401,             ///< ROS CompressedImage
    Image = 0x0402,                       ///< ROS Image, some minor format conversion needed
    PointsXYZI = 0x0501,                  ///< ROS PointCloud2, some PCL function needed
    OdometryFrontWheelSpeed = 0x0601,     ///< For Odometry
    OdometryRearWheelSpeed = 0x0602,      ///< For Odometry
    OdometrySteeringAngle = 0x0603,       ///< For Odometry
    OdometryLocalizationResult = 0x0620,  ///< LocalizationResult, feedback by Localization Service to Hera
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
            RotationalSpeed,
            SubVendorType,
            Kernel,
            KernelAuxiliary,
            BaudRate,
            BaudRateAuxiliary,
            SerialMsgType,
            Compress,
            CompressQuality,
            Exposure,
            WhiteBalanceRed,
            WhiteBalanceBlue,
            FrameRate)

}  // namespace device
}  // namespace hera
}  // namespace wayz