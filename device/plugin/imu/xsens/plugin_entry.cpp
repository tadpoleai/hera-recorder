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

#include "common/include/utils/endian.hpp"
#include "data/imu_data.hpp"
#include "plugin_common.hpp"
#include "plugin_data.hpp"
#include "plugin_param.hpp"

#ifdef WITH_DRIVER
#include "driver/serial/serial_port_binary.hpp"
#include "xsens_timestamp_calculator.hpp"
#endif

namespace wayz {
namespace hera {
namespace device {
namespace imu {
namespace xsens {

///
/// @brief Xsens MTi-600 Series
///
HERA_PLUGIN_DEFINE_START(4)

#ifdef WITH_DRIVER
HERA_PLUGIN_DEFINE_FUNCTIONS

TimestampCalculator* timestamp_calculator_{nullptr};

driver::SerialPortBinary* serial_port_{nullptr};           ///< pointer to SerialTransport object, for receiving data
common::ThreadQueue<driver::SerialData>* queue_{nullptr};  ///< queue of serial data

#endif

HERA_PLUGIN_DEFINE_END

HERA_PLUGIN_EXPORT(ImuXsens, "imu/xsens")

#ifdef WITH_DRIVER

/// Open serial port by kernel, baud rate, serial msg type,
/// and get a thread-safe queue
HeraErrno DevicePlugin::connect()
{
    serial_port_ = new driver::SerialPortBinary(
            local_parameters_.get_Kernel(),
            driver::SerialConfig(local_parameters_.get_Baud()),
            driver::SerialPortBinaryConfig({
                    .lead_bytes = "\xFA",
                    .tail_bytes = "",
                    .checksum_protocol = driver::SerialPortBinaryConfig::ChecksumProtocol::XOR8,
                    .checksum_range = driver::SerialPortBinaryConfig::ChecksumRange::DATA_ONLY,
                    .length_offset = 3,
                    .length_range_substract = 5,
            }));
    if (!serial_port_->is_opened()) {
        return handle_error(HeraErrno::CanNotOpenTtyDevice,
                            "Can not open device '" + local_parameters_.get_Kernel() + "'");
    }

    queue_ = serial_port_->get_queue_handler();
    if (!queue_) {
        return handle_error(HeraErrno::CanNotOpenTtyDevice, "Can not register listener");
    }

    timestamp_calculator_ = new TimestampCalculator();

    return HeraErrno::Success;
}

/// Free the serial port object
///
void DevicePlugin::disconnect()
{
    if (serial_port_ != nullptr) {
        delete serial_port_;
    }

    if (timestamp_calculator_ != nullptr) {
        delete timestamp_calculator_;
    }
    return;
}

/// Fetch data from serial port
///
data::DeviceDataPtr DevicePlugin::fetch()
{
    if (!queue_) {
        log::warn << "Xsens: Queue not registered by SerialTransport" << log::endl;
        return nullptr;
    }

    // Get Rawdata from a Real Sensor
    auto serial_data = queue_->wait_pop();
    if (serial_data == nullptr) {
        return nullptr;
    }
    auto received_length = serial_data->size();

    if (received_length <= sizeof(MessageHeader)) {
        return nullptr;
    }

    auto* message = reinterpret_cast<MessageHeader*>(serial_data->data());
    if (message->message_id != MessageId::MTData2) {
        return nullptr;
    }

    bool has_tick = false;
    uint32_t tick = 0;
    bool has_trigger_indicator = false;
    uint32_t triggger_indicated = false;

    uint8_t* itr = message->data;
    const uint8_t* end = itr + message->length;
    for (; itr < end;) {
        auto mt_data2 = (mtdata2::MTData2*)itr;
        if (mt_data2->data_id == mtdata2::DataIdBigEndian::SampleTimeFine) {
            auto d = reinterpret_cast<mtdata2::SampleTimeFine*>(itr);
            tick = common::reverse_endian(d->tick_10kHz_bigendian);
            has_tick = true;
        } else if (mt_data2->data_id == mtdata2::DataIdBigEndian::StatusWord) {
            auto d = reinterpret_cast<mtdata2::StatusWord*>(itr);
            auto sw = common::reverse_endian(d->status_word_bigendian);
            triggger_indicated = (sw & (uint32_t(1) << mtdata2::StatusWord::BitsOfFields::SyncInMarker)) != 0;
            has_trigger_indicator = true;
        }
        itr += mt_data2->data_len + sizeof(mtdata2::MTData2);
    }

    if (!has_tick || !has_trigger_indicator) {
        static bool warned = false;
        log::warn << "Xsens: Can not sync, enable XDI_SampleTimeFine and XDI_StatusWord!" << log::endl;
        warned = true;
        return nullptr;
    }

    int64_t calculated_time;
    bool is_synced = timestamp_calculator_->get_intrinsic_time(calculated_time,
                                                               time::Timestamp::now(),
                                                               tick,
                                                               triggger_indicated);

    // Total length of device data
    auto length = sizeof(XsensData) - sizeof(MessageHeader) + received_length;
    auto data = data::DeviceData::create(length,
                                         id_,
                                         DeviceVendorType::ImuXsens,
                                         DeviceDataType::ImuXsensData,
                                         sequence_++);
    auto derived_data = static_cast<XsensData*>(data.get());

    derived_data->is_synced = is_synced;
    derived_data->timestamp_intrinsic = calculated_time;

    // Use Memcpy to directly fill buf
    memcpy(&derived_data->message, serial_data->data(), received_length);

    return data;
}

HeraErrno DevicePlugin::adjust_parameter(const std::string& type, const std::string& value)
{
    return HeraErrno::OK;
}
#endif

/// Multiple raw data by defined granularity
///
data::SensorDataPtr DevicePlugin::do_convert(const data::DeviceDataPtr& storage_data,
                                             const ParametersInterface* parameters)
{
    if (!storage_data->is_type(DeviceDataType::ImuXsensData)) {
        return data::SensorData::broken_data();
    }

    // Raw DeviceData of Derived Type
    auto raw_data = static_cast<XsensData*>(storage_data.get());

    if (!raw_data->is_synced) {
        // Non Synced
        return data::SensorData::broken_data();
    }

    if (raw_data->message.message_id != MessageId::MTData2) {
        // Non MTData2
        return data::SensorData::broken_data();
    }

    // Create a SensorData from DeviceData
    auto length = sizeof(data::ImuComposed);
    auto sensor_data = data::SensorData::create_from(storage_data, SensorDataType::ImuComposed, length);
    auto imu_sensor_data = static_cast<data::ImuComposed*>(sensor_data.get());
    imu_sensor_data->have_temperature = 0;
    imu_sensor_data->have_baro_pressure = 0;
    imu_sensor_data->have_angular_velocity = 0;
    imu_sensor_data->have_linear_acceleration = 0;
    imu_sensor_data->have_magnetic_field = 0;
    imu_sensor_data->have_orientation = 0;
    imu_sensor_data->have_free_linear_acceleration = 0;

    uint8_t* itr = raw_data->message.data;
    const uint8_t* end = itr + raw_data->message.length;
    for (; itr < end;) {
        auto mt_data2 = (mtdata2::MTData2*)itr;

        switch (mt_data2->data_id) {
            // case mtdata2::DataIdBigEndian::PacketCounter: {
            //     auto d = reinterpret_cast<mtdata2::PacketCounter*>(itr);
            // } break;

            // case mtdata2::DataIdBigEndian::SampleTimeFine: {
            //     auto d = reinterpret_cast<mtdata2::SampleTimeFine*>(itr);
            //     std::cout << "TICK = " << common::reverse_endian(d->tick_10kHz_bigendian) << std::endl;
            // } break;

        case mtdata2::DataIdBigEndian::Temperature: {
            auto d = reinterpret_cast<mtdata2::Temperature*>(itr);
            imu_sensor_data->have_temperature = 1;
            imu_sensor_data->temperature = common::reverse_endian(d->temp_degree_celsius_bigendian);
        } break;

        case mtdata2::DataIdBigEndian::BaroPressure: {
            auto d = reinterpret_cast<mtdata2::BaroPressure*>(itr);
            imu_sensor_data->have_baro_pressure = 1;
            imu_sensor_data->baro_pressure = common::reverse_endian(d->baro_pressure_pascal_bigendian);
        } break;

        case mtdata2::DataIdBigEndian::Quaternion: {
            auto d = reinterpret_cast<mtdata2::Quaternion*>(itr);
            imu_sensor_data->have_orientation = 1;
            imu_sensor_data->orientation[0] = common::reverse_endian(d->quaternion_xyzw_bigendian[0]);
            imu_sensor_data->orientation[1] = common::reverse_endian(d->quaternion_xyzw_bigendian[1]);
            imu_sensor_data->orientation[2] = common::reverse_endian(d->quaternion_xyzw_bigendian[2]);
            imu_sensor_data->orientation[3] = common::reverse_endian(d->quaternion_xyzw_bigendian[3]);
        } break;

        case mtdata2::DataIdBigEndian::Acceleration: {
            auto d = reinterpret_cast<mtdata2::Acceleration*>(itr);
            imu_sensor_data->have_linear_acceleration = 1;
            imu_sensor_data->linear_acceleration[0] = common::reverse_endian(d->acceleration_xyz_bigendian[0]);
            imu_sensor_data->linear_acceleration[1] = common::reverse_endian(d->acceleration_xyz_bigendian[1]);
            imu_sensor_data->linear_acceleration[2] = common::reverse_endian(d->acceleration_xyz_bigendian[2]);
        } break;

        case mtdata2::DataIdBigEndian::RateOfTurn: {
            auto d = reinterpret_cast<mtdata2::RateOfTurn*>(itr);
            imu_sensor_data->have_angular_velocity = 1;
            imu_sensor_data->angular_velocity[0] = common::reverse_endian(d->gyr_xyz_rad_bigendian[0]);
            imu_sensor_data->angular_velocity[1] = common::reverse_endian(d->gyr_xyz_rad_bigendian[1]);
            imu_sensor_data->angular_velocity[2] = common::reverse_endian(d->gyr_xyz_rad_bigendian[2]);
        } break;

        case mtdata2::DataIdBigEndian::MagneticField: {
            auto d = reinterpret_cast<mtdata2::MagneticField*>(itr);
            imu_sensor_data->have_magnetic_field = 1;
            imu_sensor_data->magnetic_field[0] = common::reverse_endian(d->magnetic_field_xyz_bigendian[0]);
            imu_sensor_data->magnetic_field[1] = common::reverse_endian(d->magnetic_field_xyz_bigendian[1]);
            imu_sensor_data->magnetic_field[2] = common::reverse_endian(d->magnetic_field_xyz_bigendian[2]);
        } break;

            // case mtdata2::DataIdBigEndian::StatusWord: {
            //     auto d = reinterpret_cast<mtdata2::StatusWord*>(itr);
            //     std::cout << "ST = " << common::reverse_endian(d->status_word_bigendian) << std::endl;
            // } break;

        default:
            // std::cout << (uint32_t)mt_data2->data_id << ", " << (uint32_t)mt_data2->data_len << std::endl;
            break;
        }

        itr += mt_data2->data_len + sizeof(mtdata2::MTData2);
    }

    return sensor_data;
}

}  // namespace xsens
}  // namespace imu
}  // namespace device
}  // namespace hera
}  // namespace wayz