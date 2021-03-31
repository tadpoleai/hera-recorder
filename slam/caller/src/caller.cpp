///
/// @file caller.cpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Implementation of class Caller
/// @date 2020-02-05
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include "caller.hpp"

#include <string>
#include <unistd.h>

namespace wayz {
namespace hera {
namespace slam {

bool Caller::start()
{
    pid_t pid = fork();
    if (pid > 0) {
        return true;
    } else if (pid < 0) {
        return false;
    }

    // PID == 0
    auto ret = system((std::string("bash ") + SLAM_CARTO_ROOT + "/share/script/start.sh > /dev/null 2>&1").c_str());
    exit(ret);
}

void Caller::stop()
{
    pid_t pid = fork();
    if (pid != 0) {
        return;
    }

    // PID == 0
    auto ret = system((std::string("bash ") + SLAM_CARTO_ROOT + "/share/script/stop.sh > /dev/null 2>&1").c_str());
    exit(ret);
}

}  // namespace slam
}  // namespace hera
}  // namespace wayz
