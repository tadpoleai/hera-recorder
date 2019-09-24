//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include "tron_service_handler.hpp"

#include <regex>

#include <common/tron_errno.h>

namespace wayz {
namespace tron {

TronServiceHandler::TronServiceHandler() {}
TronServiceHandler::~TronServiceHandler() {
    LOG_LINE
    reset();
}
void TronServiceHandler::create_devices(Result& _return,
                                        const std::vector<DeviceInitializer>& device_initializers)
{
    printf("create_devices\n");
    _return.error = TronErrno::Success;
    _return.reason = "OK";
    int32_t id = 0;
    LOG_LINE

    if (devices_.size() != 0) {
        _return.error = TronErrno::DevicesAlreadyCreated;
        _return.reason = "Devices already Created";
        return;
    }
    LOG_LINE

    if (device_initializers.size() == 0) {
        _return.error = TronErrno::EmptyDeviceList;
        _return.reason = "Devices is Empty";
        return;
    }
    LOG_LINE

    std::regex name_regex("[a-zA-Z0-9_]{1,31}");
    LOG_LINE
    for (const auto& device_initializer : device_initializers) {
        LOG_LINE
        auto type_str = device_initializer.type;

        auto type = DeviceType::_from_string_nocase_nothrow(type_str.c_str());
        if (!type) {
            _return.error = TronErrno::InvalidDeviceType;
            _return.reason = "Device Type \"" + type_str + "\" is Invalid";
            reset();
            return;
        }
        LOG_LINE
        auto name = device_initializer.name;
        if (!std::regex_match(name, name_regex)) {
            _return.error = TronErrno::InvalidDeviceName;
            _return.reason = "Device Name \"" + name + "\" is Invalid";
            reset();
            return;
        }
        LOG_LINE
        Device* device;

        switch (type.value()) {
        case DeviceType::Dummy:
            LOG_LINE
            device = new Dummy(id++, name);
            LOG_LINE
            devices_.emplace_back(device);
            LOG_LINE
            break;
        default:
            _return.error = TronErrno::InvalidDeviceType;
            _return.reason = "Device Type " + type_str + " is Invalid";
            reset();
            LOG_LINE
            return;
        }
        LOG_LINE
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

void TronServiceHandler::get_informations(std::vector<DeviceInformation>& _return)
{
    printf("get_informations\n");
    for (const auto& device : devices_) {
        DeviceInformation device_information;
        device_information.id = device->get_id();
        device_information.type = device->get_type()._to_string();
        device_information.name = device->get_name();
        device_information.status = device->get_status();
        device_information.is_record = device->get_is_record();
        device_information.is_forward = device->get_is_forward();
        device_information.volume = device->get_volume();
        device_information.parameters = device->get_parameters();
        device_information.error = device->get_errno();
        device_information.reason = device->get_reason();
        _return.emplace_back(device_information);
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

void TronServiceHandler::control(Result& _return,
                                 const ControlCommand::type command,
                                 const bool to_all,
                                 const int32_t device_id)
{
    printf("control\n");
    _return.error = TronErrno::Success;
    _return.reason = "OK";

    if (devices_.size() == 0) {
        _return.error = TronErrno::EmptyDeviceList;
        _return.reason = "Device List is Empty";
        return;
    }

    // Single Device
    if (!to_all) {
        try {
            const auto& device = devices_.at(device_id);
            TronErrno e;
            LOG_LINE

            switch (command) {
            case ControlCommand::Start:
                e = device->start();
                LOG_LINE
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
            default:
                e = TronErrno::InvalidControlCommand;
                break;
            }
            if (e != TronErrno::Success) {
                LOG_LINE
                _return.error = e;
                LOG_LINE
                if (e != TronErrno::InvalidControlCommand) {
                    LOG_LINE
                    _return.reason = device->get_reason();
                }
            }
        } catch (std::out_of_range& oor) {
            _return.error = TronErrno::InvalidDeviceId;
            _return.reason = "Device Id " + std::to_string(device_id) + " Out of Range";
        }
    } else {
        // All Devices
        LOG_LINE
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
    }  // else of if(!to_all)
}

void TronServiceHandler::reset()
{
    LOG_LINE
    devices_.clear();
    LOG_LINE
}

}  // namespace tron
}  // namespace wayz