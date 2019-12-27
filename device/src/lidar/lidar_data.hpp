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

#include "../device_data.hpp"

namespace wayz {
namespace hera {
namespace device {
namespace data {

#pragma pack(push, 1)

///
/// @brief SensorData for Lidar
///
class PointsXYZI final : public SensorData {
public:
    ///
    /// @brief Structure for lidar point in 3D with intensity
    ///
    struct PointXYZI {
        float x;          ///< Distance on X-axis, in meter
        float y;          ///< Distance on Y-axis, in meter
        float z;          ///< Distance on Z-axis, in meter
        int32_t channel;  ///< Channel number, if multi-channel lidar, from 0
        float intensity;  ///< Intensity aka reflectivity
    };
    uint32_t point_number;  ///< Number of points
    PointXYZI points[0];    ///< Array of points, variable-lengthed
};

#pragma pack(pop)

}  // namespace data
}  // namespace device
}  // namespace hera
}  // namespace wayz