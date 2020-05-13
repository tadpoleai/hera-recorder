#include <iomanip>
#include <iostream>
#include <sstream>
#include <unistd.h>

#include "common/include/logger/logger.hpp"
#include "common/include/utils/remapper.hpp"
#include "common/include/version.hpp"
#include "converter.hpp"
#include "device/include/version.hpp"
#include "storage/include/version.hpp"

using namespace wayz::hera;
using namespace wayz::hera::convert;

Converter* g_converter_ptr = nullptr;

void print_help(char** argv)
{
    std::cout << "usage:\t" << argv[0] << " -i <source_data> [-o <output_bag_file>] [-r <remap_file>] [-sld] [-hv]"
              << std::endl;
    std::cout << "\t-i\tSource Data File\n"
              << "\t-o\tOutput Bag File, default is sample as source with .bag extension\n"
              << "\t-r\tTopic and FrameID remap json file\n"
              << "\t-s\tFlag to only show header information\n"
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
    if (g_converter_ptr) {
        g_converter_ptr->stop();
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

    std::string bag_file;
    std::string src_file;
    std::string remap_file;
    bool islog = false;
    bool isonlyshow = false;
    bool isverbose = false;
    bool isquiet = false;

    // opterr = 0;
    while (true) {
        switch (getopt(argc, argv, "i:o:r:sldqhv")) {
        case 'i':
            src_file = optarg;
            continue;
        case 'o':
            bag_file = optarg;
            continue;
        case 'r':
            remap_file = optarg;
            continue;
        case 's':
            isonlyshow = true;
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

    if (src_file.size() == 0) {
        std::cout << argv[0] << ": option requires an argument -- 'i'" << std::endl;
        print_help(argv);
        exit(0);
    }

    if (bag_file.size() == 0) {
        auto lastdot = src_file.find_last_of(".");
        if (lastdot == std::string::npos)
            bag_file = src_file;
        bag_file = src_file.substr(0, lastdot);
        bag_file += ".bag";
    }

    common::RemapperPtr remapper = nullptr;
    if (remap_file.size() != 0) {
        remapper = common::Remapper::create(remap_file);
    } else {
        remapper = common::Remapper::empty();
    }

    if (islog) {
        log::init("converter");
    } else {
        log::onlyprint();
    }

    if (isverbose) {
        log::set_level(log::LogLevel::Debug);
    } else {
        log::set_level(log::LogLevel::Info);
    }

    log::debug << "Conversion Start" << log::endl;
    auto handler = std::make_unique<Converter>(src_file, bag_file, std::move(remapper), isonlyshow);
    g_converter_ptr = handler.get();

    if (!handler->running()) {
        exit(1);
    }

    auto t_start = time::Timestamp::now();
    auto total_duration = handler->total_duration();
    auto total_duration_str = total_duration.to_str_second();
    while (handler->running()) {
        usleep(200000);

        if (isquiet) {
            continue;
        }

        time::Duration progress = handler->progress();
        auto t_now = time::Timestamp::now();
        double speed = progress / double(t_now - t_start);
        auto rest = total_duration - progress;
        time::Duration eta = rest / speed;

        std::cout << "\r";
        std::cout << "- progress:  ";
        auto progress_str = progress.to_str_second();
        std::string whitespace(total_duration_str.size() - progress_str.size() + 6, ' ');
        std::cout << whitespace << progress_str << " / " << total_duration_str;
        std::cout << "        ";

        std::cout << " speed = " << std::setw(8) << std::setfill(' ') << std::setprecision(4) << speed << "x        ";

        auto eta_str = eta.to_str_second();
        std::cout << " eta = " << std::string(10 - eta_str.size(), ' ') << eta_str;
        std::cout.flush();
    }
    std::cout << "\n";
    std::cout.flush();

    log::debug << "Conversion End" << log::endl;
}
