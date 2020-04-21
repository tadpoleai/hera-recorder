///
/// @file serialsync.cpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Implementation of class Serialsync
/// @version 0.1
/// @date 2019-11-25
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include "serialsync.hpp"

#include <cmath>
#include <functional>
#include <numeric>

namespace wayz {
namespace hera {
namespace device {
namespace gnss {
namespace serialsync {

const std::vector<DeviceParameterType> Serialsync::EssentialParameterTypes = {DeviceParameterType::Kernel,
                                                                              DeviceParameterType::KernelAuxiliary,
                                                                              DeviceParameterType::BaudRate,
                                                                              DeviceParameterType::BaudRateAuxiliary,
                                                                              DeviceParameterType::SerialMsgType};

const std::vector<DeviceParameterType> Serialsync::OptionalParameterTypes = {};

auto _ = DeviceFactory::register_type({.type = DeviceVendorType::GnssSerialsync,
                                       .type_name = "gnss/serialsync",
                                       .create = &Serialsync::create,
                                       .do_convert = &Serialsync::do_convert,
                                       .essential_parameter_types = Serialsync::EssentialParameterTypes,
                                       .optional_parameter_types = Serialsync::OptionalParameterTypes,
                                       .implemented = true});

#ifdef WITH_DRIVER
HeraErrno Serialsync::connect()
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

    serial_port_ = new driver::SerialPortSentence(kernel_, driver::SerialConfig(baud_rate_));
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
void Serialsync::disconnect()
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
data::DeviceDataPtr Serialsync::fetch()
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

    // Get Rawdata from a Real Sensor
    auto nmea_sentence_str = queue_->wait_pop();
    if (nmea_sentence_str == nullptr) {
        return nullptr;
    }

    uint64_t timestamp_aligned = now;
    uint8_t timestamp_aligned_valid = 0;
    try {
        // Tokenize nmea sentence
        std::string token;
        std::stringstream nmea(std::string(nmea_sentence_str->begin(), nmea_sentence_str->end()));

        if (!getline(nmea, token, ',')) {
            throw std::runtime_error("Can not tokenize 1st token");
        }
        // Only accept NMEA::GPGGA
        /// @see https://www.gpsinformation.org/dale/nmea.htm#GGA
        if (token != "$GPGGA") {
            throw std::runtime_error("got non-gpgga sentence '" + token + "'");
        }

        // Get original time of NMEA (hhmmss)
        if (!getline(nmea, token, ',')) {
            throw std::runtime_error("Can not tokenize 2nd token");
        }

        // Valid time info (No signal from satellites)
        if (token.size() != 0) {
            // Tokenize GPTS
            auto hours = std::stoull(token.substr(0, 2));
            auto minutes = std::stoull(token.substr(2, 2));
            auto seconds = std::stoull(token.substr(4, 2));
            uint64_t nanoseconds = 0;
            if (token.size() != 6) {
                if (token[6] != '.') {
                    throw std::runtime_error("Can not tokenize gpts '" + token + "'");
                }
                nanoseconds = std::stoull(token.substr(7));
                switch (token.size()) {
                case 8:                             // e.g. 121212.4
                    nanoseconds *= 100'000'000ULL;  // 100ms
                    break;
                case 9:                            // e.g. 121212.45
                    nanoseconds *= 10'000'000ULL;  // 10ms
                    break;
                case 10:                          // e.g. 121212.450
                    nanoseconds *= 1'000'000ULL;  // 1ms
                    break;
                default:
                    throw std::runtime_error("Can not tokenize gpts '" + token + "'");
                }
            }

            // Calculate original timestamp
            uint64_t timestamp_nmea_original =
                    hours * time::OneHour + minutes * time::OneMinute + seconds * time::OneSecond + nanoseconds;

            if (now - shiftation_timestamp_ < ShifationDelayTolerance_) {
                timestamp_nmea_original += shifation_value_ns_;

                uint64_t days = now / time::OneDay;
                int64_t now_residual = int64_t(now) - days * time::OneDay;

                if (int64_t(timestamp_nmea_original) - now_residual > time::OneDay / 2) {
                    days -= 1;
                } else if (int64_t(timestamp_nmea_original) - now_residual < -time::OneDay / 2) {
                    days += 1;
                }

                timestamp_aligned_valid = 1;
                timestamp_aligned = timestamp_nmea_original + days * time::OneDay;
            }
        }
    } catch (std::exception& err) {
        log::warn << "SerialSync: fetch(), " << err.what() << log::endl;
        return nullptr;
    }

