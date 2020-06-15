
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/types.h>

#include "common/include/version.hpp"
#include "device/include/version.hpp"
#include "replayer.hpp"
#include "storage/include/version.hpp"

using namespace wayz::hera;
using namespace wayz::hera::replay;

Replayer* g_replayer_ptr = nullptr;
struct termios g_old_termios_settings;

void print_help(char** argv)
{
    std::cout << "usage:\t" << argv[0]
              << " -i <source_data> [-c <record_config_file>] [-s <start_time>] [-r <replay_rate>] [-dq] [-hv]"
              << std::endl;
    std::cout << "\t-i\tSource data file\n"
              << "\t\t\tHera formatted file\n"

              << "\t-c\tRecord config json file\n"
              << "\t\t\tTo specific an ipc remap, the same json format as which used by device-record\n"

              << "\t-r\tStart time\n"
              << "\t\t\tFloat number[sec], default = 0.0 to specific a timepoint, the data before which would be "
                 "skipped\n"

              << "\t-r\tReplay rate\n"
              << "\t\t\tFloat number, default = 1.0\n"

              << "\t-d\tFlag set log level to debug\n"

              << "\t-q\tFlat to suppress progress\n"

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

    tcsetattr(fileno(stdin), TCSANOW, &g_old_termios_settings);

    exit(0);
}

int kbhit(struct timeval& timetv, char& ch)
{
    fd_set set;
    FD_ZERO(&set);
    FD_SET(fileno(stdin), &set);

    ch = 0;
    int res = select(fileno(stdin) + 1, &set, NULL, NULL, &timetv);
    if (res > 0) {
        auto s = read(fileno(stdin), &ch, 1);
        return s;
    }
    return 0;
}

int main(int argc, char** argv)
{
    struct sigaction sig_int_handler;
    sig_int_handler.sa_handler = sig_int_handler_func;
    sigemptyset(&sig_int_handler.sa_mask);
    sig_int_handler.sa_flags = 0;
    sigaction(SIGINT, &sig_int_handler, NULL);

    std::string filename;
    std::string config_filename;
    bool isverbose = false;
    bool isquiet = false;
    double replay_rate = 1.0;
    double start_time = 0.0;

    // opterr = 0;
    while (true) {
        switch (getopt(argc, argv, "i:c:s:r:dqhv")) {
        case 'i':
            filename = optarg;
            continue;
        case 'c':
            config_filename = optarg;
            continue;
        case 's':
            try {
                start_time = std::stod(optarg);
                if (start_time < 0) {
                    throw std::runtime_error("Start time less than zero!");
                }
            } catch (std::exception& e) {
                std::cout << "Error: " << e.what() << std::endl;
                print_help(argv);
                exit(1);
            }
            continue;
        case 'r':
            try {
                replay_rate = std::stod(optarg);
            } catch (std::exception& e) {
                std::cout << "Error: " << e.what() << std::endl;
                print_help(argv);
                exit(1);
            }
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

    log::onlyprint();
    if (isverbose) {
        log::set_level(log::LogLevel::Debug);
    } else {
        log::set_level(log::LogLevel::Info);
    }

    log::clear_line();

    tcgetattr(fileno(stdin), &g_old_termios_settings);
    auto setting = g_old_termios_settings;
    setting.c_lflag &= (~ICANON & ~ECHO);
    tcsetattr(fileno(stdin), TCSANOW, &setting);

    log::debug << "Replay rate = " << replay_rate << log::endl;

    log::debug << "Replayer Start" << log::endl;
    auto handler = std::make_unique<Replayer>(filename, replay_rate, config_filename, start_time * time::OneSecond);
    g_replayer_ptr = handler.get();

    log::flush();

    while (handler->running()) {
        char ch;
        struct timeval timetv {
            .tv_sec = 0, .tv_usec = 1000
        };

        kbhit(timetv, ch);
        if (ch == ' ') {
            handler->pause();
        }

        if (isquiet) {
            continue;
        }

        auto progress_str = handler->progress().to_str_second(true);
        auto duration_str = handler->total_duration().to_str_second(true);
        auto paused_str = handler->is_paused() ? "[||]" : "[>>]";
        auto hint_str = handler->is_paused() ? "\t\t<Space> to play " : "\t\t<Space> to pause";
        std::string ws_0(16 - progress_str.size(), ' ');
        std::string ws_1(16 - duration_str.size(), ' ');
        std::cout << "\r";
        std::cout << paused_str << ws_0 << progress_str << " / " << ws_1 << duration_str << hint_str;
        std::cout.flush();
    }
    std::cout << std::endl;

    log::debug << "Replayer End" << log::endl;

    tcsetattr(fileno(stdin), TCSANOW, &g_old_termios_settings);

    return 0;
}