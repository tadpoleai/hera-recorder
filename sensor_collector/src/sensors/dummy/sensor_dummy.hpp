//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#ifndef __sensor_dummy_hpp__
#define __sensor_dummy_hpp__
#include "../sensor_base.hpp"

namespace wayz {

class SensorDummy : public SensorBase {
public:
    SensorDummy(const std::string& storage_path, const std::string& sensor_name);
    bool do_connect_sensor() final;
    void do_disconnect_sensor() final;
    SensorData* do_sensor_fetch() final;

    std::vector<ParamPair> get_sensor_parameter_names() final;
    bool set_sensor_parameters(const std::vector<ParamPair>& sensor_parameters) final;

private:
    int64_t dummy_sensor_period_;
    int64_t dummy_sensor_value_;
};

}  // namespace wayz
#endif