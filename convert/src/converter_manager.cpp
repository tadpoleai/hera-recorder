//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include "converter_manager.hpp"

#include <iostream>

#include <common/logger/logger.hpp>
#include <common/utils/get_folder_content.hpp>

namespace wayz {
namespace tron {

ConverterManager::ConverterManager(const std::string& bag_filepath,
                                   const std::string& device_data_folder) :
    bag_(nullptr),
    total_size_(0),
    thread_(nullptr),
    running_(false),
    device_data_folder_(device_data_folder)
{
    auto content = get_folder_content(device_data_folder);
    if (!content.opened) {
        Logger::error() << "Converter: Can not open folder '" << device_data_folder << "' to read"
                        << Logger::endl;
        return;
    }
    if (!content.folders.size()) {
        Logger::error() << "Converter: Folder '" << device_data_folder << "' is empty"
                        << Logger::endl;
        return;
    }
    if (!open_bag(bag_filepath)) {
        Logger::error() << "Converter: Can not open bagfile '" << bag_filepath << "' to write"
                        << Logger::endl;
        return;
    }
    Logger::info() << "Converter: Bagfile '" << bag_filepath << "' created" << Logger::endl;

    Logger::info() << "Converter: Scanning devices" << Logger::endl;
    for (const auto& subdir : content.folders) {
        auto type = DeviceType::_from_string_nocase_nothrow(subdir.basename.c_str());
        if (!type) {
            continue;
        }
        auto devices = get_folder_content(subdir.fullname).folders;

        for (const auto& device : devices) {
            // Get Total size
            auto device_size = get_folder_content(device.fullname).total_size;

            // Create handler
            auto handler = std::make_shared<ConverterHandler>();

            switch (type.value()) {
            case DeviceType::Imu:
                handler->converter = new ImuConverter(subdir.basename,
                                                      device.basename,
                                                      device.fullname,
                                                      handler.get());
                break;
            case DeviceType::Lidar:
                handler->converter = new LidarConverter(subdir.basename,
                                                        device.basename,
                                                        device.fullname,
                                                        handler.get());
                break;
            case DeviceType::Camera:
                handler->converter = new CameraConverter(subdir.basename,
                                                         device.basename,
                                                         device.fullname,
                                                         handler.get());
                break;
            default:
                continue;
            }
            converter_handlers_.emplace_back(handler);
            total_size_ = total_size_ + device_size;
            Logger::info() << "Converter: Add " << subdir.basename << "/" << device.basename
                           << ", sized " << device_size << Logger::endl;
        }
    }

    Logger::info() << "Converter: Scanning Completed" << Logger::endl;
    Logger::info() << "Converter: " << converter_handlers_.size()
                   << " Devices, Total Size: " << total_size_ << Logger::endl;
    running_ = true;
    thread_ = new std::thread(&ConverterManager::write_thread_function, this);
    Logger::info() << "Converter: Converision Started" << Logger::endl;
}

ConverterManager::~ConverterManager()
{
    converter_handlers_.clear();
    if (thread_) {
        thread_->join();
    }
    close_bag();
}

FileSize ConverterManager::report_progress()
{
    FileSize size(0);
    for (const auto& handler : converter_handlers_) {
        size.size += handler->converter->get_converted_size();
    }
    return size;
}

bool ConverterManager::running() const
{
    return running_;
}

bool ConverterManager::open_bag(const std::string& bag_filepath)
{
    if (bag_) {
        bag_->close();
        delete bag_;
        bag_ = nullptr;
    }

    try {
        bag_ = new rosbag_direct_write::DirectBag(bag_filepath, false);
        return true;
    } catch (...) {
        Logger::error() << "Converter: Can not open bagfile '" << bag_filepath << "'"
                        << Logger::endl;
        bag_ = nullptr;
        return false;
    }
}

bool ConverterManager::close_bag()
{
    if (bag_) {
        bag_->close();
        return true;
    }
    return false;
}

void ConverterManager::write_thread_function()
{
    ConverterHandler* oldest_handler = nullptr;
    while (true) {
        if (oldest_handler == nullptr) {
            for (const auto& handler : converter_handlers_) {
                sem_wait(handler->sem_writer);
            }
        } else {
            sem_wait(oldest_handler->sem_writer);
        }

        int64_t oldest_data_time = LONG_MAX;
        oldest_handler = nullptr;
        for (const auto& handler : converter_handlers_) {
            if (handler->data.msg != nullptr) {
                if (oldest_data_time > handler->data.timestamp_ns) {
                    oldest_data_time = handler->data.timestamp_ns;
                    oldest_handler = handler.get();
                }
            }
        }
        if (!oldest_handler) {
            break;
        }

        write_and_free_one_converted_msg(&oldest_handler->data);
        sem_post(oldest_handler->sem_converter);
    }
    running_ = false;
}

void ConverterManager::write_and_free_one_converted_msg(const ConvertedData* data)
{
    switch (data->msg_type) {
    case MsgType::Invalid:
        return;
    case MsgType::SensorMsgsImu: {
        auto msg = reinterpret_cast<sensor_msgs::Imu*>(data->msg);
        bag_->write(data->topic_name, msg->header.stamp, *msg);
        delete msg;
        return;
    }
    case MsgType::SensorMsgsMagneticField: {
        auto msg = reinterpret_cast<sensor_msgs::MagneticField*>(data->msg);
        bag_->write(data->topic_name, msg->header.stamp, *msg);
        delete msg;
        return;
    }
    case MsgType::SensorMsgsPointCloud2: {
        auto msg = reinterpret_cast<sensor_msgs::PointCloud2*>(data->msg);
        bag_->write(data->topic_name, msg->header.stamp, *msg);
        delete msg;
        return;
    }
    case MsgType::SensorMsgsCompressedImage: {
        auto msg = reinterpret_cast<sensor_msgs::CompressedImage*>(data->msg);
        bag_->write(data->topic_name, msg->header.stamp, *msg);
        delete msg;
        return;
    }
    default:
        return;
    }
}

}  // namespace tron
}  // namespace wayz