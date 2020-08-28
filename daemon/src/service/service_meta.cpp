//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include "service.hpp"

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
        device_rule.parameterRulesJson = device::Factory::plugin_parameter_rules(type);
        meta_.deviceRules.emplace_back(device_rule);
    }
}

void Service::getMeta(Meta& result)
{
    log::info << "Daemon: getMeta called" << log::endl;

    result = meta_;
}

}  // namespace daemon
}  // namespace hera
}  // namespace wayz