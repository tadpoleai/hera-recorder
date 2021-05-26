///
/// @file camera.cpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Implementation of class ROSMessage for camera
/// @date 2019-12-27
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include "ros_message_impl.hpp"
#include "turbojpeg.h"

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
    auto topic_name = remapper->remap(topic_prefix + "image/compressed");
    if (topic_name.size() > 10 && topic_name.substr(topic_name.size() - 10) == "compressed") {
        auto data_impl = reinterpret_cast<device::data::CompressedImage*>(sensor_data.get());
        auto message = ROSMessage::create<ROSMessageType::CompressedImage>();
        auto ros_message = reinterpret_cast<sensor_msgs::CompressedImage*>(message->ptr);

        message->topic_name = topic_name;
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
            return {};
        }

        ros_message->data.resize(data_impl->image_data_size);
        memcpy(ros_message->data.data(), data_impl->image_data, data_impl->image_data_size);

        std::vector<ROSMessagePtr> ret;
        ret.emplace_back(std::move(message));
        return ret;
    } else {
        auto data_impl = reinterpret_cast<device::data::CompressedImage*>(sensor_data.get());
        auto message = ROSMessage::create<ROSMessageType::Image>();
        auto ros_message = reinterpret_cast<sensor_msgs::Image*>(message->ptr);

        message->topic_name = topic_name;
        message->timestamp_ns = sensor_data->timestamp_intrinsic_ns;
        ros_message->header.seq = sensor_data->sequence;
        ros_message->header.stamp = to_ros_time(sensor_data->timestamp_intrinsic_ns);
        ros_message->header.frame_id = frame_id;

        // Check if format is jpeg
        switch (data_impl->compress_format) {
        case device::data::CompressedImage::CompressFormat::JPEG:
            break;
        case device::data::CompressedImage::CompressFormat::PNG:
            // PNG is not supported yet
            return {};
            break;
        default:
            log::error << "Converter: Invalid Compress Format:" << static_cast<int>(data_impl->compress_format)
                       << log::endl;
            return {};
        }

        auto tj_instance = tjInitDecompress();
        if (!tj_instance) {
            return {};
        }
        uint8_t* src_jpeg_buff = data_impl->image_data;
        size_t src_jpeg_size = data_impl->image_data_size;

        int width, height;
        int subsamp, colorspace;
        if (tjDecompressHeader3(tj_instance, src_jpeg_buff, src_jpeg_size, &width, &height, &subsamp, &colorspace) <
            0) {
            log::warn << "Converter: Can not decompress jpeg" << log::endl;
            tjDestroy(tj_instance);
            return {};
        }

        constexpr auto pixel_format = TJPF_BGR;
        ros_message->height = height;
        ros_message->width = width;
        ros_message->step = width * tjPixelSize[pixel_format];
        ros_message->is_bigendian = 0;
        ros_message->encoding = "bgr8";
        ros_message->data.resize(width * height * tjPixelSize[pixel_format]);

        if (tjDecompress2(tj_instance,
                          src_jpeg_buff,
                          src_jpeg_size,
                          ros_message->data.data(),
                          width,
                          0,
                          height,
                          pixel_format,
                          0) != 0) {
            log::warn << "Converter: Can not decompress jpeg" << log::endl;
            tjDestroy(tj_instance);
            return {};
        }

        std::vector<ROSMessagePtr> ret;
        ret.emplace_back(std::move(message));
        tjDestroy(tj_instance);
        return ret;
    }
}

