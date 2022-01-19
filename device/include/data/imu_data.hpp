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

#include "../sensor_data.hpp"

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

///
/// @brief SensorData for Gyro, Acc, MagneticField and Filtered all data composed
///
class ImuComposed final : public SensorData {
public:
    int8_t have_temperature;
    double temperature;  ///< temperature in Degree Celsius

    int8_t have_baro_pressure;
    double baro_pressure;  ///< baro pressure in Pascals.

    int8_t have_angular_velocity;
    double angular_velocity[3];  ///< array of 3-axis angular velocity, in rad/s, CCW, right-handed

    int8_t have_linear_acceleration;
    double linear_acceleration[3];  ///< array of 3-axis linear acceleration, in m/s^2, right-handed

    int8_t have_magnetic_field;
    double magnetic_field[3];  ///< array of 3-axis magnetic field, in Tesla, right-handed

    int8_t have_orientation;
    double orientation[4];  ///< orientation in quaternion form (xyzw), right-handed

    int8_t have_free_linear_acceleration;
    double free_linear_acceleration[3];  ///< array of 3-axis free (without gravity) linear acceleration, in m/s^2,
                                         ///< right-handed
};

#pragma pack(pop)

}  // namespace data
}  // namespace device
}  // namespace hera
}  // namespace wayz