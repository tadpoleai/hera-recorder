#include "generator.hpp"
#include "unistd.h"

using namespace wayz::hera::device;

int main(int argc, char** argv)
{
    if (argc != 5) {
        _exit(1);
    }

    std::string input_filename = argv[1];
    std::string output_filename = argv[2];
    std::string category_name = argv[3];
    std::string vendor_name = argv[4];

    std::string escaped_file_content;

    parameter::DeviceDesc device_desc;

    if (!parameter::parse(input_filename, escaped_file_content, device_desc)) {
        _exit(1);
    }

    std::cout << input_filename << " parsed:" << device_desc.parameters << std::endl;

    if (!parameter::output(output_filename, escaped_file_content, device_desc, category_name, vendor_name)) {
        _exit(1);
    }
}