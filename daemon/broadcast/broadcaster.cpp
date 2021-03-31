///
/// @file broadcast.cpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief
/// @date 2020-05-27
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///
///

#include "broadcaster.hpp"

#include <errno.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "common/include/logger/logger.hpp"
#include "common/include/utils/time.hpp"
#include "common/include/version.hpp"
#include "device/include/version.hpp"
#include "storage/include/version.hpp"
//
#include "broadcaster.hpp"
#include "message.hpp"

namespace wayz {
namespace hera {
namespace daemon {

Broadcaster::Broadcaster(const std::string& name,
                         const std::vector<std::string>& ifs,
                         const bool whilelist_mode,
                         const std::string& protocol,
                         const std::string& parameter,
                         const bool is_upload_server) :
    name_(name),
    version_(common::get_version()),
    protocol_(protocol),
    parameter_(parameter),
    ifs_(ifs),
    whitelist_mode_(whilelist_mode),
    port_(is_upload_server ? UploadServerInfoBroadcastPort : BroadcastPort),
    is_upload_server_(is_upload_server),
    running_(true),
    thread_(new std::thread(&Broadcaster::thread_function, this))
{}

Broadcaster::~Broadcaster()
{
    running_ = false;
    thread_->join();
}

void Broadcaster::thread_function()
{
    log::debug << "Broadcaster: Start running" << log::endl;

    int socket_broadcast = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_broadcast < 0) {
        log::warn << "Broadcaster: Error, can not open socket" << log::endl;
        return;
    }
    int yes = 1;
    if (setsockopt(socket_broadcast, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes)) < 0) {
        log::warn << "Broadcaster: Error, can not setsockopt" << log::endl;
        return;
    }

    while (running_) {
        struct ifaddrs* ifList;
        if (getifaddrs(&ifList) < 0) {
            log::warn << "Broadcaster: Error, can not get ifs" << log::endl;
            continue;
        }

        for (auto ifa = ifList; ifa != nullptr; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr->sa_family != AF_INET) {
                continue;
            }

            bool find = false;
            std::string ifa_name(ifa->ifa_name);
            for (const auto if_name : ifs_) {
                find = (if_name == ifa_name);
                if (find) {
                    break;
                }
            }
            if (find != whitelist_mode_) {
                continue;
            }

            struct sockaddr_in b_addr;
            b_addr.sin_family = AF_INET;
            auto sin = (struct sockaddr_in*)ifa->ifa_dstaddr;
            b_addr.sin_addr.s_addr = sin->sin_addr.s_addr;
            b_addr.sin_port = htons(port_);
            sin = (struct sockaddr_in*)ifa->ifa_addr;
            auto addr = sin->sin_addr.s_addr;

            std::string buff;
            if (!is_upload_server_) {
                BroadcastPacket packet;
                packet.addr = addr;
                packet.name = name_;
                packet.version = version_;
                buff = packet.to_buff();
            } else {
                UploadServerInfoPacket packet;
                packet.addr = addr;
                packet.name = name_;
                packet.protocol = protocol_;
                packet.parameter = parameter_;
                buff = packet.to_buff();
            }

            ::sendto(socket_broadcast, buff.data(), buff.size(), 0, (struct sockaddr*)&b_addr, sizeof(b_addr));
        }
        freeifaddrs(ifList);

        static constexpr auto SleepSingleTimeUs = 100000;
        static constexpr auto SleepTimes = 50;
        /// Sleep for 100ms * 50
        for (size_t i = 0; i < SleepTimes; ++i) {
            if (!running_) {
                break;
            }
            usleep(SleepSingleTimeUs);
        }
    }

    log::debug << "Broadcaster: Stop running" << log::endl;
}

}  // namespace daemon
}  // namespace hera
}  // namespace wayz
