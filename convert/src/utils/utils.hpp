//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#pragma once

#include <ros/ros.h>

using TimestampNs = int64_t;
using DurationNs = int64_t;
constexpr int64_t OneSecondToNs = (int64_t)(1000000000L);

ros::Time to_ros_time(TimestampNs time);