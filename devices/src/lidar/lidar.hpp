//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#pragma once
#include "../device.hpp"
#include <boost/asio.hpp>

namespace wayz {
namespace tron {

static const int kDataBufferSize = 1500;
static const int kPositionBufferSize = 800;
const std::vector<double> kLut16 = {-15.0, 1.0, -13.0, 3.0, -11.0, 5.0,
                                    -9.0,  7.0, -7.0,  9.0, -5.0,  11.0,
                                    -3.0,  13.0,-1.0,  15.0};
const std::vector<double> kLut32 = {-30.67, -9.3299999, -29.33, -8.0, -28,   -6.6700001,
                                    -26.67, -5.3299999, -25.33, -4.0, -24.0, -2.6700001,
                                    -22.67, -1.33,      -21.33, 0.0,  -20.0, 1.33,
                                    -18.67, 2.6700001,  -17.33, 4.0,  -16,   5.3299999,
                                    -14.67, 6.6700001,  -13.33, 8.0,  -12.0, 9.3299999,
                                    -10.67, 10.67};
const double kTimeBetweenFirings = 1.152;
const double kTimeHalfIdle = 0.0;
const double kTimeTotalCycle = 46.080;
const std::vector<double> kLut32Alpha = {-25,    -1,     -1.667, -15.639, -11.31, 0,       -0.667,
                                         -8.843, -7.254, 0.333,  -0.333,  -6.148, -5.333,  1.333,
                                         0.667,  -4,     -4.667, 1.667,   1,      -3.3667, -3.333,
                                         3.333,  2.333,  -2.667, -3,      7,      4.667,   -2.333,
                                         -2,     15,     10.333, -1.333};
const std::vector<double> kLut32Rita = {1.4, -4.2, 1.4, -1.4, 1.4, -1.4, 4.2, -1.4,
                                        1.4, -4.2, 1.4, -1.4, 4.2, -1.4, 4.2, -1.4,
                                        1.4, -4.2, 1.4, -4.2, 4.2, -1.4, 1.4, -1.4,
                                        1.4, -1.4, 1.4, -4.2, 4.2, -1.4, 1.4, -1.4};

static const int kUnusedBytes1 = 198;
static const int kUnusedBytes2 = 3;
static const int kNmeaBytes = 306;
static const int kLaserPerFiring = 32;
static const int kFiringPerPKT = 12;

#pragma pack(push, 1)
struct LaserReturn {
    uint16_t distance;
    uint8_t intensity;
};

struct FiringData {
    uint16_t block_identifier;
    uint16_t rotational_position;
    LaserReturn laser_returns[kLaserPerFiring];
};

struct LidarRawData {
    FiringData firingData[kFiringPerPKT];
    uint32_t timestamp;
    uint8_t mode;
    uint8_t sensor_type;
};

struct PositionLidar {
    uint8_t unused_bytes_1[kUnusedBytes1];
    uint32_t timestamp;
    uint8_t pps_status;
    uint8_t unused_bytes_2[kUnusedBytes2];
    uint8_t nmea_gprmc[kNmeaBytes];
};
#pragma pack(pop)



class Lidar final : public Device {
public:
    Lidar(int32_t id, const std::string& name);
    Lidar(const Lidar&) = delete;
    Lidar& operator=(const Lidar&) = delete;
    virtual ~Lidar();

    DeviceType get_type() const final;

    TronErrno connect() final;
    void disconnect() final;
    std::shared_ptr<DeviceRawData> fetch() final;
    std::shared_ptr<SensorData> convert(const std::shared_ptr<DeviceRawData>& rawdata) final;
    TronErrno do_adjust_parameter(DeviceParameterType type, const std::string& value) final;

    // Convert should be implemented as static function
    static std::shared_ptr<SensorData> do_convert(const std::shared_ptr<DeviceRawData>& rawdata);

    // Disconnect should be implemented as non-virtual function
    void do_disconnect();
    
private:
    boost::asio::io_service io_service_;
    boost::asio::ip::udp::socket* data_socket_;
   // boost::asio::ip::udp::socket* telemetry_socket_;
    boost::asio::ip::address address_;
    boost::asio::ip::udp::endpoint receive_data_endpoint_;
    boost::asio::ip::udp::endpoint receive_position_endpoint_;
    unsigned short data_port_;
    
    char receive_buffer_[kDataBufferSize];
};

}  // namespace tron
}  // namespace wayz