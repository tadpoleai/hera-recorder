//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include "tron_service_handler.hpp"

#include <regex>

#include <common/data_def/device_types.hpp>
#include <common/logger/logger.hpp>
#include <common/tron_errno.h>

namespace wayz {
namespace tron {

TronServiceHandler::TronServiceHandler() {}
TronServiceHandler::~TronServiceHandler()
{
    clear();
}
void TronServiceHandler::set_error(Result& _return,
                                   TronErrno tron_errno,
                                   const std::string&& reason)
{
    _return.error = tron_errno;
    _return.reason = reason;
    generate_status(_return);
    Logger::error() << "TronService::SetError" << Logger::endl;
}
void TronServiceHandler::set_error_and_stop(Result& _return,
                                            TronErrno tron_errno,
                                            const std::string&& reason)
{
    _return.error = tron_errno;
    _return.reason = reason;
    clear();
    generate_status(_return);
    Logger::error() << "TronService::SetErrorAndStop" << Logger::endl;
}
void TronServiceHandler::clear()
{
    devices_.clear();
}
void TronServiceHandler::generate_status(Result& _return)
{
    _return.status.can_start = true;

    if (devices_.size()) {
        _return.status.can_start = false;
        _return.status.is_error = false;
        _return.status.can_stop = true;

        _return.status.can_record = true;
        _return.status.can_pause = true;
        _return.status.is_record = true;
        _return.status.storage_folder = devices_[0]->get_storage_folder();
    }

    for (const auto& device : devices_) {
        DeviceInformation info;
        info.id = device->get_id();
        info.type = device->get_type()._to_string();
        info.name = device->get_name();
        info.status = device->get_status();
        info.is_forward = device->get_is_forward();
        info.volume = device->get_volume();
        info.parameters = device->get_parameters();
        info.error = device->get_errno();
        info.reason = device->get_reason();
        bool is_record = device->get_is_record();

        _return.status.is_error = _return.status.is_error || info.status == "Error";
        _return.status.can_record =
                _return.status.can_record && info.status == "Inited" && !is_record;
        _return.status.can_pause = _return.status.can_pause && info.status == "Inited" && is_record;
        _return.status.is_record = _return.status.is_record && is_record;

        _return.status.devices.emplace_back(info);
    }
}

void TronServiceHandler::get_status(Result& _return)
{
    _return.error = TronErrno::Success;
    _return.reason = "OK";
    generate_status(_return);
}

void TronServiceHandler::start(Result& _return,
                               const std::vector<DeviceInitializer>& device_initializers,
                               const std::string& storage_folder)
{
    Logger::info() << "TronService::Start Called" << Logger::endl;
    _return.error = TronErrno::Success;
    _return.reason = "OK";
    int32_t id = 0;

    // Check if devices_ is not empty
    if (devices_.size() != 0) {
        return set_error(_return, TronErrno::DevicesAlreadyCreated, "");
    }
    // Check if given devices list is empty
    if (device_initializers.size() == 0) {
        Logger::error() << "TronService::Start EmptyDeviceList" << Logger::endl;
        return set_error(_return, TronErrno::EmptyDeviceList, "Device list given is empty");
    }
    // Check if storage_folder is valid
    std::regex folder_regex("[a-zA-Z0-9_]{1,64}");
    if (!std::regex_match(storage_folder, folder_regex)) {
        Logger::error() << "TronService::Start InvalidStorageFolderName: " << storage_folder
                        << Logger::endl;
        return set_error(_return,
                         TronErrno::InvalidStorageFolderName,
                         "Storage folder " + storage_folder + " is invalid");
    }
    // Precheck device list for type and name
    std::regex name_regex("[a-zA-Z0-9_]{1,32}");
    for (const auto& device_initializer : device_initializers) {
        const auto& type_str = device_initializer.type;
        auto type = DeviceType::_from_string_nocase_nothrow(type_str.c_str());
        if (!type) {
            Logger::error() << "TronService::Start InvalidDeviceType: " << type_str << Logger::endl;
            return set_error(_return,
                             TronErrno::InvalidDeviceType,
                             "Device type \"" + type_str + "\" is invalid");
        }
        const auto& name = device_initializer.name;
        if (!std::regex_match(name, name_regex)) {
            Logger::error() << "TronService::Start InvalidDeviceName: " << name << Logger::endl;
            return set_error(_return,
                             TronErrno::InvalidDeviceName,
                             "Device name \"" + name + "\" is invalid");
        }
    }

    // Do construction
    for (const auto& device_initializer : device_initializers) {
        const auto& type_str = device_initializer.type;
        auto type = DeviceType::_from_string_nocase_nothrow(type_str.c_str());
        const auto& name = device_initializer.name;

        Device* device;
        switch (type.value()) {
        case DeviceType::Dummy:
            device = new Dummy(id++, name);
            break;
        case DeviceType::Lidar:
            device = new Lidar(id++, name);
            break;
        case DeviceType::Imu:
            device = new Imu(id++, name);
            break;
        case DeviceType::Camera:
            device = new Camera(id++, name);
            break;
        default:
            Logger::error() << "TronService::Start UnimplementedDeviceType: " << type_str
                            << Logger::endl;
            return set_error_and_stop(_return,
                                      TronErrno::InvalidDeviceType,
                                      "Device type \"" + type_str + "\" is invalid");
        }

        devices_.emplace_back(device);
        device = devices_.back().get();

        // Set parameters
        for (const auto& parameter : device_initializer.parameters) {
            TronErrno e = device->set_parameter(parameter.first, parameter.second);
            if (e != TronErrno::Success) {
                Logger::error() << "TronService::Start Set parameters: " << parameter.first << ", "
                                << parameter.second << ", " << device->get_reason() << Logger::endl;
                return set_error_and_stop(_return, device->get_errno(), device->get_reason());
            }
        }
        // Set folder
        TronErrno e = device->set_storage(storage_folder);
        if (e != TronErrno::Success) {
            Logger::error() << "TronService::Start Set storage: " << name << ", "
                            << device->get_reason() << Logger::endl;
            return set_error_and_stop(_return,
                                      e,
                                      "Can not create storage folder for device \"" + name + "\"");
        }
        // Start device
        e = device->start();
        if (e != TronErrno::Success) {
            Logger::error() << "TronService::Start Start device: " << name << ", "
                            << device->get_reason() << Logger::endl;
            return set_error_and_stop(_return,
                                      e,
                                      "Device \"" + name + "\" occured " + device->get_reason());
        }
    }
    generate_status(_return);
    Logger::open_extra(storage_folder + "/record.log");
    Logger::info() << "TronService::Start Succeeded" << Logger::endl;
}

void TronServiceHandler::stop(Result& _return)
{
    Logger::info() << "TronService::Stop Called" << Logger::endl;
    _return.error = TronErrno::Success;
    _return.reason = "OK";
    if (devices_.size() == 0) {
        Logger::error() << "TronService::Stop Already stopped" << Logger::endl;
        return set_error(_return, TronErrno::EmptyDeviceList, "Already stopped");
    }
    clear();
    generate_status(_return);
    Logger::info() << "TronService::Stop Succeeded" << Logger::endl;
    Logger::close_extra();
}

void TronServiceHandler::record_or_pause(Result& _return, const bool is_record)
{
    Logger::info() << "TronService::Record/Pause Called" << Logger::endl;
    _return.error = TronErrno::Success;
    _return.reason = "OK";

    if (devices_.size() == 0) {
        Logger::error() << "TronService::Record/Pause EmptyDeviceList" << Logger::endl;
        return set_error(_return, TronErrno::EmptyDeviceList, "Device list is empty");
    }

    for (const auto& device : devices_) {
        TronErrno e;
        if (is_record) {
            e = device->start_record();
            Logger::info() << "TronService::Record/Pause StartRecord" << Logger::endl;
        } else {
            e = device->pause_record();
            Logger::info() << "TronService::Record/Pause PauseRecord" << Logger::endl;
        }
        if (e != TronErrno::Success) {
            Logger::error() << "TronService::Record/Pause Device: " << device->get_name()
                            << Logger::endl;
            set_error(_return, e, "Device \"" + device->get_name() + "\" occured error");
        }
    }
    generate_status(_return);
    Logger::info() << "TronService::Record/Pause Succeeded" << Logger::endl;
}

void TronServiceHandler::adjust_device_parameters(
        Result& _return,
        const int32_t device_id,
        const std::map<std::string, std::string>& parameters)
{
    Logger::info() << "TronService::Adjust" << Logger::endl;
    _return.error = TronErrno::Success;
    _return.reason = "OK";

    if (devices_.size() == 0) {
        Logger::error() << "TronService::Adjust EmptyDeviceList" << Logger::endl;
        return set_error(_return, TronErrno::EmptyDeviceList, "Device list is empty");
    }

    try {
        const auto& device = devices_.at(device_id);
        for (const auto& parameter : parameters) {
            TronErrno e = device->set_parameter(parameter.first, parameter.second);
            if (e != TronErrno::Success) {
                Logger::error() << "TronService::Adjust Device: " << device->get_name() << ", "
                                << parameter.first << ", " << parameter.second << ","
                                << device->get_reason() << Logger::endl;
                std::string reason = "Device \"" + device->get_name() + "\" occured " +
                                     device->get_reason() + " when applying parameter " +
                                     parameter.first;
                return set_error(_return, device->get_errno(), std::move(reason));
            }
        }
    } catch (std::out_of_range& oor) {
        Logger::error() << "TronService::Adjust DeviceId: " << std::to_string(device_id)
                        << " Out of Range" << Logger::endl;
        return set_error(_return,
                         TronErrno::InvalidDeviceId,
                         "Device id given " + std::to_string(device_id) + " out of range");
    }
    generate_status(_return);
    Logger::info() << "TronService::Adjust Succeed" << Logger::endl;
}

}  // namespace tron
}  // namespace wayz