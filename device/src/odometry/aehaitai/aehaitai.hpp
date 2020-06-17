///
/// @file aehaitai.hpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief Absolute Encoder Haitai
/// @date 2020-06-16
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include <cmath>

#include "data/odometry_data.hpp"
#include "device.hpp"
#include "device_factory.hpp"

#ifdef WITH_DRIVER
#include "driver/serial/serial_port_binary.hpp"
#include "driver/serial/serial_transport.hpp"
#endif

namespace wayz {
namespace hera {
namespace device {
namespace odometry {
namespace aehaitai {

#pragma pack(push, 1)

enum class FrameType : uint8_t { ENCODER_ANGLE = 0x01u };

struct EncoderAngle {
    uint8_t angle_h8;
    uint8_t angle_l8;
    uint8_t direction;
};

///
/// @brief Device data for AEHaitaiData, Derived from Storage Data
///
class AEHaitaiData final : public data::DeviceData {
public:
    AEHaitaiData() = delete;

public:
    struct AEHaitaiDataUnion {
        uint8_t header;
        uint8_t address;
        FrameType frame_type;
        uint8_t frame_data[0];
    };

    ///
    /// @brief Union to allow access by data or by bytes
    ///
    union {
        AEHaitaiDataUnion data;                  ///< union entry: data with structure
        uint8_t buf[sizeof(AEHaitaiDataUnion)];  ///< union entry: raw buffer of bytes
    };
};

#pragma pack(pop)

///
/// @brief Absolute Encoder Haitai, Derived from Device
///
class AEHaitai final : public Device {
public:
    ///
    /// @brief Construct a new S32VGeely object
    ///
    /// @note pass Kernel and BaudRate and SerialMsgType as essential parameters
    /// @see Device::Device()
    AEHaitai(const uint32_t id,
             const std::string& vendor_type,
             const std::string& name,
             const bool forward,
             ipc::IPCQueue<data::SensorData>* const ipc_queue,
             storage::StorageManager* const storage) :
        Device(id, vendor_type, name, forward, ipc_queue, storage, HistoryDepth_, EssentialParameterTypes)
    {}
    AEHaitai(const AEHaitai&) = delete;
    AEHaitai& operator=(const AEHaitai&) = delete;

    static DevicePtr create(const uint32_t id,
                            const std::string& vendor_type,
                            const std::string& name,
                            const bool forward,
                            ipc::IPCQueue<data::SensorData>* const ipc_queue,
                            storage::StorageManager* const storage)
    {
        return std::make_unique<AEHaitai>(id, vendor_type, name, forward, ipc_queue, storage);
    }

    ///
    /// @brief Destroy the AEHaitai object
    ///
    /// calls Device::stop()
    virtual ~AEHaitai()
    {
        stop();
    }

#ifdef WITH_DRIVER
    virtual HeraErrno connect() override;

    virtual void disconnect() override;

    virtual data::DeviceDataPtr fetch() override;

    virtual HeraErrno adjust_parameter(DeviceParameterType type, const std::string& value) override;
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
    static constexpr size_t HistoryDepth_ = 1;  ///< History Depth, 1

private:
    std::thread* thread_ask_;
    void thread_ask_function();
    static constexpr uint32_t AskFrequency = 100;

#ifdef WITH_DRIVER
    std::string kernel_;  ///< kernel name of serial
    int32_t baud_rate_;   ///< baud rate of serial

    driver::SerialPortBinary* serial_port_;           ///< for receiving nmea data
    common::ThreadQueue<driver::SerialData>* queue_;  ///< queue of nmea data
#endif
};

}  // namespace aehaitai
}  // namespace odometry
}  // namespace device
}  // namespace hera
}  // namespace wayz