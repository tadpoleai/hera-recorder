//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include "sensor_lidar.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace wayz {

#define HOUR_TO_US ((uint64_t)(3600000000))
#define HALF_HOUR_TO_US ((uint64_t)(1800000000))

SensorLidar::SensorLidar(int32_t id, const std::string& name) : SensorBase(id, name) {}

SensorLidar::~SensorLidar() {}

SensorType SensorLidar::getType() const
{
    return SensorType::Lidar;
}

TronErrno SensorLidar::doConnectSensor()
{
    if (parameters_.count(SensorParameterType::IpAddress)) {
        address_ = boost::asio::ip::address_v4::from_string(
                parameters_[SensorParameterType::IpAddress]);
    } else {
        return setError(TronErrno::InsufficientParameters);
    }

    if (parameters_.count(SensorParameterType::DataPort)) {
        data_port_ = std::stoi(parameters_[SensorParameterType::DataPort]);
    } else {
        return setError(TronErrno::InsufficientParameters);
    }

    if (parameters_.count(SensorParameterType::TelemetryPort)) {
        telemetry_port_ = std::stoi(parameters_[SensorParameterType::TelemetryPort]);
    } else {
        return setError(TronErrno::InsufficientParameters);
    }
    try {
        data_socket_ = new boost::asio::ip::udp::socket(
                io_service_,
                boost::asio::ip::udp::endpoint(boost::asio::ip::address_v4::from_string(
                                                       "255.255.255.255"),
                                               data_port_));
    } catch (const std::exception& e) {
        data_socket_ = nullptr;
    }

    try {
        telemetry_socket_ = new boost::asio::ip::udp::socket(
                io_service_,
                boost::asio::ip::udp::endpoint(boost::asio::ip::address_v4::from_string(
                                                       "255.255.255.255"),
                                               telemetry_port_));
    } catch (const std::exception& e) {
        telemetry_socket_ = nullptr;
        if (data_socket_ && data_socket_->is_open()) {
            data_socket_->close();
            delete data_socket_;
            data_socket_ = nullptr;
        }
    }

    if (data_socket_ == nullptr || telemetry_socket_ == nullptr) {
        std::cout << "create socket failed!" << std::endl;
        return setError(TronErrno::InsufficientParameters);
    }

    return TronErrno::Success;
}

void SensorLidar::doDisconnectSensor() {
    if(data_socket_ && data_socket_->is_open()){
        data_socket_->close();
        delete data_socket_;
        data_socket_ = nullptr;
    }
    if(telemetry_socket_ && telemetry_socket_->is_open()){
        telemetry_socket_->close();
        delete telemetry_socket_;
        telemetry_socket_ = nullptr;
    }
    if(io_service_.stopped()){
        io_service_.stop();
        io_service_.reset();
    }
}

std::shared_ptr<SensorRawData> SensorLidar::doFetchRawData()
{

    // Some Sensors Blocks, Simulate that
    //  std::this_thread::sleep_for(std::chrono::milliseconds(periodMs_));

    // Get Rawdata from a Real Sensor
    // Get Length of Rawdata First
    if(receive_data_endpoint_.address() != address_ || receive_data_endpoint_.port() != data_port_){
        std::cout << "Error: Lidar receive_data_endpoint error!" << std::endl;
        return NULL;
    }
    int32_t receivedRawdataLength = sizeof(receive_buffer_);

    // Create a Buff to Store Rawdata
    int32_t totalLength = sizeof(SensorRawData) + receivedRawdataLength;
    SensorRawData* data = reinterpret_cast<SensorRawData*>(new uint8_t[totalLength]);

    // Fullfil Metadata (Header) of Rawdata;
    data->length = totalLength;
    data->sensorType = SensorType::Lidar;
    data->sensorDataType = SensorDataType::LidarVelodyneScan;
    data->sequence = sequence_++;
    data->timestampReceiveNs = getSystemTimestamp();

    memset(receive_buffer_, 0, kDataBufferSize);
    data_socket_->receive_from(boost::asio::buffer(receive_buffer_, sizeof(receive_buffer_)),
                               receive_data_endpoint_);

    telemetry_socket_->receive_from(boost::asio::buffer(receive_postion_buffer_,
                                                        sizeof(receive_postion_buffer_)),
                                    receive_position_endpoint_);

    try {
        io_service_.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return NULL;
    }
    // Use Memcpy to fill Buff
    memcpy(reinterpret_cast<char*>(data->rawdataBuf), receive_buffer_, sizeof(receive_buffer_));

    // Return a Shared Ptr
    return std::shared_ptr<SensorRawData>(data);
}

