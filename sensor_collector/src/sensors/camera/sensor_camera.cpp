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
        return setError(TronErrno::CanNotOpenEthernetSensor);
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
        return setError(TronErrno::CanNotOpenEthernetSensor);
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

 
    memcpy(dataLidarBuf->points,lidar_data.points,lidar_data.point_number * sizeof(LaserPoint));
    return std::shared_ptr<SensorData>(data);
    
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