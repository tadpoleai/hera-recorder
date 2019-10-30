#include <unistd.h>

#include <common/logger/logger.hpp>
#include <common/third_party/json.hpp>

#include "converter_manager.hpp"
#include "iostream"

using namespace wayz::tron;
using json = nlohmann::json;

void print_help(char** argv)
{
    std::cout << "usage: " << argv[0] << " -i <source data folder> [-o <output bag file>]"
              << std::endl;
}

int main(int argc, char** argv)
{
    std::string bag_file;
    std::string src_folder;
    std::string remap_file;

    // opterr = 0;
    while (true) {
        switch (getopt(argc, argv, "i:o:r:h")) {
        case 'i':
            src_folder = optarg;
            continue;
        case 'o':
            bag_file = optarg;
            continue;
        case 'r':
            remap_file = optarg;
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

    json remap;
    if (remap_file.size() != 0) {
        try {
            std::ifstream remap_file_stream;
            remap_file_stream.open(remap_file.c_str(), std::ios::in);
            remap_file_stream >> remap;
            remap_file_stream.close();
        } catch (...) {
            Logger::error() << "Converter: Can not apply remap file " << remap_file << Logger::endl;
            exit(0);
        }
    }

    Logger::create("logs");
    Logger::info() << "Converter: Converter Initialized" << Logger::endl;
    auto manager = new ConverterManager(bag_file, src_folder, remap);
    do {
        usleep(1000000);
        auto converted = manager->report_progress();
        auto total = manager->total_size();
        Logger::info() << "Converter: " << converted << " / " << total << Logger::endl;
    } while (manager->running());

    delete manager;
    Logger::info() << "Converter: Conversion Completed" << Logger::endl;
}
