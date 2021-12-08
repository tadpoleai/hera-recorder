///
/// @file plugin_entry.cpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief
/// @date 2020-08-19
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
namespace serialsync {

///
/// @brief A GNSS-Devic outputs by serial port
///
/// A high-performanced (often, RTK) GNSS-Device that outputs NMEA Sentences through serial port
/// connected directly to PC, without a method to synchronize its time::Timestamp since the device does
/// not provide a PPS pin.
///
/// To solve this issue, A low-performanced, auxiliary GNSS-Device connected to "Wayz Tron Sync
/// Board", in which, NMEA Sentences outputed by auxiliary GNSS-Device is parsed and the reference
/// time-shiftation between auxiliary GNSS-Device and "Sync Board" is calculated by MCU on "Sync
/// Board", and then send by serial_transport with msgtype 1.
///
/// By parsing the NMEA Sentences of primary GNSS-Device and add the time-shiftation, the accurate
/// time::Timestamp of primary GNSS-Device is obtained
///
/// @see <a href="https://www.gpsinformation.org/dale/nmea.htm" target="_blank"
/// rel="noopener noreferrer">NMEA data</a>
HERA_PLUGIN_DEFINE_START(1)

#ifdef WITH_DRIVER
HERA_PLUGIN_DEFINE_FUNCTIONS

driver::SerialPortSentence* serial_port_{nullptr};                   ///< for receiving nmea data
driver::SerialTransport* serial_port_auxiliary_{nullptr};            ///< for receiving time shiftation data
common::ThreadQueue<driver::SerialData>* queue_{nullptr};            ///< queue of nmea data
common::ThreadQueue<driver::SerialData>* queue_auxiliary_{nullptr};  ///< queue of serial time shiftation data

///
/// @brief The time shiftation value, in ns
///
/// time::Timestamp shiftation of "Tron Sync Board"'s instrinsic time
/// (which is referenced by all-other sensors),
/// in reference to GNSS's originial time (e.g., GPST).
///
/// Here, we assume that the originial time of the primary GNSS
/// and the auxiliary GNSS are the same.
///
/// Therefore, to obtain the primary GNSS's time in reference to
/// "Tron Sync Board"'s instrinsic time, we should add this
/// shifation_value_ns_ to primary GNSS's originial time
int64_t shifation_value_ns_;

///
/// @brief The timestamp when shifation_value_ns_ received
///
/// This value is should be compared with Timestamp::now()
/// before calculation of primary GNSS's time, to ensure
/// we are using a fresh value of shifation_value_ns_
time::Timestamp shiftation_timestamp_;

#endif

HERA_PLUGIN_DEFINE_END

HERA_PLUGIN_EXPORT(GnssSerialsync, "gnss/serialsync")

#ifdef WITH_DRIVER
HeraErrno DevicePlugin::connect()
{
    serial_port_ = new driver::SerialPortSentence(local_parameters_.get_Primary(),
                                                  driver::SerialConfig(local_parameters_.get_BaudPrimary()));
    if (!serial_port_->is_opened()) {
        return handle_error(HeraErrno::CanNotOpenTtyDevice,
                            "Can not open primary device '" + local_parameters_.get_Primary() + "'");
    }
    queue_ = serial_port_->get_queue_handler();

    serial_port_auxiliary_ = driver::SerialTransport::create(local_parameters_.get_Auxiliary(),
                                                             driver::SerialConfig(local_parameters_.get_BaudAux()));
    if (!serial_port_auxiliary_->is_opened()) {
        return handle_error(HeraErrno::CanNotOpenTtyDevice,
                            "Can not open auxiliary device '" + local_parameters_.get_Auxiliary() + "'");
    }
    queue_auxiliary_ = serial_port_auxiliary_->get_queue_handler(local_parameters_.get_MsgType());
    if (!queue_auxiliary_) {
        return handle_error(HeraErrno::CanNotOpenTtyDevice, "Can not register listener");
    }

    shiftation_timestamp_ = 0;
    shifation_value_ns_ = 0;

    return HeraErrno::Success;
}

/// Free the serial port object
///
void DevicePlugin::disconnect()
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
data::DeviceDataPtr DevicePlugin::fetch()
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
        // Only accept NMEA::GPGGA / GNGGA
        /// @see https://www.gpsinformation.org/dale/nmea.htm#GGA
        if (!getline(nmea, token, ',') || token.size() < 6) {
            throw std::runtime_error("got non-gga sentence '" + token + "'");
        }

