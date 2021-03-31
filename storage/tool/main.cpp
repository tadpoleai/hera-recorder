#include <iostream>
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
              << "Tool to view / rebuild header of hera storage data" << std::endl;
    std::cout << "usage:\t" << argv[0] << " -i <source_data> [-el] [-b] [-r [-o <output_data>]] [-hv]" << std::endl;
    std::cout << "\t-i\tSource data file\n"
              << "\t-e\tFlag to print extra info in storage file\n"
              << "\t-l\tFlag to print logs in storage file\n"
              << "\t-b\tFlag to rebuild damaged storage header\n"
              << "\t-r\tFlag to reindex timestamp using intrinsic timestamp (not implemented)\n"
              << "\t-o\tOutput data file (only for reindexing)\n"
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
    std::string filename;
    std::string outfilename;
    bool rebuild = false;
    bool reindex = false;
    bool print_extra = false;
    bool print_logs = false;

    log::onlyprint();
    log::set_level(log::LogLevel::Info);

    // opterr = 0;
    while (true) {
        switch (getopt(argc, argv, "i:o:r:elbrhv")) {
        case 'i':
            filename = optarg;
            continue;
        case 'o':
            outfilename = optarg;
            continue;
        case 'e':
            print_extra = true;
            continue;
        case 'l':
            print_logs = true;
            continue;
        case 'b':
            rebuild = true;
            continue;
        case 'r':
            reindex = true;
            print_help(argv);
            exit(0);
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
        print_help(argv);
        std::cout << argv[0] << ": option requires an argument -- 'i'" << std::endl;
        exit(1);
    }

    if (outfilename.size() == 0) {
        outfilename = filename + ".reindex.hera";
    }

    auto handler = std::make_unique<Tool>(filename, print_extra, print_logs, rebuild, reindex, outfilename);

    log::flush();

    while (handler->running()) {
        std::cout << handler->progess_size() << " / " << handler->total_size() << std::endl;
        usleep(2000000);
    }
    std::cout << std::endl;

    return 0;
}