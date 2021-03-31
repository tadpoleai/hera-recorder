///
/// @file plugin_entry.cpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief
/// @date 2020-10-19
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include <chrono>
#include <cmath>
#include <cstdlib>

#include "plugin_common.hpp"
#include "plugin_param.hpp"

#ifdef WITH_DRIVER
#include "driver/serial/serial_transport.hpp"
#endif

namespace wayz {
namespace hera {
namespace device {
namespace control {
namespace rotor {

///
/// @brief Control rotor speed
///
HERA_PLUGIN_DEFINE_START(1)

#ifdef WITH_DRIVER
HERA_PLUGIN_DEFINE_FUNCTIONS

void set_speed(int8_t speed);

driver::SerialTransport* serial_port_{nullptr};  ///< pointer to SerialTransport object, for receiving data

#endif

HERA_PLUGIN_DEFINE_END

HERA_PLUGIN_EXPORT(ControlRotor, "control/rotor")

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

    set_speed(local_parameters_.get_RotorSpeed());

    return HeraErrno::Success;
}

/// Free the serial port object
///
void DevicePlugin::disconnect()
{
    if (serial_port_ != nullptr) {
        set_speed(0);
        serial_port_->free();
    }
}

void DevicePlugin::set_speed(int8_t speed)
{
    uint8_t msg = (uint8_t)0x80;
    if (speed < 0) {
        msg += (uint8_t)0xA0;
        speed = -speed;
    }
    if (speed > 63) {
        speed = 63;
    }
    msg += (uint8_t)speed;

    std::vector<uint8_t> data;
    data.push_back(msg);

    serial_port_->write_port(data);
}


/// Fetch data from serial port
///
data::DeviceDataPtr DevicePlugin::fetch()
{
    sleep(1);

    set_speed(local_parameters_.get_RotorSpeed());

    return nullptr;
}

HeraErrno DevicePlugin::adjust_parameter(const std::string& type, const std::string& value)
{
    set_speed(local_parameters_.get_RotorSpeed());

    return HeraErrno::OK;
}
#endif

data::SensorDataPtr DevicePlugin::do_convert(const data::DeviceDataPtr& storage_data,
                                             const ParametersInterface* parameters)
{
    return nullptr;
}

}  // namespace rotor
}  // namespace control
}  // namespace device
}  // namespace hera
}  // namespace wayz