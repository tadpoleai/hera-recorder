///
/// @file plugin_entry.cpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief
/// @date 2020-08-19
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include <chrono>
#include <cmath>
#include <cstdlib>

#include "data/odometry_data.hpp"
#include "plugin_common.hpp"
#include "plugin_param.hpp"

#ifdef WITH_DRIVER
#include "driver/serial/serial_port_binary.hpp"
#include "driver/serial/serial_transport.hpp"
#endif


namespace wayz {
namespace hera {
namespace device {
namespace odometry {
namespace aehaitai {

///
/// @brief Absolute Encoder Haitai, Derived from Device
///
HERA_PLUGIN_DEFINE_START("odometry/aehaitai", 0x0611, 1)

#include "plugin_data.hpp"

#ifdef WITH_DRIVER
HERA_PLUGIN_DEFINE_FUNCTIONS

void thread_ask_function();

std::thread* thread_ask_{nullptr};

driver::SerialPortBinary* serial_port_{nullptr};           ///< for receiving nmea data
common::ThreadQueue<driver::SerialData>* queue_{nullptr};  ///< queue of nmea data
#endif

static constexpr uint32_t AskFrequency = 100;

HERA_PLUGIN_DEFINE_END

#ifdef WITH_DRIVER

HeraErrno DevicePlugin::connect()
{
    auto binary_config = driver::SerialPortBinaryConfig(
            {.lead_bytes = "\x3E\x00\x01",
             .tail_bytes = "",
             .checksum_protocol = driver::SerialPortBinaryConfig::ChecksumProtocol::CRC16_MODBUS,
             .checksum_range = driver::SerialPortBinaryConfig::ChecksumRange::LEAD_BYTES_AND_DATA});
    auto serial_config = driver::SerialConfig(local_parameters_.get_Baud());
    serial_config.writable = true;
    // serial_config.low_latency_mode = true;

    serial_port_ = new driver::SerialPortBinary(local_parameters_.get_Kernel(), serial_config, binary_config);
    if (!serial_port_->is_opened()) {
        return handle_error(HeraErrno::CanNotOpenTtyDevice,
                            "Can not open primary device '" + local_parameters_.get_Kernel() + "'");
    }
    queue_ = serial_port_->get_queue_handler();

    thread_ask_ = new std::thread(&DevicePlugin::thread_ask_function, this);

    return HeraErrno::Success;
}

void DevicePlugin::disconnect()
{
    if (serial_port_ != nullptr) {
        delete serial_port_;
    }

    if (thread_ask_ != nullptr) {
        thread_ask_->join();
        delete thread_ask_;
        thread_ask_ = nullptr;
    }
}

data::DeviceDataPtr DevicePlugin::fetch()
{
    if (!queue_) {
        return nullptr;
    }

    // static const std::vector<uint8_t> fetch_command = {0x3E, 0x00, 0x01, 0xD1, 0xCC};
    // serial_port_->write_port(fetch_command);

    // Get Framed from Serial
    auto binary_framed_data = queue_->wait_pop();
    if (binary_framed_data == nullptr) {
        return nullptr;
    }

    // Total length of device data
    auto frame_data_length = binary_framed_data->size();
    auto length = sizeof(data::DeviceData) + frame_data_length;
    auto data = AEHaitaiData::create(length, id_, sequence_++);
    auto derived_data = static_cast<AEHaitaiData*>(data.get());

    memcpy(derived_data->buf, binary_framed_data->data(), frame_data_length);

    return data;
}

void DevicePlugin::thread_ask_function()
{
    auto t = time::Timestamp::now();

    while (get_status() == DeviceStatus::Connected) {
        static const std::vector<uint8_t> fetch_command = {0x3E, 0x00, 0x01, 0xD1, 0xCC};
        serial_port_->write_port(fetch_command);

        t = t + time::OneSecond / AskFrequency;
        auto now = time::Timestamp::now();
        while (now < t) {
            uint32_t to_sleep = (t - now) / 1000;
            usleep(to_sleep);
            now = time::Timestamp::now();
        }
    }
}

HeraErrno DevicePlugin::adjust_parameter(const std::string& type, const std::string& value)
{
    return HeraErrno::OK;
}
#endif

data::SensorDataPtr DevicePlugin::do_convert(const data::DeviceDataPtr& storage_data,
                                             const ParametersInterface* parameters)
{
    if (!storage_data->is_type(AEHaitaiData::TypeVal)) {
        return data::SensorData::broken_data();
    }

    // Raw DeviceData of Derived Type
    auto raw_data = static_cast<AEHaitaiData*>(storage_data.get());

    // Use Receive Time
    uint64_t timestamp_intrinsic_ns = raw_data->get_timestamp_receive_ns() - time::OneSecond / AskFrequency;

    // Judge Type of Message
    auto frame_type = raw_data->data.frame_type;
    switch (frame_type) {
    case AEHaitaiData::FrameType::ENCODER_ANGLE: {
        auto encoder_angle = static_cast<AEHaitaiData::EncoderAngle*>((void*)raw_data->data.frame_data);

        // Create a SensorData from DeviceData
        auto length = sizeof(data::Orientation);
        auto sensor_data = data::SensorData::create_from(storage_data, SensorDataType::OdometryOrientation, length);
        auto odometry_sensor_data = static_cast<data::Orientation*>(sensor_data.get());

        odometry_sensor_data->timestamp_intrinsic_ns = timestamp_intrinsic_ns;
        double raw_angle = encoder_angle->angle_h8 * 256 + encoder_angle->angle_l8;
        odometry_sensor_data->orientation = std::remainder(raw_angle / (1 << 15) * 2 * M_PI, 2 * M_PI);

        return sensor_data;
    }
    default:
        return data::SensorData::broken_data();
    }
}

}  // namespace aehaitai
}  // namespace odometry
}  // namespace device
}  // namespace hera
}  // namespace wayz