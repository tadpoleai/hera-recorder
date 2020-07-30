///
/// @file aehaitai.cpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief
/// @date 2020-06-16
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include "aehaitai.hpp"

#include "../../plugin_impl.hpp"

namespace wayz {
namespace hera {
namespace device {
namespace odometry {
namespace aehaitai {

const std::vector<std::string> AEHaitai::EssentialParameterTypes = {"Kernel", "BaudRate"};

const std::vector<std::string> AEHaitai::OptionalParameterTypes = {};

HERA_DEVICE_DRIVER_EXPORT_PLUGIN(OdometryAEHaitai, "odometry/aehaitai", AEHaitai)

#ifdef WITH_DRIVER

HeraErrno AEHaitai::connect()
{
    try {
        kernel_ = parameters_["Kernel"];
        baud_rate_ = stoi(parameters_["BaudRate"]);
    } catch (...) {
        return handle_error(HeraErrno::InvalidParameterValue);
    }

    auto binary_config = driver::SerialPortBinaryConfig(
            {.lead_bytes = "\x3E\x00\x01",
             .tail_bytes = "",
             .checksum_protocol = driver::SerialPortBinaryConfig::ChecksumProtocol::CRC16_MODBUS,
             .checksum_range = driver::SerialPortBinaryConfig::ChecksumRange::LEAD_BYTES_AND_DATA});
    auto serial_config = driver::SerialConfig(baud_rate_);
    serial_config.writable = true;
    // serial_config.low_latency_mode = true;

    serial_port_ = new driver::SerialPortBinary(kernel_, serial_config, binary_config);
    if (!serial_port_->is_opened()) {
        return handle_error(HeraErrno::CanNotOpenTtyDevice, "Can not open primary device '" + kernel_ + "'");
    }
    queue_ = serial_port_->get_queue_handler();

    thread_ask_ = new std::thread(&AEHaitai::thread_ask_function, this);

    return HeraErrno::Success;
}

void AEHaitai::disconnect()
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

data::DeviceDataPtr AEHaitai::fetch()
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
    auto data = data::DeviceData::create(length,
                                         id_,
                                         DeviceVendorType::OdometryAEHaitai,
                                         DeviceDataType::OdometryAEHaitaiData,
                                         sequence_++);
    auto derived_data = static_cast<AEHaitaiData*>(data.get());

    memcpy(derived_data->buf, binary_framed_data->data(), frame_data_length);

    return data;
}

void AEHaitai::thread_ask_function()
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

#endif

data::SensorDataPtr AEHaitai::do_convert(data::DeviceDataPtr& storage_data)
{
    if (!storage_data->is_type(DeviceDataType::OdometryAEHaitaiData)) {
        return data::SensorData::broken_data();
    }

    // Raw DeviceData of Derived Type
    auto raw_data = static_cast<AEHaitaiData*>(storage_data.get());

    // Use Receive Time
    uint64_t timestamp_intrinsic_ns = raw_data->get_timestamp_receive_ns() - time::OneSecond / AskFrequency;

    // Judge Type of Message
    auto frame_type = raw_data->data.frame_type;
    switch (frame_type) {
    case FrameType::ENCODER_ANGLE: {
        auto encoder_angle = static_cast<EncoderAngle*>((void*)raw_data->data.frame_data);

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