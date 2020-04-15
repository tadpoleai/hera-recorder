//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include <future>
#include <regex>
#include <thread>

#include "common/include/utils/folder_content.hpp"
#include "service.hpp"

namespace wayz {
namespace hera {
namespace daemon {

void Service::get(Result& result)
{
    log::debug << "Daemon::get called" << log::endl;
    std::unique_lock<std::mutex> _(mutex_);
    append_status(result);
}

void Service::append_status(Result& result)
{
    result.status.started = started_;
    result.status.recording = recording_;
    result.status.storageName = storage_name_;
    result.status.meta = meta_;

    std::vector<std::future<DeviceStatus>> promises;
    // clang-format off
    for (auto& device : devices_) {
        promises.emplace_back(std::async(std::launch::async, [result](auto* device) {
            DeviceStatus info;
            info.id = device->get_id();
            info.type = device->get_vendor_type();
            info.name = device->get_name();
            info.forward = device->get_forward();
            info.error = device->get_errno();
            info.reason = device->get_reason();
            return info;
        },
        device.get()));
        }
    // clang-format on

    try {
        result.status.profiles = profiles_;
        result.status.profileIndex = profile_index_;
    } catch (const std::exception& e) {
        log::error << e.what() << log::endl;
        log::error << "Daemon::Can not get profiles" << log::endl;
    }

    auto statfs = file::get_filesystem_status(FileNamePrefix_);
    if (statfs.opened) {
        result.status.diskUsedSpaceKB = statfs.used_space / 1024;
        result.status.diskTotalSpaceKB = statfs.total_space / 1024;
    } else {
        result.status.diskUsedSpaceKB = 0;
        result.status.diskTotalSpaceKB = 0;
    }

    for (auto& promise : promises) {
        result.status.devices.emplace_back(promise.get());
    }
}

}  // namespace daemon
}  // namespace hera
}  // namespace wayz