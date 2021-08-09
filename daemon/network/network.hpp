///
/// @file network.hpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief
/// @version 0.1
/// @date 2021-08-02
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include <stdint.h>
#include <string>
#include <vector>

namespace wayz {
namespace hera {
namespace daemon {

class IpV4Address {
public:
    IpV4Address(const uint32_t& address);

    IpV4Address(const std::string& address);

    operator uint32_t() const;

    operator std::string() const;

private:
    uint32_t s_addr;
};

struct Interface {
    std::string name;
    uint32_t childnum;
    IpV4Address address;
    IpV4Address netmask;
    IpV4Address broadcast;
    std::vector<Interface> children;
};

class Network final {
public:
    explicit Network(const std::vector<std::string>& excludes) : excludes_(excludes) {}

    const std::vector<Interface>& create(const std::string& ifBaseName,
                                         IpV4Address address,
                                         IpV4Address netmask = 0x00'FF'FF'FF);

    const std::vector<Interface>& retrieve();

    const std::vector<Interface>& update(const std::string& ifChildName,
                                         IpV4Address address,
                                         IpV4Address netmask = 0x00'FF'FF'FF);

    const std::vector<Interface>& drop(const std::string& ifChildName);

private:
    const std::vector<std::string> excludes_;

    std::vector<Interface> interfaces_;
};

}  // namespace daemon
}  // namespace hera
}  // namespace wayz
