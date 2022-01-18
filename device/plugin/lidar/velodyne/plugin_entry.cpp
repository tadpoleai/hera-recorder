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

#include "data/lidar_data.hpp"
#include "plugin_common.hpp"
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
HERA_PLUGIN_DEFINE_START("lidar/velodyne", 0x0501, 160)

#include "plugin_data.hpp"

static constexpr int64_t UsToNs_ = 1000ULL;                           ///< Multiplier from ns to us
static constexpr int64_t SecondToUs_ = 1'000'000ULL;                  ///< Multiplier from second to us
static constexpr int64_t HourToUs_ = 3600ULL * SecondToUs_;           ///< Multiplier from hour to us
static constexpr int64_t HalfHourToUs_ = 1800ULL * SecondToUs_;       ///< Multiplier from half an hour to us
static constexpr int64_t MaxDelayToleranceUs_ = 30ULL * SecondToUs_;  ///< Max valid transmission delay, in us
static constexpr size_t EthernetMTU_ = 1500;                          ///< MTU/PacketSize of ethernet interface

static data::SensorDataPtr do_convert_single_packet(const data::DeviceDataPtr& storage_data, PointFormat format);

static std::map<int32_t, double> azimuth_map_;
static std::map<int32_t, data::SensorDataPtr> sensor_data_map_;
static std::map<int32_t, std::vector<data::Points::PointXYZCIDPAT>> accumulated_points_map_;
static std::map<int32_t, int64_t> timestamp_map_;

#ifdef WITH_DRIVER
HERA_PLUGIN_DEFINE_FUNCTIONS

::sockaddr_in addr_in_;                 ///< Ip Address and Data Port of Lidar
int socket_;                            ///< Socket handler
uint8_t receive_buffer_[EthernetMTU_];  ///< UDP Receive buffer
#endif

HERA_PLUGIN_DEFINE_END

std::map<int32_t, double> DevicePlugin::azimuth_map_;
std::map<int32_t, data::SensorDataPtr> DevicePlugin::sensor_data_map_;
std::map<int32_t, std::vector<data::Points::PointXYZCIDPAT>> DevicePlugin::accumulated_points_map_;
std::map<int32_t, int64_t> DevicePlugin::timestamp_map_;

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
    data::DeviceDataPtr data{nullptr};
    switch (local_parameters_.get_SyncType()) {
    case SyncType::Full:
        data = VelodynePacketFullSynced::create(length, id_, sequence_++);
        break;
    case SyncType::Local:
        data = VelodynePacketLocalSynced::create(length, id_, sequence_++);
        break;
    case SyncType::Disabled:
        data = VelodynePacketUnSynced::create(length, id_, sequence_++);
        break;
    }

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
    PointFormat format = PointFormat::XYZI;
    bool accumulate = false;
    auto section_base = SectionBase::Angle;
    int64_t section_time = 0.1 * time::OneSecond;
    if (parameters) {
        auto velodyne_param = reinterpret_cast<const LocalParameters*>(parameters);
        format = velodyne_param->get_PointFormat();
        accumulate = velodyne_param->get_Accumulate();
        section_base = velodyne_param->get_SectionBase();
        section_time = velodyne_param->get_SectionTime() * time::OneSecond;
    }

    auto sensor_data = do_convert_single_packet(storage_data, format);
    if (!accumulate) {
        return sensor_data;
    }

    if (sensor_data->sensor_data_type != SensorDataType::Points) {
        return sensor_data;
    }

    auto lidar_data = reinterpret_cast<data::Points*>(sensor_data.get());

    if (!sensor_data_map_[sensor_data->sensor_id]) {
        sensor_data_map_[sensor_data->sensor_id] = sensor_data;
        azimuth_map_[sensor_data->sensor_id] = lidar_data->meta.azimuth;
        timestamp_map_[sensor_data->sensor_id] = sensor_data->timestamp_intrinsic_ns;
        accumulated_points_map_[sensor_data->sensor_id].clear();
        return data::SensorData::broken_data();
    }

    auto packet_diff_time = (double)(lidar_data->timestamp_intrinsic_ns -
                                     sensor_data_map_[sensor_data->sensor_id]->timestamp_intrinsic_ns) /
                            (double)time::OneSecond;
    for (size_t i = 0; i < lidar_data->point_number; ++i) {
        auto pt = lidar_data->points[i];
        if (pt.horizontal_distance != 0) {
            pt.time_offset += packet_diff_time;
            accumulated_points_map_[sensor_data->sensor_id].emplace_back(std::move(pt));
        }
    }

    bool section_trigger = true;
    if (section_base == SectionBase::Angle) {
        section_trigger = (lidar_data->meta.rotation_direction < 0 && azimuth_map_[sensor_data->sensor_id] > 0 &&
                           lidar_data->meta.azimuth < 0) ||
                          (lidar_data->meta.rotation_direction > 0 && azimuth_map_[sensor_data->sensor_id] < 0 &&
                           lidar_data->meta.azimuth > 0);
    } else if (section_base == SectionBase::Time) {
        section_trigger = sensor_data->timestamp_intrinsic_ns / section_time >
                          timestamp_map_[sensor_data->sensor_id] / section_time;
    }

    if (section_trigger) {
        auto point_number = accumulated_points_map_[sensor_data->sensor_id].size();
        auto length = sizeof(data::Points::PointXYZCIDPAT) * point_number + sizeof(data::Points);
        auto acc_sensor_data = data::SensorData::create_direct(SensorDataType::Points,
                                                               length,
                                                               sensor_data->sensor_id,
                                                               sensor_data->sequence);
        acc_sensor_data->timestamp_intrinsic_ns = sensor_data->timestamp_intrinsic_ns;
        auto lidar_acc_sensor_data = reinterpret_cast<data::Points*>(acc_sensor_data.get());
        lidar_acc_sensor_data->meta = lidar_data->meta;
        lidar_acc_sensor_data->point_number = point_number;
        memcpy(lidar_acc_sensor_data->points,
               accumulated_points_map_[sensor_data->sensor_id].data(),
               point_number * sizeof(data::Points::PointXYZCIDPAT));
        sensor_data_map_[sensor_data->sensor_id] = nullptr;

        return acc_sensor_data;
    }
    azimuth_map_[sensor_data->sensor_id] = lidar_data->meta.azimuth;
    timestamp_map_[sensor_data->sensor_id] = sensor_data->timestamp_intrinsic_ns;

    return data::SensorData::broken_data();
}

