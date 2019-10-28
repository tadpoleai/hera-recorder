//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include "camera.hpp"

#include <cmath>
#include <cstdlib>
#include <string>
#include <unistd.h>
#include <vector>

#include <arpa/inet.h>
#include <common/logger/logger.hpp>
#include <flycapture/FlyCapture2.h>

namespace wayz {
namespace tron {

Camera::Camera(int32_t id, const std::string& name) : Device(id, name) {}
Camera::~Camera()
{
    disconnect();
}

DeviceType Camera::get_type() const
{
    return DeviceType::Camera;
}

TronErrno Camera::connect()
{
    // Check Parameters
    if (parameters_.count(DeviceParameterType::IpAddress)) {
        ipAddress_ = parameters_[DeviceParameterType::IpAddress];
    } else {
        return set_error_and_die(TronErrno::InsufficientParameters, "Parameter IpAddress absent");
    }

    // Connect to Camera
    FlyCapture2::Error error;
    FlyCapture2::BusManager bus_manager;
    FlyCapture2::IPAddress flycapture_ipv4(ntohl(inet_addr(ipAddress_.c_str())));
    error = bus_manager.GetCameraFromIPAddress(flycapture_ipv4, &guid_);
    if (error != FlyCapture2::PGRERROR_OK) {
        return handle_error(error);
    }
    error = camera_.Connect(&guid_);
    if (error != FlyCapture2::PGRERROR_OK) {
        return handle_error(error);
    }

    // Get Camera Info
    FlyCapture2::CameraInfo camera_info;
    error = camera_.GetCameraInfo(&camera_info);
    if (error != FlyCapture2::PGRERROR_OK) {
        return handle_error(error);
    }

    // Set Driver to Buffer Mode
    FlyCapture2::FC2Config config;
    error = camera_.GetConfiguration(&config);
    if (error != FlyCapture2::PGRERROR_OK) {
        return handle_error(error);
    }
    config.grabTimeout = GrabTimeoutMs;
    config.numBuffers = NumBuffers;
    config.grabMode = FlyCapture2::BUFFER_FRAMES;
    error = camera_.SetConfiguration(&config);
    if (error != FlyCapture2::PGRERROR_OK) {
        return handle_error(error);
    }

    error = camera_.StartCapture();
    if (error != FlyCapture2::PGRERROR_OK) {
        return handle_error(error);
    }

    return TronErrno::Success;
}
void Camera::disconnect()
{
    stop();
    do_disconnect();
}
void Camera::do_disconnect()
{
    // Write Real Disconnection code here
    camera_.StopCapture();
    camera_.Disconnect();
    return;
}

std::shared_ptr<DeviceRawData> Camera::fetch()
{
    // Fetch rawdata from camera;
    FlyCapture2::Image raw_image;
    auto error = camera_.RetrieveBuffer(&raw_image);

    if (error != FlyCapture2::PGRERROR_OK) {
        Logger::warn() << "Camera/" << get_name() << " failed to retrieve due to "
                       << error.GetDescription() << Logger::endl;
        return nullptr;
    }

    auto now = Timestamp::now();
    int64_t timestamp_intrinsic_ns;
    auto metadata = raw_image.GetMetadata();
    bool synced =
            camera_timestamp_.get_intrinsic_time(timestamp_intrinsic_ns,
                                                 now,
                                                 FlirEmbeddedTimestamp(metadata.embeddedTimeStamp),
                                                 FlirEmbeddedShutter(metadata.embeddedShutter));
    if (!synced) {
        Logger::warn() << "Camera/" << get_name() << " have not synced" << Logger::endl;
        return nullptr;
    }

    // Create a Buff to Store Rawdata
    int32_t data_size = raw_image.GetDataSize();
    int32_t total_length = sizeof(DeviceRawData) + sizeof(DeviceRawDataImage) + data_size;
    DeviceRawData* data = reinterpret_cast<DeviceRawData*>(new uint8_t[total_length]);

    // Fullfil Metadata (Header) of Rawdata;
    data->length = total_length;
    data->device_type = DeviceType::Camera;
    data->device_data_type = DeviceDataType::ImageRaw;
    data->sequence = sequence_++;
    data->timestamp_receive_ns = now;

    // Fullfil Metadata (Header) of Camera Image;
    auto data_image = reinterpret_cast<DeviceRawDataImage*>(data->rawdata_buf);
    data_image->timestamp_intrinsic_ns = timestamp_intrinsic_ns;
    data_image->rows = raw_image.GetRows();
    data_image->cols = raw_image.GetCols();
    data_image->stride = raw_image.GetStride();
    data_image->is_compressed = 0;
    data_image->data_size = data_size;
    data_image->compressed_data_size = -1;  // Mark -1 as unknown
    data_image->format = static_cast<uint32_t>(raw_image.GetPixelFormat());
    data_image->bayer = static_cast<uint32_t>(raw_image.GetBayerTileFormat());
    data_image->meta_timestamp = raw_image.GetMetadata().embeddedTimeStamp;
    data_image->meta_shutter = raw_image.GetMetadata().embeddedShutter;

    // Use Memcpy to fill Buff
    memcpy(reinterpret_cast<char*>(data_image->data), raw_image.GetData(), data_size);

    // Return a Shared Ptr
    return std::shared_ptr<DeviceRawData>(data);
}
std::shared_ptr<SensorData> Camera::convert(const std::shared_ptr<DeviceRawData>& rawdata)
{
    return nullptr;
}

TronErrno Camera::do_adjust_parameter(DeviceParameterType type, const std::string& value)
{
    switch (type) {
    default:
        return TronErrno::UnimplementedParameter;
    }
    return TronErrno::Success;
}

TronErrno Camera::handle_error(const FlyCapture2::Error& error)
{
    Logger::error() << "camera/" << get_name() << ": " << error.GetDescription() << Logger::endl;
    return set_error_and_die(TronErrno::CanNotOpenCamera, error.GetDescription());
}

}  // namespace tron
}  // namespace wayz