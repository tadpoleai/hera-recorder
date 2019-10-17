//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include "utils.hpp"

#include <cstdint>

ros::Time to_ros_time(TimestampNs time)
{
    uint32_t sec = time / OneSecondToNs;
    uint32_t nsec = time % OneSecondToNs;
    return ros::Time(sec, nsec);
}
