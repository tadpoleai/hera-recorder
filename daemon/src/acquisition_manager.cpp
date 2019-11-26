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

void AcquisitionManager::get(Result& result)
{
    append_status(result);
}
void AcquisitionManager::start(Result& result, const std::vector<DeviceInitializer>& devices, const std::string& folder)
{
    log::info << "Acquisition::start called" << log::endl;
    // Check if already created
    if (inited_) {
        return handle_error(result, HeraErrno::DevicesAlreadyCreated);
    }

    // Check if given devices list is empty
    if (devices.size() == 0) {
        return handle_error(result, HeraErrno::EmptyDeviceList);
    }

    // Check if storage_folder is valid
    std::regex folder_regex("[a-zA-Z0-9_]{1,64}");
    if (!std::regex_match(folder, folder_regex)) {
        return handle_error(result, HeraErrno::InvalidStorageFolderName, folder + " is invalid");
    }
    folder_ = folder;

    // Precheck device list for type and name
    std::regex name_regex("[a-zA-Z0-9_]{1,32}");
    for (const auto& device : devices) {
        // Check Type
        if (!DeviceFactory::check_type(device.type)) {
            return handle_error(result, HeraErrno::InvalidDeviceType, device.type + " is invalid");
        }
        // Check Name
        if (!std::regex_match(device.name, name_regex)) {
            return handle_error(result, HeraErrno::InvalidDeviceName, device.name + " is invalid");
        }
    }

    // Start calling factory function
    auto id = 0;
    std::string full_folder = FolderPrefix_ + folder;
    for (const auto& device : devices) {
        devices_.emplace_back(DeviceFactory::create(id++, device.type, device.name, full_folder));
        auto device_ptr = devices_.back().get();

        // Define parameters
        for (const auto& parameter : device.parameters) {
            HeraErrno e = device_ptr->parameter(parameter.first, parameter.second);
            if (e != HeraErrno::OK) {
                return handle_error(result, e, device_ptr->get_reason(), true);
            }
        }
    }

    // Async start
    bool failed = false;
    std::string reason = "";
    std::vector<std::pair<Device*, std::future<HeraErrno>>> promise_pairs;
    for (const auto& device : devices_) {
        auto promise = std::async(std::launch::async, &Device::start, device.get());
        promise_pairs.emplace_back(std::make_pair(device.get(), std::move(promise)));
    }
    for (auto& promise : promise_pairs) {  // Promise all
        if (promise.second.get() != HeraErrno::OK) {
            failed = true;
            reason += promise.first->get_type() + "/" + promise.first->get_name();
            reason += " errored " + promise.first->get_reason() + "; ";
        }
    }
    if (failed) {
        return handle_error(result, HeraErrno::CanNotConnectDevices, std::move(reason), true);
    } else {
        inited_ = true;
        return handle_success(result);
    }
}

void AcquisitionManager::stop(Result& result)
{
    log::info << "Acquisition::stop called" << log::endl;
    if (!inited_) {
        return handle_error(result, HeraErrno::DeviceAlreadyClosed);
    }

    reset();
    return handle_success(result);
}

void AcquisitionManager::record(Result& result, const bool on)
{
    log::info << "Acquisition::record called" << log::endl;
    if (!inited_) {
        return handle_error(result, HeraErrno::DeviceNotReady);
    }

    record_ = on;
    if (on) {
        log::info << "Acquisition::start recording" << log::endl;
    } else {
        log::info << "Acquisition::pause recording" << log::endl;
    }
    for (const auto& device : devices_) {
        device->record(on);
    }
    return handle_success(result);
}
void AcquisitionManager::adjust(Result& result, const int32_t id, const std::map<std::string, std::string>& parameters)
{
    log::info << "Acquisition::adjust called" << log::endl;
    if (!inited_) {
        return handle_error(result, HeraErrno::DeviceNotReady);
    }

    try {
        const auto& device = devices_.at(id);
        for (const auto& parameter : parameters) {
            HeraErrno e = device->parameter(parameter.first, parameter.second);
            if (e != HeraErrno::Success) {
                return handle_error(result, e, device->get_reason());
            }
        }
    } catch (std::out_of_range& oor) {
        return handle_error(result, HeraErrno::InvalidDeviceId);
    }

    return handle_success(result);
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

void AcquisitionManager::append_status(Result& result)
{
    result.status.inited = inited_;
    result.status.record = record_;
    result.status.folder = folder_;

    for (const auto& device : devices_) {
        DeviceInformation info;
        info.id = device->get_id();
        info.type = device->get_type();
        info.name = device->get_name();
        info.status = static_cast<int32_t>(device->get_status());
        info.forward = false;
        info.volume = device->get_volume() / 1024;
        info.error = device->get_errno();
        info.reason = device->get_reason();
        result.status.devices.emplace_back(info);
    }
}

}  // namespace daemon
}  // namespace hera
}  // namespace wayz