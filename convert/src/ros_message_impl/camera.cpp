///
/// @file camera.cpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Implementation of class ROSMessage for camera
/// @date 2019-12-27
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include "../ros_message_impl.hpp"

namespace wayz {
namespace hera {
namespace convert {

template<>
std::vector<ROSMessagePtr> ROSMessage::convert<device::SensorDataType::CompressedImage>(
        device::data::SensorDataPtr& sensor_data,
        const std::string& topic_prefix,
        const std::string& frame_id,
        const common::Remapper* remapper)
{
    auto data_impl = reinterpret_cast<device::data::CompressedImage*>(sensor_data.get());
    auto message = ROSMessage::create<ROSMessageType::CompressedImage>();
    auto ros_message = reinterpret_cast<sensor_msgs::CompressedImage*>(message->ptr);
    std::vector<ROSMessagePtr> ret;

    message->topic_name = remapper->remap(topic_prefix + "compressed_image");
    message->timestamp_ns = sensor_data->timestamp_intrinsic_ns;
    ros_message->header.seq = sensor_data->sequence;
    ros_message->header.stamp = to_ros_time(sensor_data->timestamp_intrinsic_ns);
    ros_message->header.frame_id = frame_id;

    switch (data_impl->compress_format) {
    case device::data::CompressedImage::CompressFormat::JPEG:
        ros_message->format = "jpeg";
        break;
    case device::data::CompressedImage::CompressFormat::PNG:
        ros_message->format = "png";
        break;
    default:
        log::error << "Converter: Invalid Compress Format:" << static_cast<int>(data_impl->compress_format)
                   << log::endl;
        return ret;
    }

    ros_message->data.resize(data_impl->image_data_size);
    memcpy(ros_message->data.data(), data_impl->image_data, data_impl->image_data_size);

    ret.emplace_back(std::move(message));
    return ret;
}

}  // namespace convert
}  // namespace hera
}  // namespace wayz