template<>
std::vector<ROSMessagePtr> ROSMessage::convert<device::SensorDataType::Image>(device::data::SensorDataPtr& sensor_data,
                                                                              const std::string& topic_prefix,
                                                                              const std::string& frame_id,
                                                                              const common::Remapper* remapper)
{
    auto data_impl = reinterpret_cast<device::data::Image*>(sensor_data.get());
    auto message = ROSMessage::create<ROSMessageType::Image>();
    auto ros_message = reinterpret_cast<sensor_msgs::Image*>(message->ptr);
    std::vector<ROSMessagePtr> ret;

    message->topic_name = remapper->remap(topic_prefix + "image");
    message->timestamp_ns = sensor_data->timestamp_intrinsic_ns;
    ros_message->header.seq = sensor_data->sequence;
    ros_message->header.stamp = to_ros_time(sensor_data->timestamp_intrinsic_ns);
    ros_message->header.frame_id = frame_id;

    ros_message->height = data_impl->image_meta.rows;
    ros_message->width = data_impl->image_meta.cols;
    ros_message->step = data_impl->image_meta.stride;
    ros_message->is_bigendian = 0;

    switch (data_impl->image_meta.pixel_format) {
    case device::data::Image::PixelFormat::Mono8:
        ros_message->encoding = "mono8";
        break;
    case device::data::Image::PixelFormat::Mono16:
        ros_message->encoding = "mono16";
        break;
    case device::data::Image::PixelFormat::RGB8:
        ros_message->encoding = "rgb8";
        break;
    case device::data::Image::PixelFormat::BGR8:
        ros_message->encoding = "bgr8";
        break;
    case device::data::Image::PixelFormat::RGB16:
        ros_message->encoding = "rgb16";
        break;
    case device::data::Image::PixelFormat::BGR16:
        ros_message->encoding = "bgr16";
        break;
    case device::data::Image::PixelFormat::Raw8:
        switch (data_impl->image_meta.bayer_format) {
        case device::data::Image::BayerFormat::RGGB:
            ros_message->encoding = "bayer_rggb8";
            break;
        case device::data::Image::BayerFormat::GRBG:
            ros_message->encoding = "bayer_grbg8";
            break;
        case device::data::Image::BayerFormat::GBRG:
            ros_message->encoding = "bayer_gbrg8";
            break;
        case device::data::Image::BayerFormat::BGGR:
            ros_message->encoding = "bayer_bggr8";
            break;
        default:
            log::error << "Converter: Invalid BayerFormat:" << static_cast<int>(data_impl->image_meta.bayer_format)
                       << log::endl;
            return ret;
        }
        break;
    case device::data::Image::PixelFormat::Raw12:
        switch (data_impl->image_meta.bayer_format) {
        case device::data::Image::BayerFormat::RGGB:
            ros_message->encoding = "bayer_rggb12";
            break;
        case device::data::Image::BayerFormat::GRBG:
            ros_message->encoding = "bayer_grbg12";
            break;
        case device::data::Image::BayerFormat::GBRG:
            ros_message->encoding = "bayer_gbrg12";
            break;
        case device::data::Image::BayerFormat::BGGR:
            ros_message->encoding = "bayer_bggr12";
            break;
        default:
            log::error << "Converter: Invalid BayerFormat:" << static_cast<int>(data_impl->image_meta.bayer_format)
                       << log::endl;
            return ret;
        }
        break;

    case device::data::Image::PixelFormat::Raw16:
        switch (data_impl->image_meta.bayer_format) {
        case device::data::Image::BayerFormat::RGGB:
            ros_message->encoding = "bayer_rggb16";
            break;
        case device::data::Image::BayerFormat::GRBG:
            ros_message->encoding = "bayer_grbg16";
            break;
        case device::data::Image::BayerFormat::GBRG:
            ros_message->encoding = "bayer_gbrg16";
            break;
        case device::data::Image::BayerFormat::BGGR:
            ros_message->encoding = "bayer_bggr16";
            break;
        default:
            log::error << "Converter: Invalid BayerFormat:" << static_cast<int>(data_impl->image_meta.bayer_format)
                       << log::endl;
            return ret;
        }
        break;

    default:
        log::error << "Converter: Invalid Format:" << static_cast<int>(data_impl->image_meta.pixel_format) << "\n"
                   << "Please add format in convert/src/ros_message_impl/camera.cpp" << log::endl;
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
