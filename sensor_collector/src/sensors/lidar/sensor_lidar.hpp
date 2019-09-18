//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#ifndef __sensor_lidar_hpp__
#define __sensor_lidar_hpp__
#include "../sensor_base.hpp"

#include <boost/asio.hpp>

namespace wayz {

BETTER_ENUM(SensorLidarParameter, int32_t, rate = 0, value)

class SensorLidar final : public SensorBase {
public:
    SensorLidar(int32_t id, const std::string& name);
    SensorLidar(const SensorLidar&) = delete;
    SensorLidar& operator=(const SensorLidar&) = delete;
    ~SensorLidar();

    SensorType getType() const final;

    TronErrno doConnectSensor() final;
    void doDisconnectSensor() final;
    std::shared_ptr<SensorRawData> doFetchRawData() final;
    std::shared_ptr<SensorData> doConvertData(const std::shared_ptr<SensorRawData>& rawdata) final;
    TronErrno doAdjustParameter(SensorParameterType type, const std::string& value) final;

private:

    boost::asio::io_service io_service_;
    boost::asio::ip::udp::socket* data_socket_;
    boost::asio::ip::udp::socket* telemetry_socket_;
    boost::asio::ip::address address_;
    boost::asio::ip::udp::endpoint receive_data_endpoint_;
    boost::asio::ip::udp::endpoint receive_position_endpoint_;
    unsigned short data_port_;
    unsigned short telemetry_port_;
    
    char receive_buffer_[kDataBufferSize];
    char receive_postion_buffer_[kPositionBufferSize];
    
};

}  // namespace wayz
#endif