//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include <future>
#include <regex>
#include <thread>

#include "device/include/display_data.hpp"
#include "service.hpp"

namespace wayz {
namespace hera {
namespace daemon {

void Service::append_device_parameterses(std::vector<DeviceAndParameters>& result)
{
    std::vector<std::future<DeviceAndParameters>> promises;
    // clang-format off
    for (auto& device : devices_) {
        promises.emplace_back(std::async(std::launch::async, [&](auto* device) {
            DeviceAndParameters data;
            data.id = device->get_id();
            data.type = device->get_vendor_type();
            data.name = device->get_name();
            data.parametersJson = device->get_parameters_json().dump();
            return data;
        },
        device.get()));
    }
    // clang-format on
    for (auto& promise : promises) {
        result.emplace_back(promise.get());
    }
}

void Service::getDeviceAndParameterses(std::vector<DeviceAndParameters>& result)
{
    std::unique_lock<std::mutex> _(mutex_);

    append_device_parameterses(result);
}

void Service::adjustDeviceParameter(std::vector<DeviceAndParameters>& result,
                                    const int32_t deviceIndex,
                                    const std::string& type,
                                    const std::string& value)
{
    std::unique_lock<std::mutex> _(mutex_);

    if (!started_) {
        log::error << "Daemon: can not execute adjustDeviceParameter when devices stopped" << log::endl;
    } else {
        if (deviceIndex < 0 || deviceIndex >= (int32_t)devices_.size()) {
            log::warn << "Daemon: can not execute adjustDeviceParameter since deviceIndex OOR" << log::endl;
        } else {
            devices_[deviceIndex]->parameter(type, value);
        }
    }

    append_device_parameterses(result);
}

}  // namespace daemon
}  // namespace hera
}  // namespace wayz
