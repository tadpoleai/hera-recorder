///
/// @file converter.cpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Implementation of class Converter
/// @version 0.1
/// @date 2019-11-13
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include "converter.hpp"

#include "common/logger/logger.hpp"
#include "device/include.hpp"

namespace wayz {
namespace hera {
namespace convert {

/// Scan the recorded data,
/// Open the bag file for writing, and init the threads
Converter::Converter(const std::string& src_filename,
                     const std::string& bagfile,
                     common::RemapperPtr&& remapper,
                     const bool only_show) :
    running_(false),
    read_thread_(nullptr),
    bag_thread_(nullptr),
    remapper_(std::move(remapper)),
    progress_(0),
    total_duration_(0),
    storage_(storage::StorageManager::open(src_filename, true)),
    publish_sem_(new sem_t),
    receive_sem_(new sem_t),
    message(std::move(ROSMessage::create<ROSMessageType::BrokenData>()))
{
    sem_init(publish_sem_, 0, 1);
    sem_init(receive_sem_, 0, 0);

    if (storage_->header != nullptr) {
        if (!only_show) {
            log::info << "Converter: Printing info\n" << *storage_->header << log::endl;
        } else {
            std::cout << *storage_->header << std::endl;
            return;
        }
        total_duration_ = storage_->header->timestamp_end - storage_->header->timestamp_start;

        for (auto& device_name : storage_->header->device_names) {
            std::array<std::string, 3> tokens;
            std::stringstream device_name_ss(std::string(device_name.begin(), device_name.end()));
            log::debug << "Converter: Adding " << device_name << log::endl;

            for (auto&& token : tokens) {
                if (!getline(device_name_ss, token, '/')) {
                    log::error << "Converter: Can not determine topic name for " << device_name << log::endl;
                    return;
                }
                log::debug << "Converter: " << token << log::endl;
            }

            topic_prefixes_.emplace_back(std::string("/") + tokens[0] + "/" + tokens[2] + "/");
            frame_ids_.emplace_back(remapper_->remap(tokens[0] + "_" + tokens[2] + "_link"));
        }

        bag_.open(bagfile, false);
        if (bag_.is_open()) {
            log::debug << "Converter: Bag file opened as " << bagfile << log::endl;
            running_ = true;
            read_thread_ = new std::thread(&Converter::read_thread_function, this);
            bag_thread_ = new std::thread(&Converter::bag_thread_function, this);
        } else {
            log::error << "Converter: Can not open bag file " << bagfile << log::endl;
        }

    } else {
        log::error << "Converter: Can not open hera record file " << src_filename << log::endl;
        return;
    }
}

/// Wait for thread to join, and close bag file
///
Converter::~Converter()
{
    if (read_thread_) {
        read_thread_->join();
    }

    if (bag_thread_) {
        bag_thread_->join();
    }

    sem_destroy(publish_sem_);
    sem_destroy(receive_sem_);

    if (bag_.is_open()) {
        log::info << "Flushing to file system" << log::endl;
        log::info << "This may take a while depending on destination file system" << log::endl;
        bag_.close();
        log::info << "Flushed to file system" << log::endl;
    }
}

void Converter::read_thread_function()
{
    decltype(storage_->read()) data = nullptr;
    while ((data = storage_->read())) {
        auto sensor_data = device::DeviceFactory::convert(data);
        if (sensor_data->sensor_data_type != device::SensorDataType::Broken) {
            try {
                const auto& topic_prefix = topic_prefixes_[sensor_data->sensor_id];
                const auto& frame_id = frame_ids_[sensor_data->sensor_id];
                auto ros_messages = ROSMessage::convert(sensor_data, topic_prefix, frame_id, remapper_.get());
                progress_ = data->get_timestamp_receive_ns() - storage_->header->timestamp_start;
                for (auto&& ros_message : ros_messages) {
                    publish(std::move(ros_message));
                }

            } catch (std::exception& e) {
                log::warn << "Converter: Error occured when reading, that " << e.what() << log::endl;
            }
        }
    }
    publish(std::move(ROSMessage::create<ROSMessageType::EndOfFile>()));
}

void Converter::bag_thread_function()
{
    while (true) {
        auto message = std::move(receive());
        if (message->type != ROSMessageType::EndOfFile) {
            bag_ << std::move(message);
        } else {
            running_ = false;
            break;
        }
    }
}

/// Wait publish_sem_ and move from in_message to message,
/// then notify receiver by post receive_sem_
void Converter::publish(ROSMessagePtr&& in_message)
{
    sem_wait(publish_sem_);
    message = std::move(in_message);
    sem_post(receive_sem_);
}

/// Wait receive_sem_ and move from message to return,
/// then notify publisher by post publish_sem_
ROSMessagePtr Converter::receive()
{
    sem_wait(receive_sem_);
    auto out_message = std::move(message);
    sem_post(publish_sem_);
    return out_message;
}

}  // namespace convert
}  // namespace hera
}  // namespace wayz
