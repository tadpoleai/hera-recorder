//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#ifndef __utils_sys_time_hpp__
#define __utils_sys_time_hpp__

#include <cstdint>

namespace wayz {

using TimestampNs = int64_t;
using DurationNs = int64_t;
constexpr int64_t OneSecondToNs = (int64_t)(1000000000L);

TimestampNs getSystemTimestamp();

}  // namespace wayz
#endif