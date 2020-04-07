///
/// @file aceinna.cpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Implementation of class Aceinna
/// @version 0.1
/// @date 2019-11-07
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include "aceinna.hpp"


namespace wayz {
namespace hera {
namespace device {
namespace imu {
namespace aceinna {

const std::vector<DeviceParameterType> Aceinna::EssentialParameterTypes = {DeviceParameterType::Kernel,
                                                                           DeviceParameterType::BaudRate,
                                                                           DeviceParameterType::SerialMsgType};

const std::vector<DeviceParameterType> Aceinna::OptionalParameterTypes = {};

/// Open serial port by kernel, baud rate, serial msg type,
/// and get a thread-safe queue
HeraErrno Aceinna::connect()
{
    try {
        kernel_ = parameters_[DeviceParameterType::Kernel];
        baud_rate_ = stoi(parameters_[DeviceParameterType::BaudRate]);
        serial_msg_type_ = stoi(parameters_[DeviceParameterType::SerialMsgType]);
    } catch (...) {
        return handle_error(HeraErrno::InvalidParameterValue);
    }

    serial_port_ = utils::SerialTransport::create(kernel_, utils::SerialConfig(baud_rate_));
    if (!serial_port_->is_opened()) {
        return handle_error(HeraErrno::CanNotOpenTtyDevice, "Can not open device '" + kernel_ + "'");
    }

    queue_ = serial_port_->get_queue_handler(serial_msg_type_);
    if (!queue_) {
        return handle_error(HeraErrno::CanNotOpenTtyDevice, "Can not register listener");
    }

    return HeraErrno::Success;
}

/// Free the serial port object
///
void Aceinna::disconnect()
{
    if (serial_port_ != nullptr) {
        serial_port_->free();
    }
}

/// Fetch data from serial port
///
data::DeviceDataPtr Aceinna::fetch()
{
    if (!queue_) {
        log::warn << "Aceinna: Queue not registered by SerialTransport" << log::endl;
        return nullptr;
    }

    // Get Rawdata from a Real Sensor
    auto serial_data = queue_->wait_pop();
    if (serial_data == nullptr) {
        return nullptr;
    }
    auto received_length = serial_data->size();

    // Total length of device data
    auto length = sizeof(AceinnaData);
    auto data = data::DeviceData::create(length,
                                         id_,
                                         DeviceVendorType::ImuAceinna,
                                         DeviceDataType::ImuAceinnaData,
                                         sequence_++);
    auto derived_data = static_cast<AceinnaData*>(data.get());

    if (received_length != sizeof(derived_data->buf)) {
        log::warn << "Aceinna: Received size does not match" << log::endl;
        return nullptr;
    }
    // Use Memcpy to directly fill buf
    memcpy(derived_data->buf, serial_data->data(), received_length);

    return data;
}

HeraErrno Aceinna::adjust_parameter(DeviceParameterType type, const std::string& value)
{
    switch (type) {
    case DeviceParameterType::Kernel:
    case DeviceParameterType::SerialMsgType:
        return HeraErrno::ImmutableParameter;
    default:
        return HeraErrno::UnimplementedParameter;
    }
    return HeraErrno::Success;
}

/// Multiple raw data by defined granularity
///
data::SensorDataPtr Aceinna::do_convert(data::DeviceDataPtr& storage_data)
{
    if (!storage_data->is_type(DeviceDataType::ImuAceinnaData)) {
        return data::SensorData::broken_data();
    }

    // Raw DeviceData of Derived Type
    auto raw_data = static_cast<AceinnaData*>(storage_data.get());

    // Create a SensorData from DeviceData
    auto length = sizeof(data::ImuMagneticField);
    auto sensor_data = data::SensorData::create_from(storage_data, SensorDataType::ImuMagneticField, length);
    auto imu_sensor_data = static_cast<data::ImuMagneticField*>(sensor_data.get());

    // Parse Data
    imu_sensor_data->timestamp_intrinsic_ns = raw_data->data.timestamp;
    for (auto i = 0; i < 3; ++i) {
        imu_sensor_data->angular_velocity[i] = raw_data->data.gyro[i] * GyroGranularity_;
        imu_sensor_data->linear_acceleration[i] = raw_data->data.accel[i] * AccelGranularity_;
        imu_sensor_data->magnetic_field[i] = raw_data->data.magnetic[i] * MagneticGranularity_;
    }

    return sensor_data;
}

}  // namespace aceinna
}  // namespace imu
}  // namespace device
}  // namespace hera
}  // namespace wayz