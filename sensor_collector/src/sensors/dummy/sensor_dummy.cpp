//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include "sensor_dummy.hpp"

#include <chrono>

#include <common/data_message/all_data.hpp>

namespace wayz {

SensorDummy::SensorDummy(const std::string& storage_path, const std::string& sensor_name) :
    SensorBase(storage_path, sensor_name),
    dummy_sensor_period_(1000.0){};

bool SensorDummy::do_connect_sensor(const std::vector<Dictionary>& sensor_parameters)
{
    for (auto param : sensor_parameters) {
        if (param.first == "rate") {
            dummy_sensor_period_ = 1000.0 / atof(param.second.c_str());
        }
    }
    return true;
}

void SensorDummy::do_disconnect_sensor()
{
    return;
}

SensorData* SensorDummy::do_sensor_fetch()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(dummy_sensor_period_));
    auto now = std::chrono::system_clock::now();
    auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
    int64_t now_value = now_ms.time_since_epoch().count();

    // Create SensorData
    int32_t data_length = sizeof(SensorData) + sizeof(DataDummy);
    SensorData* sensor_data = reinterpret_cast<SensorData*>(new char[data_length]);
    sensor_data->length = data_length;
    sensor_data->sensor_type = static_cast<int32_t>(SensorType::dummy);
    sensor_data->sensor_datatype = static_cast<int32_t>(SensorDataType::dummy);
    sensor_data->is_timestamp_synced = 0;
    sensor_data->timestamp_us = 0;
    sensor_data->timestamp_recv_us = now_value;

    // Data
    DataDummy* data_dummy = reinterpret_cast<DataDummy*>(sensor_data->rawdata);
    data_dummy->dummy_int = 0x12345678;
    data_dummy->dummy_float = 1;
    data_dummy->dummy_array[0] = 0x33;
    data_dummy->dummy_array[1] = 0x55;

    return sensor_data;
}

}  // namespace wayz