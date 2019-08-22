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
    bool do_connect_sensor(const std::vector<Dictionary>& sensor_parameters) final;
    void do_disconnect_sensor() final;
    SensorData* do_sensor_fetch() final;

public:
    int64_t dummy_sensor_period_;
};

}  // namespace wayz
#endif