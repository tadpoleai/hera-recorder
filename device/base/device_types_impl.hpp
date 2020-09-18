///
/// @file device_types.hpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Definition of DeviceVendorType DeviceDataType
/// @version 0.1
/// @date 2020-06-18
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include "types.hpp"

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
    OdometryAEHaitai = 0x0611,   ///< Odometry, absolute encoder connected by serial, vendor is haitai

    ExternalAndroid = 0xF001,  ///< External Sensor, Android Record Program, no driver
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
    InsNovatelSpanBinaryData = 0x0381,      ///< Binary Data of InsNovatelSpan
    CameraFlirCompressedImage = 0x0401,     ///< For Flir's camera's compressed image
    CameraFlirRawImage = 0x0402,            ///< For Flir's camera's raw image
    CameraS32VSalRawImage = 0x0403,         ///< For S32vMipi's camera's raw image
    CameraS32VSalCompressedImage = 0x0404,  ///< For S32vMipi's camera's compressed image
    LidarVelodynePacket = 0x0501,           ///< For Velodyne's raw UDP packet
    LidarVelodynePacketUnsync = 0x0502,     ///< For Velodyne's raw UDP packet, sync input is invalid
    Lidar3iroBotixPacket = 0x0503,          ///< 2D Lidar Raw Packet, vendor is 3irobotix
    OdometryS32VGeelyCANFrame = 0x0601,     ///< For OdometryS32V Geely
    OdometryAEHaitaiData = 0x0611,          ///< For OdometryS32V Geely

    ExternalAndroidCompressedImage = 0xF001,  ///< ExternalAndroid, Camera Compressed Image
    // ExternalAndroidRawImage = 0xF002,         ///< ExternalAndroid, Camera Raw Image
    ExternalAndroidImuMagneticField = 0xF011,  ///< ExternalAndroid, Imu Data and Magentic Data
    ExternalAndroidLocation = 0xF021,          ///< ExternalAndroid, Position
    ExternalAndroidLaserScan = 0xF031,         ///< ExternalAndroid, 2D Lidar Scan
    // ExternalAndroidPoints = 0xF032,           ///< ExternalAndroid, 3D Lidar Scan
};

}  // namespace device
}  // namespace hera
}  // namespace wayz
