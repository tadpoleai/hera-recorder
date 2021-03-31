///
/// @file version.cpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief
/// @date 2020-04-24
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///
///

#include "version.hpp"

namespace wayz {
namespace hera {
namespace common {

std::string get_version()
{
#ifdef GIT_INFO_ENABLED
#include "git_info.cpp"
    return std::string(GIT_COMMIT_HEAD);
#else
    return "Unknown Commit Head";
#endif
}

}  // namespace common
}  // namespace hera
}  // namespace wayz