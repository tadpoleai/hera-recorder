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
    std::cerr << "usage:\t" << argv[0] << " -i <source_data> [-o <output_bag_file>] [-r <remap_file>]\n"
              << "\t[-s <start_time_sec> ] [-t <time_duration_sec>]\n"
              << "\t[-p <device_id|device_category|device_name>:<ParamType>=<ParamValue> [-p ...]]\n"
              << "\t[-ldq] [-hv]" << std::endl;

    std::cerr << "\t-i\tSource data file\n"
              << "\t\t\tHera formatted file\n"

              << "\t-o\tOutput Bag File\n"
              << "\t\t\tOutputfile name of ROS Bag, default is the same with source data file, replacing extension "
                 "with '.bag'\n "

              << "\t-r\tRemap json file\n"
              << "\t\t\tSpecific remap of 'Topic' and 'FrameID' in ROS Message, default = none\n"

              << "\t-s\tStart time\n"
              << "\t\t\tFloat number[sec], default = 0.0, to specific a timepoint, the data before which would be "
                 "skipped\n"

              << "\t-t\tDuration\n"
              << "\t\t\tFloat number[sec], default = 0.0(disabled), only convert data within a certain duration\n"

              << "\t-p\tParameters\n"
              << "\t\t\tOverride parameters of devices in covnertion, default = none\n"
              << "\t\t\t\t'-p 0:Gamma:2.2' overrides device id = 0, parameter type = 'Gamma', to new value "
                 "'2.2'\n"
              << "\t\t\t\t'-p camera/flir:Gamma:2.2' overrides device category 'camera/flir', parameter type = "
                 "'Gamma', to new value '2.2'\n"

              << "\t-l\tFlag to output a log file\n"

              << "\t-d\tFlag set log level to debug\n"

              << "\t-q\tFlat to suppress progress\n"

              << "\t-f\tPrint convert parameter rules and exit\n"

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

void print_convert_param_rules(char** argv)
{
    log::onlyprint();
    log::set_level(log::LogLevel::Error);
    print_version(argv);
    std::cout << "\nPrinting parameter maps.\n" << std::endl;
    for (auto& type : device::Factory::plugin_types()) {
        auto plain_rules = device::Factory::plugin_param_plain_rules(type);
        if (!plain_rules.empty()) {
            std::cout << "Vendor: " << type << std::endl;
            std::cout << "Accept parameters below:" << plain_rules << std::endl;
        }
    }
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
    std::vector<std::tuple<std::string, std::string, std::string>> parameter_tuple_list;
    bool islog = false;
    bool isverbose = false;
    bool isquiet = false;
    int32_t start_time = 0;
    int32_t duration = 0;

    // opterr = 0;
    while (true) {
        switch (getopt(argc, argv, "i:o:r:s:t:p:ldqfhv")) {
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
            try {
                start_time = std::stoi(optarg);
                continue;
            } catch (...) {
                print_version(argv);
                exit(0);
            }
        case 't':
            try {
                duration = std::stoi(optarg);
                continue;
            } catch (...) {
                print_version(argv);
                exit(0);
            }
        case 'p': {
            std::stringstream optarg_ss(optarg);
            std::string token_list[3];
            for (auto&& token : token_list) {
                if (!getline(optarg_ss, token, ':')) {
                    std::cerr << "Can not tokenize -p " << optarg << std::endl;
                    print_help(argv);
                    exit(1);
                }
            }
            parameter_tuple_list.emplace_back(std::make_tuple(token_list[0], token_list[1], token_list[2]));
        }
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
        case 'f':
            print_convert_param_rules(argv);
            exit(0);
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
        if (start_time > 0) {
            bag_file += "_s" + std::to_string(start_time);
        }
        if (duration > 0) {
            bag_file += "_t" + std::to_string(duration);
        }
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

    log::clear_line();

    if (isverbose) {
        log::set_level(log::LogLevel::Debug);
    } else {
        log::set_level(log::LogLevel::Info);
    }

    log::debug << "Conversion Start" << log::endl;
    auto handler = std::make_unique<Converter>(src_file,
                                               bag_file,
                                               std::move(remapper),
                                               parameter_tuple_list,
                                               start_time,
                                               duration);
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

        double speed = (progress - start_time * time::OneSecond) / double(t_now - t_start);
        auto rest = total_duration - progress;
        time::Duration eta = rest / speed;

        std::cout << "\r";
        std::cout << "- progress:  ";
        auto progress_str = progress.to_str_second();
        std::string whitespace(total_duration_str.size() - progress_str.size() + 4, ' ');
        std::cout << whitespace << progress_str << " / " << total_duration_str;
        std::cout << "  ";

        if (speed > 0) {
            std::cout << " speed = " << std::setw(8) << std::setfill(' ') << std::setprecision(4) << speed << "x  ";
        } else {
            std::cout << " speed = "
                      << "      ---"
                      << "  ";
        }

        auto eta_str = eta.to_str_second();
        std::cout << " eta = " << std::string(9 - eta_str.size(), ' ') << eta_str;
        std::cout.flush();
    }
    std::cout << "\n";
    std::cout.flush();

    log::debug << "Conversion End" << log::endl;
}
