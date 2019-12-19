#include <unistd.h>

#include "aligned_replayer.hpp"

using namespace wayz::hera;
using namespace wayz::hera::replay;

void print_help(char** argv)
{
    std::cout << "usage: " << argv[0] << " -i <source_data_folder> [-r <replay_rate>] [-l] [-v]" << std::endl;
}

int main(int argc, char** argv)
{
    std::string src_folder;
    bool islog = false;
    bool isverbose = false;
    double replay_rate = 1.0;

    // opterr = 0;
    while (true) {
        switch (getopt(argc, argv, "i:o:r:hlv")) {
        case 'i':
            src_folder = optarg;
            continue;
        case 'r':
            try {
                replay_rate = std::stod(optarg);
            } catch (...) {
                print_help(argv);
                exit(1);
            }
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
    auto handler = std::make_unique<AlignedReplayer>(src_folder, replay_rate);

    while (handler->running()) {
    }

    log::debug << "Replayer End" << log::endl;
    return 0;
}