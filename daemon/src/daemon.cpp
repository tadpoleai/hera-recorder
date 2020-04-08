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

#include <common/include/logger/logger.hpp>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/THttpServer.h>
#include <thrift/transport/THttpTransport.h>
#include <thrift/transport/TServerSocket.h>

#include "service.hpp"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;
using namespace ::wayz::hera;

TThreadedServer* g_server_ptr = nullptr;   ///< global pointer to TSimpleServer
daemon::Service* g_handler_ptr = nullptr;  ///< global pointer to Service

///
/// @brief Handler Ctrl+C(SIGINT)
///
/// @param s signal
void sig_int_handler_func(int s)
{
    if (g_server_ptr) {
        g_server_ptr->stop();
        g_handler_ptr->reset();
    }
    log::info << "HeraMain: Sigint Received, Stopping" << log::endl;
}

/// Main process
int main(int argc, char** argv)
{
    struct sigaction sig_int_handler;
    sig_int_handler.sa_handler = sig_int_handler_func;
    sigemptyset(&sig_int_handler.sa_mask);
    sig_int_handler.sa_flags = 0;
    sigaction(SIGINT, &sig_int_handler, NULL);

    std::string filename_prefix = "./";
    std::string profile_json = "./profiles.json";
    std::string log_file = "hera-daemon";
    if (argc >= 2) {
        filename_prefix = argv[1];
    }
    if (argc >= 3) {
        profile_json = argv[2];
    }
    if (argc >= 4) {
        log_file = argv[3];
    }

    log::init(log_file);

    int port = 9090;
    std::shared_ptr<daemon::Service> handler(new daemon::Service(filename_prefix, profile_json));
    g_handler_ptr = handler.get();
    std::shared_ptr<TProcessor> processor(new daemon::ServiceProcessor(handler));
    std::shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
    std::shared_ptr<TTransportFactory> transportFactory(new THttpServerTransportFactory());
    std::shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

    g_server_ptr = new TThreadedServer(processor, serverTransport, transportFactory, protocolFactory);
    log::info << "HeraMain: Daemon Started" << log::endl;
    g_server_ptr->serve();
    log::info << "HeraMain: Daemon Stoped" << log::endl;
    return 0;
}
