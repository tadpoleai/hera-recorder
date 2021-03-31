///
/// @file plugin_data.hpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief
/// @date 2021-01-12
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include "data/gnss_data.hpp"

namespace wayz {
namespace hera {
namespace device {
namespace gnss {
namespace serialremotesync {

#pragma pack(push, 1)

///
/// @brief Device data for SerialRemotesync's Serial Data containing NMEA Sentence, etc.
///
class SerialRemotesyncNmea final : public data::DeviceData {
public:
    SerialRemotesyncNmea() = delete;

public:
    struct SerialRemotesyncNmeaUnion {
        uint8_t timestamp_valid;       ///< boolean, indicating if timestamp_intrinsic_ns is valid
        uint64_t timestamp_ns;         ///< valid timestamp_ns in reference time coordinatem, if timestamp_valid is true
        uint8_t nmea_sentence_length;  ///< length of nmea_sentence, in bytes
        uint8_t nmea_sentence[];       ///< NMEA Sentence, full
    };

    ///
    /// @brief Union to allow access by data or by bytes
    ///
    union {
        SerialRemotesyncNmeaUnion data;                  ///< union entry: data with structure
        uint8_t buf[sizeof(SerialRemotesyncNmeaUnion)];  ///< union entry: raw buffer of bytes
    };
};

#pragma pack(pop)

}  // namespace serialremotesync
}  // namespace gnss
}  // namespace device
}  // namespace hera
}  // namespace wayz