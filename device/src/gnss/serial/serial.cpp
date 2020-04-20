///
/// @file serial.cpp
/// @author chunchen.wang (chunchen.wang@wayz.ai)
/// @brief
/// @version 0.1
/// @date 2020-04-13
///
/// Copyright 2018 Wayz.ai. All Rights Reserved.
///
///

#include "serial.hpp"

#include <cmath>
#include <functional>
#include <numeric>

namespace wayz {
namespace hera {
namespace device {
namespace gnss {
namespace serial {

const std::vector<DeviceParameterType> Serial::EssentialParameterTypes = {DeviceParameterType::Kernel,
                                                                          DeviceParameterType::BaudRate};

const std::vector<DeviceParameterType> Serial::OptionalParameterTypes = {};

auto _ = DeviceFactory::register_type({.type = DeviceVendorType::GnssSerial,
                                       .type_name = "gnss/serial",
                                       .create = &Serial::create,
                                       .do_convert = &Serial::do_convert,
                                       .essential_parameter_types = Serial::EssentialParameterTypes,
                                       .optional_parameter_types = Serial::OptionalParameterTypes,
                                       .implemented = true});

#ifdef WITH_DRIVER
HeraErrno Serial::connect()
{
    try {
        kernel_ = parameters_[DeviceParameterType::Kernel];
        baud_rate_ = stoi(parameters_[DeviceParameterType::BaudRate]);
    } catch (...) {
        return handle_error(HeraErrno::InvalidParameterValue);
    }

    serial_port_ = new driver::SerialPortSentence(kernel_, driver::SerialConfig(baud_rate_));
    if (!serial_port_->is_opened()) {
        return handle_error(HeraErrno::CanNotOpenTtyDevice, "Can not open primary device '" + kernel_ + "'");
    }
    queue_ = serial_port_->get_queue_handler();
    return HeraErrno::Success;
}

/// Free the serial port object
///
void Serial::disconnect()
{
    if (serial_port_ != nullptr) {
        delete serial_port_;
    }
}

/// Fetch data from serial port
///
data::DeviceDataPtr Serial::fetch()
{
    if (!queue_) {
        return nullptr;
    }

    // Get Rawdata from a Real Sensor
    auto nmea_sentence_str = queue_->wait_pop();
    if (nmea_sentence_str == nullptr) {
        return nullptr;
    }

    // Total length of device data
    auto nmea_sentence_length = nmea_sentence_str->size();
    auto length = sizeof(SerialNmea) + nmea_sentence_length;
    auto data = data::DeviceData::create(length,
                                         id_,
                                         DeviceVendorType::GnssSerial,
                                         DeviceDataType::GnssSerialNmea,
                                         sequence_++);
    auto derived_data = static_cast<SerialNmea*>(data.get());

    std::string nmea_std_str;
    for (auto c : *nmea_sentence_str) {
        nmea_std_str.push_back(c);
    }

    log::debug << "Received NMEA: " << nmea_std_str << log::endl;

    derived_data->data.nmea_sentence_length = nmea_sentence_length;
    memcpy(derived_data->data.nmea_sentence, nmea_sentence_str->data(), nmea_sentence_length);

    return data;
}

HeraErrno Serial::adjust_parameter(DeviceParameterType type, const std::string& value)
{
    switch (type) {
    case DeviceParameterType::Kernel:
    case DeviceParameterType::BaudRate:
        return HeraErrno::ImmutableParameter;
    default:
        return HeraErrno::UnimplementedParameter;
    }
    return HeraErrno::Success;
}
#endif

data::SensorDataPtr Serial::do_convert(data::DeviceDataPtr& storage_data)
{
    if (!storage_data->is_type(DeviceDataType::GnssSerialNmea)) {
        return data::SensorData::broken_data();
    }

    log::debug << "Serial:Convert!" << log::endl;

    // Raw DeviceData of Derived Type
    auto raw_data = static_cast<SerialNmea*>(storage_data.get());

    // Create a SensorData from DeviceData
    auto length = sizeof(data::NavSatFix);
    auto sensor_data = data::SensorData::create_from(storage_data, SensorDataType::NavSatFix, length);
    auto navsatfix_sensor_data = static_cast<data::NavSatFix*>(sensor_data.get());

    // Initialize an invalid data template
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
                    return data::SensorData::broken_data();
                default:
                    throw std::runtime_error("Can not tokenize gpts '" + token + "'");
                }
            }

            // Calculate UTC time of NMEA
            uint64_t timestamp_nmea_original =
                    hours * time::OneHour + minutes * time::OneMinute + seconds * time::OneSecond + nanoseconds;

            uint64_t days = raw_data->get_timestamp_receive_ns() / time::OneDay;
            int64_t now_residual = int64_t(raw_data->get_timestamp_receive_ns()) - days * time::OneDay;

            if (int64_t(timestamp_nmea_original) - now_residual > time::OneDay / 2) {
                days -= 1;
            } else if (int64_t(timestamp_nmea_original) - now_residual < -time::OneDay / 2) {
                days += 1;
            }

            sensor_data->timestamp_intrinsic_ns = timestamp_nmea_original + days * time::OneDay;
        } else {
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
        log::debug << "navsatfix_sensor_data->latitude: " << navsatfix_sensor_data->latitude
                   << " navsatfix_sensor_data->longitude: " << navsatfix_sensor_data->longitude
                   << " navsatfix_sensor_data->altitude " << navsatfix_sensor_data->altitude << log::endl;
    } catch (std::exception& err) {
        log::warn << "Serial: convert(), " << err.what() << log::endl;
        return data::SensorData::broken_data();
    }

    return sensor_data;
}

}  // namespace serial
}  // namespace gnss
}  // namespace device
}  // namespace hera
}  // namespace wayz
