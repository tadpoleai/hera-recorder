#include <stdio.h>

#include <hera/device/include.hpp>

#include "unistd.h"

using namespace wayz::hera;

void handle_image_data(device::data::Image* data_ptr);

int main(int argc, const char** argv)
{
    log::onlyprint();

    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <hera_file>" << std::endl;
        return 1;
    }

    auto storage = storage::StorageManager::open(argv[1], true, true, false);
    if (storage->header) {
        std::cout << *storage->header << std::endl;
    } else {
        std::cout << "Can not read " << argv[1] << std::endl;
    }

    while (auto storage_data = storage->read()) {
        std::cout << "Read data: id = " << storage_data->get_device_id() << std::endl;

        auto sensor_data = device::Factory::convert(storage_data, nullptr);
        switch (sensor_data->sensor_data_type) {
        case device::SensorDataType::Image:
            std::cout << "Got Image data" << std::endl;
            handle_image_data(reinterpret_cast<device::data::Image*>(sensor_data.get()));
            break;
        default:
            std::cout << "Not Image data, skipping" << std::endl;
            break;
        }
    }

    std::cout << "Read finished" << std::endl;
}

void handle_image_data(device::data::Image* data_ptr)
{
    std::cout << "- Image height = " << data_ptr->image_meta.rows << std::endl;
    std::cout << "- Image width = " << data_ptr->image_meta.cols << std::endl;
    std::cout << "- Image format = " << int(data_ptr->image_meta.pixel_format) << std::endl;
    std::cout << "- Image bayer = " << int(data_ptr->image_meta.bayer_format) << std::endl;
}