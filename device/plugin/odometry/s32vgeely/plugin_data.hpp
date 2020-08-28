///
/// @file plugin_data.hpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief
/// @date 2020-08-19
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include "data/odometry_data.hpp"

namespace wayz {
namespace hera {
namespace device {
namespace odometry {
namespace s32vgeely {

#pragma pack(push, 1)

///
/// @brief Device data for S32VGeely Series Car, Derived from Storage Data
///
class S32VGeelyData final : public data::DeviceData {
public:
    S32VGeelyData() = delete;

public:
    struct S32VGeelyCANPacket {
        uint64_t timestamp_ns;  ///< Timestamp given by can driver
        uint32_t id_can;        ///< CAN ID (Arbitration Id) of the message sender.
        uint16_t dlc_can;       ///< CAN DLC, number of bytes of the payload.
        uint8_t data_can[0];    ///< CAN Packet payload.
    };

    ///
    /// @brief Union to allow access by data or by bytes
    ///
    union {
        S32VGeelyCANPacket data;                  ///< union entry: data with structure
        uint8_t buf[sizeof(S32VGeelyCANPacket)];  ///< union entry: raw buffer of bytes
    };
};

#pragma pack(pop)

}  // namespace s32vgeely
}  // namespace odometry
}  // namespace device
}  // namespace hera
}  // namespace wayz