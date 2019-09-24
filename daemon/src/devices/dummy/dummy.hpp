//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#pragma once
#include "../device.hpp"

namespace wayz {
namespace tron {

class Dummy final : public Device {
public:
    Dummy(int32_t id, const std::string& name);
    Dummy(const Dummy&) = delete;
    Dummy& operator=(const Dummy&) = delete;
    virtual ~Dummy();

    DeviceType get_type() const final;

    TronErrno connect() final;
    void disconnect() final;
    std::shared_ptr<DeviceRawData> fetch() final;
    std::shared_ptr<SensorData> convert(const std::shared_ptr<DeviceRawData>& rawdata) final;
    TronErrno do_adjust_parameter(DeviceParameterType type, const std::string& value) final;

    // Convert should be implemented as static function
    static std::shared_ptr<SensorData> do_convert(const std::shared_ptr<DeviceRawData>& rawdata);

    // Disconnect should be implemented as non-virtual function
    void do_disconnect();

private:
    int64_t period_ms_;
    int64_t value_;
};

}  // namespace tron
}  // namespace wayz