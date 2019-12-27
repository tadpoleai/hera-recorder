//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include "serial_port.hpp"

#include <fcntl.h>
#include <iostream>
#include <poll.h>
#include <termios.h>
#include <unistd.h>

#include <common/logger/logger.hpp>
#include <linux/serial.h>
#include <sys/ioctl.h>

namespace wayz {
namespace hera {
namespace device {
namespace utils {

SerialConfig::SerialConfig(int32_t baud_rate,
                           int32_t data_bits,
                           int32_t stop_bits,
                           Parity parity,
                           bool flow_control,
                           bool low_latency_mode,
                           bool writable) :
    baud_rate(baud_rate),
    data_bits(data_bits),
    stop_bits(stop_bits),
    parity(parity),
    flow_control(flow_control),
    low_latency_mode(low_latency_mode),
    writable(writable)
{}

const std::set<int32_t> SerialPort::ValidBaudRates_ =
        {50, 75, 110, 134, 150, 200, 300, 600, 1200, 1800, 2400, 9600, 19200, 38400, 57600, 115200, 230400};

SerialPort::SerialPort() : fd_(-1) {}
SerialPort::~SerialPort()
{
    close_port();
}

bool SerialPort::die(std::string&& msg)
{
    log::error << "SerialPort: " << msg << log::endl;
    reason_ = std::forward<std::string>(msg);
    return false;
}

bool SerialPort::open_port(const std::string& kernel, const SerialConfig& serial_config)
{
    close_port();

    if (ValidBaudRates_.count(serial_config.baud_rate) == 0) {
        return die("Invalid baud rate " + std::to_string(serial_config.baud_rate));
    }

    if (serial_config.stop_bits != 1 && serial_config.stop_bits != 2) {
        return die("Invalid stop bits " + std::to_string(serial_config.stop_bits));
    }

    if (serial_config.data_bits != 7 && serial_config.data_bits != 8) {
        return die("Invalid data bits " + std::to_string(serial_config.stop_bits));
    }

    fd_ = open(kernel.c_str(), serial_config.writable ? O_RDWR : O_RDONLY);
    if (fd_ == -1) {
        return die("Error opening: " + kernel);
    }

    struct termios term;
    if (tcgetattr(fd_, &term) < 0) {
        close_port();
        return die("Error setting attribuites");
    }

    cfmakeraw(&term);

    if (serial_config.stop_bits == 2) {
        term.c_cflag |= CSTOPB;
    } else {
        term.c_cflag &= ~CSTOPB;
    }

    switch (serial_config.parity) {
    case SerialConfig::Parity::Even:
        term.c_cflag |= PARENB;
        term.c_cflag &= ~PARODD;
        break;
    case SerialConfig::Parity::Odd:
        term.c_cflag &= ~PARENB;
        term.c_cflag |= PARODD;
        break;
    case SerialConfig::Parity::None:
        term.c_cflag &= ~PARENB;
        term.c_cflag &= ~PARODD;
        break;
    default:
        term.c_cflag &= ~PARENB;
        term.c_cflag &= ~PARODD;
        break;
    }

    if (serial_config.data_bits == 8) {
        term.c_cflag &= ~CSIZE;
        term.c_cflag |= CS8;
    } else {
        term.c_cflag &= ~CSIZE;
        term.c_cflag |= CS7;
    }

    if (cfsetspeed(&term, serial_config.baud_rate) < 0) {
        close_port();
        return die("Error setting baud rate: " + std::to_string(serial_config.baud_rate));
    }

    if (tcsetattr(fd_, TCSAFLUSH, &term) < 0) {
        close_port();
        return die("Error settting attribuites " + kernel);
    }

    if (serial_config.low_latency_mode && !set_low_latency_mode()) {
        close_port();
        return die("Error setting low latency mode");
    }

    return true;
}

void SerialPort::close_port()
{
    if (fd_ < 0) {
        return;
    }
    close(fd_);
    fd_ = -1;
}

bool SerialPort::set_low_latency_mode()
{
    struct serial_struct serial_info;
    if (ioctl(fd_, TIOCGSERIAL, &serial_info) < 0) {
        return false;
    }

    serial_info.flags |= ASYNC_LOW_LATENCY;
    if (ioctl(fd_, TIOCSSERIAL, &serial_info) < 0) {
        return false;
    }

    return true;
}

std::vector<uint8_t> SerialPort::read_port(size_t max_size, int32_t timeout_ms)
{
    if (fd_ < 0) {
        return std::vector<uint8_t>();
    }

    struct pollfd fds[1];
    fds[0].fd = fd_;
    fds[0].events = POLLIN;

    int poll_return = poll(fds, 1, timeout_ms);
    if (poll_return == 0) {
        // Timed out
        return std::vector<uint8_t>();
    } else if (poll_return < 0) {
        // Error
        return std::vector<uint8_t>();
    }

    size_t size_to_read = max_size;
    if (size_to_read <= 0) {
        int bytes;
        ioctl(fd_, FIONREAD, &bytes);
        if (bytes < 0) {
            // Error
            return std::vector<uint8_t>();
        }
        size_to_read = static_cast<size_t>(bytes);
    }

    std::vector<uint8_t> result;
    result.resize(size_to_read);

    int read_size = read(fd_, result.data(), size_to_read);
    result.resize(read_size);

    return result;
}

std::string SerialPort::error_reason() const noexcept
{
    return reason_;
}

}  // namespace utils
}  // namespace device
}  // namespace hera
}  // namespace wayz
