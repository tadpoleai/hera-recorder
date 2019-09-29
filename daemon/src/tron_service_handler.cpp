//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include "tron_service_handler.hpp"

#include <regex>

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
}
void TronServiceHandler::set_error_and_stop(Result& _return,
                                            TronErrno tron_errno,
                                            const std::string&& reason)
{
    _return.error = tron_errno;
    _return.reason = reason;
    clear();
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
        _return.status.can_record = _return.status.can_record && info.status == "Inited" && !is_record;
        _return.status.can_pause = _return.status.can_pause && info.status == "Inited" && is_record;
        _return.status.is_record = _return.status.is_record && is_record;

        _return.status.devices.emplace_back(info);
    }
}

void TronServiceHandler::get_status(Result& _return) {
    std::cout << "get_status" << std::endl;
    _return.error = TronErrno::Success;
    _return.reason = "OK";
    generate_status(_return);
}

void TronServiceHandler::start(Result& _return,
                               const std::vector<DeviceInitializer>& device_initializers,
                               const std::string& storage_folder)
{
    std::cout << "start" << std::endl;
    _return.error = TronErrno::Success;
    _return.reason = "OK";
    int32_t id = 0;

    // Check if devices_ is not empty
    if (devices_.size() != 0) {
        return set_error(_return, TronErrno::DevicesAlreadyCreated, "");
    }
    // Check if given devices list is empty
    if (device_initializers.size() == 0) {
        return set_error(_return, TronErrno::EmptyDeviceList, "Device list given is empty");
    }
    // Check if storage_folder is valid
    std::regex folder_regex("[a-zA-Z0-9_]{1,63}");
    if (!std::regex_match(storage_folder, folder_regex)) {
        return set_error(_return,
                         TronErrno::InvalidStorageFolderName,
                         "Storage folder " + storage_folder + " is invalid");
    }
    // Precheck device list for type and name
    std::regex name_regex("[a-zA-Z0-9_]{1,31}");
    for (const auto& device_initializer : device_initializers) {
        const auto& type_str = device_initializer.type;
        auto type = DeviceType::_from_string_nocase_nothrow(type_str.c_str());
        if (!type) {
            return set_error(_return,
                             TronErrno::InvalidDeviceType,
                             "Device type \"" + type_str + "\" is invalid");
        }
        const auto& name = device_initializer.name;
        if (!std::regex_match(name, name_regex)) {
            return set_error(_return,
                             TronErrno::InvalidDeviceName,
                             "Device type \"" + name + "\" is invalid");
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
        default:
            return set_error_and_stop(_return,
                                      TronErrno::InvalidDeviceType,
                                      "Device type \"" + type_str + "\" is invalid");
        }

        // Set parameters
        for (const auto& parameter : device_initializer.parameters) {
            TronErrno e = device->set_parameter(parameter.first, parameter.second);
            if (e != TronErrno::Success) {
                return set_error_and_stop(_return, device->get_errno(), device->get_reason());
            }
        }
        // Set folder
        TronErrno e = device->set_storage(storage_folder);
        if (e != TronErrno::Success) {
            return set_error_and_stop(_return,
                                      e,
                                      "Can not create storage folder for device \"" + name + "\"");
        }
        // Start device
        e = device->start();
        if (e != TronErrno::Success) {
            return set_error_and_stop(_return,
                                      e,
                                      "Device \"" + name + "\" occured " + device->get_reason());
        }
        devices_.emplace_back(device);
    }
    generate_status(_return);
}

void TronServiceHandler::stop(Result& _return)
{
    std::cout << "stop" << std::endl;
    if (devices_.size() != 0) {
        return set_error_and_stop(_return, TronErrno::Success, "OK");
    } else {
        return set_error(_return, TronErrno::EmptyDeviceList, "Already stopped");
    }
    generate_status(_return);
}

void TronServiceHandler::record_or_pause(Result& _return, const bool is_record)
{
    std::cout << "record_or_pause" << std::endl;
    _return.error = TronErrno::Success;
    _return.reason = "OK";

    if (devices_.size() == 0) {
        return set_error(_return, TronErrno::EmptyDeviceList, "Device list is empty");
    }

    for (const auto& device : devices_) {
        TronErrno e;
        if (is_record) {
            e = device->start_record();
            printf("start_record\n");
        } else {
            e = device->pause_record();
            printf("pause_record\n");
        }
        if (e != TronErrno::Success) {
            set_error(_return, e, "Device \"" + device->get_name() + "\" occured error");
        }
    }
    generate_status(_return);
}

void TronServiceHandler::adjust_device_parameters(
        Result& _return,
        const int32_t device_id,
        const std::map<std::string, std::string>& parameters)
{
    printf("adjust_device_parameters\n");
    _return.error = TronErrno::Success;
    _return.reason = "OK";

    if (devices_.size() == 0) {
        return set_error(_return, TronErrno::EmptyDeviceList, "Device list is empty");
    }

    try {
        const auto& device = devices_.at(device_id);
        for (const auto& parameter : parameters) {
            TronErrno e = device->set_parameter(parameter.first, parameter.second);
            if (e != TronErrno::Success) {
                std::string reason = "Device \"" + device->get_name() + "\" occured " +
                                     device->get_reason() + " when applying parameter " +
                                     parameter.first;
                return set_error(_return, device->get_errno(), std::move(reason));
            }
        }
    } catch (std::out_of_range& oor) {
        return set_error(_return, TronErrno::InvalidDeviceId,
                         "Device id given " + std::to_string(device_id) + " out of range");
    }
    generate_status(_return);
}

}  // namespace tron
}  // namespace wayz