///
/// @file imu_data.hpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Derived classes of SensorData for Lidar
/// @version 0.1
/// @date 2019-11-07
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
/// @brief SensorData for Imu and MagneticField composed
///
class ImuMagneticField final : public SensorData {
public:
    double angular_velocity[3];     ///< array of 3-axis angular velocity, in rad/s, CCW, right-handed
    double linear_acceleration[3];  ///< array of 3-axis linear acceleration, in m/s^2, right-handed
    double magnetic_field[3];       ///< array of 3-axis magnetic field, in Tesla, right-handed
};

#pragma pack(pop)

}  // namespace data
}  // namespace device
}  // namespace hera
}  // namespace wayz