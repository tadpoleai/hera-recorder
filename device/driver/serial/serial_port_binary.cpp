//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include "serial_port_binary.hpp"

#include <common/include/logger/logger.hpp>

#include "driver/checksum/crc.hpp"
#include "string.h"

namespace wayz {
namespace hera {
namespace device {
namespace driver {

SerialPortBinary::SerialPortBinary(const std::string& kernel,
                                   const SerialConfig& serial_config,
                                   const SerialPortBinaryConfig& binary_config) :
    port_(nullptr),
    port_opened_(false),
    thread_run_(false),
    thread_fetch_(nullptr),
    binary_config_(binary_config),
    buffer_inited_(false)
{
    if (binary_config_.lead_bytes.empty()) {
        log::error << "SerialPortBinary: Can not open since LeadBytes is empty";
        return;
    }

    port_ = new SerialPort();
    port_opened_ = port_->open_port(kernel, serial_config);

    if (port_opened_) {
        buffer_.reserve(BufferReverseSize_);
        thread_run_ = true;
        thread_fetch_ = new std::thread(&SerialPortBinary::fetch_thread_function, this);
    }
}

SerialPortBinary::SerialPortBinary(const std::string& kernel, const SerialPortBinaryConfig& binary_config) :
    port_(nullptr),
    port_opened_(false),
    thread_run_(false),
    thread_fetch_(nullptr),
    binary_config_(binary_config),
    buffer_inited_(false)
{
    if (binary_config_.lead_bytes.empty()) {
        log::error << "SerialPortBinary: Can not open since LeadBytes is empty";
        return;
    }

    port_ = new SerialPort();
    port_opened_ = port_->open_port(kernel);

    if (port_opened_) {
        buffer_.reserve(BufferReverseSize_);
        thread_run_ = true;
        thread_fetch_ = new std::thread(&SerialPortBinary::fetch_thread_function, this);
    }
}

SerialPortBinary::~SerialPortBinary()
{
    thread_run_ = false;
    if (thread_fetch_ != nullptr) {
        thread_fetch_->join();
    }
    if (port_ != nullptr) {
        port_->close_port();
        delete port_;
    }
}

bool SerialPortBinary::is_opened() const
{
    return port_opened_;
}

common::ThreadQueue<SerialData>* SerialPortBinary::get_queue_handler() const
{
    return (common::ThreadQueue<SerialData>*)(&queue_);
}

size_t SerialPortBinary::write_port(const std::vector<uint8_t>& data)
{
    return port_->write_port(data);
}

size_t SerialPortBinary::write_port(const std::string& data)
{
    return port_->write_port(data);
}

void SerialPortBinary::fetch_thread_function()
{
    buffer_.clear();
    buffer_.reserve(BufferReverseSize_);

    while (thread_run_) {
        // Read new chars from port
        const auto new_chars = port_->read_port();
        if (!new_chars.size()) {
            continue;
        }

        auto buffer_sz = buffer_.size();
        buffer_.resize(buffer_sz + new_chars.size());
        memcpy((char*)buffer_.data() + buffer_sz, new_chars.data(), new_chars.size());

        std::size_t start = 0;
        std::size_t erase_pointer = 0;
        while (true) {
            // Find lead bytes
            start = buffer_.find(binary_config_.lead_bytes, start);
            if (start == std::string::npos) {
                break;
            }

            // Find tail bytes
            decltype(start) end;
            if (binary_config_.tail_bytes.empty()) {
                end = buffer_.find(binary_config_.lead_bytes, start + 1);
            } else {
                end = buffer_.find(binary_config_.tail_bytes, start + 1);
            }
            if (end == std::string::npos) {
                break;
            }

            // Range of this message
            auto msg_start = start;
            auto msg_end = end;

            // Local Next start
            start = end + binary_config_.tail_bytes.size();

            // Buffer to cstr
            const char* c_buffer = buffer_.c_str();

            // Checksum
            bool checksum_verified = false;
            switch (binary_config_.checksum_protocol) {
            case SerialPortBinaryConfig::ChecksumProtocol::NONE:
                checksum_verified = true;
                break;
            case SerialPortBinaryConfig::ChecksumProtocol::CRC32_ISO_3309: {
                if (msg_end - msg_start < 4) {
                    break;
                }

                auto crc32_start = msg_start;
                if (binary_config_.checksum_range == SerialPortBinaryConfig::ChecksumRange::DATA_ONLY) {
                    crc32_start += binary_config_.lead_bytes.size();
                }
                auto crc32_end = msg_end - 4;
                auto crc32_length = crc32_end - crc32_start;

                uint32_t received_crc32 = 0;
                memcpy(&received_crc32, (unsigned char*)c_buffer + crc32_end, 4);

                uint32_t calculated_crc32 =
                        driver::CalculateBlockCRC32(crc32_length, (unsigned char*)c_buffer + crc32_start);
                checksum_verified = (calculated_crc32 == received_crc32);
            } break;
            case SerialPortBinaryConfig::ChecksumProtocol::CRC16_MODBUS: {
                if (msg_end - msg_start < 2) {
                    break;
                }

                auto crc16_start = msg_start;
                if (binary_config_.checksum_range == SerialPortBinaryConfig::ChecksumRange::DATA_ONLY) {
                    crc16_start += binary_config_.lead_bytes.size();
                }
                auto crc16_end = msg_end - 2;
                auto crc16_length = crc16_end - crc16_start;

                uint32_t received_crc16 = 0;
                memcpy(&received_crc16, (unsigned char*)c_buffer + crc16_end, 2);

                uint32_t calculated_crc16 =
                        driver::CalculateBlockCRC16(crc16_length, (unsigned char*)c_buffer + crc16_start);
                checksum_verified = (calculated_crc16 == received_crc16);
            } break;

            default:
                break;
            }

            if (checksum_verified) {
                erase_pointer = msg_end;
                auto message = std::make_shared<SerialData>();
                message->resize(msg_end - msg_start);
                memcpy(message->data(), (unsigned char*)c_buffer + msg_start, msg_end - msg_start);
                queue_.emplace(std::move(message));
            } else {
                // log::warn << "SerialPortBinary: Checksum mismatched data" << log::endl;
            }
        }

        // Advance buffer
        if (erase_pointer != 0) {
            buffer_.erase(0, erase_pointer);
        }
    }
}

}  // namespace driver
}  // namespace device
}  // namespace hera
}  // namespace wayz
