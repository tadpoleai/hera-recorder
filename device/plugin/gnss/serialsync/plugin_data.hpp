///
/// @file plugin_data.hpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief
/// @date 2020-08-19
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include "data/gnss_data.hpp"

namespace wayz {
namespace hera {
namespace device {
namespace gnss {
namespace serialsync {

#pragma pack(push, 1)

///
/// @brief Device data for Serialsync's Serial Data containing NMEA Sentence, etc.
///
class SerialSyncNmea final : public data::DeviceData {
public:
    SerialSyncNmea() = delete;

public:
    ///
    /// @brief Data structure of NMEA Sentence packet
    ///
    struct SerialSyncNmeaUnion {
        ///
        /// @brief time::Timestamp of message, UTC, in ns
        ///
        /// Obtained by calculating from NMEA Sentence and time shiftation
        ///
        uint64_t timestamp_intrinsic_ns;
        uint8_t timestamp_valid;        ///< boolean, indicating if timestamp_intrinsic_ns is valid
        uint32_t nmea_sentence_length;  ///< length of nmea_sentence, in bytes
        uint8_t nmea_sentence[];        /// NMEA Sentence, (usually) without trailing <LF>
    };

    ///
    /// @brief Union to allow access by data or by bytes
    ///
    union {
        SerialSyncNmeaUnion data;                  ///< union entry: data with structure
        uint8_t buf[sizeof(SerialSyncNmeaUnion)];  ///< union entry: raw buffer of bytes
    };
};

#pragma pack(pop)

}  // namespace serialsync
}  // namespace gnss
}  // namespace device
}  // namespace hera
}  // namespace wayz