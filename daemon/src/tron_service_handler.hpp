//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#pragma once

#include <common/rpc/gen-cpp/TronService.h>

#include "devices/device.hpp"

namespace wayz {
namespace tron {

class TronServiceHandler final : public TronServiceIf {
public:
    TronServiceHandler();
    virtual ~TronServiceHandler();
    void get_status(Result& _return) final;
    void start(Result& _return,
               const std::vector<DeviceInitializer>& device_initializers,
               const std::string& storage_folder) final;
    void stop(Result& _return) final;
    void record_or_pause(Result& _return, const bool is_record) final;
    void adjust_device_parameters(Result& _return,
                                  const int32_t device_id,
                                  const std::map<std::string, std::string>& parameters) final;
    void clear();

private:
    void set_error(Result& _return, TronErrno tron_errno, const std::string&& reason);
    void set_error_and_stop(Result& _return, TronErrno tron_errno, const std::string&& reason);
    void generate_status(Result& _return);

    std::vector<std::unique_ptr<Device>> devices_;
};

}  // namespace tron
}  // namespace wayz