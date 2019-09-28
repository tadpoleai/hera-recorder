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
    reset();
}
void TronServiceHandler::create_devices(Result& _return,
                                        const std::vector<DeviceInitializer>& device_initializers)
{
    printf("create_devices\n");
    _return.error = TronErrno::Success;
    _return.reason = "OK";
    int32_t id = 0;

    if (devices_.size() != 0) {
        _return.error = TronErrno::DevicesAlreadyCreated;
        _return.reason = "Devices already Created";
        return;
    }

    if (device_initializers.size() == 0) {
        _return.error = TronErrno::EmptyDeviceList;
        _return.reason = "Devices is Empty";
        return;
    }

    std::regex name_regex("[a-zA-Z0-9_]{1,31}");
    for (const auto& device_initializer : device_initializers) {
        auto type_str = device_initializer.type;

        auto type = DeviceType::_from_string_nocase_nothrow(type_str.c_str());
        if (!type) {
            _return.error = TronErrno::InvalidDeviceType;
            _return.reason = "Device Type \"" + type_str + "\" is Invalid";
            reset();
            return;
        }
        auto name = device_initializer.name;
        if (!std::regex_match(name, name_regex)) {
            _return.error = TronErrno::InvalidDeviceName;
            _return.reason = "Device Name \"" + name + "\" is Invalid";
            reset();
            return;
        }
        Device* device;

        switch (type.value()) {
        case DeviceType::Dummy:
            device = new Dummy(id++, name);
            devices_.emplace_back(device);
            break;
        default:
            _return.error = TronErrno::InvalidDeviceType;
            _return.reason = "Device Type " + type_str + " is Invalid";
            reset();
            return;
        }

        for (const auto& parameter : device_initializer.parameters) {
            TronErrno e = devices_.back()->set_parameter(parameter.first, parameter.second);
            if (e != TronErrno::Success) {
                _return.error = devices_.back()->get_errno();
                _return.reason = devices_.back()->get_reason();
                reset();
                return;
            }
        }
    }
}

void TronServiceHandler::get_information(ResultInformation& _return)
{
    printf("get_informations\n");
    _return.error = TronErrno::Success;
    _return.reason = "OK";
    _return.can_create = true;

    if (devices_.size()) {
        _return.can_create = false;
        _return.is_error = false;
        _return.is_storage_set = false;

        _return.can_start = true;
        _return.can_stop = true;
        _return.can_record = true;
        _return.can_pause = true;
        _return.is_record = true;
        _return.can_set_storage = true;
    }

    for (const auto& device : devices_) {
        DeviceInformation info;
        info.id = device->get_id();
        info.type = device->get_type()._to_string();
        info.name = device->get_name();
        info.status = device->get_status();
        bool is_record = device->get_is_record();
        info.is_forward = device->get_is_forward();
        info.volume = device->get_volume();
        info.parameters = device->get_parameters();
        bool is_storage_set = device->get_is_storage_set();
        std::string storage_folder = device->get_storage_folder();
        info.error = device->get_errno();
        info.reason = device->get_reason();

        _return.is_error = _return.is_error || info.status == "Error";
        _return.is_storage_set = _return.is_storage_set || _return.is_storage_set;

        _return.can_start = _return.can_start && info.status == "Uninited";
        _return.can_stop = true;
        _return.can_record = _return.can_record && info.status == "Inited" && !is_record;
        _return.can_pause = _return.can_pause && info.status == "Inited" && is_record;
        _return.is_record = _return.is_record && is_record;
        _return.can_set_storage = _return.can_set_storage && !is_storage_set;
        _return.storage_folder = std::move(storage_folder);
        
        _return.devices.emplace_back(info);
    }
}

void TronServiceHandler::set_storage(Result& _return, const std::string& folder)
{
    printf("set_storage\n");
    _return.error = TronErrno::Success;
    _return.reason = "OK";

    if (devices_.size() == 0) {
        _return.error = TronErrno::EmptyDeviceList;
        _return.reason = "Device List is Empty";
        return;
    }

    for (const auto& device : devices_) {
        TronErrno e = device->set_storage(folder);
        if (e != TronErrno::Success) {
            _return.error = device->get_errno();
            _return.reason = device->get_reason();
        }
    }
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
        _return.error = TronErrno::EmptyDeviceList;
        _return.reason = "Device List is Empty";
        return;
    }

    try {
        const auto& device = devices_.at(device_id);
        for (const auto& parameter : parameters) {
            TronErrno e = device->set_parameter(parameter.first, parameter.second);
            if (e != TronErrno::Success) {
                _return.error = device->get_errno();
                _return.reason = device->get_reason();
            }
        }
    } catch (std::out_of_range& oor) {
        _return.error = TronErrno::InvalidDeviceId;
        _return.reason = "Device Id " + std::to_string(device_id) + " Out of Range";
        return;
    }
    return;
}

void TronServiceHandler::control(Result& _return, const ControlCommand::type command)
{
    printf("control\n");
    _return.error = TronErrno::Success;
    _return.reason = "OK";

    if (devices_.size() == 0) {
        _return.error = TronErrno::EmptyDeviceList;
        _return.reason = "Device List is Empty";
        return;
    }

    // All Devices
    for (const auto& device : devices_) {
        TronErrno e;
        switch (command) {
        case ControlCommand::Start:
            e = device->start();
            break;
        case ControlCommand::Stop:
            e = device->stop();
            break;
        case ControlCommand::StartRecord:
            e = device->start_record();
            break;
        case ControlCommand::PauseRecord:
            e = device->pause_record();
            break;
        case ControlCommand::EnableForward:
            e = device->enable_forward();
            break;
        case ControlCommand::DisableForward:
            e = device->disable_forward();
            break;
        case ControlCommand::Reset:
            e = TronErrno::Success;
            reset();
            break;
        default:
            e = TronErrno::InvalidControlCommand;
            break;
        }
        if (e != TronErrno::Success) {
            _return.error = e;
            if (e != TronErrno::InvalidControlCommand) {
                _return.reason = device->get_reason();
            }
        }
    }
}

void TronServiceHandler::reset()
{
    devices_.clear();
}

}  // namespace tron
}  // namespace wayz