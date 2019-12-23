#include <unistd.h>

#include "aligned_converter.hpp"
#include "common/logger/logger.hpp"
#include "common/utils/remapper.hpp"

using namespace wayz::hera;
using namespace wayz::hera::convert;

void print_help(char** argv)
{
    std::cout << "usage: " << argv[0] << " -i <source data folder> [-o <output bag file>] [-l] [-v]" << std::endl;
}

int main(int argc, char** argv)
{
    std::string bag_file;
    std::string src_folder;
    std::string remap_file;
    bool islog = false;
    bool isverbose = false;

    // opterr = 0;
    while (true) {
        switch (getopt(argc, argv, "i:o:r:hlv")) {
        case 'i':
            src_folder = optarg;
            continue;
        case 'o':
            bag_file = optarg;
            continue;
        case 'r':
            remap_file = optarg;
            continue;
        case 'l':
            islog = true;
            continue;
        case 'v':
            isverbose = true;
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

    RemapperPtr remapper = nullptr;
    if (remap_file.size() != 0) {
        auto remapper = Remapper::create(remap_file);
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
    auto handler = std::make_unique<AlignedConverter>(src_folder, bag_file, std::move(remapper));

    if (!handler->running()) {
        exit(1);
    }

    usleep(100000);
    auto converted = handler->converted_size();
    auto last_converted = converted;
    auto total = handler->total_size();
    auto ts = Timestamp::now();
    auto last_ts = ts;
    auto gamma = 1.0;
    constexpr auto Damp = 0.7;
    constexpr auto MinGamma = 0.05;
    auto speed = 0.0;

    while (handler->running()) {
        if (isverbose) {
            usleep(1000000);
        } else {
            usleep(2000000);
        }

        converted = handler->converted_size();
        auto new_converted = converted - last_converted;
        last_converted = converted;

        ts = Timestamp::now();
        auto duration = ts - last_ts;
        last_ts = ts;

        auto raw_speed = (double)new_converted / duration;
        speed = raw_speed * gamma + speed * (1 - gamma);
        gamma *= Damp;
        if (gamma < MinGamma) {
            gamma = MinGamma;
        }

        auto rest = total - converted;
        if (rest == 0) {
            break;
        }

        Duration eta = rest / speed;
        log::info << "Converter: " << converted << " / " << total << '\t' << "eta: " << eta.to_str_second()
                  << log::endl;
    }

    log::debug << "Conversion End" << log::endl;
}