//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include <future>
#include <regex>
#include <thread>

#include "common/include/utils/folder_content.hpp"
#include "common/include/utils/time.hpp"
#include "service.hpp"
#include "storage/include/storage.hpp"

namespace wayz {
namespace hera {
namespace daemon {

void Service::getStorage(StorageInfo& result)
{
    log::info << "Daemon::start called getStorage" << log::endl;

    std::unique_lock<std::mutex> _(mutex_);

    append_storage_info(result);
}

void Service::deleteStorage(StorageInfo& result, const std::string& name)
{
    log::info << "Daemon::start called deleteStorage, name = " << name << log::endl;

    std::unique_lock<std::mutex> _(mutex_);

    auto folder = file::get_folder_content(StorageFolder_);
    for (auto& file : folder.files) {
        if (name == file.basename) {
            ::remove(file.fullname.c_str());
            log::info << "Daemon::delete " << file.fullname << log::endl;
            break;
        }
    }

    append_storage_info(result);
}

void Service::append_storage_info(StorageInfo& result)
{
    auto t0 = time::Timestamp::now();

    auto statfs = file::get_filesystem_status(StorageFolder_);
    if (statfs.opened) {
        result.diskUsedSpaceKB = statfs.used_space / 1024;
        result.diskTotalSpaceKB = statfs.total_space / 1024;
    } else {
        result.diskUsedSpaceKB = 0;
        result.diskTotalSpaceKB = 0;
    }

    auto folder = file::get_folder_content(StorageFolder_);

    result.storageRecordFiles.reserve(folder.files.size());

    for (auto& file : folder.files) {
        std::ifstream in_file;
        in_file.open(file.fullname, std::ios::binary);
        if (!in_file.is_open()) {
            continue;
        }
        auto header = storage::StorageDataHeader::read_from(in_file, false, false);
        in_file.close();

        StorageRecordFile info;
        info.name = file.basename;
        info.sizeKB = file.size / 1024;
        info.startTime = time::Timestamp(header->timestamp_start).to_datetime("%Y-%m-%d %H:%M:%S");
        info.endTime = time::Timestamp(header->timestamp_end).to_datetime("%Y-%m-%d %H:%M:%S");
        info.devices.resize(header->device_names.size());

        try {
            for (size_t i = 0; i < header->device_names.size(); ++i) {
                info.devices[i].typeName = header->device_names[i];
                info.devices[i].messageNum = header->device_message_nums[i];
                info.devices[i].dataSizeKB = header->device_data_sizes[i] / 1024;
            }
        } catch (...) {
        };

        result.storageRecordFiles.emplace_back(info);
    }

    std::sort(result.storageRecordFiles.begin(),
              result.storageRecordFiles.end(),
              [](const StorageRecordFile& lhs, const StorageRecordFile& rhs) { return lhs.name < rhs.name; });

    auto t1 = time::Timestamp::now();

    log::debug << "Daemon::storage scaning used " << t1 - t0 << log::endl;
}

}  // namespace daemon
}  // namespace hera
}  // namespace wayz