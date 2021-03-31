///
/// @file caller.hpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Header of class Caller
/// @date 2020-02-05
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include <string>
#include <unistd.h>

namespace wayz {
namespace hera {
namespace slam {

///
/// @brief Class to call processes of slam
///
class Caller final {
public:
    Caller() = delete;

    ~Caller() = delete;

    /// Start the processes of hera slam
    bool static start();

    /// Kill the processes of hera slam
    /// @note this routine kills all processes match pattern as follows
    /// ros, robot, carto, slam
    /// @note DO NOT RUN IT WITH OTHER ROS PROGRAMS
    void static stop();
};

}  // namespace slam
}  // namespace hera
}  // namespace wayz
