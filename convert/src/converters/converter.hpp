//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#pragma once

#include <cstdint>
#include <fstream>
#include <mutex>
#include <semaphore.h>
#include <string>
#include <thread>
#include <vector>

#include <common/data_def/device_types.hpp>
#include <common/tron_errno.h>
#include <devices/all_devices.hpp>

#include "../defs/defs.hpp"
#include "../utils/utils.hpp"

namespace wayz {
namespace tron {

class Converter;

struct ConvertedData {
    MsgType msg_type;
    void* msg;
    int64_t timestamp_ns;
    std::string topic_name;
    ConvertedData() : msg_type(MsgType::Invalid), msg(nullptr) {}
};

struct ConverterHandler {
    Converter* converter;
    sem_t* sem_writer;
    sem_t* sem_converter;
    ConvertedData data;
    ConverterHandler();
    ~ConverterHandler();
};

class Converter {
public:
    Converter(const std::string& device_type,
              const std::string& device_name,
              const std::string& device_data_folder,
              ConverterHandler* handler);
    Converter(const Device&) = delete;
    Converter& operator=(const Converter&) = delete;
    virtual ~Converter();

    int64_t get_converted_size() const;

protected:
    int64_t converted_size_;
    std::string frame_id_;
    std::string topic_name_prefix_;

    std::thread* thread_;
    ConverterHandler* managed_this_;

private:
    void convert_thread_function();
    bool open_device_data_file();
    std::shared_ptr<DeviceRawData> read_one_data();
    virtual bool convert_one_data(const std::shared_ptr<DeviceRawData>& raw_data) = 0;

    bool inited_;
    bool finished_;

    std::string device_data_folder_;
    static const int64_t FileNameWidth = 4;
    int64_t file_number_counter_;
    std::ifstream device_data_file_;
};

}  // namespace tron
}  // namespace wayz