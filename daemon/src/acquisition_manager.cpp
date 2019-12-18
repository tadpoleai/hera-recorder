//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include "acquisition_manager.hpp"

#include <future>
#include <regex>
#include <thread>

namespace wayz {
namespace hera {
namespace daemon {

void AcquisitionManager::handle_error(Result& result, HeraErrno hera_errno, std::string&& reason, bool die)
{
    result.error = hera_errno;
    result.reason = std::forward<std::string>(reason);
    log::error << "Acquisition::Error: " << hera_errno << result.reason << log::endl;
    if (die) {
        reset();
    }
    append_status(result);
}

void AcquisitionManager::handle_success(Result& result)
{
    result.error = HeraErrno::OK;
    result.reason = "OK";
    log::info << "Acquisition::succeed" << log::endl;
    append_status(result);
}

void AcquisitionManager::reset()
{
    std::vector<std::future<void>> promises;
    for (const auto& device : devices_) {
        promises.emplace_back(std::async(std::launch::async, &Device::stop, device.get()));
    }
    for (auto& promise : promises) {
        promise.get();
    }
    devices_.clear();
    inited_ = false;
    record_ = false;
}

}  // namespace daemon
}  // namespace hera
}  // namespace wayz