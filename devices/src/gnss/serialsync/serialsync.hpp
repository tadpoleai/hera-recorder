///
/// @file aceinna.hpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Header of class Serialsync and class SerialsyncStorageData, etc.
/// @version 0.1
/// @date 2019-11-25
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include "../../device.hpp"
#include "../../utils/serial_port_sentence.hpp"
#include "../../utils/serial_transport.hpp"
#include "../gnss_data.hpp"

namespace wayz {
namespace hera {
namespace gnss {

#pragma pack(push, 1)

///
/// @brief Storage data for Serialsync's Serial Data containing NMEA Sentence, etc.
///
class SerialsyncNmeaStorageData final : public StorageData {
public:
    SerialsyncNmeaStorageData() = delete;

public:
    ///
    /// @brief Data structure of NMEA Sentence packet
    ///
    struct NmeaRawData {
        ///
        /// @brief timestamp of message, UTC, in ns
        ///
        /// Obtained by calculating from NMEA Sentence and time shiftation
        ///
        uint64_t timestamp_intrinsic_ns;
        uint8_t timestamp_valid;        ///< boolean, indicating if timestamp_intrinsic_ns is valid
        uint32_t nmea_sentence_length;  ///< length of nmea_sentence, in bytes
        uint8_t nmea_sentence[];        /// NMEA Sentence, (usually) without trailing <LF>
    };

    ///
    /// @brief Union to allow access by data or by bytes
    ///
    union {
        NmeaRawData data;                  ///< union entry: data with structure
        uint8_t buf[sizeof(NmeaRawData)];  ///< union entry: raw buffer of bytes
    };
};

#pragma pack(pop)

///
/// @brief A GNSS-Devic outputs by serial port
///
/// A high-performanced (often, RTK) GNSS-Device that outputs NMEA Sentences through serial port
/// connected directly to PC, without a method to synchronize its timestamp since the device does
/// not provide a PPS pin.
///
/// To solve this issue, A low-performanced, auxiliary GNSS-Device connected to "Wayz Tron Sync
/// Board", in which, NMEA Sentences outputed by auxiliary GNSS-Device is parsed and the reference
/// time-shiftation between auxiliary GNSS-Device and "Sync Board" is calculated by MCU on "Sync
/// Board", and then send by serial_transport with msgtype 1.
///
/// By parsing the NMEA Sentences of primary GNSS-Device and add the time-shiftation, the accurate
/// timestamp of primary GNSS-Device is obtained
///
/// @see <a href="https://www.gpsinformation.org/dale/nmea.htm" target="_blank"
/// rel="noopener noreferrer">NMEA data</a>
class Serialsync final : public Device {
public:
    ///
    /// @brief Construct a new Serialsync object
    ///
    /// @note pass Kernel and BaudRate and KernelAuxiliary and BaudRateAuxiliary and
    /// SerialMsgType as essential parameters
    /// @see Device::Device()
    Serialsync(DeviceIdType id,
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
               {DeviceParameterType::Kernel,
                DeviceParameterType::KernelAuxiliary,
                DeviceParameterType::BaudRate,
                DeviceParameterType::BaudRateAuxiliary,
                DeviceParameterType::SerialMsgType}),
        serial_port_(nullptr),
        serial_port_auxiliary_(nullptr),
        queue_(nullptr),
        queue_auxiliary_(nullptr)
    {}
    Serialsync(const Serialsync&) = delete;
    Serialsync& operator=(const Serialsync&) = delete;

    ///
    /// @brief Destroy the Serialsync object
    ///
    /// calls Device::stop()
    virtual ~Serialsync()
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

    std::string kernel_;            ///< kernel name of primary serial
    std::string kernel_auxiliary_;  ///< kernel name of auxiliary serial
    int32_t baud_rate_;             ///< baud rate of primary serial
    int32_t baud_rate_auxiliary_;   ///< baud rate of auxiliary serial

    /// @brief serial msg type of message for auxiliary serial
    ///
    /// For time shiftation data, usually 1
    /// @see SerialTransport
    int32_t serial_msg_type_;

    SerialPortSentence* serial_port_;           ///< for receiving nmea data
    SerialTransport* serial_port_auxiliary_;    ///< for receiving time shiftation data
    ThreadQueue<SerialData>* queue_;            ///< queue of nmea data
    ThreadQueue<SerialData>* queue_auxiliary_;  ///< queue of serial time shiftation data

    ///
    /// @brief The time shiftation value, in ns
    ///
    /// Timestamp shiftation of "Tron Sync Board"'s instrinsic time
    /// (which is referenced by all-other sensors),
    /// in reference to GNSS's originial time (e.g., GPST).
    ///
    /// Here, we assume that the originial time of the primary GNSS
    /// and the auxiliary GNSS are the same.
    ///
    /// Therefore, to obtain the primary GNSS's time in reference to
    /// "Tron Sync Board"'s instrinsic time, we should add this
    /// shifation_value_ns_ to primary GNSS's originial time
    int64_t shifation_value_ns_;

    ///
    /// @brief The timestamp when shifation_value_ns_ received
    ///
    /// This value is should be compared with Timestamp::now()
    /// before calculation of primary GNSS's time, to ensure
    /// we are using a fresh value of shifation_value_ns_
    Timestamp shiftation_timestamp_;

    ///
    /// @brief The tolerance of shiftation_timestamp_ and now();
    ///
    /// @see shiftation_timestamp_
    static constexpr Duration ShifationDelayTolerance_ = 10 * OneSecond;
};

}  // namespace gnss
}  // namespace hera
}  // namespace wayz
