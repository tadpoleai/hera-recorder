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
#include <ctime>

#include "hesai_defs.hpp"
#include "plugin_common.hpp"
#include "plugin_data.hpp"
#include "plugin_param.hpp"

#ifdef WITH_DRIVER
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#endif

namespace wayz {
namespace hera {
namespace device {
namespace lidar {
namespace hesai {

///
/// @brief Hesai Lidar
///
HERA_PLUGIN_DEFINE_START(250)

static constexpr size_t EthernetMTU_ = 1500;  ///< MTU/PacketSize of ethernet interface

static constexpr time::Duration MaxDelayTolerance = 30 * time::OneSecond;

#ifdef WITH_DRIVER
HERA_PLUGIN_DEFINE_FUNCTIONS

::sockaddr_in addr_in_;                 ///< Ip Address and Data Port of Lidar
int socket_;                            ///< Socket handler
uint8_t receive_buffer_[EthernetMTU_];  ///< UDP Receive buffer
#endif

HERA_PLUGIN_DEFINE_END

HERA_PLUGIN_EXPORT(LidarHesai, "lidar/hesai")

#ifdef WITH_DRIVER

/// Turn on Lidar's Laser by Remote Control (curl),
/// then created a socket by IP and UDP Port
HeraErrno DevicePlugin::connect()
{
    log::debug << "Hesai:: Connecting to Hesai by binding port: " << local_parameters_.get_ListenPort() << log::endl;

    std::string url = "http://" + local_parameters_.get_IpAddress() +
                      "/pandar.cgi?action=set&object=lidar_data&key=standbymode&value=0";
    int ret = system((std::string("curl -m 0.2 \"") + url + "\"").c_str());
    if (ret == 0) {
        log::info << "Hesai::Succeeded to power on lidar " << get_name() << log::endl;
    } else {
        return handle_error(HeraErrno::CanNotOpenEthernetDevice, "Hesai::Failed to power on " + get_name());
    }

    int32_t lidar_mode = int32_t(local_parameters_.get_ReturnType());
    url = "http://" + local_parameters_.get_IpAddress() +
          "/pandar.cgi?action=set&object=lidar_data&key=lidar_mode&value=" + std::to_string(lidar_mode);
    ret = system((std::string("curl -m 0.2 \"") + url + "\"").c_str());
    if (ret == 0) {
        log::info << "Hesai::Succeeded to set return type of lidar " << get_name() << log::endl;
    } else {
        return handle_error(HeraErrno::CanNotOpenEthernetDevice, "Hesai::Failed to set parameter of " + get_name());
    }

    int32_t spin_speed = int32_t(local_parameters_.get_Speed()) + 1;
    url = "http://" + local_parameters_.get_IpAddress() +
          "/pandar.cgi?action=set&object=lidar&key=spin_speed&value=" + std::to_string(spin_speed);
    ret = system((std::string("curl -m 0.2 \"") + url + "\"").c_str());
    if (ret == 0) {
        log::info << "Hesai::Succeeded to set spin speed of lidar " << get_name() << log::endl;
    } else {
        return handle_error(HeraErrno::CanNotOpenEthernetDevice, "Hesai::Failed to set parameter of " + get_name());
    }

    try {
        addr_in_.sin_family = AF_INET;
        addr_in_.sin_port = htons(local_parameters_.get_ListenPort());
        addr_in_.sin_addr.s_addr = 0;

        socket_ = ::socket(AF_INET, SOCK_DGRAM, 0);
        if (::bind(socket_, (::sockaddr*)&addr_in_, sizeof(addr_in_)) < 0) {
            return handle_error(HeraErrno::CanNotOpenEthernetDevice, "Hesai::Bind port, can not bind");
        }
        static const timeval TimeOut = {0, 50000};
        setsockopt(socket_, SOL_SOCKET, SO_RCVTIMEO, &TimeOut, sizeof(TimeOut));
    } catch (...) {
        return handle_error(HeraErrno::CanNotOpenEthernetDevice);
    }

    log::debug << "Hesai:: Connection succeed" << log::endl;
    return HeraErrno::Success;
}

/// Turn off Lidar's Laser by Remote Control (curl),
/// then delete the socket
void DevicePlugin::disconnect()
{
    log::debug << "Hesai:: socket closing" << log::endl;
    ::close(socket_);

    std::string url = "http://" + local_parameters_.get_IpAddress() +
                      "/pandar.cgi?action=set&object=lidar_data&key=standbymode&value=1";
    int ret = system((std::string("curl -m 0.2 \"") + url + "\"").c_str());
    if (ret == 0) {
        log::info << "Hesai::Succeeded to power off lidar " << get_name() << log::endl;
    } else {
        log::error << "Hesai::Failed to power off " << get_name();
    }
    log::debug << "Hesai:: socket closed successfullly " << log::endl;
}

/// Wait a UDP packet from socket, then create a HesaiStorage,
/// after that, spin io_service
data::DeviceDataPtr DevicePlugin::fetch()
{
    auto received_length = ::recv(socket_, receive_buffer_, sizeof(receive_buffer_), 0);

    if (received_length != sizeof(HesaiPacket::buf)) {
        return nullptr;
    }

    // Total length of device data
    auto length = sizeof(HesaiPacket);
    auto data_type = DeviceDataType::LidarHesaiPacketFullSynced;
    switch (local_parameters_.get_SyncType()) {
    case SyncType::Full:
        data_type = DeviceDataType::LidarHesaiPacketFullSynced;
        break;
    case SyncType::Local:
        data_type = DeviceDataType::LidarHesaiPacketLocalSynced;
        break;
    case SyncType::Disabled:
        data_type = DeviceDataType::LidarHesaiPacketUnSynced;
        break;
    }

    auto data = data::DeviceData::create(length, id_, DeviceVendorType::LidarHesai, data_type, sequence_++);
    auto derived_data = static_cast<HesaiPacket*>(data.get());

    // Use Memcpy to directly fill buf
    memcpy(derived_data->buf, &receive_buffer_, received_length);

    return data;
}

HeraErrno DevicePlugin::adjust_parameter(const std::string& type, const std::string& value)
{
    if (type == "Speed") {
        int32_t spin_speed = int32_t(local_parameters_.get_Speed()) + 1;
        std::string url = "http://" + local_parameters_.get_IpAddress() +
                          "/pandar.cgi?action=set&object=lidar&key=spin_speed&value=" + std::to_string(spin_speed);
        int ret = system((std::string("curl -m 0.2 \"") + url + "\"").c_str());
        if (ret == 0) {
            log::info << "Hesai::Set lidar lidar " << get_name() << " speed as "
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
    if (!storage_data->is_type(DeviceDataType::LidarHesaiPacketFullSynced) &&
        !storage_data->is_type(DeviceDataType::LidarHesaiPacketLocalSynced) &&
        !storage_data->is_type(DeviceDataType::LidarHesaiPacketUnSynced)) {
        return data::SensorData::broken_data();
    }

    // Raw DeviceData of Derived Type
    auto raw_data = static_cast<HesaiPacket*>(storage_data.get());

    // Parse Data
    /// Check LidarType and ReturnMode
    if (raw_data->data.lidar_type == HesaiPacket::LidarType::PandarXT32) {
    } else {
        // Unsupported lidar type;
        log::warn << "Hesai '" << storage_data->get_device_id() << "': Unknown lidar type '"
                  << int32_t(raw_data->data.lidar_type) << "'" << log::endl;
        return data::SensorData::broken_data();
    }

    double is_dual = false;
    data::Points::ReturnType return_type;
    switch (raw_data->data.return_mode) {
    case HesaiPacket::ReturnMode::Strongest:
        return_type = data::Points::ReturnType::Strongest;
        break;
    case HesaiPacket::ReturnMode::LastReturn:
        return_type = data::Points::ReturnType::Last;
        break;
    case HesaiPacket::ReturnMode::DualReturn:
        return_type = data::Points::ReturnType::Dual;
        is_dual = true;
        break;
    default:
        log::warn << "Hesai '" << storage_data->get_device_id() << "': Unknown return mode '"
                  << int32_t(raw_data->data.return_mode) << "'" << log::endl;
        return data::SensorData::broken_data();
    }

    if (raw_data->data.laser_num != NumChannelPerDataBlock) {
        log::warn << "Hesai: Laser Num != 32" << log::endl;
        return data::SensorData::broken_data();
    }
    if (raw_data->data.block_num != NumDataBlockPerPacket) {
        log::warn << "Hesai: Block Num != 8" << log::endl;
        return data::SensorData::broken_data();
    }

    /// Get motor speed between data blocks
    double motor_speed = (double)(raw_data->data.motor_speed) * hesai::MotorSpeedGranularity;

    // Create a temp vector
    std::vector<data::Points::PointXYZCIDPAT> lidar_points;
    lidar_points.reserve(hesai::NumPointsPerPacket);

    for (auto data_block_index = 0; data_block_index < hesai::NumDataBlockPerPacket; ++data_block_index) {
        const auto* data_block = &raw_data->data.data_blocks[data_block_index];

        double azimuth_base = hesai::AzimuthGranularity * (data_block->azimuth) + M_PI;
        for (auto channel_index = 0; channel_index < hesai::NumChannelPerDataBlock; ++channel_index) {
            const auto* channel = &data_block->channels[channel_index];

            data::Points::PointXYZCIDPAT lidar_point;
            lidar_point.intensity = channel->reflectivity;

            switch (raw_data->data.lidar_type) {
            case HesaiPacket::LidarType::PandarXT32: {
                double azimuth = std::remainder(-(azimuth_base + motor_speed * hesai::pandarxt32::GetAbsoluteTimeChange(
                                                                                       channel_index)),
                                                2 * M_PI);
                double distance = channel->distance * hesai::pandarxt32::DistanceGranularity;

                double pitch = hesai::pandarxt32::VerticalAngles[channel_index];
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
    auto length = sizeof(data::Points::PointXYZCIDPAT) * point_number + sizeof(data::Points);
    auto sensor_data = data::SensorData::create_from(storage_data, SensorDataType::Points, length);
    auto lidar_sensor_data = static_cast<data::Points*>(sensor_data.get());

    // Memcpy from temp vector
    lidar_sensor_data->point_number = point_number;

    if (!is_dual) {
        memcpy(lidar_sensor_data->points,
               lidar_points.data(),
               sizeof(data::Points::PointXYZCIDPAT) * point_number);
    } else {
        for (auto data_block_index = 0; data_block_index < hesai::NumDataBlockPerPacket / 2; ++data_block_index) {
            memcpy(&lidar_sensor_data->points[data_block_index * hesai::NumChannelPerDataBlock],
                   &lidar_points[2 * data_block_index * hesai::NumChannelPerDataBlock],
                   sizeof(data::Points::PointXYZCIDPAT) * hesai::NumChannelPerDataBlock);
            memcpy(&lidar_sensor_data->points[data_block_index * hesai::NumChannelPerDataBlock +
                                                       hesai::NumPointsPerPacket / 2],
                   &lidar_points[(2 * data_block_index + 1) * hesai::NumChannelPerDataBlock],
                   sizeof(data::Points::PointXYZCIDPAT) * hesai::NumChannelPerDataBlock);
        }
    }

    struct tm timeinfo;
    timeinfo.tm_year = raw_data->data.date_time.year;
    timeinfo.tm_mon = raw_data->data.date_time.month - 1;
    timeinfo.tm_mday = raw_data->data.date_time.day;
    timeinfo.tm_hour = raw_data->data.date_time.hour;
    timeinfo.tm_min = raw_data->data.date_time.minute;
    timeinfo.tm_sec = raw_data->data.date_time.second;
    timeinfo.tm_gmtoff = 0;
    auto t_packet_sec = timegm(&timeinfo);

    int64_t t_packet = time::Timestamp(t_packet_sec, raw_data->data.timestamp_subsec * time::OneMicroSecond);
    int64_t t_recv = (raw_data->get_timestamp_receive_ns());

    // Calculate laser firing timestamp of the first laser beam
    if (storage_data->is_type(DeviceDataType::LidarHesaiPacketFullSynced)) {
        if (labs(t_packet - t_recv) > MaxDelayTolerance) {
            // Synchronization lost
            /// @note If Synchronization is lost, i.e. t_recv is more than t_fire by MaxDelayTolerance,
            /// data will be abandon
            log::warn << "Hesai: Data with ts = " << t_packet << ", received at " << t_recv << " too far" << log::endl;
            return data::SensorData::broken_data();
        }
        lidar_sensor_data->timestamp_intrinsic_ns = t_packet;
    } else if (storage_data->is_type(DeviceDataType::LidarHesaiPacketLocalSynced)) {
        lidar_sensor_data->timestamp_intrinsic_ns = t_packet;
    } else {
        lidar_sensor_data->timestamp_intrinsic_ns = t_recv;
    }

    // Fill Metadata
    lidar_sensor_data->meta.return_type = return_type;
    switch (raw_data->data.lidar_type) {
    case HesaiPacket::LidarType::PandarXT32:
        lidar_sensor_data->meta.vendor = data::Points::LidarVendor::VendorHesai;

        lidar_sensor_data->meta.rotation_direction = -1;

        lidar_sensor_data->meta.num_channel = 32;
        lidar_sensor_data->meta.nominal_pitch_increment = hesai::pandarxt32::VerticalAngleIncrement;

        lidar_sensor_data->meta.time_increment = hesai::pandarxt32::TimePerPoint;
        lidar_sensor_data->meta.time_increment_horizontal = hesai::pandarxt32::TimeHorizontal;
        lidar_sensor_data->meta.total_time = hesai::pandarxt32::TimeHorizontal * hesai::NumDataBlockPerPacket;

        lidar_sensor_data->meta.nominal_min_range = hesai::pandarxt32::MinNominalRange;
        lidar_sensor_data->meta.nominal_max_range = hesai::pandarxt32::MaxNominalRange;
        break;
    }

    if (is_dual) {
        lidar_sensor_data->meta.total_time /= 2;
    }

    return sensor_data;
}

}  // namespace hesai
}  // namespace lidar
}  // namespace device
}  // namespace hera
}  // namespace wayz