/// Check the LidarType and ReturnMode first,
/// if valid, do convertion by LidarType
data::SensorDataPtr DevicePlugin::do_convert_single_packet(const data::DeviceDataPtr& storage_data, PointFormat format)
{
    if (!storage_data->is_type(VelodynePacketFullSynced::TypeVal) &&
        !storage_data->is_type(VelodynePacketLocalSynced::TypeVal) &&
        !storage_data->is_type(VelodynePacketUnSynced::TypeVal)) {
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
    data::Points::ReturnType return_type;
    switch (raw_data->data.return_mode) {
    case VelodynePacket::ReturnMode::Strongest:
        return_type = data::Points::ReturnType::Strongest;
        break;
    case VelodynePacket::ReturnMode::LastReturn:
        return_type = data::Points::ReturnType::Last;
        break;
    case VelodynePacket::ReturnMode::DualReturn:
        return_type = data::Points::ReturnType::Dual;
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
    double azimuth = (int32_t)(raw_data->data.data_blocks[0].azimuth) * velodyne::AzimuthGranularity;
    if (azimuth_diff_overall < 0) {
        azimuth_diff_overall += 2 * M_PI;
    }

    double azimuth_gap = azimuth_diff_overall / (VelodynePacket::NumDataBlockPerPacket - 1);
    if (is_dual) {
        azimuth_gap *= 2;
    }

    // Create a temp vector
    std::vector<data::Points::PointXYZCIDPAT> lidar_points;
    lidar_points.reserve(VelodynePacket::NumPointsPerPacket);

    for (auto data_block_index = 0; data_block_index < VelodynePacket::NumDataBlockPerPacket; ++data_block_index) {
        const auto* data_block = &raw_data->data.data_blocks[data_block_index];
        const auto data_block_index_real = is_dual ? data_block_index / 2 : data_block_index;

        double azimuth_base = velodyne::AzimuthGranularity * (data_block->azimuth);
        for (auto channel_index = 0; channel_index < VelodynePacket::NumChannelPerDataBlock; ++channel_index) {
            const auto* channel = &data_block->channels[channel_index];

            data::Points::PointXYZCIDPAT lidar_point;
            lidar_point.intensity = channel->reflectivity;

            switch (raw_data->data.lidar_type) {
            case VelodynePacket::LidarType::VLP16C: {
                double azimuth =
                        std::remainder(-(azimuth_base +
                                         azimuth_gap * velodyne::vlp16c::GetRelativeAzimuthChange(channel_index)),
                                       2 * M_PI);
                float time_offset = velodyne::vlp16c::GetTimeOffset(channel_index, data_block_index_real);
                double distance = channel->distance * velodyne::vlp16c::DistanceGranularity;

                double pitch = velodyne::vlp16c::VerticalAngles[channel_index % 16];
                double horizontal_distance = distance * cos(pitch);
                lidar_point.x = horizontal_distance * cos(azimuth);
                lidar_point.y = horizontal_distance * sin(azimuth);
                lidar_point.z = distance * sin(pitch) + velodyne::vlp16c::VerticalCorrection[channel_index % 16];
                lidar_point.channel = channel_index % 16;
                lidar_point.ring = velodyne::vlp16c::Rings[lidar_point.channel];
                lidar_point.horizontal_distance = horizontal_distance;
                lidar_point.azimuth = azimuth;
                lidar_point.pitch = pitch;
                lidar_point.time_offset = time_offset;
            } break;

            case VelodynePacket::LidarType::VLP32C: {
                double azimuth =
                        std::remainder(-(azimuth_base + velodyne::vlp32c::AzimuthOffset[channel_index] +
                                         azimuth_gap * velodyne::vlp32c::GetRelativeAzimuthChange(channel_index)),
                                       2 * M_PI);
                float time_offset = velodyne::vlp32c::GetTimeOffset(channel_index, data_block_index_real);
                double distance = channel->distance * velodyne::vlp32c::DistanceGranularity;

                double pitch = velodyne::vlp32c::VerticalAngles[channel_index];
                double horizontal_distance = distance * cos(pitch);
                lidar_point.x = horizontal_distance * cos(azimuth);
                lidar_point.y = horizontal_distance * sin(azimuth);
                lidar_point.z = distance * sin(pitch);
                lidar_point.channel = channel_index;
                lidar_point.ring = velodyne::vlp32c::Rings[lidar_point.channel];
                lidar_point.horizontal_distance = horizontal_distance;
                lidar_point.azimuth = azimuth;
                lidar_point.pitch = pitch;
                lidar_point.time_offset = time_offset;
            } break;

            case VelodynePacket::LidarType::HDL32E: {
                double azimuth =
                        std::remainder(-(azimuth_base +
                                         azimuth_gap * velodyne::hdl32e::GetRelativeAzimuthChange(channel_index)) +
                                               velodyne::hdl32e::AzimuthCorrection,
                                       2 * M_PI);
                float time_offset = velodyne::hdl32e::GetTimeOffset(channel_index, data_block_index_real);
                double distance = channel->distance * velodyne::hdl32e::DistanceGranularity;

                double pitch = velodyne::hdl32e::VerticalAngles[channel_index];
                double horizontal_distance = distance * cos(pitch);
                lidar_point.x = horizontal_distance * cos(azimuth);
                lidar_point.y = horizontal_distance * sin(azimuth);
                lidar_point.z = distance * sin(pitch);
                lidar_point.channel = channel_index;
                lidar_point.ring = velodyne::hdl32e::Rings[lidar_point.channel];
                lidar_point.horizontal_distance = horizontal_distance;
                lidar_point.azimuth = azimuth;
                lidar_point.pitch = pitch;
                lidar_point.time_offset = time_offset;
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
    auto length = sizeof(data::Points::PointXYZCIDPAT) * point_number + sizeof(data::Points);
    auto sensor_data = data::SensorData::create_from(storage_data, SensorDataType::Points, length);
    auto lidar_sensor_data = static_cast<data::Points*>(sensor_data.get());

    // Memcpy from temp vector
    lidar_sensor_data->point_number = point_number;

    if (!is_dual) {
        memcpy(lidar_sensor_data->points, lidar_points.data(), sizeof(data::Points::PointXYZCIDPAT) * point_number);
    } else {
        for (auto data_block_index = 0; data_block_index < VelodynePacket::NumDataBlockPerPacket / 2;
             ++data_block_index) {
            memcpy(&lidar_sensor_data->points[data_block_index * VelodynePacket::NumChannelPerDataBlock],
                   &lidar_points[2 * data_block_index * VelodynePacket::NumChannelPerDataBlock],
                   sizeof(data::Points::PointXYZCIDPAT) * VelodynePacket::NumChannelPerDataBlock);
            memcpy(&lidar_sensor_data->points[data_block_index * VelodynePacket::NumChannelPerDataBlock +
                                              VelodynePacket::NumPointsPerPacket / 2],
                   &lidar_points[(2 * data_block_index + 1) * VelodynePacket::NumChannelPerDataBlock],
                   sizeof(data::Points::PointXYZCIDPAT) * VelodynePacket::NumChannelPerDataBlock);
        }
    }

    // Calculate laser firing timestamp of the first laser beam
    if (storage_data->is_type(VelodynePacketFullSynced::TypeVal)) {
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
    } else if (storage_data->is_type(VelodynePacketLocalSynced::TypeVal)) {
        int64_t t_packet_us = (int64_t)(raw_data->data.timestamp);
        lidar_sensor_data->timestamp_intrinsic_ns = t_packet_us * UsToNs_;
    } else {
        lidar_sensor_data->timestamp_intrinsic_ns = raw_data->get_timestamp_receive_ns();
    }

    // Fill Metadata
    lidar_sensor_data->meta.return_type = return_type;
    switch (format) {
    case PointFormat::XYZI:
        lidar_sensor_data->meta.point_format = data::Points::PointFormat::XYZI;
        break;
    case PointFormat::XYZIRT:
        lidar_sensor_data->meta.point_format = data::Points::PointFormat::XYZIRT;
        break;
    }
    switch (raw_data->data.lidar_type) {
    case VelodynePacket::LidarType::VLP16C:
        lidar_sensor_data->meta.vendor = data::Points::LidarVendor::VelodyneVLP16C;

        lidar_sensor_data->meta.rotation_direction = -1;

        lidar_sensor_data->meta.num_channel = 16;
        lidar_sensor_data->meta.nominal_pitch_increment = velodyne::vlp16c::VerticalAngleIncrement;

        lidar_sensor_data->meta.time_increment = velodyne::vlp16c::TimePerPoint / time::OneSecond;
        lidar_sensor_data->meta.time_increment_horizontal = velodyne::vlp16c::TimeHorizontal / time::OneSecond;
        lidar_sensor_data->meta.total_time =
                velodyne::vlp16c::TimeHorizontal / time::OneSecond * 2 * VelodynePacket::NumDataBlockPerPacket;
        lidar_sensor_data->meta.azimuth = std::remainder(-azimuth, 2 * M_PI);

        lidar_sensor_data->meta.nominal_min_range = velodyne::vlp16c::MinNominalRange;
        lidar_sensor_data->meta.nominal_max_range = velodyne::vlp16c::MaxNominalRange;
        break;

    case VelodynePacket::LidarType::VLP32C:
        lidar_sensor_data->meta.vendor = data::Points::LidarVendor::VelodyneVLP32C;

        lidar_sensor_data->meta.rotation_direction = -1;

        lidar_sensor_data->meta.num_channel = 32;
        lidar_sensor_data->meta.nominal_pitch_increment = velodyne::vlp32c::VerticalAngleIncrement;

        lidar_sensor_data->meta.time_increment = velodyne::vlp32c::TimePerPoint / time::OneSecond;
        lidar_sensor_data->meta.time_increment_horizontal = velodyne::vlp32c::TimeHorizontal / time::OneSecond;
        lidar_sensor_data->meta.total_time =
                velodyne::vlp32c::TimeHorizontal / time::OneSecond * VelodynePacket::NumDataBlockPerPacket;
        lidar_sensor_data->meta.azimuth = std::remainder(-azimuth, 2 * M_PI);

        lidar_sensor_data->meta.nominal_min_range = velodyne::vlp32c::MinNominalRange;
        lidar_sensor_data->meta.nominal_max_range = velodyne::vlp32c::MaxNominalRange;
        break;

    case VelodynePacket::LidarType::HDL32E:
        lidar_sensor_data->meta.vendor = data::Points::LidarVendor::VelodyneHDL32E;

        lidar_sensor_data->meta.rotation_direction = -1;

        lidar_sensor_data->meta.num_channel = 32;
        lidar_sensor_data->meta.nominal_pitch_increment = velodyne::hdl32e::VerticalAngleIncrement;

        lidar_sensor_data->meta.time_increment = velodyne::hdl32e::TimePerPoint / time::OneSecond;
        lidar_sensor_data->meta.time_increment_horizontal = velodyne::hdl32e::TimeHorizontal / time::OneSecond;
        lidar_sensor_data->meta.total_time =
                velodyne::hdl32e::TimeHorizontal / time::OneSecond * VelodynePacket::NumDataBlockPerPacket;
        lidar_sensor_data->meta.azimuth = std::remainder(-azimuth - velodyne::hdl32e::AzimuthCorrection, 2 * M_PI);

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