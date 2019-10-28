//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#pragma once
#include <flycapture/FlyCapture2.h>

#include "../device.hpp"
#include "camera_timestamp.hpp"

namespace wayz {
namespace tron {

#pragma pack(push, 1)
struct DeviceRawDataImage {
    int64_t timestamp_intrinsic_ns;
    int32_t rows;
    int32_t cols;
    int32_t stride;
    int32_t is_compressed;
    int32_t data_size;
    int32_t compressed_data_size;
    uint32_t format;          // See FlyCapture2::PixelFormat;
    uint32_t bayer;           // See FlyCapture2::BayerTileFormat;
    uint32_t meta_timestamp;  // See FlyCapture2::ImageMetadata
    uint32_t meta_shutter;    // See FlyCapture2::ImageMetadata
    uint8_t data[0];
};
#pragma pack(pop)

class Camera final : public Device {
public:
    Camera(int32_t id, const std::string& name);
    Camera(const Camera&) = delete;
    Camera& operator=(const Camera&) = delete;
    virtual ~Camera();

    DeviceType get_type() const final;

    TronErrno connect() final;
    void disconnect() final;
    std::shared_ptr<DeviceRawData> fetch() final;
    std::shared_ptr<SensorData> convert(const std::shared_ptr<DeviceRawData>& rawdata) final;
    TronErrno do_adjust_parameter(DeviceParameterType type, const std::string& value) final;

    std::shared_ptr<DeviceRawData> compress(const std::shared_ptr<DeviceRawData>& rawdata) final;

    // Convert should be implemented as static function
    // No convertion needed for CompressedImage

    // Disconnect should be implemented as non-virtual function
    void do_disconnect();

private:
    static constexpr int32_t GrabTimeoutMs = 200;
    static constexpr int32_t NumBuffers = 5;

private:
    TronErrno handle_error(const FlyCapture2::Error& error);

    std::string ipAddress_;

    FlyCapture2::Camera camera_;
    FlyCapture2::PGRGuid guid_;
    FlyCapture2::InterfaceType interface_type_;
    CameraTimestamp camera_timestamp_;
};

}  // namespace tron
}  // namespace wayz