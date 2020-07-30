///
/// @file feedback.hpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief Feed LocalizationResult back
/// @date 2020-04-20
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include "data/odometry_data.hpp"

#ifdef WITH_DRIVER
#include "driver/can/can_port.hpp"

namespace wayz {
namespace hera {
namespace device {
namespace odometry {
namespace feedback {

void feedback(const data::LocalizationResult* result, driver::CANPort* port);

}  // namespace feedback
}  // namespace odometry
}  // namespace device
}  // namespace hera
}  // namespace wayz

#endif