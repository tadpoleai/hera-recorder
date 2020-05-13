#include <unistd.h>

#include "common/include/version.hpp"
#include "device/include/version.hpp"
#include "replayer.hpp"
#include "storage/include/version.hpp"

using namespace wayz::hera;
using namespace wayz::hera::replay;

Replayer* g_replayer_ptr = nullptr;

void print_help(char** argv)
{
    std::cout << "usage:\t" << argv[0] << " -i <source_data> [-r <replay_rate>] [-pld] [-hv]" << std::endl;
    std::cout << "\t-i\tSource Data File\n"
              << "\t-r\tReplay Rate\n"
              << "\t-p\tFlag to really play\n"
              << "\t-l\tFlag to output log file\n"
              << "\t-d\tFlag to debug output\n"
              << "\t-q\tFlag to off progress output\n"
              << "\t-h\tPrint this help and exit\n"
              << "\t-v\tPrint version and exit\n"
              << std::endl;
}

void print_version(char** argv)
{
    std::cout << argv[0] << std::endl;
    std::cout << "libhera-common: " << common::get_version() << std::endl;
    std::cout << "libhera-device: " << device::get_version() << std::endl;
    std::cout << "libhera-storage: " << storage::get_version() << std::endl;
    std::cout << "Copyright 2018 Wayz.ai. All Rights Reserved." << std::endl;
}

void sig_int_handler_func(int s)
{
    log::info << "Convert: Sigint Received, Stopping" << log::endl;
    if (g_replayer_ptr) {
        g_replayer_ptr->stop();
    }
    exit(0);
}

int main(int argc, char** argv)
{
    struct sigaction sig_int_handler;
    sig_int_handler.sa_handler = sig_int_handler_func;
    sigemptyset(&sig_int_handler.sa_mask);
    sig_int_handler.sa_flags = 0;
    sigaction(SIGINT, &sig_int_handler, NULL);

    std::string filename;
    bool islog = false;
    bool isverbose = false;
    bool isplay = false;
    bool isquiet = false;
    double replay_rate = 1.0;

    // opterr = 0;
    while (true) {
        switch (getopt(argc, argv, "i:o:r:pldqhv")) {
        case 'i':
            filename = optarg;
            continue;
        case 'r':
            try {
                replay_rate = std::stod(optarg);
            } catch (...) {
                print_help(argv);
                exit(1);
            }
            continue;
        case 'p':
            isplay = true;
            continue;
        case 'l':
            islog = true;
            continue;
        case 'd':
            isverbose = true;
            continue;
        case 'q':
            isquiet = true;
            continue;
        case -1:
            break;
        case 'h':
            print_help(argv);
            exit(0);
        case 'v':
            print_version(argv);
            exit(0);
        default:
            print_help(argv);
            exit(1);
        }
        break;
    }

    if (filename.size() == 0) {
        std::cout << argv[0] << ": option requires an argument -- 'i'" << std::endl;
        print_help(argv);
        exit(1);
    }

    if (islog) {
        log::init("replayer");
    } else {
        log::onlyprint();
    }

    if (isverbose) {
        log::set_level(log::LogLevel::Debug);
    } else {
        log::set_level(log::LogLevel::Info);
    }

    log::debug << "Replay rate = " << replay_rate << log::endl;

    log::debug << "Replayer Start" << log::endl;
    auto handler = std::make_unique<Replayer>(filename, replay_rate, !isplay);
    g_replayer_ptr = handler.get();

    log::flush();

    while (handler->running()) {
        usleep(1000);
        if (isquiet) {
            continue;
        }

        time::Duration progress = handler->progress();
        std::cout << "\r";
        std::cout << "Replayer: " << progress << " of " << handler->total_duration();
        std::cout << "             ";
        std::cout.flush();
    }
    std::cout << std::endl;

    log::debug << "Replayer End" << log::endl;
    return 0;
}