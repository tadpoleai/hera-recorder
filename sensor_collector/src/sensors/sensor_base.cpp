//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include "sensor_base.hpp"

#include <cstdint>
#include <map>
#include <string>
#include <thread>

namespace wayz {

SensorBase::SensorBase(const std::string& storage_path, const std::string& sensor_name) :
    sensor_status_(SensorStatus::stopped),
    sensor_realtime_forwarding_(false)
{
    create_storage(storage_path, sensor_name);
}

}