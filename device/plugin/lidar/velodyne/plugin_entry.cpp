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
#include "velodyne_defs.hpp"

#ifdef WITH_DRIVER
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#endif

namespace wayz {
namespace hera {
namespace device {
namespace lidar {
namespace velodyne {

///
/// @brief Velodyne Lidar
///
HERA_PLUGIN_DEFINE_START(160)

static constexpr int64_t UsToNs_ = 1000ULL;                           ///< Multiplier from ns to us
static constexpr int64_t SecondToUs_ = 1'000'000ULL;                  ///< Multiplier from second to us
static constexpr int64_t HourToUs_ = 3600ULL * SecondToUs_;           ///< Multiplier from hour to us
static constexpr int64_t HalfHourToUs_ = 1800ULL * SecondToUs_;       ///< Multiplier from half an hour to us
static constexpr int64_t MaxDelayToleranceUs_ = 30ULL * SecondToUs_;  ///< Max valid transmission delay, in us
static constexpr size_t EthernetMTU_ = 1500;                          ///< MTU/PacketSize of ethernet interface

#ifdef WITH_DRIVER
HERA_PLUGIN_DEFINE_FUNCTIONS

::sockaddr_in addr_in_;                 ///< Ip Address and Data Port of Lidar
int socket_;                            ///< Socket handler
uint8_t receive_buffer_[EthernetMTU_];  ///< UDP Receive buffer
#endif

HERA_PLUGIN_DEFINE_END

HERA_PLUGIN_EXPORT(LidarVelodyne, "lidar/velodyne")

#ifdef WITH_DRIVER

/// Turn on Lidar's Laser by Remote Control (curl),
/// then created a socket by IP and UDP Port
HeraErrno DevicePlugin::connect()
{
    log::debug << "Velodyne:: Connecting to velodyne by binding port: " << local_parameters_.get_ListenPort()
               << log::endl;

    int ret = system(("curl -m 0.2 --data \"motor=on&laser=on&rpm=" + std::to_string(local_parameters_.get_Speed()) +
                      "&returns=" + local_parameters_.get_ReturnType()._to_string() + "\" http://" +
                      local_parameters_.get_IpAddress() + "/cgi/setting")
                             .c_str());
    if (ret == 0) {
        log::info << "Velodyne::Succeeded to power on lidar " << get_name() << log::endl;
    } else {
        return handle_error(HeraErrno::CanNotOpenEthernetDevice, "Velodyne::Failed to power on " + get_name());
    }

    try {
        addr_in_.sin_family = AF_INET;
        addr_in_.sin_port = htons(local_parameters_.get_ListenPort());
        addr_in_.sin_addr.s_addr = 0;

        socket_ = ::socket(AF_INET, SOCK_DGRAM, 0);
        if (::bind(socket_, (::sockaddr*)&addr_in_, sizeof(addr_in_)) < 0) {
            return handle_error(HeraErrno::CanNotOpenEthernetDevice, "Velodyne::Bind port, can not bind");
        }
        static const timeval TimeOut = {0, 50000};
        setsockopt(socket_, SOL_SOCKET, SO_RCVTIMEO, &TimeOut, sizeof(TimeOut));
    } catch (...) {
        return handle_error(HeraErrno::CanNotOpenEthernetDevice);
    }

    log::debug << "Velodyne:: Connection succeed" << log::endl;
    return HeraErrno::Success;
}

/// Turn off Lidar's Laser by Remote Control (curl),
/// then delete the socket
void DevicePlugin::disconnect()
{
    log::debug << "Velodyne:: socket closing" << log::endl;
    ::close(socket_);

    int ret = system(("curl -m 0.2 --data \"motor=off&laser=off&rpm=300\" http://" + local_parameters_.get_IpAddress() +
                      "/cgi/setting")
                             .c_str());
    if (ret == 0) {
        log::info << "Velodyne::Succeeded to power off lidar " << get_name() << log::endl;
    } else {
        log::error << "Velodyne::Failed to power off " << get_name();
    }
    log::debug << "Velodyne:: socket closed successfullly " << log::endl;
}

/// Wait a UDP packet from socket, then create a VelodyneStorage,
/// after that, spin io_service
data::DeviceDataPtr DevicePlugin::fetch()
{
    auto received_length = ::recv(socket_, receive_buffer_, sizeof(receive_buffer_), 0);

    if (received_length != sizeof(VelodynePacket::buf)) {
        return nullptr;
    }

    // Total length of device data
    auto length = sizeof(VelodynePacket);
    auto data_type = DeviceDataType::LidarVelodynePacketFullSynced;
    switch (local_parameters_.get_SyncType()) {
    case SyncType::Full:
        data_type = DeviceDataType::LidarVelodynePacketFullSynced;
        break;
    case SyncType::Local:
        data_type = DeviceDataType::LidarVelodynePacketLocalSynced;
        break;
    case SyncType::Disabled:
        data_type = DeviceDataType::LidarVelodynePacketUnSynced;
        break;
    }

    auto data = data::DeviceData::create(length, id_, DeviceVendorType::LidarVelodyne, data_type, sequence_++);
    auto derived_data = static_cast<VelodynePacket*>(data.get());

    // Use Memcpy to directly fill buf
    memcpy(derived_data->buf, &receive_buffer_, received_length);

    return data;
}

HeraErrno DevicePlugin::adjust_parameter(const std::string& type, const std::string& value)
{
    if (type == "Speed") {
        int ret =
                system(("curl -m 0.2 --data \"motor=on&laser=on&rpm=" + std::to_string(local_parameters_.get_Speed()) +
                        "\" http://" + local_parameters_.get_IpAddress() + "/cgi/setting")
                               .c_str());
        if (ret == 0) {
            log::info << "Velodyne::Set lidar lidar " << get_name() << " speed as "
                      << std::to_string(local_parameters_.get_Speed()) << log::endl;
            return HeraErrno::OK;
        } else {
            return HeraErrno::CanNotOpenEthernetDevice;
        }
    } else {
        return HeraErrno::OK;
    }
}
#endif

/// Check the LidarType and ReturnMode first,
/// if valid, do convertion by LidarType
data::SensorDataPtr DevicePlugin::do_convert(const data::DeviceDataPtr& storage_data,
                                             const ParametersInterface* parameters)
{
    if (!storage_data->is_type(DeviceDataType::LidarVelodynePacketFullSynced) &&
        !storage_data->is_type(DeviceDataType::LidarVelodynePacketLocalSynced) &&
        !storage_data->is_type(DeviceDataType::LidarVelodynePacketUnSynced)) {
        return data::SensorData::broken_data();
    }

    // Raw DeviceData of Derived Type
    auto raw_data = static_cast<VelodynePacket*>(storage_data.get());

    // Parse Data
    /// Check LidarType and ReturnMode
    /// @see @ref VLP-16C-Manual section: 9.3.1.7, Factory Bytes, page: 56
    if (raw_data->data.lidar_type == VelodynePacket::LidarType::VLP16C ||
        raw_data->data.lidar_type == VelodynePacket::LidarType::VLP32C ||
        raw_data->data.lidar_type == VelodynePacket::LidarType::HDL32E) {
    } else {
        // Unsupported lidar type;
        log::warn << "Velodyne '" << storage_data->get_device_id() << "': Unknown lidar type '"
                  << int32_t(raw_data->data.lidar_type) << "'" << log::endl;
        return data::SensorData::broken_data();
    }

    double is_dual = false;
    data::PointsXYZI::ReturnType return_type;
    switch (raw_data->data.return_mode) {
    case VelodynePacket::ReturnMode::Strongest:
        return_type = data::PointsXYZI::ReturnType::Strongest;
        break;
    case VelodynePacket::ReturnMode::LastReturn:
        return_type = data::PointsXYZI::ReturnType::Last;
        break;
    case VelodynePacket::ReturnMode::DualReturn:
        return_type = data::PointsXYZI::ReturnType::Dual;
        is_dual = true;
        break;
    default:
        log::warn << "Velodyne '" << storage_data->get_device_id() << "': Unknown return mode '"
                  << int32_t(raw_data->data.return_mode) << "'" << log::endl;
        return data::SensorData::broken_data();
    }

    /// Get azimuth gap between data blocks
    /// @see @ref VLP-16C-Manual section: 9.5, Precision Azimuth Calculation, page: 65
    double azimuth_diff_overall =
            ((int32_t)(raw_data->data.data_blocks[VelodynePacket::NumDataBlockPerPacket - 1].azimuth) -
             (int32_t)(raw_data->data.data_blocks[0].azimuth)) *
            velodyne::AzimuthGranularity;
    if (azimuth_diff_overall < 0) {
        azimuth_diff_overall += 2 * M_PI;
    }

    double azimuth_gap = azimuth_diff_overall / (VelodynePacket::NumDataBlockPerPacket - 1);
    if (is_dual) {
        azimuth_gap *= 2;
    }

    // Create a temp vector
    std::vector<data::PointsXYZI::PointXYZI> lidar_points;
    lidar_points.reserve(VelodynePacket::NumPointsPerPacket);

    for (auto data_block_index = 0; data_block_index < VelodynePacket::NumDataBlockPerPacket; ++data_block_index) {
        const auto* data_block = &raw_data->data.data_blocks[data_block_index];

        double azimuth_base = velodyne::AzimuthGranularity * (data_block->azimuth);
        for (auto channel_index = 0; channel_index < VelodynePacket::NumChannelPerDataBlock; ++channel_index) {
            const auto* channel = &data_block->channels[channel_index];

            data::PointsXYZI::PointXYZI lidar_point;
            lidar_point.intensity = channel->reflectivity;

            switch (raw_data->data.lidar_type) {
            case VelodynePacket::LidarType::VLP16C: {
                double azimuth =
                        std::remainder(-(azimuth_base +
                                         azimuth_gap * velodyne::vlp16c::GetRelativeAzimuthChange(channel_index)),
                                       2 * M_PI);
                double distance = channel->distance * velodyne::vlp16c::DistanceGranularity;

                double pitch = velodyne::vlp16c::VerticalAngles[channel_index % 16];
                double horizontal_distance = distance * cos(pitch);
                lidar_point.x = horizontal_distance * cos(azimuth);
                lidar_point.y = horizontal_distance * sin(azimuth);
                lidar_point.z = distance * sin(pitch) + velodyne::vlp16c::VerticalCorrection[channel_index % 16];
                lidar_point.channel = channel_index % 16;
                lidar_point.horizontal_distance = horizontal_distance;
                lidar_point.azimuth = azimuth;
                lidar_point.pitch = pitch;
            } break;

            case VelodynePacket::LidarType::VLP32C: {
                double azimuth =
                        std::remainder(-(azimuth_base + velodyne::vlp32c::AzimuthOffset[channel_index] +
                                         azimuth_gap * velodyne::vlp32c::GetRelativeAzimuthChange(channel_index)),
                                       2 * M_PI);
                double distance = channel->distance * velodyne::vlp32c::DistanceGranularity;

                double pitch = velodyne::vlp32c::VerticalAngles[channel_index];
                double horizontal_distance = distance * cos(pitch);
                lidar_point.x = horizontal_distance * cos(azimuth);
                lidar_point.y = horizontal_distance * sin(azimuth);
                lidar_point.z = distance * sin(pitch);
                lidar_point.channel = channel_index;
                lidar_point.horizontal_distance = horizontal_distance;
                lidar_point.azimuth = azimuth;
                lidar_point.pitch = pitch;
            } break;

            case VelodynePacket::LidarType::HDL32E: {
                double azimuth =
                        std::remainder(-(azimuth_base +
                                         azimuth_gap * velodyne::hdl32e::GetRelativeAzimuthChange(channel_index)) +
                                               velodyne::hdl32e::AzimuthCorrection,
                                       2 * M_PI);
                double distance = channel->distance * velodyne::hdl32e::DistanceGranularity;

                double pitch = velodyne::hdl32e::VerticalAngles[channel_index];
                double horizontal_distance = distance * cos(pitch);
                lidar_point.x = horizontal_distance * cos(azimuth);
                lidar_point.y = horizontal_distance * sin(azimuth);
                lidar_point.z = distance * sin(pitch);
                lidar_point.channel = channel_index;
                lidar_point.horizontal_distance = horizontal_distance;
                lidar_point.azimuth = azimuth;
                lidar_point.pitch = pitch;
            } break;

            default:
                return data::SensorData::broken_data();
                break;
            }  // switch lidar type

            lidar_points.emplace_back(lidar_point);
        }  // for channel
    }      // for data_block

    // Create a SensorData from DeviceData
    auto point_number = lidar_points.size();
    auto length = sizeof(data::PointsXYZI::PointXYZI) * point_number + sizeof(data::PointsXYZI);
    auto sensor_data = data::SensorData::create_from(storage_data, SensorDataType::PointsXYZI, length);
    auto lidar_sensor_data = static_cast<data::PointsXYZI*>(sensor_data.get());

    // Memcpy from temp vector
    lidar_sensor_data->point_number = point_number;

    if (!is_dual) {
        memcpy(lidar_sensor_data->points, lidar_points.data(), sizeof(data::PointsXYZI::PointXYZI) * point_number);
    } else {
        for (auto data_block_index = 0; data_block_index < VelodynePacket::NumDataBlockPerPacket / 2;
             ++data_block_index) {
            memcpy(&lidar_sensor_data->points[data_block_index * VelodynePacket::NumChannelPerDataBlock],
                   &lidar_points[2 * data_block_index * VelodynePacket::NumChannelPerDataBlock],
                   sizeof(data::PointsXYZI::PointXYZI) * VelodynePacket::NumChannelPerDataBlock);
            memcpy(&lidar_sensor_data->points[data_block_index * VelodynePacket::NumChannelPerDataBlock +
                                              VelodynePacket::NumPointsPerPacket / 2],
                   &lidar_points[(2 * data_block_index + 1) * VelodynePacket::NumChannelPerDataBlock],
                   sizeof(data::PointsXYZI::PointXYZI) * VelodynePacket::NumChannelPerDataBlock);
        }
    }

    // Calculate laser firing timestamp of the first laser beam
    if (storage_data->is_type(DeviceDataType::LidarVelodynePacketFullSynced)) {
        int64_t t_recv_us = (raw_data->get_timestamp_receive_ns()) / UsToNs_;
        int64_t t_packet_us = (int64_t)(raw_data->data.timestamp);
        int64_t t_fire_us = HourToUs_ * ((t_recv_us - t_packet_us + HalfHourToUs_) / HourToUs_) + t_packet_us;
        if (labs(t_fire_us - t_recv_us) > MaxDelayToleranceUs_) {
            // Synchronization lost
            /// @note If Synchronization is lost, i.e. t_recv is more than t_fire by MaxDelayTolerance,
            /// data will be abandon
            log::warn << "Velodyne: Data with ts = " << t_fire_us << "us, received at " << t_recv_us << "us too far"
                      << log::endl;
            return data::SensorData::broken_data();
        }
        lidar_sensor_data->timestamp_intrinsic_ns = t_fire_us * UsToNs_;
    } else if (storage_data->is_type(DeviceDataType::LidarVelodynePacketLocalSynced)) {
        int64_t t_packet_us = (int64_t)(raw_data->data.timestamp);
        lidar_sensor_data->timestamp_intrinsic_ns = t_packet_us * UsToNs_;
    } else {
        lidar_sensor_data->timestamp_intrinsic_ns = raw_data->get_timestamp_receive_ns();
    }

    // Fill Metadata
    lidar_sensor_data->meta.return_type = return_type;
    switch (raw_data->data.lidar_type) {
    case VelodynePacket::LidarType::VLP16C:
        lidar_sensor_data->meta.vendor = data::PointsXYZI::LidarVendor::VelodyneVLP16C;

        lidar_sensor_data->meta.rotation_direction = -1;

        lidar_sensor_data->meta.num_channel = 16;
        lidar_sensor_data->meta.nominal_pitch_increment = velodyne::vlp16c::VerticalAngleIncrement;

        lidar_sensor_data->meta.time_increment = velodyne::vlp16c::TimePerPoint / time::OneSecond;
        lidar_sensor_data->meta.time_increment_horizontal = velodyne::vlp16c::TimeHorizontal / time::OneSecond;
        lidar_sensor_data->meta.total_time =
                velodyne::vlp16c::TimeHorizontal / time::OneSecond * 2 * VelodynePacket::NumDataBlockPerPacket;

        lidar_sensor_data->meta.nominal_min_range = velodyne::vlp16c::MinNominalRange;
        lidar_sensor_data->meta.nominal_max_range = velodyne::vlp16c::MaxNominalRange;
        break;

    case VelodynePacket::LidarType::VLP32C:
        lidar_sensor_data->meta.vendor = data::PointsXYZI::LidarVendor::VelodyneVLP32C;

        lidar_sensor_data->meta.rotation_direction = -1;

        lidar_sensor_data->meta.num_channel = 32;
        lidar_sensor_data->meta.nominal_pitch_increment = velodyne::vlp32c::VerticalAngleIncrement;

        lidar_sensor_data->meta.time_increment = velodyne::vlp32c::TimePerPoint / time::OneSecond;
        lidar_sensor_data->meta.time_increment_horizontal = velodyne::vlp32c::TimeHorizontal / time::OneSecond;
        lidar_sensor_data->meta.total_time =
                velodyne::vlp32c::TimeHorizontal / time::OneSecond * VelodynePacket::NumDataBlockPerPacket;

        lidar_sensor_data->meta.nominal_min_range = velodyne::vlp32c::MinNominalRange;
        lidar_sensor_data->meta.nominal_max_range = velodyne::vlp32c::MaxNominalRange;
        break;

    case VelodynePacket::LidarType::HDL32E:
        lidar_sensor_data->meta.vendor = data::PointsXYZI::LidarVendor::VelodyneHDL32E;

        lidar_sensor_data->meta.rotation_direction = -1;

        lidar_sensor_data->meta.num_channel = 32;
        lidar_sensor_data->meta.nominal_pitch_increment = velodyne::hdl32e::VerticalAngleIncrement;

        lidar_sensor_data->meta.time_increment = velodyne::hdl32e::TimePerPoint / time::OneSecond;
        lidar_sensor_data->meta.time_increment_horizontal = velodyne::hdl32e::TimeHorizontal / time::OneSecond;
        lidar_sensor_data->meta.total_time =
                velodyne::hdl32e::TimeHorizontal / time::OneSecond * VelodynePacket::NumDataBlockPerPacket;

        lidar_sensor_data->meta.nominal_min_range = velodyne::hdl32e::MinNominalRange;
        lidar_sensor_data->meta.nominal_max_range = velodyne::hdl32e::MaxNominalRange;
        break;
    }

    if (is_dual) {
        lidar_sensor_data->meta.total_time /= 2;
    }

    return sensor_data;
}

}  // namespace velodyne
}  // namespace lidar
}  // namespace device
}  // namespace hera
}  // namespace wayz