//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include <cstdlib>
#include <cstring>
#include <ctime>
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

namespace {

// Devices without a reliable RTC (e.g. a Jetson used offline as a WiFi hotspot with no NTP
// source) can boot with a bogus clock and never correct it, since NTP has nothing to reach.
// A client's own wall clock (phone/laptop connecting to the hotspot) is a much better source
// in that situation, so start() accepts one and self-corrects if the drift is large enough
// to matter for filenames/timestamps, but small enough drift (normal NTP jitter, RPC latency)
// is left alone to avoid needlessly stepping the clock on every start().
constexpr int64_t kClockCorrectionThresholdMs = 5000;

void maybe_correct_system_clock(const double client_epoch_ms_raw)
{
    if (client_epoch_ms_raw <= 0.0) {
        return;  // Unset (old client, or client didn't supply one) -- don't touch the clock.
    }
    const int64_t client_epoch_ms = static_cast<int64_t>(client_epoch_ms_raw);

    struct timespec now{};
    clock_gettime(CLOCK_REALTIME, &now);
    const int64_t system_epoch_ms = static_cast<int64_t>(now.tv_sec) * 1000 + now.tv_nsec / 1000000;
    const int64_t drift_ms = client_epoch_ms - system_epoch_ms;

    if (drift_ms < kClockCorrectionThresholdMs && drift_ms > -kClockCorrectionThresholdMs) {
        return;
    }

    struct timespec corrected{};
    corrected.tv_sec = static_cast<time_t>(client_epoch_ms / 1000);
    corrected.tv_nsec = static_cast<long>((client_epoch_ms % 1000) * 1000000);
    if (clock_settime(CLOCK_REALTIME, &corrected) != 0) {
        log::warn << "Daemon: failed to correct system clock (drift=" << drift_ms
                  << "ms): " << std::strerror(errno) << log::endl;
        return;
    }

    log::info << "Daemon: corrected system clock by " << drift_ms
              << "ms using client-supplied time (no reliable RTC/NTP source)" << log::endl;

    // Best-effort: also push the corrected time to the hardware RTC so a reboot without
    // network access starts closer to correct. Not fatal if there's no writable RTC --
    // the system clock correction above is what actually matters for this session.
    if (std::system("hwclock --systohc 2>/dev/null") != 0) {
        log::warn << "Daemon: hwclock --systohc failed (system clock is still corrected)" << log::endl;
    }
}

}  // namespace

void Service::getStatus(AcquisitionStatus& result)
{
    std::unique_lock<std::mutex> _(mutex_);

    append_acquisition_status(result);
}

