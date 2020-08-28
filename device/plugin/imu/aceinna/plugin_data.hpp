///
/// @file plugin_data.hpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief
/// @date 2020-08-19
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include "data/imu_data.hpp"

namespace wayz {
namespace hera {
namespace device {
namespace imu {
namespace aceinna {

#pragma pack(push, 1)

///
/// @brief Device data for Aceinna 9-axis Imu, Derived from Storage Data
///
class AceinnaData final : public data::DeviceData {
public:
    AceinnaData() = delete;

public:
    ///
    /// @brief Data structure of one packet of Aceinna
    ///
    /// The packet is sent by Wayz Tron Sync Board's serial output, on SerialMsgType  0
    ///
    struct AceinnaDataUnion {
        uint64_t timestamp;   ///< timestamp of 'DataReady' pin's falling edge, UTC, in ns
        int16_t gyro[3];      ///< array of Gyro raw data
        int16_t accel[3];     ///< array of Accel raw data
        int16_t magnetic[3];  ///< array of Magnetic raw data
    };

    ///
    /// @brief Union to allow access by data or by bytes
    ///
    union {
        AceinnaDataUnion data;                  ///< union entry: data with structure
        uint8_t buf[sizeof(AceinnaDataUnion)];  ///< union entry: raw buffer of bytes
    };
};

#pragma pack(pop)

}  // namespace aceinna
}  // namespace imu
}  // namespace device
}  // namespace hera
}  // namespace wayz