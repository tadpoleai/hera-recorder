//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include "sensor_dummy.hpp"

#include <chrono>
#include <cstdlib>

#include <common/data_message/all_data.hpp>
#include <common/third_party/enum.h>

namespace wayz {

SensorDummy::SensorDummy(const std::string& storage_path, const std::string& sensor_name) :
    SensorBase(storage_path, sensor_name),
    dummy_sensor_period_(1000),
    dummy_sensor_value_(0x12345678){};

bool SensorDummy::do_connect_sensor()
{
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
    data_dummy->dummy_int = dummy_sensor_value_;
    data_dummy->dummy_float = static_cast<float>(dummy_sensor_value_);
    data_dummy->dummy_array[0] = 0x33;
    data_dummy->dummy_array[1] = 0x55;
    data_dummy->dummy_array[2] = 0x77;
    data_dummy->dummy_array[3] = 0xFF;

    return sensor_data;
}

BETTER_ENUM(SensorDummyParameter, int32_t, rate = 0, value)

std::vector<ParamPair> SensorDummy::get_sensor_parameter_names()
{
    std::vector<ParamPair> ret_list;
    for (auto param : SensorDummyParameter::_values()) {
        ret_list.emplace_back(
                std::make_pair<int32_t, std::string>(param._to_integral(), param._to_string()));
    }
    return ret_list;
}

bool SensorDummy::set_sensor_parameters(const std::vector<ParamPair>& sensor_parameters)
{
    for (auto param_pair : sensor_parameters) {
        auto param_name = SensorDummyParameter::_from_integral(param_pair.first);
        switch (param_name) {
        case SensorDummyParameter::rate:
            dummy_sensor_period_ = 1000.0 / std::stof(param_pair.second);
            break;
        case SensorDummyParameter::value:
            dummy_sensor_value_ = std::stoi(param_pair.second, 0, 0);
            break;
        default:
            break;
        }
    }
    return true;
}

}  // namespace wayz