///
/// @file s32vcan_port.hpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief Header of Class CANPort
/// @date 2020-04-20
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include <atomic>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <memory>
#include <mutex>
#include <string>

namespace wayz {
namespace hera {
namespace device {
namespace driver {

#pragma pack(push, 1)

class CANPacket;

using CANPacketPtr = std::shared_ptr<CANPacket>;

class CANPacket {
public:
    static inline CANPacketPtr create(const uint32_t id, const uint16_t dlc)
    {
        auto* data = reinterpret_cast<CANPacket*>(new uint8_t[dlc + sizeof(CANPacket)]);
        bzero(data, dlc + sizeof(CANPacket));
        data->id = id;
        data->dlc = dlc;
        return CANPacketPtr(data, [](void* ptr) { delete[](uint8_t*)(ptr); });
    }

public:
    timespec timestamp;  ///< Timestamp of when packet received
    uint32_t id;         ///< CAN ID
    uint16_t dlc;        ///< CAN DLC (Length)
    uint8_t data[0];     ///< CAN Data
};

#pragma pack(pop)

///
/// @brief A driver class for CAN Port
///
class CANPort {
public:
    ///
    /// @brief Construct a new CANPort object
    ///
    /// @param data_port index of port in config file
    ///
    CANPort(const uint32_t data_port);

    CANPort(const CANPort&) = delete;
    CANPort& operator=(const CANPort&) = delete;

    ///
    /// @brief Destroy the CANPort object
    ///
    ~CANPort();

    bool inline is_open() const noexcept
    {
        return port_opened_;
    }

public:
    ///
    /// @brief Receive a CAN Packet from CAN Bus
    ///
    /// @return CANPacketPtr a shared ptr to received data if received, otherwise, nullptr
    ///
    CANPacketPtr read();

    ///
    /// @brief Send a CAN Packet to CAN Bus
    ///
    /// @param data a shared ptr to CAN data
    /// @return true Packet sent successfully
    /// @return false failed
    ///
    bool write(const CANPacketPtr& data);

private:
    ///
    /// @brief mutex for constructor for calling global init
    ///
    static std::mutex mutex_;

    ///
    /// @brief reference count for calling global init and global deinit
    ///
    static int32_t reference_count_;

    ///
    /// @brief whether port is opened successfully
    ///
    bool port_opened_;

    ///
    /// @brief data port of can bus
    ///
    uint32_t port_;
};


}  // namespace driver
}  // namespace device
}  // namespace hera
}  // namespace wayz