///
/// @file upload.cpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief Base and Factory of class upload::Manager
/// @date 2020-05-13
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include "upload.hpp"

#include "common/include/logger/logger.hpp"
#include "upload/nfs/nfs.hpp"
#include "upload/rsync/rsync.hpp"

namespace wayz {
namespace hera {
namespace storage {
namespace upload {

std::unique_ptr<Manager> Manager::create(const Config& config)
{
    switch (config.remote_protocol) {
    case UploadProtocol::RSYNC:
        log::debug << "New RSYNC Process" << log::endl;
        return std::make_unique<Rsync>(config);
        break;

    case UploadProtocol::NFS:
        log::debug << "New NFS Process" << log::endl;
        return std::make_unique<Nfs>(config);
        break;

    default:
        return nullptr;
    }
}

void Manager::set_error(const std::string& reason)
{
    status_.stage = Stage::Error;
    status_.error_reason += reason + " \n ";

    log::error << "UploadManager: Error, " << reason << log::endl;
}

}  // namespace upload
}  // namespace storage
}  // namespace hera
}  // namespace wayz
