///
/// @file odometry_data.hpp
/// @author chunchen.wang (chunchen.wang@wayz.ai)
/// @brief
/// @version 0.1
/// @date 2020-04-08
///
/// Copyright 2018 Wayz.ai. All Rights Reserved.
///
///
#pragma once

#include "../device_data.hpp"

namespace wayz {
namespace hera {
namespace device {
namespace data {

#pragma pack(push, 1)

///
/// @brief SensorData for odometry FrongWheelSpeedAngle
///
class FrontWheelSpeed final : public SensorData {
public:
    double left;   ///< speed of left front wheel
    double right;  ///< speed of right front wheel
};
///
/// @brief SensorData for odometry RearWheelSpeedAngle
///
class RearWheelSpeed final : public SensorData {
public:
    double left;   ///< speed of left rear wheel
    double right;  ///< speed of right rear wheel
};

///
/// @brief SensorData for odometry SteeringAngle
///
class SteeringAngle final : public SensorData {
public:
    double steering_angle;  ///< steering angle (degree)
};


#pragma pack(pop)

}  // namespace data
}  // namespace device
}  // namespace hera
}  // namespace wayz