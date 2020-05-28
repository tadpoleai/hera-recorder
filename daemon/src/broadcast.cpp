///
/// @file broadcast.cpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief
/// @date 2020-05-27
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///
///

#include "broadcast.hpp"

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

#include "broadcast_packet.hpp"
#include "common/include/logger/logger.hpp"
#include "common/include/utils/time.hpp"
#include "common/include/version.hpp"
#include "device/include/version.hpp"
#include "storage/include/version.hpp"

namespace wayz {
namespace hera {
namespace daemon {

Broadcast::Broadcast(const std::string& name, const std::vector<std::string>& ifs, const bool whilelist_mode) :
    name_(name),
    version_("\n  Common: " + common::get_version() + "\n  Device: " + device::get_version() +
             "\n  storage: " + storage::get_version()),
    ifs_(ifs),
    whitelist_mode_(whilelist_mode),
    running_(true),
    thread_(new std::thread(&Broadcast::thread_function, this))
{}

Broadcast::~Broadcast()
{
    running_ = false;
    thread_->join();
}

void Broadcast::thread_function()
{
    log::debug << "Broadcast: Start running" << log::endl;

    int socket_broadcast = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_broadcast < 0) {
        log::warn << "Broadcast: Error, can not open socket" << log::endl;
        return;
    }
    int yes = 1;
    if (setsockopt(socket_broadcast, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes)) < 0) {
        log::warn << "Broadcast: Error, can not setsockopt" << log::endl;
        return;
    }

    auto packet_length = sizeof(BroadcastPacket) + name_.size() + version_.size();
    char* buf = new char[packet_length];
    BroadcastPacket* packet = (BroadcastPacket*)(void*)(buf);
    packet->name_len = name_.size();
    packet->version_len = version_.size();
    memcpy(packet->message_start, name_.data(), name_.size());
    memcpy(packet->message_start + name_.size(), version_.data(), version_.size());

    while (running_) {
        struct ifaddrs* ifList;
        if (getifaddrs(&ifList) < 0) {
            log::warn << "Broadcast: Error, can not get ifs" << log::endl;
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
            b_addr.sin_port = htons(BroadCastPort);

            sin = (struct sockaddr_in*)ifa->ifa_addr;
            packet->addr = sin->sin_addr.s_addr;

            sendto(socket_broadcast, buf, packet_length, 0, (struct sockaddr*)&b_addr, sizeof(b_addr));
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

    delete[] buf;
    log::debug << "Broadcast: Stop running" << log::endl;
}

}  // namespace daemon
}  // namespace hera
}  // namespace wayz
