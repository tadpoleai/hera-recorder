//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#ifndef __sensor_dummy_hpp__
#define __sensor_dummy_hpp__
#include "../sensor_base.hpp"

namespace wayz {

class SensorDummy final : public SensorBase {
public:
    SensorDummy(int32_t id, const std::string& name);
    SensorDummy(const SensorDummy&) = delete;
    SensorDummy& operator=(const SensorDummy&) = delete;
    ~SensorDummy();

    SensorType getType() const final;

    TronErrno doConnectSensor() final;
    void doDisconnectSensor() final;
    std::shared_ptr<SensorRawData> doFetchRawData() final;
    std::shared_ptr<SensorData> doConvertData(const std::shared_ptr<SensorRawData>& rawdata) final;
    TronErrno doAdjustParameter(SensorParameterType type, const std::string& value) final;

private:
    int64_t periodMs_;
    int64_t value_;
};

}  // namespace wayz
#endif