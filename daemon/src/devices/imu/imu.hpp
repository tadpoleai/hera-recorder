//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#pragma once
#include "../device.hpp"
#include "../utils/serial_transport.hpp"

namespace wayz {
namespace tron {

#pragma pack(push, 1)
struct DeviceRawDataImu {
    int64_t timestamp_ns;
    int16_t gyro[3];
    int16_t acc[3];
    int16_t mag[3];
};
#pragma pack(pop)

class Imu final : public Device {
public:
    Imu(int32_t id, const std::string& name);
    Imu(const Imu&) = delete;
    Imu& operator=(const Imu&) = delete;
    virtual ~Imu();

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
    std::string kernel_;
    int32_t baud_rate_;
    int32_t serial_msg_type_;

    SerialTransport* serial_port_;
    ThreadsafeQueue<std::shared_ptr<SerialData>>* queue_;

    static constexpr double GravitySI_ = 9.80655;
};

}  // namespace tron
}  // namespace wayz