#include <stdio.h>

#include <hera/device/include.hpp>

#include "unistd.h"

using namespace wayz::hera;

void handle_image_data(device::data::CompressedImage* data_ptr);

bool handle_storage_data(device::data::DeviceDataPtr data_ptr);

int main(int argc, const char** argv)
{
    log::onlyprint();
    log::set_level(log::LogLevel::Error);

    if (argc != 2 && argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <hera_file> [image.idcs]" << std::endl;
        return 1;
    }

    auto storage = storage::StorageManager::open(argv[1], true, true, false);
    if (storage->header) {
        std::cout << *storage->header << std::endl;
    } else {
        std::cout << "Can not read " << argv[1] << std::endl;
    }

    std::ofstream ofs_idcs;
    std::ifstream ifs_idcs;
    if (argc != 3) {
        std::ofstream ofs_idcs;
        ofs_idcs.open(std::string(argv[1]) + ".image.idcs", std::ios::binary);
        decltype(storage->read_data_and_index()) storage_data_and_index;

        while (storage_data_and_index = storage->read_data_and_index(), storage_data_and_index.data) {
            // std::cout << "Read data: id = " << storage_data_and_index.data->get_device_id() << std::endl;
            if (handle_storage_data(storage_data_and_index.data)) {
                ofs_idcs.write((char*)&storage_data_and_index.index, 8);
            }
        }
    } else {
        ifs_idcs.open(argv[2], std::ios::binary);
        decltype(storage->read()) storage_data;
        int64_t index;

        while (ifs_idcs.read((char*)(&index), 8), ifs_idcs.gcount()) {
            storage_data = storage->read_data_by_index(index);
            handle_storage_data(storage_data);
        }
    }

    std::cout << "Read finished" << std::endl;
}

bool handle_storage_data(device::data::DeviceDataPtr data_ptr)
{
    auto sensor_data = device::Factory::convert(data_ptr, nullptr);
    if (!sensor_data) {
        return false;
    }
    switch (sensor_data->sensor_data_type) {
    case device::SensorDataType::CompressedImage:
        std::cout << "Got Image data" << std::endl;
        handle_image_data(reinterpret_cast<device::data::CompressedImage*>(sensor_data.get()));
        return true;
    default:
        // std::cout << "Not Image data, skipping" << std::endl;
        return false;
    }
}

void handle_image_data(device::data::CompressedImage* data_ptr)
{
    std::cout << "- JPEG Size = " << data_ptr->image_data_size << std::endl;
}