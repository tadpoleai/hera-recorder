///
/// @file xsens_defs.hpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Constant definitions of Xsens IMU MTData2
/// @version 0.1
/// @date 2021-09-03
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include <cmath>
#include <cstdint>
#include <string>

#include "common/include/utils/time.hpp"

namespace wayz {
namespace hera {
namespace device {
namespace imu {
///
/// @ref https://www.xsens.com/hubfs/Downloads/Manuals/MT_Low-Level_Documentation.pdf
///
namespace xsens {

#pragma pack(push, 1)

enum class MessageId : uint8_t {
    // State
    WakeUp = 0x3E,              ///< to host
    GoToConfig = 0x30,          ///< to MT
    GoToConfigAck = 0x31,       ///< to host
    GoToMeasurement = 0x10,     ///< to MT
    GoToMeasurementAck = 0x11,  ///< to host
    Reset = 0x40,               ///< to MT

    // // Informational
    // ReqDID = 0x00,    ///< to MT, Request to send the device identifier (serial number)
    // DeviceID = 0x01,  ///< to host, Acknowledge of ReqDID message, with data
    // ReqProductCode = 0x1C,
    // ProductCode = 0x1D,
    // ReqHardwareVersion = 0x1E,
    // HardwareVersion = 0x1F,
    // ReqFWRev = 0x12,
    // FirmwareRev = 0x13,
    // RunSelftest = 0x24,
    // SelftestResults = 0x25,

    Error = 0x42,
    Warning = 0x43,

    // // Device Message
    // SetOrReqBaudrate = 0x18,
    // SerOrReqOptionFlags = 0x48,
    // SetOrReqLocationID = 0x84,
    // RestoreFactoryDef = 0x0E,
    // SetOrReqTransmitDelay = 0xDC,

    // // Sync
    // SetOrReqSyncSettings = 0x2C,

    // // Config
    // ReqConfiguration = 0x0C,
    // Configuration = 0x0D,
    // SetOrReqOutputConfiguration = 0xC0,
    // SetOrReqStringOutputType = 0x8E,
    // SetOrReqPeriod = 0x04,

    ///...
    MTData2 = 0x36,

    SetFilterProfile = 0x64,
    SetOutputConfiguration = 0xC0,
};

enum class ErrorCode : uint8_t {
    PeriodOOR = 0x03,       ///< Period sent is not within valid range
    MessageInvalid = 0x04,  ///<  Message sent is invalid
    TimerOverflow = 0x1E,   ///< Timer overflow, this can be caused to high output frequency or sending too
                            ///< much data to MT during measurement
    BaudrateOOR = 0x20,     ///< Baud rate sent is not within valid range
    ParameterOOR = 0x21,    ///<  Parameter sent is invalid or not within range
    DeviceError = 0x28      ///< Device Error – try updating the firmware; extra device error contains 5 bytes
};

constexpr uint8_t Preamble = 0xFA;

struct MessageHeader {
    uint8_t preamble;      // Indicator of start of packet (0xFA)
    uint8_t bus_id;        // Bus Id or Address (0xFF)
    MessageId message_id;  // Message Id
    uint8_t length;
    uint8_t data[0];
};

#pragma pack(pop)

namespace mtdata2 {

#pragma pack(push, 1)

enum class DataIdBigEndian : uint16_t {
    PacketCounter = 0x2010,
    SampleTimeFine = 0x6010,
    Temperature = 0x1008,
    BaroPressure = 0x1030,
    Quaternion = 0x1020,
    Acceleration = 0x2040,
    RateOfTurn = 0x2080,
    MagneticField = 0x20C0,
    StatusWord = 0x20E0,
};

enum class Format : uint16_t {
    Float32 = 0x000,
    Fp1220 = 0x100,
    Fp1632 = 0x200,
    Float64 = 0x300,
};

struct MTData2 {
    DataIdBigEndian data_id;
    uint8_t data_len;
};

struct PacketCounter : public MTData2 {
    uint16_t packet_counter_bigendian;
};

struct SampleTimeFine : public MTData2 {
    uint32_t tick_10kHz_bigendian;
};

struct Temperature : public MTData2 {
    float temp_degree_celsius_bigendian;
};

struct BaroPressure : public MTData2 {
    float baro_pressure_pascal_bigendian;
};

struct Quaternion : public MTData2 {
    float quaternion_wxyz_bigendian[4];
};

struct Acceleration : public MTData2 {
    float acceleration_xyz_bigendian[3];
};

struct RateOfTurn : public MTData2 {
    float gyr_xyz_rad_bigendian[3];
};

struct MagneticField : public MTData2 {
    float magnetic_field_xyz_bigendian[3];
};

struct StatusWord : public MTData2 {
    struct BitsOfFields {
        static constexpr size_t SyncInMarker = 21;
    };

    uint32_t status_word_bigendian;
};

#pragma pack(pop)

}  // namespace mtdata2

}  // namespace xsens
}  // namespace imu
}  // namespace device
}  // namespace hera
}  // namespace wayz