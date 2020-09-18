///
/// @file android_data.hpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief DeviceData used by ExternalAndroid
/// @date 2020-07-29
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include <cmath>

#include "data/camera_data.hpp"
#include "device_data.hpp"

namespace wayz {
namespace hera {
namespace device {
namespace external {
namespace android {

#pragma pack(push, 1)

class AndroidCompressedImage final : public data::DeviceData {
public:
    AndroidCompressedImage() = delete;

public:
    data::CompressedImage::CompressFormat compress_format;  ///< format of compression
    uint32_t image_data_size;                               ///< size of image_data, in bytes
    uint8_t image_data[0];                                  ///< compressed image data
};

class AndroidImuMagneticField final : public data::DeviceData {
public:
    AndroidImuMagneticField() = delete;

public:
    double angular_velocity[3];     ///< array of 3-axis angular velocity, in rad/s, CCW, right-handed
    double linear_acceleration[3];  ///< array of 3-axis linear acceleration, in m/s^2, right-handed
    double magnetic_field[3];       ///< array of 3-axis magnetic field, in Tesla, right-handed
};

class AndroidLocation final : public data::DeviceData {
public:
    AndroidLocation() = delete;

public:
    double longitude;
    double latitude;
    double altitude;

    ///
    /// @brief Horizontal accuracy in meters
    /// We define horizontal accuracy as the radius of 68% confidence. In other
    /// words, if you draw a circle centered at this location's
    /// latitude and longitude, and with a radius equal to the accuracy,
    /// then there is a 68% probability that the true location is inside
    /// the circle.
    /// @see Location.java
    ///
    float horizontal_accuracy;

    ///
    /// @brief Vertical accuracy in meters, similar with horizontal_accuracy
    /// @see Location.java
    ///
    float vertical_accuracy;

    ///
    /// @brief bearing in degrees.
    /// Bearing is the horizontal direction of travel of this device,
    /// and is not related to the device orientation. It is guaranteed to
    /// be in the range (0.0, 360.0] if the device has a bearing.
    /// If this location does not have a bearing then 0.0 is returned.
    /// @see Location.java
    ///
    float bearing_degree;

    ///
    /// @brief speed in meters/second over ground
    /// If this location does not have a speed then 0.0 is returned.
    /// @see Location.java
    float speed;
};

///
/// @brief Same as ROS LaserScan
///
class AndroidLaserScan final : public data::DeviceData {
public:
    AndroidLaserScan() = delete;

public:
    float angle_min;        ///< start angle of the scan [rad]
    float angle_max;        ///< end angle of the scan [rad]
    float angle_increment;  ///< angular distance between measurements [rad]

    float time_increment;  ///< time between measurements [seconds] - if your scanner
                           /// is moving, this will be used in interpolating position
                           /// of 3d points
    float scan_time;       ///< time between scans [seconds]

    float range_min;  ///< minimum range value [m]
    float range_max;  ///< maximum range value [m]

    float ranges_length;               ///< range data [m] (Note: values < range_min or > range_max should be discarded)
    float intensities_length;          ///< intensity data [device-specific units].  If your  device does not provide
                                       ///< intensities, please leave the array empty.
    float ranges_intensities_data[0];  /// < Access ranges and intensites by ptr
};

#pragma pack(pop)

}  // namespace android
}  // namespace external
}  // namespace device
}  // namespace hera
}  // namespace wayz
