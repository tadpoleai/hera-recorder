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

#include <sys/stat.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/THttpServer.h>
#include <thrift/transport/THttpTransport.h>
#include <thrift/transport/TServerSocket.h>

#include "common/include/logger/logger.hpp"
#include "common/include/version.hpp"
#include "device/include/version.hpp"
#include "storage/include/upload.hpp"
#include "storage/include/version.hpp"
//
#include "broadcast/broadcaster.hpp"
#include "config.hpp"
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

    std::string config_path = "/etc/hera.conf";
    if (argc != 1) {
        config_path = argv[1];
    }

    auto config = daemon::Config::read_config(config_path);

    log::init(config.log_prefix);
    log::set_sleep_before_exiting(true);
    log::set_level(config.log_level);

    log::info << "libhera-common: " << common::get_version() << log::endl;
    log::info << "libhera-device: " << device::get_version() << log::endl;
    log::info << "libhera-storage: " << storage::get_version() << log::endl;
    log::info << "Copyright 2018 Wayz.ai. All Rights Reserved." << log::endl;

    device::Factory::load_plugins(true, config.device_plugin_directory);
    storage::upload::Transmission::load_plugins(config.upload_plugin_directory);
    mkdir(config.data_directory.c_str(), 0775);

    log::flush();

    std::shared_ptr<daemon::Service> handler(new daemon::Service(config));
    g_handler_ptr = handler.get();
    std::shared_ptr<TProcessor> processor(new daemon::ServiceProcessor(handler));
    std::shared_ptr<TServerTransport> serverTransport(new TServerSocket(config.listen_port));
    std::shared_ptr<TTransportFactory> transportFactory(new THttpServerTransportFactory());
    std::shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());
    g_server_ptr = new TThreadedServer(processor, serverTransport, transportFactory, protocolFactory);

    g_broadcaster_ptr = new daemon::Broadcaster(config.name, config.network_exclude_interfaces, false);

    log::info << "HeraMain: Daemon Started" << log::endl;
    g_server_ptr->serve();

    delete g_broadcaster_ptr;
    log::info << "HeraMain: Daemon Stoped" << log::endl;
    return 0;
}
