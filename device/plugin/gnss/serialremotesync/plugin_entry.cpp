///
/// @file plugin_entry.cpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief
/// @date 2021-01-12
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include <chrono>
#include <cmath>
#include <cstdlib>

#include "plugin_common.hpp"
#include "plugin_data.hpp"
#include "plugin_param.hpp"

#ifdef WITH_DRIVER
#include "driver/serial/serial_port_sentence.hpp"
#include "driver/serial/serial_transport.hpp"
#endif

namespace wayz {
namespace hera {
namespace device {
namespace gnss {
namespace serialremotesync {

///
/// @brief A GNSS-Devic outputs by serial port
///
/// A single GNSS-Device that outputs NMEA Sentences through serial port connected to Sync Board,
/// with its PPS output, Sync Board can then catch PPS signal and shift packet time to its reference
///
/// @see <a href="https://www.gpsinformation.org/dale/nmea.htm" target="_blank"
/// rel="noopener noreferrer">NMEA data</a>
HERA_PLUGIN_DEFINE_START(1)

#ifdef WITH_DRIVER
HERA_PLUGIN_DEFINE_FUNCTIONS

driver::SerialTransport* serial_port_{nullptr};            ///< for receiving nmea data
common::ThreadQueue<driver::SerialData>* queue_{nullptr};  ///< queue of nmea data

#endif

HERA_PLUGIN_DEFINE_END

HERA_PLUGIN_EXPORT(GnssSerialRemotesync, "gnss/serialremotesync")

#ifdef WITH_DRIVER
HeraErrno DevicePlugin::connect()
{
    serial_port_ = driver::SerialTransport::create(local_parameters_.get_Kernel(),
                                                   driver::SerialConfig(local_parameters_.get_Baud()));
    if (!serial_port_->is_opened()) {
        return handle_error(HeraErrno::CanNotOpenTtyDevice,
                            "Can not open device '" + local_parameters_.get_Kernel() + "'");
    }
    queue_ = serial_port_->get_queue_handler(local_parameters_.get_MsgType());
    if (!queue_) {
        return handle_error(HeraErrno::CanNotOpenTtyDevice, "Can not register listener");
    }

    return HeraErrno::Success;
}

/// Free the serial port object
///
void DevicePlugin::disconnect()
{
    if (serial_port_ != nullptr) {
        serial_port_->free();
    }
}

/// Fetch data from serial port
///
data::DeviceDataPtr DevicePlugin::fetch()
{
    if (!queue_) {
        return nullptr;
    }

    // Get Timestamp shiftation from queue_auxiliary
    auto serial_data_str = queue_->wait_pop();
    if (serial_data_str == nullptr) {
        return nullptr;
    }

    // Total length of device data
    auto data_length = serial_data_str->size();
    auto total_length = sizeof(data::DeviceData) + data_length;
    auto data = data::DeviceData::create(total_length,
                                         id_,
                                         DeviceVendorType::GnssSerialRemotesync,
                                         DeviceDataType::GnssSerialRemotesyncNmea,
                                         sequence_++);
    auto derived_data = static_cast<SerialRemotesyncNmea*>(data.get());

    // Copy data
    /// @todo calculate time
    memcpy((void*)&derived_data->data, serial_data_str->data(), data_length);

    auto logger = log::info << "Received: Data = [";

    auto nmea_data = (SerialRemotesyncNmea::SerialRemotesyncNmeaUnion*)(void*)serial_data_str->data();

    if (nmea_data->timestamp_valid) {
        logger << "Valid, ";
    } else {
        logger << "Invalid, ";
    }

    logger << "Time =" << time::Timestamp(nmea_data->timestamp_ns) << ", Sentence = ";

    for (size_t i = 0; i < nmea_data->nmea_sentence_length; i++) {
        logger << nmea_data->nmea_sentence[i];
    }

    logger << log::endl;

    return data;
}

HeraErrno DevicePlugin::adjust_parameter(const std::string& type, const std::string& value)
{
    return HeraErrno::OK;
}
#endif

data::SensorDataPtr DevicePlugin::do_convert(const data::DeviceDataPtr& storage_data,
                                             const ParametersInterface* parameters)
{
    if (!storage_data->is_type(DeviceDataType::GnssSerialRemotesyncNmea)) {
        return data::SensorData::broken_data();
    }

    // Raw DeviceData of Derived Type
    auto raw_data = static_cast<SerialRemotesyncNmea*>(storage_data.get());

    // Create a SensorData from DeviceData
    auto length = sizeof(data::NavSatFix);
    auto sensor_data = data::SensorData::create_from(storage_data, SensorDataType::NavSatFix, length);
    auto navsatfix_sensor_data = static_cast<data::NavSatFix*>(sensor_data.get());

    // Initialize an invalid data template
    navsatfix_sensor_data->timestamp_intrinsic_ns = raw_data->data.timestamp_ns;
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
        // log::warn << "Timestamp is not valid!" << log::endl;
        return sensor_data;
    } else {
        // log::info << "Timestamp is valid!" << log::endl;
    }

    // std::string nmea_sentence_str =
    //         std::string((char*)raw_data->data.nmea_sentence, raw_data->data.nmea_sentence_length);
    // log::debug << "Sentence = '" << nmea_sentence_str << "'" << log::endl;

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

        log::debug << "Sentence = '" << nmea_sentence_str << "'" << log::endl;

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

        // // Check sum (*hh) (Token 16)
        // /// @note detect LR
        // if (!getline(nmea, token, '\r')) {
        //     throw std::runtime_error("Can not tokenize token 16");
        // }
        // if (token.size() != 2) {
        //     throw std::runtime_error("Can not tokenize token 16");
        // }
        // uint8_t reported_checksum = std::stoul(token, 0, 16);
        // auto pos = nmea_sentence_str.find('*');
        // uint8_t calculated_checksum = std::accumulate(nmea_sentence_str.begin() + 1,
        //                                               nmea_sentence_str.begin() + pos,
        //                                               uint8_t(0),
        //                                               std::bit_xor<uint8_t>());

        // if (reported_checksum != calculated_checksum) {
        //     throw std::runtime_error("Checksum failed with reported 0x" + token + " and calculated " +
        //                              std::to_string(calculated_checksum));
        // }

        // Copy data
        navsatfix_sensor_data->latitude = latitude;
        navsatfix_sensor_data->longitude = longitude;
        navsatfix_sensor_data->altitude = altitude;
        navsatfix_sensor_data->status.status = fixed;
    } catch (std::exception& err) {
        log::warn << "SerialRemoteSync: error, convert(), " << err.what() << log::endl;
        return data::SensorData::broken_data();
    }

    return sensor_data;
}

}  // namespace serialremotesync
}  // namespace gnss
}  // namespace device
}  // namespace hera
}  // namespace wayz
