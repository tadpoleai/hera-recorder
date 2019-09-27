//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//
#include "system_timestamp.hpp"

#include <ctime>

namespace wayz {
namespace tron {

TimestampNs get_system_timestamp()
{
    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    TimestampNs time = (int64_t)(ts.tv_sec) * OneSecondToNs + (int64_t)(ts.tv_nsec);
    return time;
}

}  // namespace tron
}  // namespace wayz