///
/// @file geely.hpp
/// @author chunchen.wang (chunchen.wang@wayz.ai)
/// @brief
/// @version 0.1
/// @date 2020-04-08
///
/// Copyright 2018 Wayz.ai. All Rights Reserved.
///
///
#pragma once

#include <cmath>

#include "data/odometry_data.hpp"
#include "device.hpp"
#include "device_factory.hpp"


#ifdef WITH_DRIVER
#ifdef DEVICE_DRIVER_CAN
#include "../../utils/can/can_interface.h"
#endif
#endif


namespace wayz {
namespace hera {
namespace device {
namespace odometry {
namespace geely {

#pragma pack(push, 1)

///
/// @brief Device data for Geely Series Car, Derived from Storage Data
///
class GeelyData final : public data::DeviceData {
public:
    GeelyData() = delete;

public:
    struct S32VCANData {
        struct timeval timeStamp;
        uint32_t arbitrationId;  ///< CAN ID (Arbitration Id) of the message sender.
        uint16_t dlc;            ///< Number of bytes of the payload.
        uint8_t packet[64];      ///< Packet Payload.
    };
    ///
    /// @brief Union to allow access by data or by bytes
    ///
    union {
        S32VCANData data;                  ///< union entry: data with structure
        uint8_t buf[sizeof(S32VCANData)];  ///< union entry: raw buffer of bytes
    };
};

#pragma pack(pop)

///
/// @brief For Geely Series Car, Derived from Device
///
class Geely final : public Device {
public:
    ///
    /// @brief Construct a new Geely object
    ///
    /// @note pass Kernel and BaudRate and SerialMsgType as essential parameters
    /// @see Device::Device()
    Geely(const uint32_t id,
          const std::string& vendor_type,
          const std::string& name,
          const bool forward,
          ipc::IPCQueue<data::SensorData>* const ipc_queue,
          storage::StorageManager* const storage) :
        Device(id, vendor_type, name, forward, ipc_queue, storage, HistoryDepth_, EssentialParameterTypes)
    {}
    Geely(const Geely&) = delete;
    Geely& operator=(const Geely&) = delete;

    static DevicePtr create(const uint32_t id,
                            const std::string& vendor_type,
                            const std::string& name,
                            const bool forward,
                            ipc::IPCQueue<data::SensorData>* const ipc_queue,
                            storage::StorageManager* const storage)
    {
        return std::make_unique<Geely>(id, vendor_type, name, forward, ipc_queue, storage);
    }

    ///
    /// @brief Destroy the Geely object
    ///
    /// calls Device::stop()
    virtual ~Geely()
    {
        stop();
    }

#ifdef WITH_DRIVER
#ifdef DEVICE_DRIVER_CAN
    virtual HeraErrno connect() override;

    virtual void disconnect() override;

    virtual data::DeviceDataPtr fetch() override;

    virtual HeraErrno adjust_parameter(DeviceParameterType type, const std::string& value) override;
#endif
#endif

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
    static constexpr size_t HistoryDepth_ = 1;                      ///< History Depth, 1
    static constexpr int8_t MaxCanPacketReceiveNum_ = 1;            ///< Max num of receiving CAN packet
    static constexpr int8_t CanPacketInnerSize_ = 8;                ///< inner size of one CAN packet
    static constexpr int16_t CanPacketIDOfRearWheelSpeed_ = 0x123;  ///< CAN packet ID of front wheel speed
    static constexpr int16_t CanPacketIDOfAngular_ = 0xE0;          ///< CAN packet ID of front wheel speed

#ifdef WITH_DRIVER
#ifdef DEVICE_DRIVER_CAN
    struct SCANPacket can_packet_;
#endif
#endif
    int8_t data_port_;  ///< CAN data port
};

}  // namespace geely
}  // namespace odometry
}  // namespace device
}  // namespace hera
}  // namespace wayz