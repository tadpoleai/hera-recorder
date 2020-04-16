//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include "service.hpp"

#include <future>
#include <regex>
#include <thread>

namespace wayz {
namespace hera {
namespace daemon {

void Service::generate_meta()
{
    log::debug << "Daemon::generate meta called" << log::endl;
    const auto DeviceTypes = device::DeviceFactory::types();
    for (const auto& type_name : DeviceTypes) {
        auto parameter_types = device::DeviceFactory::parameter_types(type_name);
        DeviceTypeMeta type_meta;
        type_meta.name = type_name;
        type_meta.essentialParameterTypes = parameter_types.first;
        type_meta.optionalParameterTypes = parameter_types.second;
        meta_.deviceTypeMetas.emplace_back(type_meta);
    }
}

void Service::handle_error(Result& result, HeraErrno hera_errno, std::string&& reason, bool die)
{
    result.error = hera_errno;
    result.reason = std::forward<std::string>(reason);
    log::error << "Daemon::Error: " << hera_errno << result.reason << log::endl;
    if (die) {
        reset();
    }
    append_status(result);
}

void Service::handle_success(Result& result)
{
    result.error = HeraErrno::OK;
    result.reason = "OK";
    log::info << "Daemon::Succeed" << log::endl;
    append_status(result);
}

void Service::reset()
{
    std::vector<std::future<void>> promises;
    for (const auto& device : devices_) {
        promises.emplace_back(std::async(std::launch::async, &device::Device::stop, device.get()));
    }
    for (auto& promise : promises) {
        promise.get();
    }
    devices_.clear();
    storage_.reset();
    ipc_queue_.reset();
    if (use_slam_) {
#ifdef WITH_SLAM
        log::debug << "Daemon:: calling slam stop" << log::endl;
        if (system("hera-slam-caller-stop") != 0) {
            log::warn << "Daemon:: Something wrong with slam stop" << log::endl;
        } else {
            log::info << "Daemon:: called slam stop" << log::endl;
        }
#endif
    }
    started_ = false;
    recording_ = false;
}

}  // namespace daemon
}  // namespace hera
}  // namespace wayz