std::shared_ptr<SensorData> SensorLidar::doConvertData(
        const std::shared_ptr<SensorRawData>& rawdata)
{

    DataLidar lidar_data ;

    LidarRawData* packet = reinterpret_cast<LidarRawData*>(rawdata->rawdataBuf);

    int max_laser_number = 0;
    if (packet->sensor_type == 0x28 || packet->sensor_type == 0x21) {
        max_laser_number = 32;
    } else if (packet->sensor_type == 0x22) {
        max_laser_number = 16;
    } else {
        std::cout << "sensor type :" << packet->sensor_type << " not support!" << std::endl;
    }

    bool time_synced;
    if (max_laser_number > 0) {
        uint64_t msec = getSystemTimestamp();
        uint64_t lidartime =
                HOUR_TO_US * (uint64_t)((uint64_t)(msec - packet->timestamp + HALF_HOUR_TO_US) /
                                        HOUR_TO_US) +
                packet->timestamp;
        int64_t delay = msec - lidartime;
        if (delay < 0 || delay > 5000000) {
            time_synced = false;
            lidartime = msec - 2000;
        } else
            time_synced = true;

        double interpolated = 0.0;
        double const PI = acos(double(-1));

        if (packet->firingData[1].rotational_position < packet->firingData[0].rotational_position) {
            interpolated = ((packet->firingData[1].rotational_position + 36000) -
                            packet->firingData[0].rotational_position) /
                           2.0;
        } else {
            interpolated = (packet->firingData[1].rotational_position -
                            packet->firingData[0].rotational_position) /
                           2.0;
        }

        lidar_data.point_number = 0;
        for (int firing_index = 0; firing_index < kFiringPerPKT; firing_index++) {
            const FiringData firing_data = packet->firingData[firing_index];

            for (int laser_index = 0; laser_index < kLaserPerFiring; laser_index++) {
                double azimuth = static_cast<double>(firing_data.rotational_position);
                if (packet->sensor_type == 0x21) {
                    double laser_relative_time = kLaserPerFiring * kTimeBetweenFirings +
                                                 kTimeHalfIdle * (laser_index / max_laser_number);
                    azimuth += interpolated * laser_relative_time / kTimeTotalCycle;
                }
                if (laser_index >= max_laser_number) {
                    azimuth += interpolated;
                }
                if (azimuth >= 36000) {
                    azimuth -= 36000;
                }
                azimuth = azimuth / 100.0;
                if (packet->sensor_type == 0x21) {
                    if (firing_data.laser_returns[laser_index].distance < 0.001) {
                        continue;
                    }
                }
                double distance =
                        static_cast<double>(firing_data.laser_returns[laser_index].distance) * 4.0 /
                        1000.0;
                double vertical = 0.0;
                if (max_laser_number == 32) {
                    if (packet->sensor_type == 0x21) {
                        vertical = kLut32[laser_index];
                    } else if (packet->sensor_type == 0x28) {
                        vertical = kLut32Alpha[laser_index];
                        azimuth += kLut32Rita[laser_index];
                    }
                } else {
                    vertical = kLut16[laser_index % max_laser_number];
                }
                int32_t intensity =
                        static_cast<int32_t>(firing_data.laser_returns[laser_index].intensity);
                azimuth = azimuth * PI / 180.0;
                vertical = vertical * PI / 180.0;

                double x = distance * cos(vertical) * sin(azimuth);
                double y = distance * cos(vertical) * cos(azimuth);
                double z = distance * sin(vertical);

                LaserPoint laser_point;
                laser_point.x = x;
                laser_point.y = y;
                laser_point.z = z;
                laser_point.ring = laser_index;
                laser_point.intensity = intensity;
                if (laser_point.x == 0.0f && laser_point.y == 0.0f && laser_point.z == 0.0f) {
                    continue;
                }
                lidar_data.sensor_type = packet->sensor_type;
                lidar_data.points[lidar_data.point_number] = laser_point;
                lidar_data.point_number++;
            }
        }

            // Create a Buff to Store Data
        int32_t totalLength = sizeof(SensorData) + sizeof(lidar_data);
        SensorData* data = reinterpret_cast<SensorData*>(new uint8_t[totalLength]);
        DataLidar* dataLidarBuf = reinterpret_cast<DataLidar*>(data->dataBuf);

        // Fullfil Metadata (Header) of Data;
        data->length = totalLength;
        data->sensorType = rawdata->sensorType;
        data->sensorDataType = rawdata->sensorDataType;
        data->sequence = rawdata->sequence;
        data->timestampReceiveNs = rawdata->timestampReceiveNs;
        dataLidarBuf->point_number = lidar_data.point_number;
        dataLidarBuf->sensor_type = lidar_data.sensor_type;
        memcpy(dataLidarBuf->points,lidar_data.points,lidar_data.point_number * sizeof(LaserPoint));
        return std::shared_ptr<SensorData>(data);
    }
    return NULL;
}

TronErrno SensorLidar::doAdjustParameter(SensorParameterType type, const std::string& value)
{
    switch (type) {
    case SensorParameterType::IpAddress:
         address_ = boost::asio::ip::address_v4::from_string(value);
        break;
    default:
        return TronErrno::UnimplementedParameter;
    }
    return TronErrno::Success;
}


}  // namespace wayz