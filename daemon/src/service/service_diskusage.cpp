
//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include "common/include/utils/folder_content.hpp"
#include "service.hpp"

namespace wayz {
namespace hera {
namespace daemon {

void Service::getDiskUsageStatus(DiskUsageStatus& result)
{
    auto statfs = file::get_filesystem_status(DataDirectory_);
    if (statfs.opened) {
        result.diskUsedSpace = statfs.used_space;
        result.diskTotalSpace = statfs.total_space;
    } else {
        log::warn << "Daemon: can not get disk usage on " << DataDirectory_ << log::endl;
        result.diskUsedSpace = 0;
        result.diskTotalSpace = 0;
    }
}

}  // namespace daemon
}  // namespace hera
}  // namespace wayz