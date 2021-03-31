///
/// @file message.hpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief
/// @date 2020-08-28
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include <string>

namespace wayz {
namespace hera {
namespace daemon {

class BroadcastPacket {
public:
    std::string to_buff();

    static bool from_buff(const std::string& buff, BroadcastPacket& data);

public:
    uint32_t addr;
    std::string addr_string;
    std::string name;
    std::string version;
};

struct UploadServerInfoPacket {
public:
    std::string to_buff();

    static bool from_buff(const std::string& buff, UploadServerInfoPacket& data);

public:
    uint32_t addr;
    std::string addr_string;
    std::string name;
    std::string protocol;
    std::string parameter;
};

static constexpr uint16_t BroadcastPort = 10639;                  ///< Port number of broadcasting of hera-daemon
static constexpr uint16_t UploadServerInfoBroadcastPort = 10640;  ///< Port number of upload server

}  // namespace daemon
}  // namespace hera
}  // namespace wayz