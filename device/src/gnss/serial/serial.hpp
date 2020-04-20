///
/// @file serial.hpp
/// @author chunchen.wang (chunchen.wang@wayz.ai)
/// @brief
/// @version 0.1
/// @date 2020-04-13
///
/// Copyright 2018 Wayz.ai. All Rights Reserved.
///
///

#pragma once

#include "data/gnss_data.hpp"
#include "device.hpp"
#include "device_factory.hpp"
#include "driver/serial/serial_port_sentence.hpp"

namespace wayz {
namespace hera {
namespace device {
namespace gnss {
namespace serial {

#pragma pack(push, 1)

///
/// @brief Device data for Serial's Serial Data containing NMEA Sentence, etc.
///
class SerialNmea final : public data::DeviceData {
public:
    SerialNmea() = delete;

public:
    ///
    /// @brief Data structure of NMEA Sentence packet
    ///
    struct SerialNmeaUnion {
        uint32_t nmea_sentence_length;  ///< length of nmea_sentence, in bytes
        uint8_t nmea_sentence[];        /// NMEA Sentence, (usually) without trailing <LF>
    };

    ///
    /// @brief Union to allow access by data or by bytes
    ///
    union {
        SerialNmeaUnion data;                  ///< union entry: data with structure
        uint8_t buf[sizeof(SerialNmeaUnion)];  ///< union entry: raw buffer of bytes
    };
};

#pragma pack(pop)

///
/// @brief A GNSS-Devic outputs by serial port
///
/// A high-performanced (often, RTK) GNSS-Device that outputs NMEA Sentences through serial port
/// connected directly to PC, without a method to synchronize its time::Timestamp since the device does
/// not provide a PPS pin.
///
/// @see <a href="https://www.gpsinformation.org/dale/nmea.htm" target="_blank"
/// rel="noopener noreferrer">NMEA data</a>
class Serial final : public Device {
public:
    ///
    /// @brief Construct a new Serial object
    ///
    /// @note pass Kernel and BaudRate as essential parameters
    /// @see Device::Device()
    Serial(const uint32_t id,
           const std::string& vendor_type,
           const std::string& name,
           const bool forward,
           ipc::IPCQueue<data::SensorData>* const ipc_queue,
           storage::StorageManager* const storage) :
        Device(id, vendor_type, name, forward, ipc_queue, storage, HistoryDepth_, EssentialParameterTypes)
#ifdef WITH_DRIVER
        ,
        serial_port_(nullptr),
        queue_(nullptr)
#endif
    {}
    Serial(const Serial&) = delete;
    Serial& operator=(const Serial&) = delete;

    static DevicePtr create(const uint32_t id,
                            const std::string& vendor_type,
                            const std::string& name,
                            const bool forward,
                            ipc::IPCQueue<data::SensorData>* const ipc_queue,
                            storage::StorageManager* const storage)
    {
        return std::make_unique<Serial>(id, vendor_type, name, forward, ipc_queue, storage);
    }

    ///
    /// @brief Destroy the Serial object
    ///
    /// calls Device::stop()
    virtual ~Serial()
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

#ifdef WITH_DRIVER
    std::string kernel_;  ///< kernel name of primary serial
    int32_t baud_rate_;   ///< baud rate of primary serial

    driver::SerialPortSentence* serial_port_;         ///< for receiving nmea data
    common::ThreadQueue<driver::SerialData>* queue_;  ///< queue of nmea data
#endif
};

}  // namespace serial
}  // namespace gnss
}  // namespace device
}  // namespace hera
}  // namespace wayz
