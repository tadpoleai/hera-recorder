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
    FlyCapture2::IPAddress flycapture_ipv4(inet_addr(ipAddress_.c_str()));
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
   // error = camera_.SetConfiguration(&config);
   // if (error != FlyCapture2::PGRERROR_OK) {
   //     return handle_error(error);
   // }

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
    FlyCapture2::Image raw_image;
    auto error = camera_.RetrieveBuffer(&raw_image);

    auto now = Timestamp::now();
    if (error != FlyCapture2::PGRERROR_OK) {
        Logger::warn() << "Camera/" << get_name() << " failed to retrieve due to "
                       << error.GetDescription() << Logger::endl;
        return nullptr;
    }

    int64_t timestamp_intrinsic_ns;
    auto metadata = raw_image.GetMetadata();
    camera_timestamp_.get_intrinsic_time(timestamp_intrinsic_ns,
                                         now,
                                         FlirEmbeddedTimestamp(metadata.embeddedTimeStamp),
                                         FlirEmbeddedShutter(metadata.embeddedShutter));

    Logger::debug() << "Camera Time is: " << Timestamp(timestamp_intrinsic_ns) << " Delay is "
                    << (now - timestamp_intrinsic_ns) / 1000000L << "ms" << Logger::endl;
    return nullptr;
}
std::shared_ptr<SensorData> Camera::convert(const std::shared_ptr<DeviceRawData>& rawdata)
{
    return do_convert(rawdata);
}
std::shared_ptr<SensorData> Camera::do_convert(const std::shared_ptr<DeviceRawData>& rawdata)
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