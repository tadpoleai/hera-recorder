///
/// @file finder.cpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Finder machine running hera-daemon on local net
/// @version 0.1
/// @date 2019-11-08
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "broadcast_packet.hpp"

using namespace wayz::hera::daemon;

/// Main process
int main(int argc, char** argv)
{
    int sockfd;
    char buffer[1024];

    struct sockaddr_in servaddr, cliaddr;

    // Creating socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        std::cerr << "Can not open socket, Exiting!" << std::endl;
        exit(1);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    // Filling server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(BroadCastPort);

    // Bind the socket with the server address
    if (bind(sockfd, (const struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        std::cerr << "Can not bind port" << BroadCastPort << ", Exiting" << std::endl;
        exit(1);
    }

    struct timeval timeout = {10, 0};  // 10 sec
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    std::cout << "Waiting for Broadcast Message from Hera Daemon..." << std::endl;
    while (1) {
        auto len = sizeof(cliaddr);

        ssize_t n = recvfrom(sockfd, (char*)buffer, 1024, MSG_WAITALL, (struct sockaddr*)&cliaddr, (socklen_t*)&len);
        if (n < 0) {
            std::cerr << "Time outed..." << std::endl;
            exit(1);
        }

        if (n <= (ssize_t)sizeof(BroadcastPacket)) {
            std::cerr << "Received Invalid Lengthed Message" << std::endl;
            continue;
        }

        BroadcastPacket* packet = (BroadcastPacket*)(void*)(buffer);
        if (packet->name_len > 512 || packet->version_len > 512 || packet->name_len + packet->version_len > 512) {
            std::cerr << "Received Invalid Message" << std::endl;
            std::cerr << "Reason: Invalid Length, name_len = " << packet->name_len
                      << ", version_len = " << packet->version_len << std::endl;
            continue;
        }

        std::string name;
        std::string version;

        char addr_string[128];
        name.resize(packet->name_len);
        version.resize(packet->version_len);
        memcpy((void*)name.data(), packet->message_start, packet->name_len);
        memcpy((void*)version.data(), packet->message_start + packet->name_len, packet->version_len);
        inet_ntop(AF_INET, &cliaddr.sin_addr, addr_string, sizeof(addr_string));

        std::cout << std::endl;
        std::cout << "Received Broadcast Message From Hera Daemon" << std::endl;
        std::cout << "  - Name: " << name << std::endl;
        std::cout << "  - IpAddress: " << addr_string << std::endl;
        std::cout << "  - Version: " << version << std::endl;
    }

    return 0;
}
