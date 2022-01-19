//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include <algorithm>
#include <future>
#include <numeric>
#include <regex>
#include <sstream>
#include <thread>
#include <vector>

#include "common/include/utils/folder_content.hpp"
#include "common/include/utils/time.hpp"
#include "service.hpp"
#include "storage/include/storage.hpp"

namespace wayz {
namespace hera {
namespace daemon {

void Service::getUploadServers(std::vector<std::string>& result)
{
    log::info << "Daemon: called getUploadServers" << log::endl;

    std::unique_lock<std::mutex> _(mutex_);

    for (auto& server : remote_servers_) {
        result.push_back(server.remark);
    }
}

void Service::getUploadProcesses(std::vector<UploadProcess>& result)
{
    std::unique_lock<std::mutex> _(mutex_);

    append_upload_processes(result);
}

void Service::getLocalDisks(std::vector<LocalDisk>& result)
{
    log::info << "Daemon:: called get Local Disks" << log::endl;

    std::unique_lock<std::mutex> _(mutex_);

    auto root_id = file::get_filesystem_status("/").device_id;
    auto mountpoint = file::get_folder_content(LocalDiskMountPoint_);

    if (mountpoint.opened) {
        for (auto& disk : mountpoint.folders) {
            auto fs = file::get_filesystem_status(disk.fullname);
            if (fs.device_id == root_id) {
                continue;
            }

            LocalDisk local_disk;
            local_disk.name = disk.basename;
            local_disk.diskUsageStatus.diskTotalSpace = fs.total_space;
            local_disk.diskUsageStatus.diskUsedSpace = fs.used_space;

            result.emplace_back(std::move(local_disk));
        }
    }
}

void Service::getLocalDiskFolders(std::vector<std::string>& result, const std::vector<std::string>& path)
{
    log::info << "Daemon:: called getLocalDiskFolders" << log::endl;

    std::unique_lock<std::mutex> _(mutex_);

    if (path.empty()) {
        log::warn << " Invalid Path calling getLocalDiskFolders" << log::endl;
        return;
    }
    for (auto& layer : path) {
        if (layer.size() == 0) {
            log::warn << " Invalid Path calling getLocalDiskFolders" << log::endl;
            return;
        }
        if (layer[0] == '.') {
            log::warn << " Invalid Path calling getLocalDiskFolders" << log::endl;
            return;
        }
    }

    std::stringstream path_string;
    static const char* delim = "/";
    std::copy(path.begin(), path.end(), std::ostream_iterator<std::string>(path_string, delim));

    auto parent = file::get_folder_content(LocalDiskMountPoint_ + "/" + path_string.str());
    if (parent.opened) {
        for (auto& child : parent.folders) {
            result.push_back(child.basename);
        }
    }
}

void Service::requestUpload(std::vector<UploadProcess>& result, const std::vector<UploadRequest>& requests)
{
    log::info << "Daemon::start called requestUpload" << log::endl;

    std::unique_lock<std::mutex> _(mutex_);

    for (auto& request : requests) {
        // Find existing process
        bool found = false;
        auto it = upload_managers_.begin();
        for (; it != upload_managers_.end(); ++it) {
            if (request.remote.empty()) {
                if ((*it)->get_config().name == request.name &&
                    (*it)->get_config().destination == LocalDiskMountPoint_ + '/' + request.extraPath) {
                    found = true;
                    break;
                }
            } else {
                if ((*it)->get_config().name == request.name && (*it)->get_config().remark == request.remote) {
                    found = true;
                    break;
                }
            }
        }

        if (found) {
            auto config = (*it)->get_config();
            switch (request.operationType) {
            case UploadOperationType::type::Start:
                if (!(*it)->running()) {
                    (*it).reset();
                    (*it) = storage::upload::Transmission::create(config);
                }
                break;

            case UploadOperationType::type::Complete:
                if (!(*it)->running()) {
                    upload_managers_.erase(it);
                }
                break;

            case UploadOperationType::type::Retry:
                if (!(*it)->running()) {
                    (*it).reset();
                    (*it) = storage::upload::Transmission::create(config);
                }
                break;

            case UploadOperationType::type::Abort:
                if ((*it)->running()) {
                    (*it)->terminate();
                }
                break;
            default:
                break;
            }

            return append_upload_processes(result);
        }

        // Not found, start new
        if (request.operationType != UploadOperationType::type::Start) {
            return append_upload_processes(result);
        }

        // Find source file
        log::debug << "Finding source file, " << request.name << log::endl;
        auto folder = file::get_folder_content(DataDirectory_);
        bool file_found = false;
        std::string fullname;
        for (auto& file : folder.files) {
            if (request.name == file.basename) {
                file_found = true;
                fullname = file.fullname;
                break;
            }
        }

        if (!file_found) {
            return append_upload_processes(result);
        }

        // Find remote server
        log::debug << "Finding remote server, " << request.remote << log::endl;
        bool server_found = false;
        Config::UploadServer server;
        for (auto& r : remote_servers_) {
            if (request.remote == r.remark) {
                server_found = true;
                server = r;
                break;
            }
        }

        if (!server_found && !request.remote.empty()) {
            return append_upload_processes(result);
        }

        try {
            if (server_found) {  // Remote Server
                auto m = storage::upload::Transmission::create({
                        .name = request.name,
                        .source = fullname,
                        .protocol = server.protocol,
                        .destination = server.destination,
                        .remark = server.remark,
                });
                if (m) {
                    upload_managers_.emplace_back(std::move(m));
                }
            } else {  // Local
                auto m = storage::upload::Transmission::create(
                        {.name = request.name,
                         .source = fullname,
                         .protocol = "local",
                         .destination = LocalDiskMountPoint_ + '/' + request.extraPath,
                         .remark = ""});
                if (m) {
                    upload_managers_.emplace_back(std::move(m));
                }
            }
        } catch (...) {
        }
    }

    return append_upload_processes(result);
}

void Service::append_upload_processes(std::vector<UploadProcess>& result)
{
    result.reserve(upload_managers_.size());
    for (const auto& m : upload_managers_) {
        UploadProcess p;
        p.waiting = m->waiting();
        p.running = m->running();
        p.errored = m->errored();
        p.reason = m->get_status().error_reason;

        p.request.name = m->get_config().name;
        p.request.remote = m->get_config().remark;
        if (p.request.remote.empty()) {
            p.request.extraPath = m->get_config().destination.substr(LocalDiskMountPoint_.size() + 1);
        }

        p.totalSize = m->get_status().total_size;
        p.processedSize = m->get_status().processed_size;

        p.speed = m->get_status().speed_literal;
        p.eta = m->get_status().eta;

        result.emplace_back(std::move(p));
    }
}

}  // namespace daemon
}  // namespace hera
}  // namespace wayz