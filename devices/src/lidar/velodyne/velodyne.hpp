///
/// @file lidar_velodyne.hpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Header of class Velodyne
/// @version 0.1
/// @date 2019-11-06
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once
#include <cmath>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "velodyne_data.hpp"
#include "velodyne_defs.hpp"

namespace wayz {
namespace hera {
namespace lidar {

///
/// @brief Velodyne Lidar, Derived from Device
///
class Velodyne final : public Device {
public:
    ///
    /// @brief Construct a new Velodyne object
    ///
    /// @note pass IpAddress and DataPort as essential parameters
    /// @see Device::Device()
    Velodyne(DeviceIdType id,
             const std::string& type,
             const std::string& name,
             const std::string& folder,
             bool read_mode) :
        Device(id,
               type,
               name,
               folder,
               read_mode,
               HistoryDepth_,
               {DeviceParameterType::IpAddress, DeviceParameterType::DataPort})
    {}
    Velodyne(const Velodyne&) = delete;
    Velodyne& operator=(const Velodyne&) = delete;

    ///
    /// @brief Destroy the Velodyne object
    ///
    /// calls Device::stop()
    virtual ~Velodyne()
    {
        stop();
    }

    virtual HeraErrno connect() override;

    virtual void disconnect() override;

    virtual StorageDataPtr fetch() override;

    virtual SensorDataPtr convert(StorageDataPtr& storage_data) override;

    virtual HeraErrno adjust_parameter(DeviceParameterType type, const std::string& value) override;

private:
    static constexpr size_t HistoryDepth_ = 160;  ///< History Depth, ~+1500packets, 600rpm, 160 = 1 circle

    static constexpr int64_t UsToNs_ = 1000ULL;                           ///< Multiplier from ns to us
    static constexpr int64_t SecondToUs_ = 1'000'000ULL;                  ///< Multiplier from second to us
    static constexpr int64_t HourToUs_ = 3600ULL * SecondToUs_;           ///< Multiplier from hour to us
    static constexpr int64_t HalfHourToUs_ = 1800ULL * SecondToUs_;       ///< Multiplier from half an hour to us
    static constexpr int64_t MaxDelayToleranceUs_ = 30ULL * SecondToUs_;  ///< Max valid transmission delay, in us

private:
    static constexpr size_t EthernetMTU_ = 1500;  ///< MTU/PacketSize of ethernet interface
    static const timeval TimeOut_;                ///< Timeout for UDP recvform

    ::sockaddr_in addr_in_;                 ///< Ip Address and Data Port of Lidar
    int socket_;                            ///< Socket handler
    uint8_t receive_buffer_[EthernetMTU_];  ///< UDP Receive buffer
};

}  // namespace lidar
}  // namespace hera
}  // namespace wayz