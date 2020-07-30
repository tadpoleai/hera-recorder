///
/// @file novatelspan.hpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief Header of class Novatel SPAN and its Data
/// @date 2020-06-04
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///
///

#pragma once

#include <cmath>

#include "data/ins_data.hpp"
#include "device.hpp"

#ifdef WITH_DRIVER
#include "driver/serial/serial_port_binary.hpp"
#include "driver/serial/serial_transport.hpp"
#endif

namespace wayz {
namespace hera {
namespace device {
namespace ins {
namespace novatelspan {

#pragma pack(push, 1)

struct BESTPOS {
    data::SolutionStatus solution_status;
    data::PositionVelocityType position_type;
    double latitude;
    double longitude;
    double height;
    float undulation;
    uint32_t datum_id;
    float latitude_deviation;
    float longitude_deviation;
    float height_deviation;
    char base_station_id[4];
    float differential_age_sec;
    float solution_age_sec;
    uint8_t satellites_tracked_number;
    uint8_t satellites_used_number;
    uint8_t satellites_l1_number;
    uint8_t satellites_multi_number;
    uint8_t reserved;
    uint8_t extended_solution_status;
    uint8_t galileo_and_beidou_signal_mask;
    uint8_t gps_and_glonass_signal_mask;
};

struct INSPOS {
    uint32_t week;
    double seconds_into_week;
    double latitude;
    double longitude;
    double height;
    data::InertialSolutionStatus ins_status;
};

struct INSPOSX {
    data::InertialSolutionStatus ins_status;
    data::PositionVelocityType position_type;
    double latitude;
    double longitude;
    double height;
    float undulation;
    float latitude_deviation;
    float longitude_deviation;
    float height_deviation;
    uint32_t extended_solution_status;
    uint16_t time_since_update;
};

struct CORRIMUDATA {
    uint32_t gnss_week;
    double gnss_ms;
    double rotational_speed_sample[3];
    double acceleration_sample[3];
};

enum class MessageIdType : uint16_t { BESTPOS = 42u, INSPOS = 265u, INSPOSX = 1459u, CORRIMUDATA = 812u };

///
/// @brief Data for recording NovatelSpan
///
class NovatelSpanBinaryData final : public data::DeviceData {
public:
    NovatelSpanBinaryData() = delete;

public:
    ///
    /// @brief @see NovatelSpan
    ///
    uint64_t shiftation_timestamp;
    uint64_t shifation_value_ns;

    ///
    /// @brief Data structure of one packet of Novatel
    ///
    struct NovatelBinaryStruct {
        /// Header
        uint8_t sync[3];
        uint8_t header_length;
        MessageIdType message_id;
        uint8_t message_type;
        uint8_t port_address;
        uint16_t message_length;
        uint16_t sequence;
        uint8_t idle_time;
        uint8_t time_status;
        uint16_t gnss_week;
        uint32_t gnss_ms;
        uint32_t receiver_status;
        uint16_t reserved;
        uint16_t receiver_sw_version;

        // Payload
        uint8_t payload[0];
    };

    ///
    /// @brief Union to allow access by data or by bytes
    ///
    union {
        NovatelBinaryStruct novatel_header;                ///< union entry: data with structure
        uint8_t novatel_buf[sizeof(NovatelBinaryStruct)];  ///< union entry: raw buffer of bytes
    };
};

#pragma pack(pop)

///
/// @brief Novatel SPAN Series, connected by usb and send binary data
///
/// @note Set up SPAN by windows client
/// And log the follows
/// > log com2 gpgga ontime 1
/// > log com1 bestposb ontime 0.05
/// > log com1 corrimudatab ontime 0.01
///
/// @note Same as 'gnss/serialsync'
/// An auxiliary GNSS-Device connected to "Wayz Tron Sync
/// Board", in which, NMEA Sentences outputed by auxiliary GNSS-Device is parsed and the reference
/// time-shiftation between auxiliary GNSS-Device and "Sync Board" is calculated by MCU on "Sync
/// Board", and then send by serial_transport with msgtype 1.
///
class NovatelSpan final : public Device {
public:
    ///
    /// @brief Construct a new NovatelSpan object
    ///
    /// @see Device::Device()
    NovatelSpan(const uint32_t id,
                const std::string& vendor_type,
                const std::string& name,
                const bool forward,
                ipc::IPCQueue<data::SensorData>* const ipc_queue,
                storage::StorageManager* const storage) :
        Device(id, vendor_type, name, forward, ipc_queue, storage, HistoryDepth_, EssentialParameterTypes)
    {
#ifdef WITH_DRIVER
        serial_port_ = nullptr;
#endif
    }
    NovatelSpan(const NovatelSpan&) = delete;
    NovatelSpan& operator=(const NovatelSpan&) = delete;

    ///
    /// @brief Destroy the NovatelSpan object
    ///
    /// calls Device::stop()
    virtual ~NovatelSpan()
    {
        stop();
    }

#ifdef WITH_DRIVER
    virtual HeraErrno connect() override;

    virtual void disconnect() override;

    virtual data::DeviceDataPtr fetch() override;

    // virtual HeraErrno adjust_parameter(const std::string& type, const std::string& value) override;
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
    static const std::vector<std::string> EssentialParameterTypes;  ///< Essential Parameters for device

    static const std::vector<std::string> OptionalParameterTypes;  ///< Optional Parameters for device

private:
    static constexpr size_t HistoryDepth_ = 10;  ///< History Depth, 50

private:
#ifdef WITH_DRIVER
    std::string kernel_;            ///< kernel name of primary serial
    std::string kernel_auxiliary_;  ///< kernel name of auxiliary serial
    int32_t baud_rate_;             ///< baud rate of primary serial
    int32_t baud_rate_auxiliary_;   ///< baud rate of auxiliary serial

    /// @brief serial msg type of message for auxiliary serial
    ///
    /// For time shiftation data, usually 1
    /// @see SerialTransport
    int32_t serial_msg_type_;

    driver::SerialPortBinary* serial_port_;                     ///< for receiving nmea data
    driver::SerialTransport* serial_port_auxiliary_;            ///< for receiving time shiftation data
    common::ThreadQueue<driver::SerialData>* queue_;            ///< queue of nmea data
    common::ThreadQueue<driver::SerialData>* queue_auxiliary_;  ///< queue of serial time shiftation data

    ///
    /// @brief The time shiftation value, in ns
    ///
    /// time::Timestamp shiftation of "Tron Sync Board"'s instrinsic time
    /// (which is referenced by all-other sensors),
    /// in reference to GNSS's originial time (e.g., GPST).
    ///
    /// Here, we assume that the originial time of the Novatel
    /// and the auxiliary GNSS are the same.
    ///
    /// Therefore, to obtain the Novatel's time in reference to
    /// "Tron Sync Board"'s instrinsic time, we should add this
    /// shifation_value_ns_ to Novatel's originial time
    int64_t shifation_value_ns_;

    ///
    /// @brief The timestamp when shifation_value_ns_ received
    ///
    /// This value is should be compared with Timestamp::now()
    /// before calculation of Novatel's time, to ensure
    /// we are using a fresh value of shifation_value_ns_
    time::Timestamp shiftation_timestamp_;
#endif
};

}  // namespace novatelspan
}  // namespace ins
}  // namespace device
}  // namespace hera
}  // namespace wayz