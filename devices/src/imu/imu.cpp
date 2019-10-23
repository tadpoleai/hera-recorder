//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include "imu.hpp"

#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>

#include <common/logger/logger.hpp>

namespace wayz {
namespace tron {

Imu::Imu(int32_t id, const std::string& name) : Device(id, name) {}
Imu::~Imu()
{
    disconnect();
}

DeviceType Imu::get_type() const
{
    return DeviceType::Imu;
}

TronErrno Imu::connect()
{
    if (parameters_.count(DeviceParameterType::Kernel)) {
        kernel_ = parameters_[DeviceParameterType::Kernel];
    } else {
        return set_error_and_die(TronErrno::InsufficientParameters, "Paramater Kernel absent");
    }

    if (parameters_.count(DeviceParameterType::BaudRate)) {
        baud_rate_ = stoi(parameters_[DeviceParameterType::BaudRate]);
    } else {
        return set_error_and_die(TronErrno::InsufficientParameters, "Paramater BaudRate absent");
    }

    if (parameters_.count(DeviceParameterType::SerialMsgType)) {
        serial_msg_type_ = stoi(parameters_[DeviceParameterType::SerialMsgType]);
    } else {
        return set_error_and_die(TronErrno::InsufficientParameters,
                                 "Paramater SerialMsgType absent");
    }

    serial_port_ = SerialTransport::create(kernel_, SerialConfig(baud_rate_));
    if (!serial_port_->is_opened()) {
        return set_error_and_die(TronErrno::CanNotOpenTtyDevice,
                                 "Can not open device '" + kernel_ + "'");
    }

    queue_ = serial_port_->get_queue_handler(serial_msg_type_);
    if (!queue_) {
        return set_error_and_die(TronErrno::CanNotOpenTtyDevice,
                                 "Can not register listener of msg_type '" +
                                         std::to_string(serial_msg_type_) + "' on device '" +
                                         kernel_ + "'");
    }

    return TronErrno::Success;
}
void Imu::disconnect()
{
    stop();
    do_disconnect();
}
void Imu::do_disconnect()
{
    // Write Real Disconnection code here
    serial_port_->free();
    return;
}

std::shared_ptr<DeviceRawData> Imu::fetch()
{
    if (!queue_ || queue_->empty()) {
        return nullptr;
    }

    // Get Rawdata from a Real Sensor
    auto serial_data = queue_->wait_and_pop();

    // Get Length of Rawdata First
    int32_t received_rawdata_length = serial_data->size();

    // Create a Buff to Store Rawdata
    int32_t total_length = sizeof(DeviceRawData) + received_rawdata_length;
    DeviceRawData* data = reinterpret_cast<DeviceRawData*>(new uint8_t[total_length]);

    // Fullfil Metadata (Header) of Rawdata;
    data->length = total_length;
    data->device_type = DeviceType::Imu;
    data->device_data_type = DeviceDataType::Imu;
    data->sequence = sequence_++;
    data->timestamp_receive_ns = Timestamp::now();

    // Use Memcpy to fill Buff
    memcpy(reinterpret_cast<uint8_t*>(data->rawdata_buf),
           serial_data->data(),
           received_rawdata_length);

    // Return a Shared Ptr
    return std::shared_ptr<DeviceRawData>(data);
}
std::shared_ptr<SensorData> Imu::convert(const std::shared_ptr<DeviceRawData>& rawdata)
{
    return do_convert(rawdata);
}
std::shared_ptr<SensorData> Imu::do_convert(const std::shared_ptr<DeviceRawData>& rawdata)
{
    // Create a Buff to Store Data
    int32_t total_length = sizeof(SensorData) + sizeof(DataImu);
    SensorData* data = reinterpret_cast<SensorData*>(new uint8_t[total_length]);

    // Fullfil Metadata (Header) of Data;
    data->length = total_length;
    data->device_type = rawdata->device_type;
    data->device_data_type = rawdata->device_data_type;
    data->sequence = rawdata->sequence;
    data->timestamp_receive_ns = rawdata->timestamp_receive_ns;

    // A Pointer to Real Data
    DataImu* data_imu_buf = reinterpret_cast<DataImu*>(data->data_buf);

    // Parse Rawdata
    const auto* rawdata_buf = reinterpret_cast<DeviceRawDataImu*>(rawdata->rawdata_buf);

    // Calculate Intrinsic Time
    data->timestamp_intrinsic_ns = rawdata_buf->timestamp_ns;

    // Convert
    for (int32_t i = 0; i < 3; ++i) {
        data_imu_buf->angular_velocity[i] = rawdata_buf->gyro[i] * (M_PI / 200.0 / 180.0);
        data_imu_buf->linear_acceleration[i] = rawdata_buf->acc[i] * (GravitySI_ / 4000.0);
        data_imu_buf->magnetic_field[i] = rawdata_buf->mag[i] * (1 / 16000.0);
    }

    return std::shared_ptr<SensorData>(data);
}
TronErrno Imu::do_adjust_parameter(DeviceParameterType type, const std::string& value)
{
    switch (type) {
    default:
        return TronErrno::UnimplementedParameter;
    }
    return TronErrno::Success;
}

}  // namespace tron
}  // namespace wayz