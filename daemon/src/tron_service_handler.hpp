//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#pragma once

#include <common/data_def/device_types.hpp>
#include <common/rpc/gen-cpp/TronService.h>

#include "devices/device.hpp"
#include "devices/dummy/dummy.hpp"

namespace wayz {
namespace tron {

class TronServiceHandler final : public TronServiceIf {
public:
    TronServiceHandler();
    void create_devices(Result& _return,
                        const std::vector<DeviceInitializer>& device_initializers) final;
    void get_informations(std::vector<DeviceInformation>& _return) final;
    void set_storage(Result& _return, const std::string& folder) final;
    void adjust_device_parameters(Result& _return,
                                 const int32_t device_id,
                                 const std::map<std::string, std::string>& parameters) final;
    void control(Result& _return,
                 const ControlCommand::type command,
                 const bool to_all,
                 const int32_t device_id) final;

private:
    std::vector<std::unique_ptr<Device>> devices_;
    void reset();
};

}  // namespace tron
}  // namespace wayz