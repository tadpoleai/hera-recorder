//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#pragma once
#include <cmath>

#include <boost/asio.hpp>

#include "../device.hpp"

namespace wayz {
namespace tron {

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
    // For more details, refer to the following document
    // pdf: VLP-16 User Manual Note, section: 9.3, Packet Types and Definitions, page: 54 - 60
    static constexpr size_t NumChannelPerDataBlock_ = 32;
    static constexpr size_t NumDataBlockPerPacket_ = 12;
    static constexpr size_t NumLidarPoints_ = NumChannelPerDataBlock_ * NumDataBlockPerPacket_;

    enum class ReturnMode : uint8_t { Strongest = 0x37, LastReturn = 0x38, DualReturn = 0x39 };

    struct VelodyneChannelData {
        uint16_t distance;
        uint8_t reflectivity;
    } __attribute__((packed, aligned(1)));

    struct VelodyneDataBlock {
        uint16_t flag;
        uint16_t azimuth;
        VelodyneChannelData channels[NumChannelPerDataBlock_];
    } __attribute__((packed, aligned(1)));

    struct DeviceRawDataLidar {
        VelodyneDataBlock data_blocks[NumDataBlockPerPacket_];
        uint32_t timestamp;
        ReturnMode return_mode;
        LidarType lidar_type;
    } __attribute__((packed, aligned(1)));

private:
    static constexpr int64_t UsToNs_ = (int64_t)(1000);
    static constexpr int64_t SecondToUs_ = (int64_t)(1000000);
    static constexpr int64_t HourToUs_ = (int64_t)(3600) * SecondToUs_;
    static constexpr int64_t HalfHourToUs_ = (int64_t)(1800) * SecondToUs_;
    static constexpr int64_t MaxDelayToleranceUs_ = 30 * SecondToUs_;
    static constexpr double AzimuthGranularity_ = M_PI / 18000.0;

    // For more details, refer to the following document
    // pdf: VLP-16 User Manual, table: Vertical Angles by Laser ID and Model, page: 53 - 54
    // also, figure: 9-7, Single Return Mode Timing Offsets (in μs), page: 64
    // also, section: 9.3.1.3, Data Point, Page 55
    static double VerticalAngles16C_[16];
    static double VerticalCorrection16C_[16];
    static constexpr double DistanceGranularity16C_ = 0.002;
    static double GetRelativeAzimuthChange16C(size_t index)
    {
        if (index < 16) {
            return 2.304 / 110.592 * index;
        } else {
            return 55.296 + 2.304 / 110.592 * index;
        }
    }

    // For more details, refer to the following document
    // pdf: VLP-32C User Manual, table: VLP-32C Data Order in Data Block, page: 57 - 58
    // also, figure: 9-7, Single Return Mode Timing Offsets (in μs), page: 64
    // also, section: 9.3.1.3, Data Point, Page 54
    static double VerticalAngles32C_[32];
    static double AzimuthOffset32C_[32];
    static constexpr double DistanceGranularity32C_ = 0.004;
    static double GetRelativeAzimuthChange32C(size_t index)
    {
        return 2.304 / 55.296 * (index / 2);
    }

    // For more details, refer to the following document
    // pdf: HDL-32E User Manual, table: HDL-32E Laser Firing Order, page: 62 - 63
    // also, figure: 9-6, Single Return Mode Timing Offsets (in μs), page: 67
    // also, section: 9.3.1.3, Data Point, Page 59
    static double VerticalAngles32E_[32];
    static constexpr double DistanceGranularity32E_ = 0.002;
    static double GetRelativeAzimuthChange32E(size_t index)
    {
        return 1.152 / 46.080 * index;
    }

private:
    boost::asio::io_service io_service_;
    boost::asio::ip::udp::socket* data_socket_;
    boost::asio::ip::address address_;
    boost::asio::ip::udp::endpoint receive_data_endpoint_;
    unsigned short data_port_;

    static constexpr size_t kDataBufferSize = 1500;
    char receive_buffer_[kDataBufferSize];
};

}  // namespace tron
}  // namespace wayz