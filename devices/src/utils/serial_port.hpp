//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#pragma once
#include <cstdint>
#include <set>
#include <string>
#include <vector>
#include <iostream>

namespace wayz {
namespace hera {

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

    bool open_port(const std::string& kernel, const SerialConfig& serial_config);
    void close_port();
    std::vector<uint8_t> read_port(size_t max_size = 0, int32_t timeout_ms = 10);

private:
    bool die(const std::string& msg);
    bool set_low_latency_mode();

    static const std::set<int32_t> ValidBaudRates_;
    int fd_;
};

}  // namespace hera
}  // namespace wayz