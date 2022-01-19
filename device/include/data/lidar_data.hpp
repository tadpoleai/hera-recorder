///
/// @file lidar_data.hpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Derived classes of SensorData for Lidar
/// @version 0.1
/// @date 2019-11-06
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include "../sensor_data.hpp"

namespace wayz {
namespace hera {
namespace device {
namespace data {

#pragma pack(push, 1)

///
/// @brief Same as ROS LaserScan
///
class LaserScan final : public SensorData {
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

    int32_t ranges_length;             ///< range data [m] (Note: values < range_min or > range_max should be discarded)
    int32_t intensities_length;        ///< intensity data [device-specific units].  If your  device does not provide
                                       ///< intensities, please leave the array empty.
    float ranges_intensities_data[0];  /// < Access ranges and intensites by ptr
};

///
/// @brief SensorData for Lidar
///
class Points final : public SensorData {
public:
    enum class LidarVendor : uint32_t {
        VendorVelodyne = 0x100,
        VelodyneVLP16C = 0x101,
        VelodyneVLP32C = 0x102,
        VelodyneHDL32E = 0x103,
        VendorHesai = 0x200,
        VendorSick = 0x400,
        VendorLeishen = 0x800
    };

    enum class ReturnType : uint32_t {
        Strongest = 0x0,  ///< Single Strongest Return
        Last = 0x1,       ///< Last Return
        Dual = 0x2,       ///< Dual Return
        First = 0x3       ///< First Return
    };

    enum class PointFormat : uint32_t {
        XYZI = 0x0,
        XYZIRT = 0x1,
    };

    struct MetaType {
        LidarVendor vendor;

        ReturnType return_type;

        PointFormat point_format;

        int32_t rotation_direction;

        int32_t num_channel;
        double nominal_pitch_increment;

        double time_increment;
        double time_increment_horizontal;
        double total_time;

        double nominal_min_range;
        double nominal_max_range;

        double azimuth;
    };

    ///
    /// @brief Structure for lidar point in 3D with intensity
    ///
    struct PointXYZCIDPAT {
        float x;                    ///< Distance on X-axis, in meter
        float y;                    ///< Distance on Y-axis, in meter
        float z;                    ///< Distance on Z-axis, in meter
        int32_t ring;               ///< Rings number, from 0, z- + z+
        float intensity;            ///< Intensity aka reflectivity
        float horizontal_distance;  ///< Hypot distance on XY-plane, in meter
        float pitch;                ///< Pitch, in rad
        float azimuth;              ///< Azimuth, in rad
        float time_offset;          ///< Point's time difference w.r.t timestamp_intrinsic
        int32_t channel;            ///< Channel number, if multi-channel lidar, from 0
    };

    MetaType meta;
    uint32_t point_number;     ///< Number of points
    PointXYZCIDPAT points[0];  ///< Array of points, variable-lengthed
                               /// For single-return-type lidar data, data are arranged sequencently
                               /// For dual-return-type lidar data, first-return data are arranged first, after
                               /// which second-return data are arranged
};

#pragma pack(pop)

}  // namespace data
}  // namespace device
}  // namespace hera
}  // namespace wayz