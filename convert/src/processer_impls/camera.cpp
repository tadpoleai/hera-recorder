#include "../processer.hpp"

namespace wayz {
namespace hera {
namespace convert {

template<>
void Processer::process<SensorDataType::CompressedImage>(SensorDataPtr& data)
{
    auto data_impl = reinterpret_cast<camera::CompressedImageSensorData*>(data.get());
    auto message = ROSMessage::create<ROSMessageType::CompressedImage>();
    auto ros_message = reinterpret_cast<sensor_msgs::CompressedImage*>(message->ptr);
    message->topic_name = remap(topic_prefix_ + "compressed_image");
    message->timestamp_ns = data->timestamp_intrinsic_ns;
    ros_message->header.seq = data->sequence;
    ros_message->header.stamp = to_ros_time(data->timestamp_intrinsic_ns);
    ros_message->header.frame_id = frame_id_;

    switch (data_impl->compress_format) {
    case camera::CompressFormat::JPEG:
        ros_message->format = "jpeg";
        break;
    case camera::CompressFormat::PNG:
        ros_message->format = "png";
        break;
    default:
        log::error << "Converter: Invalid Compress Format:" << static_cast<int>(data_impl->compress_format)
                   << log::endl;
        return;
    }

    ros_message->data.resize(data_impl->image_data_size);
    memcpy(ros_message->data.data(), data_impl->image_data, data_impl->image_data_size);

    publish(std::move(message));
}

}  // namespace convert
}  // namespace hera
}  // namespace wayz