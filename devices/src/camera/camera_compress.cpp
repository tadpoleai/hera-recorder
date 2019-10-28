//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include <cstdlib>
#include <string>
#include <turbojpeg.h>
#include <vector>

#include <common/logger/logger.hpp>
#include <flycapture/FlyCapture2.h>

#include "camera.hpp"

namespace wayz {
namespace tron {

std::shared_ptr<DeviceRawData> Camera::compress(const std::shared_ptr<DeviceRawData>& rawdata)
{
    if (!rawdata) {
        return nullptr;
    }

    auto data_image = reinterpret_cast<DeviceRawDataImage*>(rawdata->rawdata_buf);

    // Convert raw image into RGB
    FlyCapture2::Image raw_image(data_image->rows,
                                 data_image->cols,
                                 data_image->stride,
                                 data_image->data,
                                 data_image->data_size,
                                 data_image->data_size,
                                 static_cast<FlyCapture2::PixelFormat>(data_image->format),
                                 static_cast<FlyCapture2::BayerTileFormat>(data_image->bayer));
    raw_image.SetColorProcessing(FlyCapture2::EDGE_SENSING);

    FlyCapture2::Image converted_image;
    auto error = raw_image.Convert(FlyCapture2::PIXEL_FORMAT_RGB8, &converted_image);
    if (error != FlyCapture2::PGRERROR_OK) {
        Logger::error() << "Camera/" << get_name() << " failed to convert rawdata in to rgb8"
                        << error.GetDescription() << Logger::endl;
        return nullptr;
    }

    // Start Compression
    auto tj_instance = tjInitCompress();
    if (!tj_instance) {
        Logger::error() << "Camera/" << get_name() << " failed to compress" << Logger::endl;
        return nullptr;
    }

    uint8_t* dst_jpeg = nullptr;
    uint64_t dst_jpeg_size = 0;
    tjCompress2(tj_instance,
                converted_image.GetData(),
                data_image->cols,
                0,
                data_image->rows,
                TJPF_RGB,
                &dst_jpeg,
                &dst_jpeg_size,
                TJSAMP_444,
                95,
                TJFLAG_FASTDCT);
    tjDestroy(tj_instance);
    tj_instance = nullptr;

    // New a buff to contain compressed data
    int32_t total_compressed_length =
            sizeof(DeviceRawData) + sizeof(DeviceRawDataImage) + dst_jpeg_size;
    auto compressed_rawdata = std::shared_ptr<DeviceRawData>(
            reinterpret_cast<DeviceRawData*>(new uint8_t[total_compressed_length]));
    auto compressed_data_image =
            reinterpret_cast<DeviceRawDataImage*>(compressed_rawdata->rawdata_buf);

    // Copy Header
    compressed_rawdata->length = total_compressed_length;
    compressed_rawdata->device_data_type = DeviceDataType::ImageJpeg;
    compressed_rawdata->sequence = rawdata->sequence;
    compressed_rawdata->timestamp_receive_ns = rawdata->timestamp_receive_ns;
    compressed_data_image->timestamp_intrinsic_ns = data_image->timestamp_intrinsic_ns;

    // Set as compressed
    compressed_data_image->is_compressed = 1;
    compressed_data_image->format = -1;  // Mark -1 as Jpeg;
    compressed_data_image->compressed_data_size = dst_jpeg_size;

    // Copy Jpeg Content
    memcpy(compressed_data_image->data, dst_jpeg, dst_jpeg_size);

    return compressed_rawdata;
}

}  // namespace tron
}  // namespace wayz