    // Total length of device data
    auto nmea_sentence_length = nmea_sentence_str->size();
    auto length = sizeof(SerialSyncNmea) + nmea_sentence_length;
    auto data = data::DeviceData::create(length,
                                         id_,
                                         DeviceVendorType::GnssSerialsync,
                                         DeviceDataType::GnssSerialsyncNmea,
                                         sequence_++);
    auto derived_data = static_cast<SerialSyncNmea*>(data.get());

    // Copy data
    /// @todo calculate time
    derived_data->data.timestamp_intrinsic_ns = timestamp_aligned;
    derived_data->data.timestamp_valid = timestamp_aligned_valid;
    derived_data->data.nmea_sentence_length = nmea_sentence_length;
    memcpy(derived_data->data.nmea_sentence, nmea_sentence_str->data(), nmea_sentence_length);

    return data;
}

HeraErrno Serialsync::adjust_parameter(DeviceParameterType type, const std::string& value)
{
    switch (type) {
    case DeviceParameterType::Kernel:
    case DeviceParameterType::KernelAuxiliary:
    case DeviceParameterType::BaudRate:
    case DeviceParameterType::BaudRateAuxiliary:
    case DeviceParameterType::SerialMsgType:
        return HeraErrno::ImmutableParameter;
    default:
        return HeraErrno::UnimplementedParameter;
    }
    return HeraErrno::Success;
}
#endif

data::SensorDataPtr Serialsync::do_convert(data::DeviceDataPtr& storage_data)
{
    if (!storage_data->is_type(DeviceDataType::GnssSerialsyncNmea)) {
        return data::SensorData::broken_data();
    }

    // Raw DeviceData of Derived Type
    auto raw_data = static_cast<SerialSyncNmea*>(storage_data.get());

    // Create a SensorData from DeviceData
    auto length = sizeof(data::NavSatFix);
    auto sensor_data = data::SensorData::create_from(storage_data, SensorDataType::NavSatFix, length);
    auto navsatfix_sensor_data = static_cast<data::NavSatFix*>(sensor_data.get());


    // Initialize an invalid data template
    navsatfix_sensor_data->timestamp_intrinsic_ns = raw_data->data.timestamp_intrinsic_ns;
    navsatfix_sensor_data->status.status = data::NavSatFix::StatusType::NO_Fix;
    navsatfix_sensor_data->status.service = data::NavSatFix::ServiceType::GPS;
    navsatfix_sensor_data->position_covariance_type = data::NavSatFix::PositionCovarianceType::Unknown;
    navsatfix_sensor_data->position_covariance[0] = -1;
    for (auto i = 1; i < 9; ++i) {
        navsatfix_sensor_data->position_covariance[i] = 0;
    }
    navsatfix_sensor_data->latitude = NAN;
    navsatfix_sensor_data->longitude = NAN;
    navsatfix_sensor_data->altitude = NAN;

    // Return if time::Timestamp is not valid
    if (raw_data->data.timestamp_valid == 0) {
        return sensor_data;
    }

    // Parse NMEA Sentence
    try {
        double latitude = NAN;
        double longitude = NAN;
        double altitude = NAN;
        auto fixed = data::NavSatFix::StatusType::NO_Fix;

        // Tokenize nmea sentence
        std::string token;
        std::string nmea_sentence_str =
                std::string((char*)raw_data->data.nmea_sentence, raw_data->data.nmea_sentence_length);
        std::stringstream nmea(nmea_sentence_str);

        // Sentence Identifier (Token 1)
        if (!getline(nmea, token, ',')) {
            throw std::runtime_error("Can not tokenize token 1");
        }
        // Only accept NMEA::GPGGA
        /// @see https://www.gpsinformation.org/dale/nmea.htm#GGA
        if (token != "$GPGGA") {
            throw std::runtime_error("got non-gpgga sentence '" + token + "'");
        }

        // UTC time status of position (hhmmss) (Token 2)
        if (!getline(nmea, token, ',')) {
            throw std::runtime_error("Can not tokenize token 2");
        }

        // Latitude (Token 3)
        if (!getline(nmea, token, ',')) {
            throw std::runtime_error("Can not tokenize token 3");
        }

        auto lat_degree = std::stoull(token.substr(0, 2));
        auto lat_minute = std::stod(token.substr(2));
        latitude = lat_degree + lat_minute / 60.0;

        // Latitude Direction (Token 4)
        if (!getline(nmea, token, ',')) {
            throw std::runtime_error("Can not tokenize token 4");
        }
        if (token.size() != 1) {
            throw std::runtime_error("Invalid latitude direction '" + token + "'");
        }
        switch (token[0]) {
        case 'N':
            break;
        case 'S':
            latitude = -latitude;
            break;
        default:
            throw std::runtime_error("Invalid latitude direction '" + token + "'");
            break;
        }

        // Longitude (Token 5)
        if (!getline(nmea, token, ',')) {
            throw std::runtime_error("Can not tokenize token 5");
        }
        auto lon_degree = std::stoull(token.substr(0, 3));
        auto lon_minute = std::stod(token.substr(3));
        longitude = lon_degree + lon_minute / 60.0;

        // Longitude Direction (Token 6)
        if (!getline(nmea, token, ',')) {
            throw std::runtime_error("Can not tokenize token 6");
        }
        if (token.size() != 1) {
            throw std::runtime_error("Invalid longitude direction '" + token + "'");
        }
        switch (token[0]) {
        case 'E':
            break;
        case 'W':
            longitude = -longitude;
            break;
        default:
            throw std::runtime_error("Invalid longitude direction '" + token + "'");
            break;
        }

        // GPS Quality Indicators (Token 7)
        if (!getline(nmea, token, ',')) {
            throw std::runtime_error("Can not tokenize token 7");
        }
        if (token.size() != 1) {
            throw std::runtime_error("Invalid longitude direction '" + token + "'");
        }
        switch (token[0]) {
        case '4':
            fixed = data::NavSatFix::StatusType::FIX;
            break;
        default:
            fixed = data::NavSatFix::StatusType::NO_Fix;
            break;
        }

        // Number of satellites in use (Token 8)
        if (!getline(nmea, token, ',')) {
            throw std::runtime_error("Can not tokenize token 8");
        }

        // Horizontal dilution of precision (Token 9)
        if (!getline(nmea, token, ',')) {
            throw std::runtime_error("Can not tokenize token 9");
        }

        // Antenna altitude above/below mean sea level (Token 10)
        if (!getline(nmea, token, ',')) {
            throw std::runtime_error("Can not tokenize token 10");
        }
        altitude = std::stod(token);

        // Units of antenna altitude (M = meters) (Token 11)
        if (!getline(nmea, token, ',')) {
            throw std::runtime_error("Can not tokenize token 11");
        }
        if (token != "M") {
            throw std::runtime_error("Invalid altitude units '" + token + "'");
        }

        // Undulation - the relationship between the geoid and the WGS84 ellipsoid (Token 12)
        if (!getline(nmea, token, ',')) {
            throw std::runtime_error("Can not tokenize token 12");
        }
        altitude += std::stod(token);

        // Units of undulation (M = meters) (Token 13)
        if (!getline(nmea, token, ',')) {
            throw std::runtime_error("Can not tokenize token 13");
        }
        if (token != "M") {
            throw std::runtime_error("Invalid undulation units '" + token + "'");
        }

        // Age of correction data (in seconds) (Token 14)
        if (!getline(nmea, token, ',')) {
            throw std::runtime_error("Can not tokenize token 14");
        }

        // Differential base station ID (Token 15)
        /// @note between last tokens, separater is '*' instead of ','
        if (!getline(nmea, token, '*')) {
            throw std::runtime_error("Can not tokenize token 15");
        }

        // Check sum (*hh) (Token 16)
        /// @note detect LR
        if (!getline(nmea, token, '\r')) {
            throw std::runtime_error("Can not tokenize token 16");
        }
        if (token.size() != 2) {
            throw std::runtime_error("Can not tokenize token 16");
        }
        uint8_t reported_checksum = std::stoul(token, 0, 16);
        auto pos = nmea_sentence_str.find('*');
        uint8_t calculated_checksum = std::accumulate(nmea_sentence_str.begin() + 1,
                                                      nmea_sentence_str.begin() + pos,
                                                      uint8_t(0),
                                                      std::bit_xor<uint8_t>());

        if (reported_checksum != calculated_checksum) {
            throw std::runtime_error("Checksum failed with reported 0x" + token + " and calculated " +
                                     std::to_string(calculated_checksum));
        }

        // Copy data
        navsatfix_sensor_data->latitude = latitude;
        navsatfix_sensor_data->longitude = longitude;
        navsatfix_sensor_data->altitude = altitude;
        navsatfix_sensor_data->status.status = fixed;
    } catch (std::exception& err) {
        log::warn << "SerialSync: convert(), " << err.what() << log::endl;
        return data::SensorData::broken_data();
    }

    return sensor_data;
}

}  // namespace serialsync
}  // namespace gnss
}  // namespace device
}  // namespace hera
}  // namespace wayz
