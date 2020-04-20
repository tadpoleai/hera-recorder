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
#ifdef DEVICE_DRIVER_S32VCAN
#include "driver/can/can_port.hpp"
#endif
#endif

namespace wayz {
namespace hera {
namespace device {
namespace odometry {
namespace s32vgeely {

#pragma pack(push, 1)

///
/// @brief Device data for S32VGeely Series Car, Derived from Storage Data
///
class S32VGeelyData final : public data::DeviceData {
public:
    S32VGeelyData() = delete;

public:
    struct S32VGeelyCANPacket {
        uint64_t timestamp_ns;  ///< Timestamp given by can driver
        uint32_t id_can;        ///< CAN ID (Arbitration Id) of the message sender.
        uint16_t dlc_can;       ///< CAN DLC, number of bytes of the payload.
        uint8_t data_can[0];    ///< CAN Packet payload.
    };

    ///
    /// @brief Union to allow access by data or by bytes
    ///
    union {
        S32VGeelyCANPacket data;                  ///< union entry: data with structure
        uint8_t buf[sizeof(S32VGeelyCANPacket)];  ///< union entry: raw buffer of bytes
    };
};

#pragma pack(pop)

///
/// @brief For S32VGeely Series Car, Derived from Device
///
class S32VGeely final : public Device {
public:
    ///
    /// @brief Construct a new S32VGeely object
    ///
    /// @note pass Kernel and BaudRate and SerialMsgType as essential parameters
    /// @see Device::Device()
    S32VGeely(const uint32_t id,
              const std::string& vendor_type,
              const std::string& name,
              const bool forward,
              ipc::IPCQueue<data::SensorData>* const ipc_queue,
              storage::StorageManager* const storage) :
        Device(id, vendor_type, name, forward, ipc_queue, storage, HistoryDepth_, EssentialParameterTypes),
        thread_feedback_(nullptr)
    {}
    S32VGeely(const S32VGeely&) = delete;
    S32VGeely& operator=(const S32VGeely&) = delete;

    static DevicePtr create(const uint32_t id,
                            const std::string& vendor_type,
                            const std::string& name,
                            const bool forward,
                            ipc::IPCQueue<data::SensorData>* const ipc_queue,
                            storage::StorageManager* const storage)
    {
        return std::make_unique<S32VGeely>(id, vendor_type, name, forward, ipc_queue, storage);
    }

    ///
    /// @brief Destroy the S32VGeely object
    ///
    /// calls Device::stop()
    virtual ~S32VGeely()
    {
        stop();
    }

#ifdef WITH_DRIVER
#ifdef DEVICE_DRIVER_S32VCAN
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
    static constexpr size_t HistoryDepth_ = 1;            ///< History Depth, 1
    static constexpr int8_t MaxCanPacketReceiveNum_ = 1;  ///< Max num of receiving CAN packet, 1

private:
    std::thread* thread_feedback_;
    void feedback_thread_function();
    std::unique_ptr<ipc::IPCQueue<data::SensorData>> ipc_feedback_;

#ifdef WITH_DRIVER
#ifdef DEVICE_DRIVER_S32VCAN
    driver::CANPort* can_port_;
#endif
#endif
};

}  // namespace s32vgeely
}  // namespace odometry
}  // namespace device
}  // namespace hera
}  // namespace wayz