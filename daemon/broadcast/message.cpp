///
/// @file message.cpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief
/// @date 2020-08-28
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include "message.hpp"

#include <cstring>
#include <iostream>
#include <memory>

#include <arpa/inet.h>

namespace wayz {
namespace hera {
namespace daemon {

std::string BroadcastPacket::to_buff()
{
    std::string buff;
    buff.resize(12 + name.size() + version.size());
    *(uint32_t*)((char*)buff.data()) = addr;
    *(uint32_t*)((char*)buff.data() + 4) = name.size();
    *(uint32_t*)((char*)buff.data() + 8) = version.size();

    memcpy((char*)buff.data() + 12, (char*)name.data(), name.size());
    memcpy((char*)buff.data() + 12 + name.size(), (char*)version.data(), version.size());

    return buff;
}

bool BroadcastPacket::from_buff(const std::string& buff, BroadcastPacket& data)
{
    if (buff.size() < 12) {
        return false;
    }

    auto addr = *(uint32_t*)((char*)buff.data());
    auto name_size = *(uint32_t*)((char*)buff.data() + 4);
    auto version_size = *(uint32_t*)((char*)buff.data() + 8);

    if (buff.size() < 12 + name_size + version_size) {
        std::cout << "?????";
        return false;
    }

    data.addr = addr;
    data.name.resize(name_size);
    data.version.resize(version_size);

    memcpy((char*)data.name.data(), (char*)buff.data() + 12, name_size);
    memcpy((char*)data.version.data(), (char*)buff.data() + 12 + name_size, version_size);

    char addr_buff[128];
    inet_ntop(AF_INET, &addr, addr_buff, sizeof(addr_buff));
    data.addr_string = addr_buff;

    return true;
}

std::string UploadServerInfoPacket::to_buff()
{
    std::string buff;
    buff.resize(16 + name.size() + protocol.size() + parameter.size());
    *(uint32_t*)((char*)buff.data()) = addr;
    *(uint32_t*)((char*)buff.data() + 4) = name.size();
    *(uint32_t*)((char*)buff.data() + 8) = protocol.size();
    *(uint32_t*)((char*)buff.data() + 12) = parameter.size();

    memcpy((char*)buff.data() + 16, (char*)name.data(), name.size());
    memcpy((char*)buff.data() + 16 + name.size(), (char*)protocol.data(), protocol.size());
    memcpy((char*)buff.data() + 16 + name.size() + protocol.size(), (char*)parameter.data(), parameter.size());

    return buff;
}

bool UploadServerInfoPacket::from_buff(const std::string& buff, UploadServerInfoPacket& data)
{
    if (buff.size() < 16) {
        return false;
    }

    auto addr = *(uint32_t*)((char*)buff.data());
    auto name_size = *(uint32_t*)((char*)buff.data() + 4);
    auto protocol_size = *(uint32_t*)((char*)buff.data() + 8);
    auto parameter_size = *(uint32_t*)((char*)buff.data() + 12);

    if (buff.size() < 16 + name_size + protocol_size + parameter_size) {
        return false;
    }

    data.addr = addr;
    data.name.resize(name_size);
    data.protocol.resize(protocol_size);
    data.parameter.resize(parameter_size);

    memcpy((char*)data.name.data(), (char*)buff.data() + 16, name_size);
    memcpy((char*)data.protocol.data(), (char*)buff.data() + 16 + name_size, protocol_size);
    memcpy((char*)data.parameter.data(), (char*)buff.data() + 16 + name_size + protocol_size, parameter_size);

    char addr_buff[128];
    inet_ntop(AF_INET, &addr, addr_buff, sizeof(addr_buff));
    data.addr_string = addr_buff;

    return true;
}

}  // namespace daemon
}  // namespace hera
}  // namespace wayz