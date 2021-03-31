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

#include "plugin_common.hpp"
#include "plugin_data.hpp"
#include "plugin_param.hpp"

#ifdef WITH_DRIVER
#include "driver/serial/serial_transport.hpp"
#endif

namespace wayz {
namespace hera {
namespace device {
namespace imu {
namespace aceinna {

///
/// @brief Aceinna 9-axis Imu, Derived from Device
///
HERA_PLUGIN_DEFINE_START(1)

#ifdef WITH_DRIVER
HERA_PLUGIN_DEFINE_FUNCTIONS

driver::SerialTransport* serial_port_{nullptr};            ///< pointer to SerialTransport object, for receiving data
common::ThreadQueue<driver::SerialData>* queue_{nullptr};  ///< queue of serial data

#endif

///
/// @brief Gravity scale constant, in m/s^2, defined by Aceinna
///
static constexpr double Gravity_ = 9.80655;

///
/// @brief Gyro Channel Granularity of Aceinna, in m/s^2
///
/// @note The granularity may change if Wayz Tron Sync Board send other parameters
static constexpr double GyroGranularity_ = M_PI / 200.0 / 180.0;

///
/// @brief Accel Channel Granularity of Aceinna, in rad/s
///
/// @note The granularity may change if Wayz Tron Sync Board send other parameters
static constexpr double AccelGranularity_ = Gravity_ / 4000.0;

///
/// @brief Magnetic Channel Granularity of Aceinna, in Tesla
///
/// @note The granularity may change if Wayz Tron Sync Board send other parameters
/// @todo Check the unit in Manual to confirm it is in Tesla
static constexpr double MagneticGranularity_ = 1.0 / 16000.0;

HERA_PLUGIN_DEFINE_END

HERA_PLUGIN_EXPORT(ImuAceinna, "imu/aceinna")

#ifdef WITH_DRIVER

/// Open serial port by kernel, baud rate, serial msg type,
/// and get a thread-safe queue
HeraErrno DevicePlugin::connect()
{
    serial_port_ = driver::SerialTransport::create(local_parameters_.get_Kernel(),
                                                   driver::SerialConfig(local_parameters_.get_Baud()));
    if (!serial_port_->is_opened()) {
        return handle_error(HeraErrno::CanNotOpenTtyDevice,
                            "Can not open device '" + local_parameters_.get_Kernel() + "'");
    }

    queue_ = serial_port_->get_queue_handler(local_parameters_.get_MsgType());
    if (!queue_) {
        return handle_error(HeraErrno::CanNotOpenTtyDevice, "Can not register listener");
    }

    return HeraErrno::Success;
}

/// Free the serial port object
///
void DevicePlugin::disconnect()
{
    if (serial_port_ != nullptr) {
        serial_port_->free();
    }
}

/// Fetch data from serial port
///
data::DeviceDataPtr DevicePlugin::fetch()
{
    if (!queue_) {
        log::warn << "Aceinna: Queue not registered by SerialTransport" << log::endl;
        return nullptr;
    }

    // Get Rawdata from a Real Sensor
    auto serial_data = queue_->wait_pop();
    if (serial_data == nullptr) {
        return nullptr;
    }
    auto received_length = serial_data->size();

    if (received_length != sizeof(AceinnaData::buf)) {
        log::warn << "Aceinna: Received size does not match" << log::endl;
        return nullptr;
    }

    // Total length of device data
    auto length = sizeof(AceinnaData);
    auto data = data::DeviceData::create(length,
                                         id_,
                                         DeviceVendorType::ImuAceinna,
                                         DeviceDataType::ImuAceinnaData,
                                         sequence_++);
    auto derived_data = static_cast<AceinnaData*>(data.get());

    // Use Memcpy to directly fill buf
    memcpy(derived_data->buf, serial_data->data(), received_length);

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
    if (!storage_data->is_type(DeviceDataType::ImuAceinnaData)) {
        return data::SensorData::broken_data();
    }

    // Raw DeviceData of Derived Type
    auto raw_data = static_cast<AceinnaData*>(storage_data.get());

    // Create a SensorData from DeviceData
    auto length = sizeof(data::ImuMagneticField);
    auto sensor_data = data::SensorData::create_from(storage_data, SensorDataType::ImuMagneticField, length);
    auto imu_sensor_data = static_cast<data::ImuMagneticField*>(sensor_data.get());

    // Parse Data
    imu_sensor_data->timestamp_intrinsic_ns = raw_data->data.timestamp;
    for (auto i = 0; i < 3; ++i) {
        imu_sensor_data->angular_velocity[i] = raw_data->data.gyro[i] * GyroGranularity_;
        imu_sensor_data->linear_acceleration[i] = raw_data->data.accel[i] * AccelGranularity_;
        imu_sensor_data->magnetic_field[i] = raw_data->data.magnetic[i] * MagneticGranularity_;
    }

    return sensor_data;
}

}  // namespace aceinna
}  // namespace imu
}  // namespace device
}  // namespace hera
}  // namespace wayz