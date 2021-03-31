/// 
/// @file finder.cpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief 
/// @date 2020-09-01
/// 
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
/// 

#include <iostream>

#include "broadcast/listener.hpp"
#include "common/include/logger/logger.hpp"

using namespace wayz::hera;
using namespace wayz::hera::daemon;

void callback(const BroadcastPacket& packet)
{
    log::info << "Received Broadcast Message From Hera Daemon"
              << "\n  - Name: " << packet.name << "\n  - IpAddress: " << packet.addr_string
              << "\n  - Version: " << packet.version << log::endl;
}

void callback_upload(const UploadServerInfoPacket& packet)
{
    log::info << "Received Broadcast Message From Upload Server"
              << "\n  - Name: " << packet.name << "\n  - IpAddress: " << packet.addr_string
              << "\n  - Protocol: " << packet.protocol << log::endl;
}

///
/// @brief Handler Ctrl+C(SIGINT)
///
/// @param s signal
void sig_int_handler_func(int s)
{
    exit(0);
}

/// Main process
int main(int argc, char** argv)
{
    log::onlyprint();

    struct sigaction sig_int_handler;
    sig_int_handler.sa_handler = sig_int_handler_func;
    sigemptyset(&sig_int_handler.sa_mask);
    sig_int_handler.sa_flags = 0;
    sigaction(SIGINT, &sig_int_handler, NULL);

    log::info << "This program can find running hera-daemon and upload server in local area network." << log::endl;
    log::info << "Press 'Enter' twice to exit..." << log::endl;

    auto listener = std::make_unique<Listener>(false, (void*)&callback);
    auto listener_upload = std::make_unique<Listener>(true, (void*)&callback_upload);

    std::cin.ignore();
    std::cin.get();

    listener.release();
    listener_upload.release();

    return 0;
}
