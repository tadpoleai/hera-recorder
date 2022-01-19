//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#pragma once

#include <atomic>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

#include <common/include/utils/thread_queue.hpp>

#include "serial_port.hpp"

namespace wayz {
namespace hera {
namespace device {
namespace driver {

using SerialData = std::vector<uint8_t>;

///
/// @brief Config of serial data protocol
///
struct SerialPortBinaryConfig {
    std::string lead_bytes;

    std::string tail_bytes;

    ///
    /// @brief Checksum algorithm to use
    ///
    enum class ChecksumProtocol {
        NONE,            ///< 0 Bytes, Not use checksum
        XOR8,            ///< 1 Bytes, XOR of data bytes
        CRC32_ISO_3309,  ///< 4 Bytes, Polynomial = 0x04C11DB7, 0xEDB88320(R)
        CRC16_MODBUS,    ///< 2 Bytes
    };
    ChecksumProtocol checksum_protocol;

    ///
    /// @brief The range for checksum calculating
    ///
    enum class ChecksumRange {
        DATA_ONLY,           ///< Calculate checksum only by data
        LEAD_BYTES_AND_DATA  ///< includes lead bytes and data
    };
    ChecksumRange checksum_range;

    size_t length_offset{0};  ///< offset of data length (8bit) w.r.t. start of lead bytes, 0 for non

    size_t length_range_substract{
            0};  ///< By default the data length counted from lead_bytes(included) to tail_bytes(not included), for
                 ///< other appls, set this variable to substract the length
};

///
/// @brief Receive serial data in binary
///
/// This class is for openning serial and receive framed data in binary format. Usually, sensors/devices sending data in
/// binary format, uses a framed transportation in the following protocol:
///
/// one frame = "<LeadBytes> <?> <DataLength(8bit)> <?> <Data>... <Checksum> <TailBytes>", in which
/// <LeadBytes> is a fixed-length predefined byte sequence, marking the start of frame
/// <Data> is variable-length data
/// <DataLength(8bit)> is data length (optional), indicating data length from <LeadBytes> to <TailBytes>
/// <Checksum> is a fixed-length checksum of data (optional)
/// <TailBytes> is a fixed-length predefined byte sequence, marking the end of frame (optional)
///
/// SerialPortBinary will detect <LeadBytes> and then detect <TailBytes>, if <TailBytes> is specified,
/// else, it will detect next <LeadBytes> as the end of last frame
///
class SerialPortBinary {
public:
    ///
    /// @brief Construct a new Serial Port Binary Sentence object
    ///
    /// @param kernel kernel name (device name) of serial device
    /// @param serial_config serial configuration
    /// @param binary_config data protocol configuration
    SerialPortBinary(const std::string& kernel,
                     const SerialConfig& serial_config,
                     const SerialPortBinaryConfig& binary_config);

    ///
    /// @brief Construct a new Serial Port Binary Sentence object
    ///
    /// @param kernel kernel name (device name) of serial device
    /// @param binary_config data protocol configuration
    SerialPortBinary(const std::string& kernel, const SerialPortBinaryConfig& binary_config);

    SerialPortBinary(const SerialPortBinary&) = delete;
    SerialPortBinary& operator=(const SerialPortBinary&) = delete;
    ~SerialPortBinary();

    common::ThreadQueue<SerialData>* get_queue_handler() const;
    bool is_opened() const;

    size_t write_port(const std::vector<uint8_t>& data);
    size_t write_port(const std::string& data);

private:
    void fetch_thread_function();

    SerialPort* port_;
    bool port_opened_;
    std::atomic<bool> thread_run_;
    std::thread* thread_fetch_;

    SerialPortBinaryConfig binary_config_;
    static constexpr size_t BufferReverseSize_ = 256;
    std::string buffer_;

    common::ThreadQueue<SerialData> queue_;
};


}  // namespace driver
}  // namespace device
}  // namespace hera
}  // namespace wayz
