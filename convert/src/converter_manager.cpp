//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include "converter_manager.hpp"

#include <iostream>

#include <common/logger/logger.hpp>

namespace wayz {
namespace tron {

ConverterManager::ConverterManager(const std::string& bag_filepath,
                                   const std::string& device_data_folder) :
    bag_(nullptr),
    thread_(nullptr),
    device_data_folder_(device_data_folder),
    handler_to_wait_(nullptr)
{
    if (!open_bag(bag_filepath)) {
        return;
    }

    auto imu_handler = std::make_shared<ConverterHandler>();
    imu_handler->converter = new ImuConverter("imu",
                                              "internal",
                                              "../20191018103800_record_test_1min/Imu/internal/",
                                              imu_handler.get());

    auto lidar0_handler = std::make_shared<ConverterHandler>();
    lidar0_handler->converter = new LidarConverter("lidar",
                                                   "top",
                                                   "../20191018103800_record_test_1min/Lidar/top/",
                                                   lidar0_handler.get());

    auto lidar1_handler = std::make_shared<ConverterHandler>();
    lidar1_handler->converter = new LidarConverter("lidar",
                                                   "back",
                                                   "../20191018103800_record_test_1min/Lidar/back/",
                                                   lidar1_handler.get());

    converter_handlers_.emplace_back(imu_handler);
    converter_handlers_.emplace_back(lidar0_handler);
    converter_handlers_.emplace_back(lidar1_handler);

    thread_ = new std::thread(&ConverterManager::write_thread_function, this);
}

ConverterManager::~ConverterManager()
{
    converter_handlers_.clear();
    if (thread_) {
        thread_->join();
    }
    close_bag();
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
        Logger::error() << "Converter: Can not open bagfile" << bag_filepath << Logger::endl;
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
    while (true) {
        if (handler_to_wait_ == nullptr) {
            for (const auto& handler : converter_handlers_) {
                sem_wait(handler->sem_writer);
            }
        } else {
            sem_wait(handler_to_wait_->sem_writer);
        }

        int64_t oldest_data_time = LONG_MAX;
        handler_to_wait_ = nullptr;
        for (const auto& handler : converter_handlers_) {
            if (handler->data.msg != nullptr) {
                if (oldest_data_time > handler->data.timestamp_ns) {
                    oldest_data_time = handler->data.timestamp_ns;
                    handler_to_wait_ = handler.get();
                }
            }
        }
        if (!handler_to_wait_) {
            Logger::info() << "Converter: Conversion Completed" << Logger::endl;
            break;
        }

        write_and_free_one_converted_msg(&handler_to_wait_->data);
        sem_post(handler_to_wait_->sem_converter);
    }
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
    default:
        return;
    }
}

}  // namespace tron
}  // namespace wayz