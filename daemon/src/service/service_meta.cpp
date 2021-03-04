//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include "service.hpp"
//
#include "common/include/version.hpp"
#include "device/include/version.hpp"
#include "storage/include/version.hpp"

namespace wayz {
namespace hera {
namespace daemon {

void Service::generate_meta()
{
    log::info << "Daemon: Generating meta" << log::endl;

    const auto DeviceTypes = device::Factory::plugin_types();
    for (const auto& type : DeviceTypes) {
        DeviceRule device_rule;
        device_rule.name = type;
        device_rule.description = device::Factory::plugin_description(type);
        meta_.deviceRules.emplace_back(device_rule);
    }

    meta_.daemonVersion = "Common: " + common::get_version() + "\n" + "Device: " + device::get_version() + "\n" +
                          "Storage: " + storage::get_version();
}

void Service::getMeta(Meta& result)
{
    log::info << "Daemon: getMeta called" << log::endl;

    result = meta_;
}

}  // namespace daemon
}  // namespace hera
}  // namespace wayz