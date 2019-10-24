#include <unistd.h>

#include <common/logger/logger.hpp>

#include "converter_manager.hpp"
#include "iostream"

using namespace wayz::tron;

void print_help(char** argv)
{
    std::cout << "usage: " << argv[0] << " -i <source data folder> [-o <output bag file>]"
              << std::endl;
}

int main(int argc, char** argv)
{
    std::string bag_file;
    std::string src_folder;

    // opterr = 0;
    while (true) {
        switch (getopt(argc, argv, "i:o:h")) {
        case 'i':
            src_folder = optarg;
            continue;
        case 'o':
            bag_file = optarg;
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

    if (src_folder.size() == 0) {
        std::cout << argv[0] << ": option requires an argument -- 'i'" << std::endl;
        print_help(argv);
        exit(0);
    }

    if (bag_file.size() == 0) {
        bag_file = src_folder;
        if (bag_file.back() == '/') {
            bag_file.pop_back();
        }
        bag_file += ".bag";
    }

    Logger::create("logs");
    Logger::info() << "Converter: Converter Initialized" << Logger::endl;
    auto manager = new ConverterManager(bag_file, src_folder);
    do {
        usleep(1000000);
        auto converted = manager->report_progress();
        auto total = manager->total_size();
        Logger::info() << "Converter: " << converted << " / " << total << Logger::endl;
    } while (manager->running());

    delete manager;
    Logger::info() << "Converter: Conversion Completed" << Logger::endl;
}
