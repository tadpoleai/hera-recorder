///
/// @file geely.cpp
/// @author chunchen.wang (chunchen.wang@wayz.ai)
/// @brief
/// @version 0.1
/// @date 2020-04-08
///
/// Copyright 2018 Wayz.ai. All Rights Reserved.
///
///

#include "s32vgeely.hpp"

namespace wayz {
namespace hera {
namespace device {
namespace odometry {
namespace s32vgeely {

const std::vector<DeviceParameterType> S32VGeely::EssentialParameterTypes = {DeviceParameterType::DataPort};

const std::vector<DeviceParameterType> S32VGeely::OptionalParameterTypes = {};

auto _ = DeviceFactory::register_type({.type = DeviceVendorType::OdometryS32VGeely,
                                       .type_name = "odometry/s32vgeely",
                                       .create = &S32VGeely::create,
                                       .do_convert = &S32VGeely::do_convert,
                                       .essential_parameter_types = S32VGeely::EssentialParameterTypes,
                                       .optional_parameter_types = S32VGeely::OptionalParameterTypes,
#ifdef WITH_DRIVER
#ifdef DEVICE_DRIVER_CAN
                                       .implemented = true
#else
                                       .implemented = false
#endif
#else
                                       .implemented = false
#endif
});

#ifdef WITH_DRIVER
#ifdef DEVICE_DRIVER_CAN
HeraErrno S32VGeely::connect()
{
    SALCanInit(nullptr);
    try {
        data_port_ = stoi(parameters_[DeviceParameterType::DataPort]);
    } catch (const std::exception& e) {
        return handle_error(HeraErrno::InvalidParameterValue,
                            "S32VGeely:: invalid data port" + parameters_[DeviceParameterType::DataPort]);
    }

    SALStatus status = SALCanOpenPort(data_port_);
    if (status != SALStatus::SUCCESS) {
        return handle_error(HeraErrno::CanNotOpenCanDevice, "Can not open CAN port: " + data_port_);
    }

    thread_feedback_ = new std::thread(&S32VGeely::feedback_thread_function, this);

    return HeraErrno::Success;
}

/// Free the can data port object
///
void S32VGeely::disconnect()
{
    SALStatus status = SALCanClosePort(data_port_);

    if (status != SALStatus::SUCCESS) {
        log::error << "S32VGeely: disconnect: SALCanClosePort returned " << int(status) << log::endl;
    }

    if (thread_feedback_ != nullptr) {
        thread_feedback_->join();
        delete thread_feedback_;
        thread_feedback_ = nullptr;
    }
}

/// Fetch data from serial port
///
data::DeviceDataPtr S32VGeely::fetch()
{
    uint32_t recv_num = 0;
    SALCanRecvPacket(data_port_, &can_packet_, &recv_num, MaxCanPacketReceiveNum_);

    if (recv_num == 0) {
        usleep(500);
        return nullptr;
    }

    // Total length of device data
    auto length = sizeof(S32VGeelyData) + can_packet_.dlc;
    auto data = data::DeviceData::create(length,
                                         id_,
                                         DeviceVendorType::OdometryS32VGeely,
                                         DeviceDataType::OdometryS32VGeelyCANFrame,
                                         sequence_++);

    auto derived_data = static_cast<S32VGeelyData*>(data.get());
    // Use Memcpy to directly fill buf
    memcpy(derived_data->buf, &can_packet_, sizeof(S32VGeelyData::S32VGeelyCANPacket) + can_packet_.dlc);

    return data;
}

HeraErrno S32VGeely::adjust_parameter(DeviceParameterType type, const std::string& value)
{
    switch (type) {
    case DeviceParameterType::DataPort:
        return HeraErrno::ImmutableParameter;
    default:
        return HeraErrno::UnimplementedParameter;
    }
    return HeraErrno::Success;
}

void S32VGeely::feedback_thread_function()
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
            }
        } else {
            usleep(1000);
        }
    }

