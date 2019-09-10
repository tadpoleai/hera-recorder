//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#ifndef __sensor_dummy_hpp__
#define __sensor_dummy_hpp__
#include "../sensor_base.hpp"

namespace wayz {

BETTER_ENUM(SensorDummyParameter, int32_t, rate = 0, value)

class SensorDummy final : public SensorBase {
public:
    SensorDummy(const std::string& sensor_name);
    ~SensorDummy();
    bool do_connect_sensor() final;
    void do_disconnect_sensor() final;
    SensorData* do_sensor_fetch() final;

    static std::vector<std::string> get_sensor_dummy_parameter_names();
    std::vector<std::string> get_sensor_parameter_names() final;
    bool set_sensor_parameters(const std::vector<ParamPair>& sensor_parameters) final;

private:
    int64_t dummy_sensor_period_;
    int64_t dummy_sensor_value_;
};

}  // namespace wayz
#endif