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
namespace device {
namespace lidar {
namespace velodyne {

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
    Velodyne(const uint32_t id,
             const std::string& vendor_type,
             const std::string& name,
             const bool forward,
             ipc::IPCQueue<data::SensorData>* const ipc_queue,
             storage::StorageManager* const storage) :
        Device(id, vendor_type, name, forward, ipc_queue, storage, HistoryDepth_, EssentialParameterTypes)
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

    virtual data::DeviceDataPtr fetch() override;

    virtual HeraErrno adjust_parameter(DeviceParameterType type, const std::string& value) override;

    virtual data::SensorDataPtr convert(data::DeviceDataPtr& storage_data) override
    {
        return do_convert(storage_data);
    }

    ///
    /// @brief Static convert function for read / convert / replay
    ///
    static data::SensorDataPtr do_convert(data::DeviceDataPtr& storage_data);

public:
    static const std::vector<DeviceParameterType> EssentialParameterTypes;  ///< Essential Parameters for device

    static const std::vector<DeviceParameterType> OptionalParameterTypes;  ///< Optional Parameters for device

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

}  // namespace velodyne
}  // namespace lidar
}  // namespace device
}  // namespace hera
}  // namespace wayz