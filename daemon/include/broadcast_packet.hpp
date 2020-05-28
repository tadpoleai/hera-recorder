///
/// @file broadcast_packet.hpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief Broadcasted packet
/// @date 2020-05-27
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include <string>
#include <thread>
#include <vector>

namespace wayz {
namespace hera {
namespace daemon {

#pragma pack(push, 1)

///
/// @brief SensorData for Imu and MagneticField composed
///
struct BroadcastPacket {
public:
    uint32_t addr;
    uint32_t name_len;
    uint32_t version_len;
    char message_start[0];
};

static constexpr uint16_t BroadCastPort = 10639;  ///< Port number of broadcasting

#pragma pack(pop)

}  // namespace daemon
}  // namespace hera
}  // namespace wayz