void Service::start(AcquisitionStatus& result, const double clientEpochMs)
{
    log::info << "Daemon::start called" << log::endl;

    maybe_correct_system_clock(clientEpochMs);

    std::unique_lock<std::mutex> _(mutex_);
    // Check if already started
    if (started_) {
        return handle_error(result, HeraErrno::DevicesAlreadyCreated, "Daemon is already started");
    }

    detail_device_index_ = -1;

    // Check given profilesIndex
    if (acquisition_setting_.profileIndex < 0 ||
        acquisition_setting_.profileIndex >= (int32_t)acquisition_setting_.profiles.size()) {
        return handle_error(result, HeraErrno::ErrorReadProfiles, "ProfileIndex out of range");
    }
    const auto& profile = acquisition_setting_.profiles[acquisition_setting_.profileIndex];

    // Check if device list if empty
    if (profile.devices.size() == 0) {
        return handle_error(result, HeraErrno::EmptyDeviceList, "Devices in given profile is empty");
    }

    // Check if filename is valid
    std::regex name_regex("[a-zA-Z0-9_]{1,64}");
    if (!std::regex_match(acquisition_setting_.operatorInfo.operatorName, name_regex)) {
        return handle_error(result,
                            HeraErrno::InvalidStorageFileName,
                            "OperatorName given '" + acquisition_setting_.operatorInfo.operatorName +
                                    "' is not safe, use [a-zA-Z0-9_]");
    }
    if (!std::regex_match(acquisition_setting_.operatorInfo.place, name_regex)) {
        return handle_error(result,
                            HeraErrno::InvalidStorageFileName,
                            "Place given '" + acquisition_setting_.operatorInfo.place +
                                    "' is not safe, use [a-zA-Z0-9_]");
    }

    auto now = time::Timestamp::now();
    start_time_sec_ = now.tv_sec;
    auto storage_filename = now.to_datetime() + "_" + acquisition_setting_.operatorInfo.operatorName + "_" +
                            acquisition_setting_.operatorInfo.place;

    // Precheck device list for type and name
    std::regex device_name_regex("[a-zA-Z0-9_]{1,32}");
    for (const auto& device : profile.devices) {
        // Check device type
        if (!device::Factory::check_type(device.type)) {
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
    std::string full_storage_name = DataDirectory_ + "/" + storage_filename + FileNameSuffix_;
    storage_ = storage::StorageManager::open(full_storage_name, false);
    for (const auto& device : profile.devices) {
        devices_.emplace_back(device::Factory::create(
                device_id++, device.type, device.name, device.forward, ipc_queue_.get(), storage_.get()));
        auto device_ptr = devices_.back().get();

        // Define parameters
        for (const auto& parameter : device.parameters) {
            HeraErrno e = device_ptr->parameter(parameter.type, parameter.value);
            if (e != HeraErrno::OK) {
                return handle_error(result, e, device_ptr->get_reason(), true);
            }
        }
    }
    storage_->finish_add_device();

    // Write Extra Info
    storage_->header->extra_info["profile"] = profile;
    storage_->header->extra_info["operator"] = acquisition_setting_.operatorInfo.operatorName;
    storage_->header->extra_info["place"] = acquisition_setting_.operatorInfo.place;
    storage_->header->extra_info["slam"] = acquisition_setting_.operatorInfo.slam;

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

    // Frequency Calculate
    frequecy_calculator_ = std::make_unique<FrequecyCalculator>(&devices_);

    storage_filename_ = storage_filename;

    if (failed) {
        return handle_error(result, HeraErrno::CanNotConnectDevices, std::move(reason), true);
    } else {
        if (acquisition_setting_.operatorInfo.slam) {
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

void Service::stop(AcquisitionStatus& result)
{
    log::info << "Daemon::stop called" << log::endl;

    std::unique_lock<std::mutex> _(mutex_);
    if (!started_) {
        return handle_error(result, HeraErrno::DeviceAlreadyClosed, "Daemon is already stopped");
    }

    log::debug << "Daemon::calling device reset" << log::endl;
    reset();

    if (acquisition_setting_.operatorInfo.slam) {
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

void Service::setRecord(AcquisitionStatus& result, const bool on)
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

void Service::reset()
{
    std::vector<std::future<void>> promises;
    for (const auto& device : devices_) {
        promises.emplace_back(std::async(std::launch::async, &device::Device::stop, device.get()));
    }
    for (auto& promise : promises) {
        promise.get();
    }
    detail_device_index_ = -1;
    frequecy_calculator_.reset();
    devices_.clear();
    storage_.reset();
    ipc_queue_.reset();
    if (acquisition_setting_.operatorInfo.slam) {
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


void Service::handle_error(AcquisitionStatus& result, HeraErrno hera_errno, std::string&& reason, bool die)
{
    result.error = hera_errno;
    result.reason = std::forward<std::string>(reason);
    log::error << "Daemon::Error: " << hera_errno << result.reason << log::endl;
    if (die) {
        reset();
    }
    append_acquisition_status(result);
}

void Service::handle_success(AcquisitionStatus& result)
{
    result.error = HeraErrno::OK;
    result.reason = "OK";
    log::info << "Daemon::Succeed" << log::endl;
    append_acquisition_status(result);
}

void Service::append_acquisition_status(AcquisitionStatus& result)
{
    result.started = started_;
    result.recording = recording_;
    result.storageFileName = storage_filename_;
}

}  // namespace daemon
}  // namespace hera
}  // namespace wayz