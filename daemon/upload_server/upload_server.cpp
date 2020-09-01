///
/// @file upload_server.cpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief Broadcast Self as Upload Server
/// @date 2020-08-28
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include <iostream>
#include <memory>

#include "broadcast/broadcaster.hpp"
#include "common/include/logger/logger.hpp"
#include "common/include/third_party/json.hpp"

using namespace wayz::hera;

using json = nlohmann::json;

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
    struct sigaction sig_int_handler;
    sig_int_handler.sa_handler = sig_int_handler_func;
    sigemptyset(&sig_int_handler.sa_mask);
    sig_int_handler.sa_flags = 0;
    sigaction(SIGINT, &sig_int_handler, NULL);

    log::onlyprint();

    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " config.json" << std::endl;
        exit(-1);
    }

    std::string name;
    std::string protocol;
    std::string parameter;
    bool broadcast_whitelist = true;
    std::vector<std::string> broadcast_ifs;

    try {
        std::ifstream i(argv[1]);
        json config;
        i >> config;

        try {
            name = config["name"];
        } catch (...) {
            std::cerr << "No 'name' in config" << std::endl;
            exit(-1);
        }
        std::cout << "name = " << name << std::endl;

        try {
            protocol = config["protocol"];
        } catch (...) {
            std::cerr << "No 'protocol' in config" << std::endl;
            exit(-1);
        }
        std::cout << "protocol = " << protocol << std::endl;

        try {
            parameter = config["parameter"];
        } catch (...) {
            std::cerr << "No 'parameter' in config" << std::endl;
            exit(-1);
        }
        std::cout << "parameter = " << parameter << std::endl;

        try {
            json broadcast = config["broadcast"];
            broadcast_whitelist = broadcast["mode"].get<bool>();
            std::cout << "Broadcast Mode: " << int(broadcast_whitelist) << std::endl;
            for (const auto& ifname : broadcast["ifs"]) {
                broadcast_ifs.push_back(ifname);
                std::cout << "Broadcast Interface: " << ifname << std::endl;
            }
        } catch (...) {
            std::cerr << "No 'broadcast' in config" << std::endl;
            exit(-1);
        }
    } catch (std::exception& e) {
        std::cout << "Error occured when parsing json file " << argv[1] << ": " << e.what() << std::endl;
        exit(-1);
    }

    log::info << "Upload server: Start broadcasting" << log::endl;
    auto upload_broadcaster =
            std::make_unique<daemon::Broadcaster>(name, broadcast_ifs, broadcast_whitelist, protocol, parameter, true);

    std::cin.ignore();
    std::cin.get();

    upload_broadcaster.release();

    log::info << "Upload server: Stop broadcasting" << log::endl;
    return 0;
}
