//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include <future>
#include <numeric>
#include <regex>
#include <thread>
#include <vector>

#include "common/include/utils/folder_content.hpp"
#include "common/include/utils/time.hpp"
#include "service.hpp"
#include "storage/include/storage.hpp"

namespace wayz {
namespace hera {
namespace daemon {

void Service::getUploadInfo(UploadInfo& result)
{
    std::unique_lock<std::mutex> _(mutex_);

    append_upload_info(result);
}

void Service::operateUpload(UploadInfo& result, const UploadOperationType::type op, const UploadRequest& request)
{
    log::info << "Daemon::start called operateUpload" << log::endl;

    std::unique_lock<std::mutex> _(mutex_);

    // Find existing process
    bool found = false;
    auto it = upload_managers_.begin();
    for (; it != upload_managers_.end(); ++it) {
        if ((*it)->get_config().name == request.name && (*it)->get_config().remote_remark == request.remote) {
            found = true;
            break;
        }
    }

    if (found) {
        auto config = (*it)->get_config();
        switch (op) {
        case UploadOperationType::type::Start:
            if (!(*it)->running()) {
                (*it).reset();
                (*it) = storage::upload::Manager::create(config);
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
                (*it) = storage::upload::Manager::create(config);
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

        return append_upload_info(result);
    }

    // Not found, start new
    if (op != UploadOperationType::type::Start) {
        return append_upload_info(result);
    }

    // Find source file
    auto folder = file::get_folder_content(StorageFolder_);
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
        return append_upload_info(result);
    }

    // Find remote server
    bool server_found = false;
    RemoteServerType server;
    for (auto& r : RemoteServers_) {
        if (request.remote == r.remark) {
            server_found = true;
            server = r;
            break;
        }
    }

    if (!server_found) {
        return append_upload_info(result);
    }

    try {
        auto protocol = storage::upload::UploadProtocol::_from_string_nocase(server.protocol.c_str());
        auto m = storage::upload::Manager::create({
            name : request.name,
            source : fullname,
            remote_protocol : protocol,
            remote_destination : server.destination,
            remote_remark : server.remark,
        });
        if (m) {
            upload_managers_.emplace_back(std::move(m));
        }
        return append_upload_info(result);
    } catch (...) {
        return append_upload_info(result);
    }
}

void Service::append_upload_info(UploadInfo& result)
{
    result.uploadProcesses.reserve(upload_managers_.size());
    for (const auto& m : upload_managers_) {
        UploadProcess p;
        p.running = m->running();
        p.errored = m->errored();
        p.reason = m->get_status().error_reason;

        p.request.name = m->get_config().name;
        p.request.remote = m->get_config().remote_remark;

        p.totalSizeKB = m->get_status().total_size / 1024;
        p.processedSizeKB = m->get_status().processed_size / 1024;

        p.speed = m->get_status().speed_literal;
        p.eta = m->get_status().eta;

        result.uploadProcesses.emplace_back(std::move(p));
    }

    for (const auto& r : RemoteServers_) {
        result.remoteServers.push_back(r.remark);
    }
}

}  // namespace daemon
}  // namespace hera
}  // namespace wayz