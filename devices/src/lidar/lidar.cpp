//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include "lidar.hpp"

#include <chrono>
#include <cstdlib>
#include <string>
#include <vector>

namespace wayz {
namespace tron {

Lidar::Lidar(int32_t id, const std::string& name) : Device(id, name) {}
Lidar::~Lidar()
{
    disconnect();
}

DeviceType Lidar::get_type() const
{
    return DeviceType::Lidar;
}

TronErrno Lidar::connect()
{
    if (parameters_.count(DeviceParameterType::IpAddress)) {
        address_ = boost::asio::ip::address_v4::from_string(
                parameters_[DeviceParameterType::IpAddress]);
    } else {
        return set_error_and_die(TronErrno::InsufficientParameters, "Parameter IpAddress absent");
    }

    if (parameters_.count(DeviceParameterType::DataPort)) {
        data_port_ = std::stoi(parameters_[DeviceParameterType::DataPort]);
    } else {
        return set_error_and_die(TronErrno::InsufficientParameters, "Parameter DataPort absent");
    }

    int ret = system(("curl --data \"laser=on\" http://" +
                      parameters_[DeviceParameterType::IpAddress] + "/cgi/setting")
                             .c_str());
    if (ret == 0) {
        std::cout << "success to power on " << get_name() << std::endl;
    } else {
        std::cout << "fail to power on " << get_name() << std::endl;
    }

    try {
        data_socket_ = new boost::asio::ip::udp::socket(
                io_service_,
                boost::asio::ip::udp::endpoint(boost::asio::ip::address_v4::from_string(
                                                       "255.255.255.255"),
                                               data_port_));
    } catch (const std::exception& e) {
        data_socket_ = nullptr;
        return set_error_and_die(TronErrno::CanNotOpenEthernetDevice);
    }

    if (data_socket_ == nullptr) {  // || telemetry_socket_ == nullptr) {
        std::cout << "create socket failed!" << std::endl;
        return set_error_and_die(TronErrno::InsufficientParameters);
    }
    return TronErrno::Success;
}
void Lidar::disconnect()
{
    stop();
    do_disconnect();
}
void Lidar::do_disconnect()
{
    int ret = system(("curl --data \"laser=off\" http://" +
                      parameters_[DeviceParameterType::IpAddress] + "/cgi/setting")
                             .c_str());
    if (ret == 0) {
        std::cout << "success to power off " << get_name() << std::endl;
    } else {
        std::cout << "fail to power off " << get_name() << std::endl;
    }
    if (data_socket_ && data_socket_->is_open()) {
        data_socket_->close();
        delete data_socket_;
        data_socket_ = nullptr;
    }
    if (io_service_.stopped()) {
        io_service_.stop();
        io_service_.reset();
    }
}

std::shared_ptr<DeviceRawData> Lidar::fetch()
{
    memset(receive_buffer_, 0, kDataBufferSize);
    int32_t receivedRawdataLength =
            data_socket_->receive_from(boost::asio::buffer(receive_buffer_,
                                                           sizeof(receive_buffer_)),
                                       receive_data_endpoint_);
    // Create a Buff to Store Rawdata
    int32_t totalLength = sizeof(DeviceRawData) + receivedRawdataLength;
    DeviceRawData* data = reinterpret_cast<DeviceRawData*>(new uint8_t[totalLength]);

    // Fullfil Metadata (Header) of Rawdata;
    data->length = totalLength;
    data->device_type = DeviceType::Lidar;
    data->device_data_type = DeviceDataType::LidarVelodyneScan;
    data->sequence = sequence_++;
    data->timestamp_receive_ns = get_system_timestamp();

    try {
        io_service_.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return nullptr;
    }
    // Use Memcpy to fill Buff
    memcpy(reinterpret_cast<char*>(data->rawdata_buf), receive_buffer_, receivedRawdataLength);
    // Return a Shared Ptr
    return std::shared_ptr<DeviceRawData>(data);
}
std::shared_ptr<SensorData> Lidar::convert(const std::shared_ptr<DeviceRawData>& rawdata)
{
    return do_convert(rawdata);
}
std::shared_ptr<SensorData> Lidar::do_convert(const std::shared_ptr<DeviceRawData>& rawdata)
{
    // Create a Buff to Store Data
    int32_t total_length =
            sizeof(SensorData) + sizeof(DataLidar) + sizeof(LidarPoint) * NumLidarPoints_;
    auto data =
            std::shared_ptr<SensorData>(reinterpret_cast<SensorData*>(new uint8_t[total_length]));

    // Fullfil Metadata (Header) of Data;
    data->length = total_length;
    data->device_type = rawdata->device_type;
    data->device_data_type = rawdata->device_data_type;
    data->sequence = rawdata->sequence;
    data->timestamp_receive_ns = rawdata->timestamp_receive_ns;

    // A Pointer to Real Data
    DataLidar* data_lidar = reinterpret_cast<DataLidar*>(data->data_buf);
    LidarPoint* lidar_point = data_lidar->lidar_points;
    data_lidar->point_number = 0;

    // Parse rawdata;
    const auto* rawdata_buf = reinterpret_cast<DeviceRawDataLidar*>(rawdata->rawdata_buf);

    // Check LidarType and ReturnMode
    // For more details, refer to the following document
    // pdf: VLP-16 User Manual Note, section: 9.3.1.7, Factory Bytes, page: 56
    if (rawdata_buf->lidar_type == LidarType::VelodyneVLP16C ||
        rawdata_buf->lidar_type == LidarType::VelodyneVLP32C ||
        rawdata_buf->lidar_type == LidarType::VelodyneHDL32E) {
    } else {
        // Unsupported lidar type;
        return nullptr;
    }
    if (rawdata_buf->return_mode == ReturnMode::DualReturn) {
        // Unsupported return mode
        return nullptr;
    }

    // Get azimuth gap between data blocks
    // For more details, refer to the following document
    // pdf: VLP-16 User Manual Note, section: 9.5, Precision Azimuth Calculation, page: 65
    double azimuth_gap = ((int32_t)(rawdata_buf->data_blocks[1].azimuth) -
                          (int32_t)(rawdata_buf->data_blocks[0].azimuth)) *
                         AzimuthGranularity_;
    if (azimuth_gap < 0) {
        azimuth_gap += 2 * M_PI;
    }

    // Calculate laser firing timestamp of the first laser beam
    int64_t t_recv_us = (rawdata->timestamp_receive_ns) / UsToNs_;
    int64_t t_packet_us = (int64_t)(rawdata_buf->timestamp);
    int64_t t_fire_us =
            HourToUs_ * ((t_recv_us - t_packet_us + HalfHourToUs_) / HourToUs_) + t_packet_us;
    if (t_fire_us - t_recv_us > MaxDelayToleranceUs_) {
        // Synchronization lost
        return nullptr;
    }
    data->timestamp_intrinsic_ns = t_fire_us * UsToNs_;

    // Fulfill Lidar Points
    for (size_t data_block_index = 0; data_block_index < NumDataBlockPerPacket_;
         ++data_block_index) {
        const auto* data_block = &rawdata_buf->data_blocks[data_block_index];

        // ROS axes definitions is used for Lidar's xyz
        // in which X = forward, Y = left, Z = up, right-handed
        // regardless of velodyne's original axes definitions
        // refer to web: https://www.ros.org/reps/rep-0103.html#axis-orientation
        // also, web: https://blog.csdn.net/chengde6896383/article/details/86682882
        // consequently, a +90deg (M_PI/2) shiftation is added to azimuth
        double azimuth_base = AzimuthGranularity_ * (data_block->azimuth) + M_PI / 2;

        for (size_t channel_index = 0; channel_index < NumChannelPerDataBlock_; ++channel_index) {
            const auto* channel = &data_block->channels[channel_index];

            // No reflective if distance is zero
            if (channel->distance == 0) {
                continue;
            }
            lidar_point->intensity = channel->reflectivity;

            switch (rawdata_buf->lidar_type) {
            case LidarType::VelodyneVLP16C: {
                double azimuth =
                        azimuth_base + azimuth_gap * GetRelativeAzimuthChange16C(channel_index);
                double distance = channel->distance * DistanceGranularity16C_;

                double pitch = VerticalAngles16C_[channel_index % 16];
                double distance_horizontal = distance * cos(pitch);
                lidar_point->x = distance_horizontal * sin(azimuth);
                lidar_point->y = distance_horizontal * cos(azimuth);
                lidar_point->z = distance * sin(pitch) + VerticalCorrection16C_[channel_index];
                lidar_point->channel = channel_index % 16;
            } break;

            case LidarType::VelodyneVLP32C: {
                double azimuth = azimuth_base + AzimuthOffset32C_[channel_index] +
                                 azimuth_gap * GetRelativeAzimuthChange32C(channel_index);
                double distance = channel->distance * DistanceGranularity32C_;

                double pitch = VerticalAngles32C_[channel_index];
                double distance_horizontal = distance * cos(pitch);
                lidar_point->x = distance_horizontal * sin(azimuth);
                lidar_point->y = distance_horizontal * cos(azimuth);
                lidar_point->z = distance * sin(pitch);
                lidar_point->channel = channel_index;
            } break;

            case LidarType::VelodyneHDL32E: {
                double azimuth =
                        azimuth_base + azimuth_gap * GetRelativeAzimuthChange32E(channel_index);
                double distance = channel->distance * DistanceGranularity32E_;

                double pitch = VerticalAngles32E_[channel_index];
                double distance_horizontal = distance * cos(pitch);
                lidar_point->x = distance_horizontal * sin(azimuth);
                lidar_point->y = distance_horizontal * cos(azimuth);
                lidar_point->z = distance * sin(pitch);
                lidar_point->channel = channel_index;
            } break;

            default:
                return nullptr;
                break;
            }  // switch lidar type

            data_lidar->point_number++;
            lidar_point++;
        }  // for channel
    }      // for data_block

    return std::move(data);
}
TronErrno Lidar::do_adjust_parameter(DeviceParameterType type, const std::string& value)
{
    switch (type) {
    case DeviceParameterType::IpAddress:
        address_ = boost::asio::ip::address_v4::from_string(value);
        break;
    default:
        return TronErrno::UnimplementedParameter;
    }
    return TronErrno::Success;
}

}  // namespace tron
}  // namespace wayz