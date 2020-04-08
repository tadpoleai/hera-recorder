///
/// @file flir.cpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Implementation of class Flir
/// @version 0.1
/// @date 2019-11-08
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include "flir.hpp"

#ifdef WITH_DRIVER
#include <turbojpeg.h>
#endif

#include <arpa/inet.h>
#include <common/include/logger/logger.hpp>

namespace wayz {
namespace hera {
namespace device {
namespace camera {
namespace flir {

const std::vector<DeviceParameterType> Flir::EssentialParameterTypes = {DeviceParameterType::IpAddress};

const std::vector<DeviceParameterType> Flir::OptionalParameterTypes = {DeviceParameterType::Exposure,
                                                                       DeviceParameterType::WhiteBalanceRed,
                                                                       DeviceParameterType::WhiteBalanceBlue,
                                                                       DeviceParameterType::FrameRate};

#ifdef WITH_DRIVER
#ifdef DEVICE_DRIVER_FLIR

/// Convert IP address to uint32_t(little-endian),
/// after that, use FLIR's SDK BusManager to connect.
///
/// Set SDK into buffer mode, and set grabbing timeout
HeraErrno Flir::connect()
{
    uint32_t ip_int;
    try {
        ip_ = parameters_[DeviceParameterType::IpAddress];
        ip_int = ntohl(inet_addr(ip_.c_str()));
    } catch (...) {
        return handle_error(HeraErrno::InvalidParameterValue);
    }

    // Connect to Camera
    FlyCapture2::Error error;
    FlyCapture2::BusManager bus_manager;
    FlyCapture2::IPAddress flycapture_ipv4(ip_int);
    error = bus_manager.GetCameraFromIPAddress(flycapture_ipv4, &guid_);
    if (error != FlyCapture2::PGRERROR_OK) {
        return handle_flir_error(error);
    }
    error = camera_.Connect(&guid_);
    if (error != FlyCapture2::PGRERROR_OK) {
        return handle_flir_error(error);
    }

    // Get Camera Info
    FlyCapture2::CameraInfo camera_info;
    error = camera_.GetCameraInfo(&camera_info);
    if (error != FlyCapture2::PGRERROR_OK) {
        return handle_flir_error(error);
    }
    log::info << "Flir: " << get_name() << " serial is " << camera_info.serialNumber << log::endl;

    // Set Driver to Buffer Mode
    FlyCapture2::FC2Config config;
    error = camera_.GetConfiguration(&config);
    if (error != FlyCapture2::PGRERROR_OK) {
        return handle_flir_error(error);
    }
    config.grabTimeout = GrabTimeoutMs_;
    config.numBuffers = NumBuffers_;
    config.grabMode = FlyCapture2::BUFFER_FRAMES;
    error = camera_.SetConfiguration(&config);
    if (error != FlyCapture2::PGRERROR_OK) {
        return handle_flir_error(error);
    }

    error = camera_.StartCapture();
    if (error != FlyCapture2::PGRERROR_OK) {
        return handle_flir_error(error);
    }

    return HeraErrno::Success;
}

/// Call Flir SDK StopCapture() and Disconnect()
///
void Flir::disconnect()
{
    camera_.StopCapture();
    camera_.Disconnect();
}

/// Calculate time::Timestamp by time::Timestamp_calculator and check synced.
/// Then convert fetched image into RGB and use libjpeg_turbo to
/// compress to jpeg format
/// @todo Add a parameter of storage format, CompressedImage or RawImage
/// @note Compression took ~10ms per frame sized 1280x720 on i5-8550U
data::DeviceDataPtr Flir::fetch()
{
    // Fetch rawdata from camera;
    FlyCapture2::Image raw_image;
    auto error = camera_.RetrieveBuffer(&raw_image);

    if (error != FlyCapture2::PGRERROR_OK) {
        log::warn << "Flir: " << get_name() << " failed to retrieve due to " << error.GetDescription() << log::endl;
        return nullptr;
    }

    // Check sync
    auto now = time::Timestamp::now();
    int64_t timestamp_intrinsic_ns;
    auto metadata = raw_image.GetMetadata();
    bool synced = timestamp_calculator_.get_intrinsic_time(
            // Calculate timestamp
            timestamp_intrinsic_ns,
            now,
            flir::FlirEmbeddedTimestamp(metadata.embeddedTimeStamp),
            flir::FlirEmbeddedShutter(metadata.embeddedShutter));
    if (!synced) {
        log::warn << "Flir: " << get_name() << " not synced, data abandoned" << log::endl;
        return nullptr;
    }

    // Convert raw image into RGB
    /// @todo Add parameter for ColorProcessing algorithm
    raw_image.SetColorProcessing(FlyCapture2::EDGE_SENSING);
    FlyCapture2::Image converted_image;
    error = raw_image.Convert(FlyCapture2::PIXEL_FORMAT_RGB8, &converted_image);
    if (error != FlyCapture2::PGRERROR_OK) {
        log::warn << "Flir: " << get_name() << " can not convert, data abandoned" << log::endl;
        return nullptr;
    }

    // Start Compression
    auto tj_instance = tjInitCompress();
    if (!tj_instance) {
        log::warn << "Flir: " << get_name() << " can not compress, data abandoned" << log::endl;
        return nullptr;
    }
    uint8_t* jpeg_image = nullptr;
    size_t jpeg_image_size = 0;

    /// @todo Check source image format
    /// @todo Add parameter for Sampling and Quality
    tjCompress2(tj_instance,
                converted_image.GetData(),
                converted_image.GetCols(),
                0,
                converted_image.GetRows(),
                TJPF_BGR,          // Source Image Format
                &jpeg_image,       // Output Image Pointer
                &jpeg_image_size,  // Output Image Size
                TJSAMP_422,        // YUV Binning
                95,                // Quality
                TJFLAG_FASTDCT);

    // Total length of device data
    auto length = sizeof(FlirCompressedImage) + jpeg_image_size;
    auto data = data::DeviceData::create(length,
                                         id_,
                                         DeviceVendorType::CameraFlir,
                                         DeviceDataType::CameraFlirCompressedImage,
                                         sequence_++);
    auto derived_data = static_cast<FlirCompressedImage*>(data.get());

    // Copy data
    derived_data->data.timestamp_intrinsic_ns = timestamp_intrinsic_ns;
    derived_data->data.compress_format = data::CompressedImage::CompressFormat::JPEG;
    derived_data->data.image_data_size = jpeg_image_size;
    memcpy(derived_data->data.image_data, jpeg_image, jpeg_image_size);

    tjDestroy(tj_instance);
    tjFree(jpeg_image);

    return data;
}

/// @todo Add muttable parameter for EV, Shutter Range, Brightneess, etc.
/// @todo Add immutable parameter for FPS (Need to pass parameter to Tron Sync Board)
HeraErrno Flir::adjust_parameter(DeviceParameterType type, const std::string& value)
{
    switch (type) {
    case DeviceParameterType::IpAddress: {
        return HeraErrno::ImmutableParameter;
        break;
    }
    default:
        return HeraErrno::UnimplementedParameter;
    }
}

HeraErrno Flir::handle_flir_error(const FlyCapture2::Error& error)
{
    return handle_error(HeraErrno::CanNotOpenCamera, error.GetDescription());
}

#endif
#endif

/// For jpeg format, just copy
///
data::SensorDataPtr Flir::do_convert(data::DeviceDataPtr& storage_data)
{
    if (storage_data->is_type(DeviceDataType::CameraFlirCompressedImage)) {
        // Raw DeviceData of Derived Type
        auto raw_data = static_cast<FlirCompressedImage*>(storage_data.get());

        // Create a SensorData from DeviceData
        uint32_t image_data_size = raw_data->data.image_data_size;
        uint32_t length = sizeof(data::CompressedImage) + image_data_size;
        auto sensor_data = data::SensorData::create_from(storage_data, SensorDataType::CompressedImage, length);
        auto camera_sensor_data = static_cast<data::CompressedImage*>(sensor_data.get());

        // Parse Data
        camera_sensor_data->timestamp_intrinsic_ns = raw_data->data.timestamp_intrinsic_ns;
        camera_sensor_data->compress_format = raw_data->data.compress_format;
        camera_sensor_data->image_data_size = image_data_size;
        memcpy(camera_sensor_data->image_data, &(raw_data->data.image_data), image_data_size);
        return sensor_data;
    } else if (storage_data->is_type(DeviceDataType::CameraFlirRawImage)) {
        // Return CameraRawImage
        /// @todo Implement convertion for RawImage
        log::warn << "Flir: CamareRawImage is not supported yet" << log::endl;
        return data::SensorData::broken_data();
    }
    return data::SensorData::broken_data();
}

}  // namespace flir
}  // namespace camera
}  // namespace device
}  // namespace hera
}  // namespace wayz