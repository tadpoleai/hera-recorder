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
#include "driver/can/can_port.hpp"
#include "odometry/feedback/feedback.cpp"
#include "odometry/feedback/feedback.hpp"
#endif

namespace wayz {
namespace hera {
namespace device {
namespace odometry {
namespace s32vgeely {

///
/// @brief For S32VGeely Series Car, Derived from Device
///
HERA_PLUGIN_DEFINE_START("odometry/s32vgeely", 0x0601, 1)

#include "plugin_data.hpp"

#ifdef WITH_DRIVER
HERA_PLUGIN_DEFINE_FUNCTIONS

void feedback_thread_function();

static constexpr int8_t MaxCanPacketReceiveNum_ = 1;  ///< Max num of receiving CAN packet, 1
std::thread* thread_feedback_{nullptr};
std::unique_ptr<ipc::IPCQueue<data::SensorData>> ipc_feedback_{nullptr};
driver::CANPort* can_port_{nullptr};
#endif

HERA_PLUGIN_DEFINE_END

#ifdef WITH_DRIVER

HeraErrno DevicePlugin::connect()
{
    can_port_ = new driver::CANPort(local_parameters_.get_CanChannel());

    if (!can_port_->is_open()) {
        return handle_error(HeraErrno::CanNotOpenCanDevice, "S32VGeely:: can not open");
    }

    thread_feedback_ = new std::thread(&DevicePlugin::feedback_thread_function, this);

    return HeraErrno::Success;
}

/// Free the can data port object
///
void DevicePlugin::disconnect()
{
    if (can_port_ != nullptr) {
        delete can_port_;
        can_port_ = nullptr;
    }

    if (thread_feedback_ != nullptr) {
        thread_feedback_->join();
        delete thread_feedback_;
        thread_feedback_ = nullptr;
    }
}

/// Fetch data from serial port
///
data::DeviceDataPtr DevicePlugin::fetch()
{
    if (!can_port_) {
        return nullptr;
    }

    auto packet = can_port_->read();

    if (!packet) {
        return nullptr;
    }

    // Total length of device data
    auto length = sizeof(S32VGeelyData) + packet->dlc;
    auto data = data::DeviceData::create(length,
                                         id_,
                                         DeviceVendorType::OdometryS32VGeely,
                                         S32VGeelyCANFrame::TypeVal,
                                         sequence_++);
    auto derived_data = static_cast<S32VGeelyData*>(data.get());

    derived_data->data.timestamp_ns = *(time::Timestamp*)&packet->timestamp;
    derived_data->data.id_can = packet->id;
    derived_data->data.dlc_can = packet->dlc;

    // Use Memcpy to directly fill buf
    memcpy(derived_data->data.data_can, &packet->data, packet->dlc);

    return data;
}

void DevicePlugin::feedback_thread_function()
{
    ipc_feedback_ = ipc::IPCQueue<data::SensorData>::create();
    ipc_feedback_->open(data::LocalizationResult::IPCKeyS32VGeely,
                        ipc::OpenMode::Read,
                        false,
                        data::LocalizationResult::IPCNumElement,
                        data::LocalizationResult::IPCElementSize);

    while (get_status() == DeviceStatus::Connected) {
        if (auto data = ipc_feedback_->read()) {
            if (data->sensor_data_type == SensorDataType::OdometryLocalizationResult) {
                log::info << "S32VGeely:: Received LocalizationResult" << log::endl;
                auto result = reinterpret_cast<data::LocalizationResult*>(data.get());
                feedback::feedback(result, can_port_);
            }
        } else {
            usleep(1000);
        }
    }

    ipc_feedback_->close();
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
    if (!storage_data->is_type(S32VGeelyCANFrame::TypeVal)) {
        return data::SensorData::broken_data();
    }

    // Raw DeviceData of Derived Type
    auto raw_data = static_cast<S32VGeelyCANFrame*>(storage_data.get());

    constexpr uint32_t CanMessageDLC = 8;
    constexpr uint16_t CanPacketIDOfFrontWheelSpeed = 0x122u;
    constexpr uint16_t CanPacketIDOfRearWheelSpeed = 0x123u;
    constexpr uint16_t CanPacketIDOfSteeringAngle = 0xE0u;

    constexpr double FactorOfWheelSpeed = 0.05625;
    constexpr double FactorOfSteeringAngle = 0.1;

    switch (raw_data->data.id_can) {
    case CanPacketIDOfFrontWheelSpeed: {
        if (raw_data->data.dlc_can != CanMessageDLC) {
            log::warn << "S32VGeely: DLC " << raw_data->data.dlc_can << "does not match " << CanMessageDLC << log::endl;
            return data::SensorData::broken_data();
        }

        // Create a SensorData from DeviceData
        auto length = sizeof(data::FrontWheelSpeed);
        auto sensor_data = data::SensorData::create_from(storage_data, SensorDataType::OdometryFrontWheelSpeed, length);
        // Parse Data
        auto geely_data = static_cast<data::FrontWheelSpeed*>(sensor_data.get());
        geely_data->timestamp_intrinsic_ns = raw_data->data.timestamp_ns;

        uint64_t* value = reinterpret_cast<uint64_t*>(&raw_data->data.data_can);

        uint16_t left_speed_msb = *value & 0x00ff;  // FL MSB 8bit
        left_speed_msb = (left_speed_msb << 5);

        uint16_t left_speed_lsb = *value & 0xff00;  // FL LSB 5bit
        left_speed_lsb = (left_speed_lsb >> 11);

        uint16_t left_speed = left_speed_msb + left_speed_lsb;  // FL join together
        geely_data->left = FactorOfWheelSpeed * left_speed;

        uint16_t right_speed_msb = ((*value & 0x00ff0000) >> 16);  // FR MSB 8bit
        right_speed_msb = (right_speed_msb << 5);

        uint16_t right_speed_lsb = ((*value & 0xff000000) >> 16);  // FR LSB 5bit
        right_speed_lsb = (right_speed_lsb >> 11);

        uint16_t right_speed = right_speed_msb + right_speed_lsb;  // FR join together
        geely_data->right = FactorOfWheelSpeed * right_speed;

        char buffer[64];
        sprintf(buffer,
                "%02X %02X %02X %02X %02X %02X %02X %02X",
                raw_data->data.data_can[0],
                raw_data->data.data_can[1],
                raw_data->data.data_can[2],
                raw_data->data.data_can[3],
                raw_data->data.data_can[4],
                raw_data->data.data_can[5],
                raw_data->data.data_can[6],
                raw_data->data.data_can[7]);
        log::debug << "REAR_____SPD: RECV_TIME: " << raw_data->get_timestamp_receive_ns()
                   << ", CAN_TIME: " << geely_data->timestamp_intrinsic_ns << ", CAN_ID: " << raw_data->data.id_can
                   << ", CAN DLC: " << raw_data->data.dlc_can << ", PACKET: " << buffer
                   << ", L_SPD: " << geely_data->left << ", R_SPD: " << geely_data->right << log::endl;

        return sensor_data;
    }
    case CanPacketIDOfRearWheelSpeed: {
        if (raw_data->data.dlc_can != CanMessageDLC) {
            log::warn << "S32VGeely: DLC " << raw_data->data.dlc_can << "does not match " << CanMessageDLC << log::endl;
            return data::SensorData::broken_data();
        }

        // Create a SensorData from DeviceData
        auto length = sizeof(data::RearWheelSpeed);
        auto sensor_data = data::SensorData::create_from(storage_data, SensorDataType::OdometryRearWheelSpeed, length);
        // Parse Data
        auto geely_data = static_cast<data::RearWheelSpeed*>(sensor_data.get());
        geely_data->timestamp_intrinsic_ns = raw_data->data.timestamp_ns;

        uint64_t* value = reinterpret_cast<uint64_t*>(&raw_data->data.data_can);

        uint16_t left_speed_msb = *value & 0x00ff;  // RL MSB 8bit
        left_speed_msb = (left_speed_msb << 5);

        uint16_t left_speed_lsb = *value & 0xff00;  // RL LSB 5bit
        left_speed_lsb = (left_speed_lsb >> 11);

        uint16_t left_speed = left_speed_msb + left_speed_lsb;  // RL join together
        geely_data->left = FactorOfWheelSpeed * left_speed;

        uint16_t right_speed_msb = ((*value & 0x00ff0000) >> 16);  // RR MSB 8bit
        right_speed_msb = (right_speed_msb << 5);

        uint16_t right_speed_lsb = ((*value & 0xff000000) >> 16);  // RR LSB 5bit
        right_speed_lsb = (right_speed_lsb >> 11);

        uint16_t right_speed = right_speed_msb + right_speed_lsb;  // RR join together
        geely_data->right = FactorOfWheelSpeed * right_speed;

        char buffer[64];
        sprintf(buffer,
                "%02X %02X %02X %02X %02X %02X %02X %02X",
                raw_data->data.data_can[0],
                raw_data->data.data_can[1],
                raw_data->data.data_can[2],
                raw_data->data.data_can[3],
                raw_data->data.data_can[4],
                raw_data->data.data_can[5],
                raw_data->data.data_can[6],
                raw_data->data.data_can[7]);
        log::debug << "FRONT____SPD: RECV_TIME: " << raw_data->get_timestamp_receive_ns()
                   << ", CAN_TIME: " << geely_data->timestamp_intrinsic_ns << ", CAN_ID: " << raw_data->data.id_can
                   << ", CAN DLC: " << raw_data->data.dlc_can << ", PACKET: " << buffer
                   << ", L_SPD: " << geely_data->left << ", R_SPD: " << geely_data->right << log::endl;

        return sensor_data;
    }
    case CanPacketIDOfSteeringAngle: {
        if (raw_data->data.dlc_can != CanMessageDLC) {
            log::warn << "S32VGeely: DLC " << raw_data->data.dlc_can << "does not match " << CanMessageDLC << log::endl;
            return data::SensorData::broken_data();
        }

        // Create a SensorData from DeviceData
        auto length = sizeof(data::SteeringAngle);
        auto sensor_data = data::SensorData::create_from(storage_data, SensorDataType::OdometrySteeringAngle, length);
        // Parse Data
        auto geely_data = static_cast<data::SteeringAngle*>(sensor_data.get());
        geely_data->timestamp_intrinsic_ns = raw_data->data.timestamp_ns;

        uint64_t* value = reinterpret_cast<uint64_t*>(&raw_data->data.data_can);

        uint16_t angle_msb = *value & 0x00ff;
        angle_msb = (angle_msb << 8);

        uint16_t angle_lsb = *value & 0xff00;
        angle_lsb = (angle_lsb >> 8);

        geely_data->steering_angle = FactorOfSteeringAngle * (int16_t)(angle_lsb + angle_msb);

        char buffer[64];
        sprintf(buffer,
                "%02X %02X %02X %02X %02X %02X %02X %02X",
                raw_data->data.data_can[0],
                raw_data->data.data_can[1],
                raw_data->data.data_can[2],
                raw_data->data.data_can[3],
                raw_data->data.data_can[4],
                raw_data->data.data_can[5],
                raw_data->data.data_can[6],
                raw_data->data.data_can[7]);
        log::debug << "STEER__ANGLE: RECV_TIME: " << raw_data->get_timestamp_receive_ns()
                   << ", CAN_TIME: " << geely_data->timestamp_intrinsic_ns << ", CAN_ID: " << raw_data->data.id_can
                   << ", CAN DLC: " << raw_data->data.dlc_can << ", PACKET: " << buffer
                   << ", STR: " << geely_data->steering_angle << log::endl;

        return sensor_data;
    }
    default: {
        char buffer[64];
        sprintf(buffer,
                "%02X %02X %02X %02X %02X %02X %02X %02X",
                raw_data->data.data_can[0],
                raw_data->data.data_can[1],
                raw_data->data.data_can[2],
                raw_data->data.data_can[3],
                raw_data->data.data_can[4],
                raw_data->data.data_can[5],
                raw_data->data.data_can[6],
                raw_data->data.data_can[7]);

        return data::SensorData::broken_data();
    }
    }
    return data::SensorData::broken_data();
}

}  // namespace s32vgeely
}  // namespace odometry
}  // namespace device
}  // namespace hera
}  // namespace wayz