    ipc_feedback_->close();
}
#endif
#endif

/// Multiple raw data by defined granularity
///
data::SensorDataPtr S32VGeely::do_convert(data::DeviceDataPtr& storage_data)
{
    if (!storage_data->is_type(DeviceDataType::OdometryS32VGeelyCANFrame)) {
        return data::SensorData::broken_data();
    }

    // Raw DeviceData of Derived Type
    auto raw_data = static_cast<S32VGeelyData*>(storage_data.get());

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
        geely_data->timestamp_intrinsic_ns =
                raw_data->data.timestamp_can.tv_sec * 1'000'000'000 + raw_data->data.timestamp_can.tv_usec * 1'000;

        uint64_t* value = reinterpret_cast<uint64_t*>(&raw_data->data.packet);

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
                raw_data->data.packet[0],
                raw_data->data.packet[1],
                raw_data->data.packet[2],
                raw_data->data.packet[3],
                raw_data->data.packet[4],
                raw_data->data.packet[5],
                raw_data->data.packet[6],
                raw_data->data.packet[7]);
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
        geely_data->timestamp_intrinsic_ns =
                raw_data->data.timestamp_can.tv_sec * 1'000'000'000 + raw_data->data.timestamp_can.tv_usec * 1'000;

        uint64_t* value = reinterpret_cast<uint64_t*>(&raw_data->data.packet);

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
                raw_data->data.packet[0],
                raw_data->data.packet[1],
                raw_data->data.packet[2],
                raw_data->data.packet[3],
                raw_data->data.packet[4],
                raw_data->data.packet[5],
                raw_data->data.packet[6],
                raw_data->data.packet[7]);
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
        geely_data->timestamp_intrinsic_ns =
                raw_data->data.timestamp_can.tv_sec * 1'000'000'000 + raw_data->data.timestamp_can.tv_usec * 1'000;

        uint64_t* value = reinterpret_cast<uint64_t*>(&raw_data->data.packet);

        uint16_t angle_msb = *value & 0x00ff;
        angle_msb = (angle_msb << 8);

        uint16_t angle_lsb = *value & 0xff00;
        angle_lsb = (angle_lsb >> 8);

        geely_data->steering_angle = FactorOfSteeringAngle * (int16_t)(angle_lsb + angle_msb);

        char buffer[64];
        sprintf(buffer,
                "%02X %02X %02X %02X %02X %02X %02X %02X",
                raw_data->data.packet[0],
                raw_data->data.packet[1],
                raw_data->data.packet[2],
                raw_data->data.packet[3],
                raw_data->data.packet[4],
                raw_data->data.packet[5],
                raw_data->data.packet[6],
                raw_data->data.packet[7]);
        log::debug << "STEER__ANGLE: RECV_TIME: " << raw_data->get_timestamp_receive_ns()
                   << ", CAN_TIME: " << geely_data->timestamp_intrinsic_ns << ", CAN_ID: " << raw_data->data.id_can
                   << ", CAN DLC: " << raw_data->data.dlc_can << ", PACKET: " << buffer
                   << ", STR: " << geely_data->steering_angle << log::endl;

        return sensor_data;
    }
    default: {
        auto timestamp_intrinsic_ns =
                raw_data->data.timestamp_can.tv_sec * 1'000'000'000 + raw_data->data.timestamp_can.tv_usec * 1'000;

        char buffer[64];
        sprintf(buffer,
                "%02X %02X %02X %02X %02X %02X %02X %02X",
                raw_data->data.packet[0],
                raw_data->data.packet[1],
                raw_data->data.packet[2],
                raw_data->data.packet[3],
                raw_data->data.packet[4],
                raw_data->data.packet[5],
                raw_data->data.packet[6],
                raw_data->data.packet[7]);
        log::debug << "UNKNOWN DATA: RECV_TIME: " << raw_data->get_timestamp_receive_ns()
                   << ", CAN_TIME: " << timestamp_intrinsic_ns << ", CAN_ID: " << raw_data->data.id_can
                   << ", CAN DLC: " << raw_data->data.dlc_can << ", PACKET: " << buffer << log::endl;
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