
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
    log::info << "Daemon: called getDiskUsageStatus" << log::endl;

    auto statfs = file::get_filesystem_status(StorageFolder_);
    if (statfs.opened) {
        result.diskUsedSpaceKB = statfs.used_space / 1024;
        result.diskTotalSpaceKB = statfs.total_space / 1024;
    } else {
        log::warn << "Daemon: can not get disk usage on " << StorageFolder_ << log::endl;
        result.diskUsedSpaceKB = 0;
        result.diskTotalSpaceKB = 0;
    }
}

}  // namespace daemon
}  // namespace hera
}  // namespace wayz