///
/// @file novatelspan.cpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief
/// @date 2020-06-08
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include "novatelspan.hpp"


namespace wayz {
namespace hera {
namespace device {
namespace ins {
namespace novatelspan {

const std::vector<DeviceParameterType> NovatelSpan::EssentialParameterTypes = {DeviceParameterType::Kernel,
                                                                               DeviceParameterType::KernelAuxiliary,
                                                                               DeviceParameterType::BaudRate,
                                                                               DeviceParameterType::BaudRateAuxiliary,
                                                                               DeviceParameterType::SerialMsgType};

const std::vector<DeviceParameterType> NovatelSpan::OptionalParameterTypes = {};

auto _ = DeviceFactory::register_type({.type = DeviceVendorType::InsNovatelSpan,
                                       .type_name = "ins/novatelspan",
                                       .create = &NovatelSpan::create,
                                       .do_convert = &NovatelSpan::do_convert,
                                       .essential_parameter_types = NovatelSpan::EssentialParameterTypes,
                                       .optional_parameter_types = NovatelSpan::OptionalParameterTypes,
                                       .implemented = true});

#ifdef WITH_DRIVER

HeraErrno NovatelSpan::connect()
{
    try {
        kernel_ = parameters_[DeviceParameterType::Kernel];
        kernel_auxiliary_ = parameters_[DeviceParameterType::KernelAuxiliary];
        baud_rate_ = stoi(parameters_[DeviceParameterType::BaudRate]);
        baud_rate_auxiliary_ = stoi(parameters_[DeviceParameterType::BaudRateAuxiliary]);
        serial_msg_type_ = stoi(parameters_[DeviceParameterType::SerialMsgType]);
    } catch (...) {
        return handle_error(HeraErrno::InvalidParameterValue);
    }

    auto binary_config = driver::SerialPortBinaryConfig(
            {.lead_bytes = "\xAA\x44\x12",
             .tail_bytes = "",
             .checksum_protocol = driver::SerialPortBinaryConfig::ChecksumProtocol::CRC32_ISO_3309,
             .checksum_range = driver::SerialPortBinaryConfig::ChecksumRange::LEAD_BYTES_AND_DATA});

    serial_port_ = new driver::SerialPortBinary(kernel_, driver::SerialConfig(baud_rate_), binary_config);
    if (!serial_port_->is_opened()) {
        return handle_error(HeraErrno::CanNotOpenTtyDevice, "Can not open primary device '" + kernel_ + "'");
    }
    queue_ = serial_port_->get_queue_handler();

    serial_port_auxiliary_ =
            driver::SerialTransport::create(kernel_auxiliary_, driver::SerialConfig(baud_rate_auxiliary_));
    if (!serial_port_auxiliary_->is_opened()) {
        return handle_error(HeraErrno::CanNotOpenTtyDevice,
                            "Can not open auxiliary device '" + kernel_auxiliary_ + "'");
    }
    queue_auxiliary_ = serial_port_auxiliary_->get_queue_handler(serial_msg_type_);
    if (!queue_auxiliary_) {
        return handle_error(HeraErrno::CanNotOpenTtyDevice, "Can not register listener");
    }

    shiftation_timestamp_ = 0;
    shifation_value_ns_ = 0;

    return HeraErrno::Success;
}

/// Free the serial port object
///
void NovatelSpan::disconnect()
{
    if (serial_port_ != nullptr) {
        delete serial_port_;
    }
    if (serial_port_auxiliary_ != nullptr) {
        serial_port_auxiliary_->free();
    }
}

/// Fetch data from serial port
///
data::DeviceDataPtr NovatelSpan::fetch()
{
    if (!queue_ || !queue_auxiliary_) {
        return nullptr;
    }

    auto now = time::Timestamp::now();

    // Get Timestamp shiftation from queue_auxiliary
    auto time_shift = queue_auxiliary_->pop();
    if (time_shift) {
        if (time_shift->size() == sizeof(uint64_t)) {
            memcpy(&shifation_value_ns_, time_shift->data(), sizeof(uint64_t));
            shiftation_timestamp_ = now;
        }
    }

    // Get Framed from Serial
    auto binary_framed_data = queue_->wait_pop();
    if (binary_framed_data == nullptr) {
        return nullptr;
    }

    // Total length of device data
    auto frame_data_length = binary_framed_data->size();
    auto length =
            sizeof(NovatelSpanBinaryData) - sizeof(NovatelSpanBinaryData::NovatelBinaryStruct) + frame_data_length;
    auto data = data::DeviceData::create(length,
                                         id_,
                                         DeviceVendorType::InsNovatelSpan,
                                         DeviceDataType::InsNovatelSpanBinaryData,
                                         sequence_++);
    auto derived_data = static_cast<NovatelSpanBinaryData*>(data.get());

    derived_data->shiftation_timestamp = shiftation_timestamp_;
    derived_data->shifation_value_ns = shifation_value_ns_;
    memcpy(derived_data->novatel_buf, binary_framed_data->data(), frame_data_length);

    return data;
}

HeraErrno NovatelSpan::adjust_parameter(DeviceParameterType type, const std::string& value)
{
    switch (type) {
    case DeviceParameterType::Kernel:
        return HeraErrno::ImmutableParameter;
    default:
        return HeraErrno::UnimplementedParameter;
    }
    return HeraErrno::Success;
}
#endif

/// Multiple raw data by defined granularity
///
data::SensorDataPtr NovatelSpan::do_convert(data::DeviceDataPtr& storage_data)
{
    if (!storage_data->is_type(DeviceDataType::InsNovatelSpanBinaryData)) {
        return data::SensorData::broken_data();
    }

    // Raw DeviceData of Derived Type
    auto raw_data = static_cast<NovatelSpanBinaryData*>(storage_data.get());

    // Get Timestamp
    static constexpr uint64_t GPS_ORIGIN_TIME_NS =
            315964800ULL * time::OneSecond;  ///< GPS Time start from 1980/Jan/06 00:00:00 UTC

    ///
    /// @brief UTC Leap seconds
    /// -37s, GPS - UTC = 18s, since 2017/Jan/1 UTC
    /// @todo Add / Modify code, if Leap seconds added in future
    ///
    static constexpr uint64_t UTC_LEAP_SECONDS_NS = 18ULL * time::OneSecond;  ///<

    uint64_t gnss_week = raw_data->novatel_header.gnss_week;
    uint64_t gnss_ms = raw_data->novatel_header.gnss_ms;
    uint64_t gnss_time_ns = gnss_week * (time::OneDay * 7) + gnss_ms * 1000000ULL;
    uint64_t utc_time_ns = GPS_ORIGIN_TIME_NS + gnss_time_ns - UTC_LEAP_SECONDS_NS;

    uint64_t timestamp_intrinsic_ns = utc_time_ns + raw_data->shifation_value_ns;

    // Judge Type of Message
    auto message_id = raw_data->novatel_header.message_id;
    switch (message_id) {
    case MessageIdType::BESTPOS: {
        auto bestpos = static_cast<BESTPOS*>((void*)raw_data->novatel_header.payload);

        // Create a SensorData from DeviceData
        auto length = sizeof(data::BestPosition);
        auto sensor_data = data::SensorData::create_from(storage_data, SensorDataType::InsBestPosition, length);
        auto ins_sensor_data = static_cast<data::BestPosition*>(sensor_data.get());

        // Parse Data
        ins_sensor_data->timestamp_intrinsic_ns = timestamp_intrinsic_ns;
        ins_sensor_data->solution_status = bestpos->solution_status;
        ins_sensor_data->position_type = bestpos->position_type;
        ins_sensor_data->latitude = bestpos->latitude;
        ins_sensor_data->longitude = bestpos->longitude;
        ins_sensor_data->altitude = bestpos->height + bestpos->undulation;
        ins_sensor_data->latitude_deviation = bestpos->latitude_deviation;
        ins_sensor_data->longitude_deviation = bestpos->longitude_deviation;
        ins_sensor_data->altitude_deviation = bestpos->height_deviation;

        return sensor_data;
    }
    case MessageIdType::INSPOS: {
        auto inspos = static_cast<INSPOS*>((void*)raw_data->novatel_header.payload);

        // Create a SensorData from DeviceData
        auto length = sizeof(data::InsPosition);
        auto sensor_data = data::SensorData::create_from(storage_data, SensorDataType::InsInsPosition, length);
        auto ins_sensor_data = static_cast<data::InsPosition*>(sensor_data.get());

        // Parse Data
        ins_sensor_data->timestamp_intrinsic_ns = timestamp_intrinsic_ns;
        ins_sensor_data->ins_status = inspos->ins_status;
        ins_sensor_data->position_type = data::PositionVelocityType::UNKNOWN;
        ins_sensor_data->latitude = inspos->latitude;
        ins_sensor_data->longitude = inspos->longitude;
        ins_sensor_data->altitude = inspos->height;
        ins_sensor_data->latitude_deviation = -1;
        ins_sensor_data->longitude_deviation = -1;
        ins_sensor_data->altitude_deviation = -1;

        return sensor_data;
    }
    case MessageIdType::INSPOSX: {
        auto insposx = static_cast<INSPOSX*>((void*)raw_data->novatel_header.payload);

        // Create a SensorData from DeviceData
        auto length = sizeof(data::InsPosition);
        auto sensor_data = data::SensorData::create_from(storage_data, SensorDataType::InsInsPosition, length);
        auto ins_sensor_data = static_cast<data::InsPosition*>(sensor_data.get());

        // Parse Data
        ins_sensor_data->timestamp_intrinsic_ns = timestamp_intrinsic_ns;
        ins_sensor_data->ins_status = insposx->ins_status;
        ins_sensor_data->position_type = insposx->position_type;
        ins_sensor_data->latitude = insposx->latitude;
        ins_sensor_data->longitude = insposx->longitude;
        ins_sensor_data->altitude = insposx->height + insposx->undulation;
        ins_sensor_data->latitude_deviation = insposx->latitude_deviation;
        ins_sensor_data->longitude_deviation = insposx->longitude_deviation;
        ins_sensor_data->altitude_deviation = insposx->height_deviation;

        return sensor_data;
    }
    case MessageIdType::CORRIMUDATA: {
        static constexpr auto CorrectedImuRate = 100.0;  // 100Hz

        auto corrimu = static_cast<CORRIMUDATA*>((void*)raw_data->novatel_header.payload);

        // Create a SensorData from DeviceData
        auto length = sizeof(data::CorrectedImu);
        auto sensor_data = data::SensorData::create_from(storage_data, SensorDataType::InsCorrectedImu, length);
        auto ins_sensor_data = static_cast<data::CorrectedImu*>(sensor_data.get());

        // Parse Data
        ins_sensor_data->timestamp_intrinsic_ns = timestamp_intrinsic_ns;
        for (auto i = 0; i < 3; ++i) {
            ins_sensor_data->angular_velocity[i] = corrimu->rotational_speed_sample[i] * CorrectedImuRate;
            ins_sensor_data->linear_acceleration[i] = corrimu->acceleration_sample[i] * CorrectedImuRate;
        }

        return sensor_data;
    }
    default:
        return data::SensorData::broken_data();
    }
}

}  // namespace novatelspan
}  // namespace ins
}  // namespace device
}  // namespace hera
}  // namespace wayz
