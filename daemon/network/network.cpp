///
/// @file network.cpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief
/// @version 0.1
/// @date 2021-08-02
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include "network/network.hpp"

#include <ifaddrs.h>
#include <iostream>
#include <unistd.h>
#include <vector>

#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>

#include "common/include/logger/logger.hpp"

namespace wayz {
namespace hera {
namespace daemon {

IpV4Address::IpV4Address(const uint32_t& address)
{
    s_addr = address;
}

IpV4Address::IpV4Address(const std::string& address)
{
    inet_aton(address.c_str(), (struct in_addr*)(&s_addr));
}

IpV4Address::operator uint32_t() const
{
    return s_addr;
}

IpV4Address::operator std::string() const
{
    return inet_ntoa(*(struct in_addr*)(&s_addr));
}

const std::vector<Interface>& Network::retrieve()
{
    interfaces_.clear();

    struct ifaddrs* ifList;
    if (getifaddrs(&ifList) < 0) {
        return interfaces_;
    }

    for (auto ifa = ifList; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr->sa_family != AF_INET) {
            continue;
        }

        if (ifa->ifa_flags & IFF_LOOPBACK) {
            continue;
        }

        std::string ifaName(ifa->ifa_name);

        for (const auto& exclude : excludes_) {
            if (ifaName == exclude) {
                continue;
            }
        }

        auto pos = ifaName.find(':');
        if (pos == ifaName.npos) {
            interfaces_.push_back({ifaName,
                                   {},
                                   ((struct sockaddr_in*)ifa->ifa_addr)->sin_addr.s_addr,
                                   ((struct sockaddr_in*)ifa->ifa_netmask)->sin_addr.s_addr,
                                   ((struct sockaddr_in*)ifa->ifa_ifu.ifu_broadaddr)->sin_addr.s_addr,
                                   {}});
        } else {
            std::string ifaNameBase = ifaName.substr(0, pos);
            uint32_t childnum = std::atoi(ifaName.substr(pos + 1).c_str());

            for (auto&& ifBase : interfaces_) {
                if (ifBase.name == ifaNameBase) {
                    ifBase.children.push_back({ifaName,
                                               childnum,
                                               ((struct sockaddr_in*)ifa->ifa_addr)->sin_addr.s_addr,
                                               ((struct sockaddr_in*)ifa->ifa_netmask)->sin_addr.s_addr,
                                               ((struct sockaddr_in*)ifa->ifa_ifu.ifu_broadaddr)->sin_addr.s_addr,
                                               {}});
                    break;
                }
            }
        }
    }

    freeifaddrs(ifList);
    return interfaces_;
}

const std::vector<Interface>& Network::create(const std::string& ifBaseName, IpV4Address address, IpV4Address netmask)
{
    (void)retrieve();

    uint32_t childnum = 1;
    for (auto&& ifBase : interfaces_) {
        if (ifBase.name == ifBaseName) {
            for (auto&& ifChild : ifBase.children) {
                if (ifChild.childnum >= childnum) {
                    childnum++;
                }
            }
            break;
        }
        return interfaces_;
    }

    std::string cmd = "'ifconfig' " + ifBaseName + ':' + std::to_string(childnum) + ' ' + std::string(address) +
                      " netmask " + std::string(netmask) + " up";


    auto ret = system(cmd.c_str());
    if (ret) {
        log::warn << "Network: executing " << cmd << " failed!" << log::endl;
    } else {
        log::info << "Network: executing " << cmd << " succeed!" << log::endl;
    }

    return retrieve();
}

const std::vector<Interface>& Network::update(const std::string& ifChildName, IpV4Address address, IpV4Address netmask)
{
    auto pos = ifChildName.find(':');
    if (pos != ifChildName.npos) {
        std::string cmd =
                "'ifconfig' " + ifChildName + ' ' + std::string(address) + " netmask " + std::string(netmask) + " up";
        auto ret = system(cmd.c_str());
        if (ret) {
            log::warn << "Network: executing " << cmd << " failed!" << log::endl;
        } else {
            log::info << "Network: executing " << cmd << " succeed!" << log::endl;
        }
    }

    return retrieve();
}

const std::vector<Interface>& Network::drop(const std::string& ifChildName)
{
    auto pos = ifChildName.find(':');
    if (pos != ifChildName.npos) {
        std::string cmd = "'ifconfig' " + ifChildName + " down";
        auto ret = system(cmd.c_str());
        if (ret) {
            log::warn << "Network: executing " << cmd << " failed!" << log::endl;
        } else {
            log::info << "Network: executing " << cmd << " succeed!" << log::endl;
        }
    }

    return retrieve();
}

}  // namespace daemon
}  // namespace hera
}  // namespace wayz
