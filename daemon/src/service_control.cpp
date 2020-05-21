//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include <future>
#include <regex>
#include <thread>

#include "service.hpp"

#if WITH_SLAM
#include "slam/caller/include/caller.hpp"
#endif

namespace wayz {
namespace hera {
namespace daemon {

void Service::start(Result& result, const OperatorInfo& operator_info)
{
    log::info << "Daemon::start called with op = " << operator_info.operatorName << ", place = " << operator_info.place
              << log::endl;

    std::unique_lock<std::mutex> _(mutex_);
    // Check if already started
    if (started_) {
        return handle_error(result, HeraErrno::DevicesAlreadyCreated, "Daemon is already started");
    }

    // Check given profilesIndex
    if (profile_index_ < 0 || profile_index_ >= (int32_t)profiles_.size()) {
        return handle_error(result, HeraErrno::ErrorReadProfiles, "ProfileIndex out of range");
    }
    const auto& profile = profiles_[profile_index_];

    // Check if device list if empty
    if (profile.devices.size() == 0) {
        return handle_error(result, HeraErrno::EmptyDeviceList, "Devices in given profile is empty");
    }

    // Check if filename is valid
    std::regex name_regex("[a-zA-Z0-9_]{1,64}");
    if (!std::regex_match(operator_info.operatorName, name_regex)) {
        return handle_error(result,
                            HeraErrno::InvalidStorageFileName,
                            "OperatorName given '" + operator_info.operatorName + "' is not safe, use [a-zA-Z0-9_]");
    }
    if (!std::regex_match(operator_info.place, name_regex)) {
        return handle_error(result,
                            HeraErrno::InvalidStorageFileName,
                            "Place given '" + operator_info.place + "' is not safe, use [a-zA-Z0-9_]");
    }

    auto now = time::Timestamp::now();
    start_time_sec_ = now.tv_sec;
    auto storage_name = now.to_datetime() + "_" + operator_info.operatorName + "_" + operator_info.place;

    // Precheck device list for type and name
    std::regex device_name_regex("[a-zA-Z0-9_]{1,32}");
    for (const auto& device : profile.devices) {
        // Check device type
        if (!device::DeviceFactory::check_type(device.type)) {
            return handle_error(result,
                                HeraErrno::InvalidDeviceType,
                                "Device type given '" + device.type + "' is invalid");
        }
        // Check device name
        if (!std::regex_match(device.name, device_name_regex)) {
            return handle_error(result,
                                HeraErrno::InvalidDeviceName,
                                "Device name give '" + device.name + "' is invalid");
        }
    }


    // Open IPC
    ipc_queue_ = ipc::IPCQueue<device::data::SensorData>::create();
    ipc_queue_->open(0, ipc::OpenMode::Write, true);

    // Start calling factory function
    auto device_id = 0;
    std::string full_storage_name = StorageFolder_ + "/" + storage_name + FileNameSuffix_;
    storage_ = storage::StorageManager::open(full_storage_name, false);
    for (const auto& device : profile.devices) {
        devices_.emplace_back(device::DeviceFactory::create(
                device_id++, device.type, device.name, device.forward, ipc_queue_.get(), storage_.get()));
        auto device_ptr = devices_.back().get();

        // Define parameters
        for (const auto& parameter : device.essentialParameters) {
            HeraErrno e = device_ptr->parameter(parameter.type, parameter.value);
            if (e != HeraErrno::OK) {
                return handle_error(result, e, device_ptr->get_reason(), true);
            }
        }

        // Define parameters
        for (const auto& parameter : device.optionalParameters) {
            HeraErrno e = device_ptr->parameter(parameter.type, parameter.value);
            if (e != HeraErrno::OK) {
                return handle_error(result, e, device_ptr->get_reason(), true);
            }
        }
    }
    storage_->finish_add_device();

    // Write Extra Info
    storage_->header->extra_info["profile"] = profiles_json_[profile_index_];
    storage_->header->extra_info["operator"] = operator_info.operatorName;
    storage_->header->extra_info["place"] = operator_info.place;
    storage_->header->extra_info["slam"] = operator_info.slam;

    // Async start
    bool failed = false;
    std::string reason = "";
    std::vector<std::pair<device::Device*, std::future<HeraErrno>>> promise_pairs;
    for (const auto& device : devices_) {
        auto promise = std::async(std::launch::async, &device::Device::start, device.get());
        promise_pairs.emplace_back(std::make_pair(device.get(), std::move(promise)));
    }
    for (auto& promise : promise_pairs) {  // Promise all
        if (promise.second.get() != HeraErrno::OK) {
            failed = true;
            reason += "Device " + promise.first->get_vendor_type() + "/" + promise.first->get_name();
            reason += " errored " + promise.first->get_reason() + "; ";
        }
    }

    operator_info_ = operator_info;
    operator_info_.storagePath = storage_name;

    if (failed) {
        return handle_error(result, HeraErrno::CanNotConnectDevices, std::move(reason), true);
    } else {
        if (operator_info_.slam) {
#ifdef WITH_SLAM
            if (system("hera-slam-caller-start") != 0) {
                log::warn << "Daemon:: Something wrong with slam start" << log::endl;
            } else {
                log::info << "Daemon:: called slam start" << log::endl;
            }
#endif
        }
        started_ = true;
        return handle_success(result);
    }
}

void Service::stop(Result& result)
{
    log::info << "Daemon::stop called" << log::endl;

    std::unique_lock<std::mutex> _(mutex_);
    if (!started_) {
        return handle_error(result, HeraErrno::DeviceAlreadyClosed, "Daemon is already stopped");
    }

    log::debug << "Daemon::calling device reset" << log::endl;
    reset();

    if (operator_info_.slam) {
#ifdef WITH_SLAM
        log::debug << "Daemon:: calling slam stop" << log::endl;
        if (system("hera-slam-caller-stop") != 0) {
            log::warn << "Daemon:: Something wrong with slam stop" << log::endl;
        } else {
            log::info << "Daemon:: called slam stop" << log::endl;
        }
#endif
    }
    return handle_success(result);
}

void Service::record(Result& result, const bool on)
{
    log::info << "Daemon::record called" << log::endl;

    std::unique_lock<std::mutex> _(mutex_);
    if (!started_) {
        return handle_error(result, HeraErrno::DeviceNotReady, "Daemon is not started yet");
    }

    recording_ = on;
    if (on) {
        log::info << "Daemon::start recording" << log::endl;
    } else {
        log::info << "Daemon::pause recording" << log::endl;
    }
    for (const auto& device : devices_) {
        device->record(on);
    }
    return handle_success(result);
}

void Service::adjustParameters(Result& result, const int32_t id, const std::vector<Parameter>& parameters)
{
    log::info << "Daemon::adjustParameters called" << log::endl;

    std::unique_lock<std::mutex> _(mutex_);
    if (!started_) {
        return handle_error(result, HeraErrno::DeviceNotReady, "Daemon is not started yet");
    }

    try {
        const auto& device = devices_.at(id);
        for (const auto& parameter : parameters) {
            HeraErrno e = device->parameter(parameter.type, parameter.value);
            if (e != HeraErrno::Success) {
                return handle_error(result, e, "Device errored + " + device->get_reason());
            }
        }
    } catch (std::out_of_range& oor) {
        return handle_error(result, HeraErrno::InvalidDeviceId, "Device id out of range");
    }

    return handle_success(result);
}

}  // namespace daemon
}  // namespace hera
}  // namespace wayz