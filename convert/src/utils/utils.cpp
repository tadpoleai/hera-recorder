//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include "utils.hpp"

namespace wayz {
namespace tron {

ros::Time to_ros_time(const Timestamp& ts)
{
    return ros::Time(ts.tv_sec, ts.tv_nsec);
}

}  // namespace tron
}  // namespace wayz
