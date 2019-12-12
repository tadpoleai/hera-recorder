///
/// @file lidar_velodyne.cpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Implementation of class Velodyne
/// @version 0.1
/// @date 2019-11-06
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include "velodyne.hpp"

namespace wayz {
namespace hera {
namespace lidar {

const timeval Velodyne::TimeOut_ = {0, 50000};

/// Turn on Lidar's Laser by Remote Control (curl),
/// then created a socket by IP and UDP Port
HeraErrno Velodyne::connect()
{
    log::debug << "Velodyne:: Connecting to velodyne by binding port: " << parameters_[DeviceParameterType::DataPort]
               << log::endl;
    try {
        addr_in_.sin_family = AF_INET;
        addr_in_.sin_port = htons(std::stoi(parameters_[DeviceParameterType::DataPort]));
        addr_in_.sin_addr.s_addr = 0;

        socket_ = ::socket(AF_INET, SOCK_DGRAM, 0);
        if (::bind(socket_, (::sockaddr*)&addr_in_, sizeof(addr_in_)) < 0) {
            return handle_error(HeraErrno::CanNotOpenEthernetDevice, "Can not bind");
        }
        setsockopt(socket_, SOL_SOCKET, SO_RCVTIMEO, &TimeOut_, sizeof(TimeOut_));
    } catch (...) {
        return handle_error(HeraErrno::CanNotOpenEthernetDevice);
    }

    try {
        int ret = system(
                ("curl --data \"laser=on\" http://" + parameters_[DeviceParameterType::IpAddress] + "/cgi/setting")
                        .c_str());
        if (ret == 0) {
            log::info << "Velodyne::Succeeded to power on lidar " << get_name() << log::endl;
        } else {
            return handle_error(HeraErrno::CanNotOpenEthernetDevice, "Failed to power on " + get_name());
        }
    } catch (...) {
        return handle_error(HeraErrno::CanNotOpenEthernetDevice, "Failed to power on " + get_name());
    }

    log::debug << "Velodyne::Connection succeed: " << log::endl;
    return HeraErrno::Success;
}

/// Turn off Lidar's Laser by Remote Control (curl),
/// then delete the socket
void Velodyne::disconnect()
{
    try {
        int ret = system(
                ("curl --data \"laser=off\" http://" + parameters_[DeviceParameterType::IpAddress] + "/cgi/setting")
                        .c_str());
        if (ret == 0) {
            log::info << "Velodyne::Succeeded to power off lidar " << get_name() << log::endl;
        } else {
            log::info << "Velodyne::Failed to power off lidar " << get_name() << log::endl;
        }
    } catch (...) {
        log::info << "Velodyne::Failed to power off lidar " << get_name() << log::endl;
    }

    ::close(socket_);
}

/// Wait a UDP packet from socket, then create a VelodyneStorage,
/// after that, spin io_service
StorageDataPtr Velodyne::fetch()
{
    auto received_length = ::recv(socket_, receive_buffer_, sizeof(receive_buffer_), 0);

    // Total length of storage data
    auto length = sizeof(VelodyneStorageData);
    auto data = StorageData::create(length,
                                    DeviceVendorType::LidarVelodyne,
                                    StorageDataType::LidarVelodynePacket,
                                    sequence_++);
    auto derived_data = static_cast<VelodyneStorageData*>(data.get());

    if (received_length != sizeof(derived_data->buf)) {
        return nullptr;
    }
    // Use Memcpy to directly fill buf
    memcpy(derived_data->buf, &receive_buffer_, received_length);

    return data;
}

/// Check the LidarType and ReturnMode first,
/// if valid, do convertion by LidarType
SensorDataPtr Velodyne::convert(StorageDataPtr& storage_data)
{
    if (!storage_data->is_type(StorageDataType::LidarVelodynePacket)) {
        return SensorData::broken_data();
    }

    // Raw StorageData of Derived Type
    auto raw_data = static_cast<VelodyneStorageData*>(storage_data.get());

    // Parse Data
    /// Check LidarType and ReturnMode
    /// @see @ref VLP-16C-Manual section: 9.3.1.7, Factory Bytes, page: 56
    if (raw_data->data.lidar_type == VelodyneStorageData::LidarType::VLP16C ||
        raw_data->data.lidar_type == VelodyneStorageData::LidarType::VLP32C ||
        raw_data->data.lidar_type == VelodyneStorageData::LidarType::HDL32E) {
    } else {
        // Unsupported lidar type;
        return SensorData::broken_data();
    }
    if (raw_data->data.return_mode == VelodyneStorageData::ReturnMode::DualReturn) {
        // Unsupported return mode
        return SensorData::broken_data();
    }

    /// Get azimuth gap between data blocks
    /// @see @ref VLP-16C-Manual section: 9.5, Precision Azimuth Calculation, page: 65
    double azimuth_gap =
            ((int32_t)(raw_data->data.data_blocks[1].azimuth) - (int32_t)(raw_data->data.data_blocks[0].azimuth)) *
            velodyne::AzimuthGranularity;
    if (azimuth_gap < 0) {
        azimuth_gap += 2 * M_PI;
    }

    // Create a temp vector
    std::vector<PointsXYZISensorData::PointXYZI> lidar_points;
    lidar_points.reserve(VelodyneStorageData::NumPointsPerPacket);

    for (auto data_block_index = 0; data_block_index < VelodyneStorageData::NumDataBlockPerPacket; ++data_block_index) {
        const auto* data_block = &raw_data->data.data_blocks[data_block_index];

        /// @note ROS axis definitions is used for Lidar's xyz,
        /// in which X = forward, Y = left, Z = up, right-handed,
        /// regardless of velodyne's original axes definitions.
        /// We use ROS axis definitions
        /// @see <a href="https://www.ros.org/reps/rep-0103.html#axis-orientation" target="_blank"
        /// rel="noopener noreferrer">Axis-orientation</a>
        /// @see <a href="https://blog.csdn.net/chengde6896383/article/details/86682882"
        /// target="_blank" rel="noopener noreferrer">Velodyne Coordinate System for VLP</a>

        double azimuth_base = velodyne::AzimuthGranularity * (data_block->azimuth);
        for (auto channel_index = 0; channel_index < VelodyneStorageData::NumChannelPerDataBlock; ++channel_index) {
            const auto* channel = &data_block->channels[channel_index];

            /// Check if distance is 0, indicating invalid point
            if (channel->distance == 0) {
                continue;
            }
            PointsXYZISensorData::PointXYZI lidar_point;
            lidar_point.intensity = channel->reflectivity;

            switch (raw_data->data.lidar_type) {
            case VelodyneStorageData::LidarType::VLP16C: {
                double azimuth = azimuth_base + azimuth_gap * velodyne::vlp16c::GetRelativeAzimuthChange(channel_index);
                double distance = channel->distance * velodyne::vlp16c::DistanceGranularity;

                double pitch = velodyne::vlp16c::VerticalAngles[channel_index % 16];
                double distance_horizontal = distance * cos(pitch);
                // Origin X, ROS Definition -Y
                lidar_point.y = -distance_horizontal * sin(azimuth);
                // Origin Y, ROS Definition X
                lidar_point.x = distance_horizontal * cos(azimuth);
                lidar_point.z = distance * sin(pitch) + velodyne::vlp16c::VerticalCorrection[channel_index];
                lidar_point.channel = channel_index % 16;
            } break;

            case VelodyneStorageData::LidarType::VLP32C: {
                double azimuth = azimuth_base + velodyne::vlp32c::AzimuthOffset[channel_index] +
                                 azimuth_gap * velodyne::vlp32c::GetRelativeAzimuthChange(channel_index);
                double distance = channel->distance * velodyne::vlp32c::DistanceGranularity;

                double pitch = velodyne::vlp32c::VerticalAngles[channel_index];
                double distance_horizontal = distance * cos(pitch);
                // Origin X, ROS Definition -Y
                lidar_point.y = -distance_horizontal * sin(azimuth);
                // Origin Y, ROS Definition X
                lidar_point.x = distance_horizontal * cos(azimuth);
                lidar_point.z = distance * sin(pitch);
                lidar_point.channel = channel_index;
            } break;

            case VelodyneStorageData::LidarType::HDL32E: {
                double azimuth = azimuth_base + azimuth_gap * velodyne::hdl32e::GetRelativeAzimuthChange(channel_index);
                double distance = channel->distance * velodyne::hdl32e::DistanceGranularity;

                double pitch = velodyne::hdl32e::VerticalAngles[channel_index];
                double distance_horizontal = distance * cos(pitch);
                // Origin X, ROS Definition -X
                lidar_point.x = -distance_horizontal * sin(azimuth);
                // Origin Y, ROS Definition -Y
                lidar_point.y = -distance_horizontal * cos(azimuth);
                lidar_point.z = distance * sin(pitch);
                lidar_point.channel = channel_index;
            } break;

            default:
                return SensorData::broken_data();
                break;
            }  // switch lidar type

            lidar_points.emplace_back(lidar_point);
        }  // for channel
    }      // for data_block

    // Create a SensorData from StorageData
    auto point_number = lidar_points.size();
    auto length = sizeof(PointsXYZISensorData::PointXYZI) * point_number + sizeof(PointsXYZISensorData);
    auto sensor_data = SensorData::create_from(storage_data, SensorDataType::PointsXYZI, length);
    auto lidar_sensor_data = static_cast<PointsXYZISensorData*>(sensor_data.get());

    // Memcpy from temp vector
    memcpy(lidar_sensor_data->points, lidar_points.data(), sizeof(PointsXYZISensorData::PointXYZI) * point_number);
    lidar_sensor_data->point_number = point_number;

    // Calculate laser firing timestamp of the first laser beam
    int64_t t_recv_us = (raw_data->get_timestamp_receive_ns()) / UsToNs_;
    int64_t t_packet_us = (int64_t)(raw_data->data.timestamp);
    int64_t t_fire_us = HourToUs_ * ((t_recv_us - t_packet_us + HalfHourToUs_) / HourToUs_) + t_packet_us;
    if (abs(t_fire_us - t_recv_us) > MaxDelayToleranceUs_) {
        // Synchronization lost
        /// @note If Synchronization is lost, i.e. t_recv is more than t_fire by MaxDelayTolerance,
        /// data will be abandon
        log::warn << "Velodyne: Data with ts = " << t_fire_us << "us, received at " << t_recv_us << "us too far"
                  << log::endl;
        return SensorData::broken_data();
    }
    lidar_sensor_data->timestamp_intrinsic_ns = t_fire_us * UsToNs_;

    return sensor_data;
}

HeraErrno Velodyne::adjust_parameter(DeviceParameterType type, const std::string& value)
{
    switch (type) {
    case DeviceParameterType::DataPort:
    case DeviceParameterType::IpAddress:
        return HeraErrno::ImmutableParameter;
    default:
        return HeraErrno::UnimplementedParameter;
    }
    return HeraErrno::Success;
}

}  // namespace lidar
}  // namespace hera
}  // namespace wayz