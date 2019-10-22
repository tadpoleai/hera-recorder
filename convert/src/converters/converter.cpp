//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include "converter.hpp"

namespace wayz {
namespace tron {

ConverterHandler::ConverterHandler() : converter(nullptr)
{
    sem_writer = new sem_t;
    sem_converter = new sem_t;
    sem_init(sem_writer, 0, 0);
    sem_init(sem_converter, 0, 0);
    sem_post(sem_converter);
}
ConverterHandler::~ConverterHandler()
{
    sem_destroy(sem_writer);
    sem_destroy(sem_converter);
    if (converter) {
        delete converter;
    }
}

Converter::Converter(const std::string& device_type,
                     const std::string& device_name,
                     const std::string& device_data_folder,
                     ConverterHandler* handler) :
    converted_size_(0),
    frame_id_("frame_" + device_type + "_" + device_name),
    topic_name_prefix_("/" + device_type + "/" + device_name + "/"),
    thread_(nullptr),
    managed_this_(handler),
    inited_(false),
    finished_(false),
    device_data_folder_(device_data_folder),
    file_number_counter_(0)
{
    thread_ = new std::thread(&Converter::convert_thread_function, this);
    inited_ = true;
}

Converter::~Converter() {}

int64_t Converter::get_converted_size() const
{
    return converted_size_;
}

void Converter::convert_thread_function()
{
    std::shared_ptr<DeviceRawData> raw_data;
    do {
        raw_data = read_one_data();
        convert_one_data(raw_data);
    } while (raw_data);
    finished_ = true;
}

bool Converter::open_device_data_file()
{
    if (device_data_file_.is_open()) {
        device_data_file_.close();
    }

    std::ostringstream filename;
    filename << device_data_folder_;
    filename.fill('0');
    filename.width(FileNameWidth);
    filename << file_number_counter_++;
    filename << ".bin";

    device_data_file_.open(filename.str(), std::ios::in | std::ios::binary);

    if (device_data_file_.is_open()) {
        return true;
    } else {
        return false;
    }
}

std::shared_ptr<DeviceRawData> Converter::read_one_data()
{
    if (!device_data_file_.is_open()) {
        if (!open_device_data_file()) {
            // No such file
            return nullptr;
        }
    }

    int32_t length = 0;
    device_data_file_.read(reinterpret_cast<char*>(&length), sizeof(int32_t));
    size_t size_read = device_data_file_.gcount();

    if (size_read != sizeof(int32_t)) {
        // End of file, try to read next
        device_data_file_.close();
        return read_one_data();
    }

    auto buf = new char[length];

    device_data_file_.read(buf + sizeof(int32_t), length - sizeof(int32_t));
    size_read = device_data_file_.gcount();

    if (size_read != length - sizeof(int32_t)) {
        // Error on reading data
        delete[] buf;
        return nullptr;
    }

    auto rawdata = std::shared_ptr<DeviceRawData>(reinterpret_cast<DeviceRawData*>(buf));
    rawdata->length = length;
    converted_size_ += length;
    return rawdata;
}

}  // namespace tron
}  // namespace wayz