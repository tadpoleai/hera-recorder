///
/// @file broadcast.cpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief
/// @date 2020-05-27
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///
///

#include "listener.hpp"

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

Listener::Listener(const bool is_upload_server, void* callback) :
    port_(is_upload_server ? UploadServerInfoBroadcastPort : BroadcastPort),
    is_upload_server_(is_upload_server),
    running_(true),
    thread_(new std::thread(&Listener::thread_function, this)),
    callback_(callback)
{}

Listener::~Listener()
{
    log::info << "Listener: Exiting!" << log::endl;
    running_ = false;
    thread_->join();
}

void Listener::thread_function()
{
    int sockfd;
    char buffer[1024];

    struct sockaddr_in servaddr, cliaddr;

    // Creating socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        log::error << "Listener: Can not open socket, Exiting!" << log::endl;
        return;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    // Filling server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(port_);

    log::debug << "Listener: Start listening port " << port_ << " from LAN " << log::endl;

    // Bind the socket with the server address
    if (bind(sockfd, (const struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        log::error << "Listener: Can not bind port" << port_ << ", Exiting" << log::endl;
        return;
    }

    struct timeval timeout = {1, 0};  // 1000ms
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    while (running_) {
        auto len = sizeof(cliaddr);

        ssize_t n = recvfrom(sockfd, (char*)buffer, 1024, MSG_WAITALL, (struct sockaddr*)&cliaddr, (socklen_t*)&len);
        if (n <= 0) {
            continue;
        }

        std::string string_buff(buffer, buffer + n);
        if (!is_upload_server_) {
            BroadcastPacket packet;
            if (BroadcastPacket::from_buff(string_buff, packet)) {
                (BroadcastCallback(callback_))(packet);
            } else {
                log::warn << "Listener:: Received Invaild Broadcast Packet" << log::endl;
            }
        } else {
            UploadServerInfoPacket packet;
            if (UploadServerInfoPacket::from_buff(string_buff, packet)) {
                (UploadServerInfoCallback(callback_))(packet);
            } else {
                log::warn << "Listener:: Received Invaild UploadServerInfo Packet" << log::endl;
            }
        }
    }
}

}  // namespace daemon
}  // namespace hera
}  // namespace wayz
