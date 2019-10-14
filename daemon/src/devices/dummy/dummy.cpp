//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include "dummy.hpp"

#include <chrono>
#include <cstdlib>
#include <string>
#include <vector>

namespace wayz {
namespace tron {

Dummy::Dummy(int32_t id, const std::string& name) : Device(id, name) {}
Dummy::~Dummy()
{
    disconnect();
}

DeviceType Dummy::get_type() const
{
    return DeviceType::Dummy;
}

TronErrno Dummy::connect()
{
    if (parameters_.count(DeviceParameterType::DummyValue)) {
        value_ = std::stoi(parameters_[DeviceParameterType::DummyValue]);
    } else {
        return set_error_and_die(TronErrno::InsufficientParameters, "Paramater DummyValue absent");
    }

    if (parameters_.count(DeviceParameterType::DummyRate)) {
        period_ms_ = 1000 / std::stof(parameters_[DeviceParameterType::DummyRate]);
    } else {
        return set_error_and_die(TronErrno::InsufficientParameters, "Paramater DummyRate absent");
    }
    
    printf("PR_MS: %ld\n", period_ms_);
    return TronErrno::Success;
}
void Dummy::disconnect()
{
    stop();
    do_disconnect();
}
void Dummy::do_disconnect()
{
    // Write Real Disconnection code here
    return;
}

std::shared_ptr<DeviceRawData> Dummy::fetch()
{
    // Some Sensors Blocks, Simulate that
    std::this_thread::sleep_for(std::chrono::milliseconds(period_ms_));

    // Get Rawdata from a Real Sensor
    // Get Length of Rawdata First
    int32_t received_rawdata_length = sizeof(int32_t);

    // Create a Buff to Store Rawdata
    int32_t total_length = sizeof(DeviceRawData) + received_rawdata_length;
    DeviceRawData* data = reinterpret_cast<DeviceRawData*>(new uint8_t[total_length]);

    // Fullfil Metadata (Header) of Rawdata;
    data->length = total_length;
    data->device_type = DeviceType::Dummy;
    data->device_data_type = DeviceDataType::Dummy;
    data->sequence = sequence_++;
    data->timestamp_receive_ns = get_system_timestamp();

    // Assume Rawdata is in sensor_rawata_src
    int32_t value_to_get_from_device = value_;
    uint8_t* sensor_rawata_src = reinterpret_cast<uint8_t*>(&value_to_get_from_device);

    // Use Memcpy to fill Buff
    memcpy(reinterpret_cast<uint8_t*>(data->rawdata_buf),
           sensor_rawata_src,
           received_rawdata_length);

    // Return a Shared Ptr
    return std::shared_ptr<DeviceRawData>(data);
}
std::shared_ptr<SensorData> Dummy::convert(const std::shared_ptr<DeviceRawData>& rawdata)
{
    return do_convert(rawdata);
}
std::shared_ptr<SensorData> Dummy::do_convert(const std::shared_ptr<DeviceRawData>& rawdata)
{
    // Create a Buff to Store Data
    int32_t total_length = sizeof(SensorData) + sizeof(DataDummy);
    SensorData* data = reinterpret_cast<SensorData*>(new uint8_t[total_length]);

    // Fullfil Metadata (Header) of Data;
    data->length = total_length;
    data->device_type = rawdata->device_type;
    data->device_data_type = rawdata->device_data_type;
    data->sequence = rawdata->sequence;
    data->timestamp_receive_ns = rawdata->timestamp_receive_ns;

    // A Pointer to Real Data
    DataDummy* data_dummy_buf = reinterpret_cast<DataDummy*>(data->data_buf);

    // Parse Rawdata
    const auto* rawdata_buf = rawdata->rawdata_buf;
    int32_t value_of_dummy = *(reinterpret_cast<const int32_t*>(rawdata_buf));

    // Fullfil Real Data
    data_dummy_buf->dummy_int = value_of_dummy;
    data_dummy_buf->dummy_float = value_of_dummy;
    data_dummy_buf->dummy_char_array[0] = 'W';
    data_dummy_buf->dummy_char_array[1] = 'A';
    data_dummy_buf->dummy_char_array[2] = 'Y';
    data_dummy_buf->dummy_char_array[3] = 'Z';

    return std::shared_ptr<SensorData>(data);
}
TronErrno Dummy::do_adjust_parameter(DeviceParameterType type, const std::string& value)
{
    switch (type) {
    case DeviceParameterType::DummyRate:
        period_ms_ = 1000 / std::stof(value);
        break;
    case DeviceParameterType::DummyValue:
        value_ = std::stoi(parameters_[DeviceParameterType::DummyValue]);
        break;
    default:
        return TronErrno::UnimplementedParameter;
    }
    return TronErrno::Success;
}

}  // namespace tron
}  // namespace wayz