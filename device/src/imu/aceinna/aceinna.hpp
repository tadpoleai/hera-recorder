///
/// @file aceinna.hpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Header of class Aceinna and class AceinnaDeviceData
/// @version 0.1
/// @date 2019-11-07
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include <cmath>

#include "../../utils/serial_transport.hpp"
#include "data/imu_data.hpp"
#include "device.hpp"
#include "device_factory.hpp"

namespace wayz {
namespace hera {
namespace device {
namespace imu {
namespace aceinna {

#pragma pack(push, 1)

///
/// @brief Device data for Aceinna 9-axis Imu, Derived from Storage Data
///
class AceinnaData final : public data::DeviceData {
public:
    AceinnaData() = delete;

public:
    ///
    /// @brief Data structure of one packet of Aceinna
    ///
    /// The packet is sent by Wayz Tron Sync Board's serial output, on SerialMsgType  0
    ///
    struct AceinnaDataUnion {
        uint64_t timestamp;   ///< timestamp of 'DataReady' pin's falling edge, UTC, in ns
        int16_t gyro[3];      ///< array of Gyro raw data
        int16_t accel[3];     ///< array of Accel raw data
        int16_t magnetic[3];  ///< array of Magnetic raw data
    };

    ///
    /// @brief Union to allow access by data or by bytes
    ///
    union {
        AceinnaDataUnion data;                  ///< union entry: data with structure
        uint8_t buf[sizeof(AceinnaDataUnion)];  ///< union entry: raw buffer of bytes
    };
};

#pragma pack(pop)

///
/// @brief Aceinna 9-axis Imu, Derived from Device
///
class Aceinna final : public Device {
public:
    ///
    /// @brief Construct a new Velodyne object
    ///
    /// @note pass Kernel and BaudRate and SerialMsgType as essential parameters
    /// @see Device::Device()
    Aceinna(const uint32_t id,
            const std::string& vendor_type,
            const std::string& name,
            const bool forward,
            ipc::IPCQueue<data::SensorData>* const ipc_queue,
            storage::StorageManager* const storage) :
        Device(id, vendor_type, name, forward, ipc_queue, storage, HistoryDepth_, EssentialParameterTypes)
    {}
    Aceinna(const Aceinna&) = delete;
    Aceinna& operator=(const Aceinna&) = delete;

    static DevicePtr create(const uint32_t id,
                            const std::string& vendor_type,
                            const std::string& name,
                            const bool forward,
                            ipc::IPCQueue<data::SensorData>* const ipc_queue,
                            storage::StorageManager* const storage)
    {
        return std::make_unique<Aceinna>(id, vendor_type, name, forward, ipc_queue, storage);
    }

    ///
    /// @brief Destroy the Aceinna object
    ///
    /// calls Device::stop()
    virtual ~Aceinna()
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
    static constexpr size_t HistoryDepth_ = 1;  ///< History Depth, 1

    ///
    /// @brief Gravity scale constant, in m/s^2, defined by Aceinna
    ///
    static constexpr double Gravity_ = 9.80655;

    ///
    /// @brief Gyro Channel Granularity of Aceinna, in m/s^2
    ///
    /// @note The granularity may change if Wayz Tron Sync Board send other parameters
    static constexpr double GyroGranularity_ = M_PI / 200.0 / 180.0;

    ///
    /// @brief Accel Channel Granularity of Aceinna, in rad/s
    ///
    /// @note The granularity may change if Wayz Tron Sync Board send other parameters
    static constexpr double AccelGranularity_ = Gravity_ / 4000.0;

    ///
    /// @brief Magnetic Channel Granularity of Aceinna, in Tesla
    ///
    /// @note The granularity may change if Wayz Tron Sync Board send other parameters
    /// @todo Check the unit in Manual to confirm it is in Tesla
    static constexpr double MagneticGranularity_ = 1.0 / 16000.0;

private:
    std::string kernel_;  ///< kernel name (aka device name)

    ///
    /// @brief baud rate of serial port
    ///
    /// @see SerialPort
    int32_t baud_rate_;

    /// @brief serial msg type of message
    ///
    /// For imu data, usually 0
    /// @see SerialTransport
    int32_t serial_msg_type_;

    utils::SerialTransport* serial_port_;            ///< pointer to SerialTransport object, for receiving data
    common::ThreadQueue<utils::SerialData>* queue_;  ///< queue of serial data
};

}  // namespace aceinna
}  // namespace imu
}  // namespace device
}  // namespace hera
}  // namespace wayz