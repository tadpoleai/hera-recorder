///
/// @file main.cpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief
/// @date 2020-10-22
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include <iomanip>
#include <iostream>
#include <sstream>
#include <unistd.h>

#include <hera/device/include.hpp>

#include "extracter.hpp"

using namespace wayz::hera;

void print_help(char** argv)
{
    std::cerr << "usage:\t" << argv[0] << " -i <source_data> -o <output_folder_path> [-p <proj.pto> -r <ramdisk>] \n"
              << "\t[-s <start_time_sec> ] [-t <time_duration_sec>]\n"
              << "\t[-dq] [-hv]" << std::endl;

    std::cerr << "\t-i\tSource data file\n"
              << "\t\t\tHera formatted file\n"

              << "\t-o\tOutput Folder Path\n"
              << "\t\t\tOutput folder, show be writable\n "

              << "\t-p\tHugin Project File\n"
              << "\t\t\tParanoma project file, optional\n"
              << "\t\t\tIf given, a paranoma image will be generated instead of seperated images'\n"
              << "\t\t\tThe project file should be created from image sets extracted by this program\n"
              << "\t\t\tIf not given, image sets will be extracted from hera file"

              << "\t-r\tPath to Ramdisk\n"
              << "\t\t\tA ramdisk for holding intermediate files'\n"
              << "\t\t\tIf Hugin Project File is given, intermediate files will be placed in this path\n"

              << "\t-s\tStart time\n"
              << "\t\t\tFloat number[sec], default = 0.0, to specific a timepoint, the data before which would be "
                 "skipped\n"

              << "\t-t\tDuration\n"
              << "\t\t\tFloat number[sec], default = 0.0(disabled), only convert data within a certain duration\n"

              << "\t-d\tFlag set log level to debug\n"

              << "\t-q\tFlat to suppress progress\n"

              << "\t-h\tPrint this help and exit\n"

              << std::endl;
}

void sig_int_handler_func(int s)
{
    log::info << "Sigint Received, Stopping" << log::endl;
    exit(0);
}

int main(int argc, char** argv)
{
    log::onlyprint();

    struct sigaction sig_int_handler;
    sig_int_handler.sa_handler = sig_int_handler_func;
    sigemptyset(&sig_int_handler.sa_mask);
    sig_int_handler.sa_flags = 0;
    sigaction(SIGINT, &sig_int_handler, NULL);

    std::string src_file;
    std::string output_folder;
    std::string hugin_project;
    std::string ramdisk_path;
    bool sticher_flag = false;
    bool isverbose = false;
    bool isquiet = false;
    int32_t start_time = 0;
    int32_t duration = 0;

    opterr = 0;
    while (true) {
        switch (getopt(argc, argv, "i:o:p:r:s:t:dqh")) {
        case 'i':
            src_file = optarg;
            continue;
        case 'o':
            output_folder = optarg;
            continue;
        case 'p':
            hugin_project = optarg;
            continue;
        case 'r':
            ramdisk_path = optarg;
            continue;
        case 's':
            try {
                start_time = std::stoi(optarg);
                continue;
            } catch (...) {
                std::cerr << "Can not tokenize start_time '" << optarg << "' as interger" << std::endl;
                print_help(argv);
                exit(1);
            }
        case 't':
            try {
                duration = std::stoi(optarg);
                continue;
            } catch (...) {
                std::cerr << "Can not tokenize duration '" << optarg << "' as interger" << std::endl;
                print_help(argv);
                exit(1);
            }
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
        default:
            print_help(argv);
            exit(1);
        }
        break;
    }

    if (src_file.empty() || output_folder.empty()) {
        std::cerr << "Source file / Output Folder not set" << std::endl;
        print_help(argv);
        exit(1);
    }

    if ((!hugin_project.empty()) && ramdisk_path.empty()) {
        std::cerr << "Ramdisk path not given, can not sticher" << std::endl;
        print_help(argv);
        exit(1);
    }

    log::info << "Hera Paranoma Stitcher Command Line\n" << log::endl;

    if (isverbose) {
        log::set_level(log::LogLevel::Debug);
    } else {
        log::set_level(log::LogLevel::Info);
    }

    log::debug << "Options:" << log::endl;
    log::debug << "\tSource File: " << src_file << log::endl;
    log::debug << "\tOutput Folder: " << output_folder << log::endl;
    if (!hugin_project.empty()) {
        sticher_flag = true;
        log::debug << "\tSticher Mode, with Hugin Projection File: " << hugin_project << ", Temp file in "
                   << ramdisk_path << log::endl;
    } else {
        log::debug << "\tImage Sets Extraction Mode" << log::endl;
    }
    if (start_time != 0) {
        log::debug << "\tStart Time: " << start_time << log::endl;
    }
    if (duration != 0) {
        log::debug << "\tDuration: " << duration << log::endl;
    }
    if (isquiet) {
        log::debug << "\tQuiet Mode" << log::endl;
    }

    auto extracter = std::make_unique<Extracter>(
            src_file, output_folder, start_time, duration, sticher_flag, hugin_project, ramdisk_path);

    if (!extracter->running()) {
        exit(1);
    }

    auto t_start = time::Timestamp::now();
    auto total_duration = extracter->total_duration();
    auto total_duration_str = total_duration.to_str_second();
    while (extracter->running()) {
        usleep(200000);

        if (isquiet) {
            continue;
        }

        time::Duration progress = extracter->progress();
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

    log::debug << "Extraction End" << log::endl;
}
