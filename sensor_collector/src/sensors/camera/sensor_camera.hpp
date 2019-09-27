//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#ifndef __sensor_camera_hpp__
#define __sensor_camera_hpp__
#include "../sensor_base.hpp"


namespace wayz {

BETTER_ENUM(SensorCameraParameter, int32_t, rate = 0, value)

class SensorCamera final : public SensorBase {
public:
    SensorCamera(int32_t id, const std::string& name);
    SensorCamera(const SensorCamera&) = delete;
    SensorCamera& operator=(const SensorCamera&) = delete;
    ~SensorCamera();

    SensorType getType() const final;

    TronErrno doConnectSensor() final;
    void doDisconnectSensor() final;
    std::shared_ptr<SensorRawData> doFetchRawData() final;
    std::shared_ptr<SensorData> doConvertData(const std::shared_ptr<SensorRawData>& rawdata) final;
    TronErrno doAdjustParameter(SensorParameterType type, const std::string& value) final;

private:

  
    
};

}  // namespace wayz
#endif