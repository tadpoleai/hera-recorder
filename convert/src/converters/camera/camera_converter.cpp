//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include "camera_converter.hpp"

#include <devices/src/camera/camera.hpp>
#include <sensor_msgs/CompressedImage.h>

namespace wayz {
namespace tron {

CameraConverter::CameraConverter(const std::string& device_type,
                                 const std::string& device_name,
                                 const std::string& device_data_folder,
                                 const std::string& optional_frame_id,
                                 const std::vector<std::string>& optional_topics,
                                 ConverterHandler* handler) :
    Converter(device_type, device_name, device_data_folder, optional_frame_id, handler)
{
    if (optional_topics.size() > 0) {
        image_topic_name_ = optional_topics[0];
    } else {
        image_topic_name_ = topic_name_prefix_ + "compressed_image";
    }
}

CameraConverter::~CameraConverter()
{
    if (thread_) {
        thread_->join();
    }
}

bool CameraConverter::convert_one_data(const std::shared_ptr<DeviceRawData>& raw_data)
{
    if (!raw_data) {
        sem_wait(managed_this_->sem_converter);
        managed_this_->data.msg_type = MsgType::Invalid;
        managed_this_->data.msg = nullptr;
        managed_this_->data.timestamp_ns = 0;
        managed_this_->data.topic_name = "";
        sem_post(managed_this_->sem_writer);
        return false;
    }

    DeviceRawDataImage* raw_data_image =
            reinterpret_cast<DeviceRawDataImage*>(raw_data->rawdata_buf);
    auto ros_time_intrinsic = to_ros_time(raw_data_image->timestamp_intrinsic_ns);

    auto image_msg = new sensor_msgs::CompressedImage;
    image_msg->header.stamp = ros_time_intrinsic;
    image_msg->header.seq = raw_data->sequence;
    image_msg->header.frame_id = frame_id_;
    image_msg->format = "jpeg";
    image_msg->data.resize(raw_data_image->compressed_data_size);
    memcpy(image_msg->data.data(), raw_data_image->data, raw_data_image->compressed_data_size);

    sem_wait(managed_this_->sem_converter);
    managed_this_->data.msg_type = MsgType::SensorMsgsCompressedImage;
    managed_this_->data.msg = reinterpret_cast<void*>(image_msg);
    managed_this_->data.timestamp_ns = raw_data_image->timestamp_intrinsic_ns;
    managed_this_->data.topic_name = image_topic_name_;
    sem_post(managed_this_->sem_writer);

    return true;
}

}  // namespace tron
}  // namespace wayz