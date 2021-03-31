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
namespace serial {

#pragma pack(push, 1)

///
/// @brief Device data for Serial's Serial Data containing NMEA Sentence, etc.
///
class SerialNmea final : public data::DeviceData {
public:
    SerialNmea() = delete;

public:
    ///
    /// @brief Data structure of NMEA Sentence packet
    ///
    struct SerialNmeaUnion {
        uint32_t nmea_sentence_length;  ///< length of nmea_sentence, in bytes
        uint8_t nmea_sentence[];        /// NMEA Sentence, (usually) without trailing <LF>
    };

    ///
    /// @brief Union to allow access by data or by bytes
    ///
    union {
        SerialNmeaUnion data;                  ///< union entry: data with structure
        uint8_t buf[sizeof(SerialNmeaUnion)];  ///< union entry: raw buffer of bytes
    };
};

#pragma pack(pop)

}  // namespace serial
}  // namespace gnss
}  // namespace device
}  // namespace hera
}  // namespace wayz