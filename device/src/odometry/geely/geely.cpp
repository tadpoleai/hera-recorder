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

#include "geely.hpp"

namespace wayz {
namespace hera {
namespace device {
namespace odometry {
namespace geely {

const std::vector<DeviceParameterType> Geely::EssentialParameterTypes = {DeviceParameterType::DataPort};

const std::vector<DeviceParameterType> Geely::OptionalParameterTypes = {};

auto _ = DeviceFactory::register_type({.type = DeviceVendorType::OdometryS32VGeely,
                                       .type_name = "odometry/s32vgeely",
                                       .create = &Geely::create,
                                       .do_convert = &Geely::do_convert,
                                       .essential_parameter_types = Geely::EssentialParameterTypes,
                                       .optional_parameter_types = Geely::OptionalParameterTypes,
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
HeraErrno Geely::connect()
{
    SALCanInit(nullptr);
    data_port_ = stoi(parameters_[DeviceParameterType::DataPort]);
    SALStatus status = SALCanOpenPort(data_port_);
    if (status != SALStatus::SUCCESS)
        return handle_error(HeraErrno::CanNotOpenCanDevice, "Can not open CAN port: " + data_port_);
    return HeraErrno::Success;
}

/// Free the can data port object
///
void Geely::disconnect()
{
    SALCanClosePort(data_port_);
}

/// Fetch data from serial port
///
data::DeviceDataPtr Geely::fetch()
{
    uint32_t read_size = 0;
    auto received_length = sizeof(can_packet_);

    SALCanRecvPacket(data_port_, &can_packet_, &read_size, MaxCanPacketReceiveNum_);

    // Total length of device data
    auto length = sizeof(GeelyData);
    auto data = data::DeviceData::create(length,
                                         id_,
                                         DeviceVendorType::OdometryS32VGeely,
                                         DeviceDataType::OdometryS32VGeelyCANFrame,
                                         sequence_++);
    auto derived_data = static_cast<GeelyData*>(data.get());

    if (received_length != sizeof(derived_data->buf)) {
        log::warn << "Geely: Received size does not match" << log::endl;
        return nullptr;
    }
    // Use Memcpy to directly fill buf
    memcpy(derived_data->buf, &can_packet_, received_length);

    return data;
}

HeraErrno Geely::adjust_parameter(DeviceParameterType type, const std::string& value)
{
    switch (type) {
    case DeviceParameterType::DataPort:
        return HeraErrno::ImmutableParameter;
    default:
        return HeraErrno::UnimplementedParameter;
    }
    return HeraErrno::Success;
}
#endif
#endif

/// Multiple raw data by defined granularity
///
data::SensorDataPtr Geely::do_convert(data::DeviceDataPtr& storage_data)
{
    if (!storage_data->is_type(DeviceDataType::OdometryS32VGeelyCANFrame)) {
        return data::SensorData::broken_data();
    }

    // Raw DeviceData of Derived Type
    auto raw_data = static_cast<GeelyData*>(storage_data.get());


    switch (raw_data->data.arbitrationId) {
    case CanPacketIDOfRearWheelSpeed_: {

        // Create a SensorData from DeviceData
        auto length = sizeof(data::RearWheelSpeed);
        auto sensor_data = data::SensorData::create_from(storage_data, SensorDataType::OdometryRearWheelSpeed, length);
        // Parse Data
        auto geely_data = static_cast<data::RearWheelSpeed*>(sensor_data.get());
        geely_data->timestamp_intrinsic_ns =
                raw_data->data.timeStamp.tv_sec * 1e9 + raw_data->data.timeStamp.tv_usec * 1e3;
        char buffer[256];
        sprintf(buffer,
                "%02X%02X%02X%02X%02X%02X%02X%02X",
                raw_data->data.packet[7],
                raw_data->data.packet[6],
                raw_data->data.packet[5],
                raw_data->data.packet[4],
                raw_data->data.packet[3],
                raw_data->data.packet[2],
                raw_data->data.packet[1],
                raw_data->data.packet[0]);

        uint64_t value = std::stoull(buffer, 0, 16);

        constexpr double kFactor = 0.05625;

        uint16_t left_speed_msb = value & 0x00ff;  // FL MSB 8bit
        left_speed_msb = (left_speed_msb << 5);

        uint16_t left_speed_lsb = value & 0xff00;  // FL LSB 5bit
        left_speed_lsb = (left_speed_lsb >> 11);

        uint16_t left_speed = left_speed_msb + left_speed_lsb;  // FL join together
        geely_data->left = kFactor * left_speed;

        uint16_t right_speed_msb = ((value & 0x00ff0000) >> 16);  // FR MSB 8bit
        right_speed_msb = (right_speed_msb << 5);

        uint16_t right_speed_lsb = ((value & 0xff000000) >> 16);  // FR LSB 5bit
        right_speed_lsb = (right_speed_lsb >> 11);

        uint16_t right_speed = right_speed_msb + right_speed_lsb;  // FR join together
        geely_data->right = kFactor * right_speed;

        log::debug << "geely_data timestamp: " << geely_data->timestamp_intrinsic_ns
                   << " ,ID: " << raw_data->data.arbitrationId << " ,packet " << buffer
                   << " geely_data->right: " << geely_data->right << log::endl;
        return sensor_data;
    }
    case CanPacketIDOfAngular_: {
          // Create a SensorData from DeviceData
        auto length = sizeof(data::SteeringAngle);
        auto sensor_data = data::SensorData::create_from(storage_data, SensorDataType::OdometrySteeringAngle, length);
        // Parse Data
        auto geely_data = static_cast<data::SteeringAngle*>(sensor_data.get());
        geely_data->timestamp_intrinsic_ns =
                raw_data->data.timeStamp.tv_sec * 1e9 + raw_data->data.timeStamp.tv_usec * 1e3;
        char buffer[256];
        sprintf(buffer,
                "%02X%02X%02X%02X%02X%02X%02X%02X",
                raw_data->data.packet[7],
                raw_data->data.packet[6],
                raw_data->data.packet[5],
                raw_data->data.packet[4],
                raw_data->data.packet[3],
                raw_data->data.packet[2],
                raw_data->data.packet[1],
                raw_data->data.packet[0]);

        uint64_t value = std::stoull(buffer, 0, 16);

        uint16_t angle_msb = value & 0x00ff;
        angle_msb = (angle_msb << 8);

        uint16_t angle_lsb = value & 0xff00;
        angle_lsb = (angle_lsb >> 8);

        geely_data->steering_angle = 0.1 * (int16_t)(angle_lsb + angle_msb);
        log::debug << "geely_data timestamp: " << geely_data->timestamp_intrinsic_ns
                   << " ,ID: " << raw_data->data.arbitrationId << " ,packet " << buffer
                   << " geely_data->steering_angle: " << geely_data->steering_angle << log::endl;
        return sensor_data;
    }
    }
    return data::SensorData::broken_data();
}

}  // namespace geely
}  // namespace odometry
}  // namespace device
}  // namespace hera
}  // namespace wayz