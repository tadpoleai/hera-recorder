///
/// @file aceinna.hpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Header of class Aceinna and class AceinnaStorageData
/// @version 0.1
/// @date 2019-11-07
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include <cmath>

#include "../../device.hpp"
#include "../../utils/serial_transport.hpp"
#include "../imu_data.hpp"

namespace wayz {
namespace hera {
namespace imu {

#pragma pack(push, 1)

///
/// @brief Storage data for Aceinna 9-axis Imu, Derived from Storage Data
///
class AceinnaStorageData final : public StorageData {
public:
    AceinnaStorageData() = delete;

public:
    ///
    /// @brief Data structure of one packet of Aceinna
    ///
    /// The packet is sent by Wayz Tron Sync Board's serial output, on SerialMsgType  0
    ///
    struct AceinnaRawData {
        uint64_t timestamp;   ///< timestamp of 'DataReady' pin's falling edge, UTC, in ns
        int16_t gyro[3];      ///< array of Gyro raw data
        int16_t accel[3];     ///< array of Accel raw data
        int16_t magnetic[3];  ///< array of Magnetic raw data
    };

    ///
    /// @brief Union to allow access by data or by bytes
    ///
    union {
        AceinnaRawData data;                  ///< union entry: data with structure
        uint8_t buf[sizeof(AceinnaRawData)];  ///< union entry: raw buffer of bytes
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
    Aceinna(DeviceIdType id,
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
               {DeviceParameterType::Kernel, DeviceParameterType::BaudRate, DeviceParameterType::SerialMsgType})
    {}
    Aceinna(const Aceinna&) = delete;
    Aceinna& operator=(const Aceinna&) = delete;

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

    virtual StorageDataPtr fetch() override;

    virtual SensorDataPtr convert(StorageDataPtr& storage_data) override;

    virtual HeraErrno adjust_parameter(DeviceParameterType type, const std::string& value) override;

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

    SerialTransport* serial_port_;    ///< pointer to SerialTransport object, for receiving data
    ThreadQueue<SerialData>* queue_;  ///< queue of serial data
};

}  // namespace imu
}  // namespace hera
}  // namespace wayz