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
struct DeviceRawDataCamera {
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

    // Convert should be implemented as static function
    static std::shared_ptr<SensorData> do_convert(const std::shared_ptr<DeviceRawData>& rawdata);

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