//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#pragma once

#include <cstdint>
#include <string>

#include <common/utils/system_timestamp.hpp>
#include <ros/ros.h>

namespace wayz {
namespace tron {

ros::Time to_ros_time(const Timestamp& ts);

}  // namespace tron
}  // namespace wayz