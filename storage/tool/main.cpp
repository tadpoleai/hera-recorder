#include <iomanip>
#include <iostream>
#include <sstream>
#include <unistd.h>

#include "common/include/logger/logger.hpp"
#include "common/include/version.hpp"
#include "device/include/version.hpp"
#include "storage/include/version.hpp"
#include "tool.hpp"

using namespace wayz::hera;
using namespace wayz::hera::storage;

void print_help(char** argv)
{
    std::cout << argv[0] << ": "
              << "Tool to view / trim / rebuild header of hera storage data" << std::endl;

    std::cout << "usage:\t" << argv[0] << " -i <source_data> [-el] [-b]"
              << "\t[-m -s <start_time> -t <duration> [-o <output_data>]] [-qhv]" << std::endl;

    std::cout << "\t-i\tSource data file\n"
              << "\t\t\tHera formatted file\n"

              << "\t-e\tFlag to print extra info in storage file\n"

              << "\t-l\tFlag to print logs in storage file\n"

              << "\t-b\tFlag to rebuild damaged storage header\n"

              << "\t-m\tFlag to trimming data\n"

              << "\t-s\tStart time\n"
              << "\t\t\tFloat number[sec], default = 0, to specific a timepoint, the data before which would be "
                 "trimmed\n"

              << "\t-t\tDuration\n"
              << "\t\t\tFloat number[sec], default = 0(infinite), data after a certain duration will be trimmed\n"

              << "\t-o\tOutput File\n"
              << "\t\t\tOutputfile name of Trimmed record, default is <source>_trim_s<s>_t<t>.hera'\n "

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

int main(int argc, char** argv)
{
    Tool::Config config;

    log::onlyprint();
    log::set_level(log::LogLevel::Info);

    // opterr = 0;
    while (true) {
        switch (getopt(argc, argv, "i:elbms:t:o:qhv")) {
        case 'i':
            config.filename = optarg;
            continue;
        case 'e':
            config.print_extra = true;
            continue;
        case 'l':
            config.print_logs = true;
            continue;
        case 'b':
            config.rebuild = true;
            continue;
        case 'm':
            config.trim = true;
            continue;
        case 's':
            try {
                config.start_time = std::stod(optarg);
                if (config.start_time < 0) {
                    throw std::runtime_error("");
                }
                continue;
            } catch (...) {
                std::cerr << "Can not tokenize start_time '" << optarg << "' as positive number" << std::endl;
                print_help(argv);
                exit(1);
            }
        case 't':
            try {
                config.duration = std::stod(optarg);
                if (config.duration < 0) {
                    throw std::runtime_error("");
                }
                continue;
            } catch (...) {
                std::cerr << "Can not tokenize duration '" << optarg << "' as number" << std::endl;
                print_help(argv);
                exit(1);
            }
        case 'o':
            config.outfilename = optarg;
            continue;
        case 'q':
            config.isquiet = true;
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

    if (config.filename.size() == 0) {
        print_help(argv);
        std::cout << argv[0] << ": option requires an argument -- 'i'" << std::endl;
        exit(1);
    }

    if (config.rebuild && config.trim) {
        std::cout << argv[0] << ": Rebuild & Trimming is not supported to use on the same time." << std::endl;
        print_help(argv);
        exit(1);
    }

    if (config.trim && config.start_time == 0 && config.duration == 0) {
        std::cout << argv[0] << ": Trimming should have either start_time or duration set." << std::endl;
        print_help(argv);
        exit(1);
    }

    if (config.outfilename.size() == 0) {
        config.outfilename = config.filename + "_trim";
        if (config.start_time != 0) {
            config.outfilename += "_s" + std::to_string(config.start_time);
        }
        if (config.duration != 0) {
            config.outfilename += "_t" + std::to_string(config.duration);
        }
        config.outfilename += ".hera";
    }

    auto handler = std::make_unique<Tool>(config);

    log::flush();

    auto t_start = time::Timestamp::now();
    while (handler->running()) {
        usleep(200000);

        if (config.isquiet) {
            continue;
        }

        file::FileSize progress = handler->progess_size();
        file::FileSize total_size = handler->total_size();
        auto t_now = time::Timestamp::now();

        double size_per_ns = (double)progress / double(t_now - t_start);
        file::FileSize rest_size = total_size - progress;
        time::Duration eta = rest_size / size_per_ns;

        std::cout << "\r";
        std::cout << "- progress: ";
        std::string progress_str = progress;
        std::string total_size_str = total_size;
        int32_t whitespace_size = (int32_t)total_size_str.size() - (int32_t)progress_str.size() + 4;
        if (whitespace_size < 1) {
            whitespace_size = 1;
        }
        std::string whitespace(whitespace_size, ' ');
        std::cout << whitespace << progress_str << " / " << total_size_str;
        std::cout << " ";

        file::FileSize speed_per_sec = size_per_ns * (double)(time::OneSecond);
        if (speed_per_sec > 0) {
            std::cout << " speed = " << std::setw(8) << std::setfill(' ') << std::setprecision(4) << speed_per_sec
                      << "/s ";
        } else {
            std::cout << " speed = "
                      << "      ---"
                      << "  ";
        }

        auto eta_str = eta.to_str_second();
        whitespace_size = 8 - (int32_t)eta_str.size();
        if (whitespace_size < 1) {
            whitespace_size = 1;
        }
        std::cout << " eta = " << std::string(whitespace_size, ' ') << eta_str;
        std::cout << "\r";
        std::cout.flush();
    }

    std::cout << std::endl;
    return 0;
}