        // Get original time of NMEA (hhmmss)
        if (!getline(nmea, token, ',')) {
            throw std::runtime_error("Can not tokenize 2nd token");
        }

        // Valid time info (No signal from satellites)
        if (token.size() < 6) {
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

            constexpr time::Duration ShifationDelayTolerance = 10 * time::OneSecond;
            if (now - shiftation_timestamp_ < ShifationDelayTolerance) {
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

HeraErrno DevicePlugin::adjust_parameter(const std::string& type, const std::string& value)
{
    return HeraErrno::OK;
}
#endif

data::SensorDataPtr DevicePlugin::do_convert(const data::DeviceDataPtr& storage_data,
                                             const ParametersInterface* parameters)
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
    navsatfix_sensor_data->latitude = 0;
    navsatfix_sensor_data->longitude = 0;
    navsatfix_sensor_data->altitude = 0;
    navsatfix_sensor_data->num_satellites = 0;

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
        int32_t num_satellites = 0;

        // Tokenize nmea sentence
        std::string token;
        std::string nmea_sentence_str =
                std::string((char*)raw_data->data.nmea_sentence, raw_data->data.nmea_sentence_length);
        std::stringstream nmea(nmea_sentence_str);

        // Sentence Identifier (Token 1)
        if (!getline(nmea, token, ',') || token.size() < 6) {
            throw std::runtime_error("Can not tokenize token 1");
        }
        // Only accept NMEA::GPGGA / GNGGA
        /// @see https://www.gpsinformation.org/dale/nmea.htm#GGA
        if (token != "$GPGGA" && token != "$GNGGA") {
            throw std::runtime_error("got non-gga sentence '" + token + "'");
        }

        // UTC time status of position (hhmmss) (Token 2)
        if (!getline(nmea, token, ',')) {
            throw std::runtime_error("Can not tokenize token 2");
        }

        // Valid time info
        if (token.size() < 6) {
            throw std::runtime_error("Can not tokenize gpts '" + token + "'");
        }

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
        uint64_t days = raw_data->data.timestamp_intrinsic_ns / time::OneDay;
        int64_t now_residual = int64_t(raw_data->data.timestamp_intrinsic_ns) - days * time::OneDay;

        if (int64_t(timestamp_nmea_original) - now_residual > time::OneDay / 2) {
            days -= 1;
        } else if (int64_t(timestamp_nmea_original) - now_residual < -time::OneDay / 2) {
            days += 1;
        }

        timestamp_nmea_original += days * time::OneDay;

        log::debug << "SerialSync: TRON Time(t_i): " << raw_data->data.timestamp_intrinsic_ns
                   << " GPS Time(T): " << timestamp_nmea_original << ", δ(t_i - T): "
                   << ((int64_t)raw_data->data.timestamp_intrinsic_ns - (int64_t)timestamp_nmea_original) << log::endl;

        // Latitude (Token 3)
        if (!getline(nmea, token, ',')) {
            throw std::runtime_error("Can not tokenize token 3");
        }
        if (token.size() < 4) {
            throw std::runtime_error("Invalid latitude number '" + token + "'");
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
        if (token.size() < 5) {
            throw std::runtime_error("Invalid longitude number '" + token + "'");
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
            fixed = data::NavSatFix::StatusType::GBAS_FIX;
            break;
        case '5':
            fixed = data::NavSatFix::StatusType::SBAS_Fix;
            break;
        case '0':
            fixed = data::NavSatFix::StatusType::NO_Fix;
            break;
        default:
            fixed = data::NavSatFix::StatusType::FIX;
            break;
        }

        // Number of satellites in use (Token 8)
        if (!getline(nmea, token, ',')) {
            throw std::runtime_error("Can not tokenize token 8");
        }
        if (token.size() < 1) {
            throw std::runtime_error("Invalid num satellites '" + token + "'");
        }
        num_satellites = std::stol(token);

        // Horizontal dilution of precision (Token 9)
        if (!getline(nmea, token, ',')) {
            throw std::runtime_error("Can not tokenize token 9");
        }

        // Antenna altitude above/below mean sea level (Token 10)
        if (!getline(nmea, token, ',') || token.size() < 1) {
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
        if (!getline(nmea, token, ',') || token.size() < 1) {
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
        navsatfix_sensor_data->num_satellites = num_satellites;
    } catch (std::exception& err) {
        // log::warn << "SerialSync: convert(), " << err.what() << log::endl;
        return sensor_data;
    }

    return sensor_data;
}

}  // namespace serialsync
}  // namespace gnss
}  // namespace device
}  // namespace hera
}  // namespace wayz
