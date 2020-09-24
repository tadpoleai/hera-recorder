///
/// @file daemon.cpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Main process of Acquistion Daemon
/// @version 0.1
/// @date 2019-11-08
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include <signal.h>
#include <unistd.h>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/THttpServer.h>
#include <thrift/transport/THttpTransport.h>
#include <thrift/transport/TServerSocket.h>

#include "common/include/logger/logger.hpp"
#include "common/include/third_party/json.hpp"
#include "common/include/version.hpp"
#include "device/include/version.hpp"
#include "storage/include/version.hpp"
//
#include "broadcast/broadcaster.hpp"
#include "service.hpp"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;
using namespace ::wayz::hera;

TThreadedServer* g_server_ptr = nullptr;           ///< global pointer to TSimpleServer
daemon::Service* g_handler_ptr = nullptr;          ///< global pointer to Service
daemon::Broadcaster* g_broadcaster_ptr = nullptr;  ///< global pointer to Broadcast

///
/// @brief Handler Ctrl+C(SIGINT)
///
/// @param s signal
void sig_int_handler_func(int s)
{
    log::info << "HeraMain: Sigint Received, Stopping" << log::endl;
    if (g_server_ptr) {
        g_server_ptr->stop();
        g_handler_ptr->reset();
    }
    if (g_broadcaster_ptr) {
        delete g_broadcaster_ptr;
    }
    exit(1);
}

/// Main process
int main(int argc, char** argv)
{
    struct sigaction sig_int_handler;
    sig_int_handler.sa_handler = sig_int_handler_func;
    sigemptyset(&sig_int_handler.sa_mask);
    sig_int_handler.sa_flags = 0;
    sigaction(SIGINT, &sig_int_handler, NULL);

    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " daemon.json" << std::endl;
        exit(-1);
    }

    std::string name = "Hera Default Daemon";
    std::string plugin_folder = "/usr/local/lib/hera/plugin";
    std::string storage_folder = "./";
    std::string setting_json = "./setting.json";
    std::string log_prefix = "hera-daemon";
    int listen_port = 9090;
    std::vector<daemon::RemoteServerType> remote_servers;
    bool broadcast_whitelist = true;
    std::vector<std::string> broadcast_ifs;

    try {
        std::ifstream i(argv[1]);
        json config;
        i >> config;

        try {
            name = config["name"];
            std::cout << "Name = " << name << std::endl;
        } catch (...) {
            std::cout << "Error: Daemon json file missing field 'name'!";
        }

        try {
            plugin_folder = config["pluginFolder"];
            std::cout << "PluginFolder = " << plugin_folder << std::endl;
        } catch (...) {
            std::cout << "Error: Daemon json file missing field 'pluginFolder'!";
        }

        try {
            storage_folder = config["storageFolder"];
            std::cout << "StorageFolder = " << storage_folder << std::endl;
        } catch (...) {
            std::cout << "Error: Daemon json file missing field 'storageFolder'!";
        }

        try {
            setting_json = config["settingJson"];
            std::cout << "SettingJson = " << setting_json << std::endl;
        } catch (...) {
            std::cout << "Error: Daemon json file missing field 'settingJson'!";
        }

        try {
            log_prefix = config["logPrefix"];
            std::cout << "LogPrefix = " << log_prefix << std::endl;
        } catch (...) {
            std::cout << "Error: Daemon json file missing field 'logPrefix'!";
        }

        try {
            listen_port = config["listenPort"];
            std::cout << "ListenPort = " << listen_port << std::endl;
        } catch (...) {
            std::cout << "Error: Daemon json file missing field 'listenPort'!";
        }


        try {
            for (const auto& ur_json : config["remoteServers"]) {
                daemon::RemoteServerType ur;
                ur.remark = ur_json["remark"];
                ur.protocol = ur_json["protocol"];
                ur.destination = ur_json["destination"];

                std::cout << "RemoteServer: " << ur_json << std::endl;
                remote_servers.emplace_back(std::move(ur));
            }
        } catch (...) {
            std::cout << "Error: Daemon json file missing field 'remoteServers' or format error!";
        }


        try {
            json broadcast = config["broadcast"];
            broadcast_whitelist = broadcast["mode"].get<bool>();
            std::cout << "Broadcast Mode: " << int(broadcast_whitelist) << std::endl;
            for (const auto& ifname : broadcast["ifs"]) {
                broadcast_ifs.push_back(ifname);
                std::cout << "Broadcast Interface: " << ifname << std::endl;
            }
        } catch (...) {
            std::cout << "Error: Daemon json file missing field 'broadcast' or format error!";
        }
    } catch (std::exception& e) {
        std::cout << "Error occured when parsing json file " << argv[1] << ": " << e.what() << std::endl;
        exit(-1);
    }

    log::init(log_prefix);

    log::info << "libhera-common: " << common::get_version() << log::endl;
    log::info << "libhera-device: " << device::get_version() << log::endl;
    log::info << "libhera-storage: " << storage::get_version() << log::endl;
    log::info << "Copyright 2018 Wayz.ai. All Rights Reserved." << log::endl;

    device::Factory::load_plugins(true, plugin_folder);

    std::shared_ptr<daemon::Service> handler(new daemon::Service(storage_folder, setting_json, remote_servers));
    g_handler_ptr = handler.get();
    std::shared_ptr<TProcessor> processor(new daemon::ServiceProcessor(handler));
    std::shared_ptr<TServerTransport> serverTransport(new TServerSocket(listen_port));
    std::shared_ptr<TTransportFactory> transportFactory(new THttpServerTransportFactory());
    std::shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());
    g_server_ptr = new TThreadedServer(processor, serverTransport, transportFactory, protocolFactory);

    g_broadcaster_ptr = new daemon::Broadcaster(name, broadcast_ifs, broadcast_whitelist);

    log::info << "HeraMain: Daemon Started" << log::endl;
    g_server_ptr->serve();

    delete g_broadcaster_ptr;
    log::info << "HeraMain: Daemon Stoped" << log::endl;
    return 0;
}
