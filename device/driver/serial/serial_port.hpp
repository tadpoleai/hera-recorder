///
/// @file serial_port.hpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief Serial port
/// @date 2019-10-15
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include <cstdint>
#include <iostream>
#include <set>
#include <string>
#include <vector>

namespace wayz {
namespace hera {
namespace device {
namespace driver {

class SerialConfig {
public:
    enum class Parity { None, Even, Odd };

    SerialConfig(int32_t baud_rate = 9600,
                 int32_t data_bits = 8,
                 int32_t stop_bits = 1,
                 Parity parity = Parity::None,
                 bool flow_control = false,
                 bool low_latency_mode = false,
                 bool writable = false);

    int32_t baud_rate;
    int32_t data_bits;
    int32_t stop_bits;
    Parity parity;
    bool flow_control;
    bool low_latency_mode;
    bool writable;
};

class SerialPort {
public:
    SerialPort();
    SerialPort(const SerialPort&) = delete;
    SerialPort& operator=(const SerialPort&) = delete;
    ~SerialPort();

    ///
    /// @brief Open port without configuration
    ///
    /// @param kernel device name
    /// @return if operation is successful
    bool open_port(const std::string& kernel);

    ///
    /// @brief Open port with configuration
    ///
    /// @param kernel device name
    /// @param serial_config Serial Configuration
    /// @return if operation is successful
    bool open_port(const std::string& kernel, const SerialConfig& serial_config);

    ///
    /// @brief Close port
    ///
    void close_port();

    ///
    /// @brief Read data from serial
    ///
    /// @param max_size max buffer size to read
    /// @param timeout_ms read timeout in ms
    /// @return std::vector<uint8_t> data read
    std::vector<uint8_t> read_port(size_t max_size = 0, int32_t timeout_ms = 10);

    ///
    /// @brief Send data by serial
    ///
    /// @param data data to send
    /// @return size_t size of bytes sent
    size_t write_port(const std::vector<uint8_t>& data);

    ///
    /// @brief Send data by serial
    ///
    /// @param data data to send
    /// @return size_t size of bytes sent
    size_t write_port(const std::string& data);

    std::string error_reason() const noexcept;

private:
    bool die(std::string&& msg);
    bool set_low_latency_mode();

    static const std::set<int32_t> ValidBaudRates_;
    int fd_;
    bool writable_;

    std::string reason_;
};

}  // namespace driver
}  // namespace device
}  // namespace hera
}  // namespace wayz
