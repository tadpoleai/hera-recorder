//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#pragma once

#include <cstdint>
#include <string>

namespace wayz {
namespace hera {
namespace log {

class LogLevel {
public:
    enum ValueType : uint8_t { Reserved = 0x00u, Debug = 0x10u, Info = 0x11u, Warn = 0x12u, Error = 0x13u };

    LogLevel() = default;
    constexpr LogLevel(ValueType value) noexcept : value_(value) {}
    constexpr operator ValueType() const noexcept
    {
        return value_;
    }
    explicit operator bool() = delete;
    constexpr bool operator==(LogLevel rhs) const noexcept
    {
        return value_ == rhs.value_;
    }
    constexpr bool operator!=(LogLevel rhs) const noexcept
    {
        return value_ != rhs.value_;
    }
    constexpr bool operator>(LogLevel rhs) const noexcept
    {
        return value_ > rhs.value_;
    }
    constexpr bool operator>=(LogLevel rhs) const noexcept
    {
        return value_ >= rhs.value_;
    }
    constexpr bool operator<(LogLevel rhs) const noexcept
    {
        return value_ < rhs.value_;
    }
    constexpr bool operator<=(LogLevel rhs) const noexcept
    {
        return value_ <= rhs.value_;
    }

    std::string to_string() const
    {
        switch (value_) {
        case ValueType::Debug:
            return "DEBUG";
        case ValueType::Info:
            return "INFO ";
        case ValueType::Warn:
            return "WARN ";
        case ValueType::Error:
            return "ERROR";
        default:
            return "UNKWN";
        }
    }

    std::string to_color_prefix() const
    {
        switch (value_) {
        case ValueType::Debug:
            return "\e[33m";
        case ValueType::Info:
            return "\e[32m";
        case ValueType::Warn:
            return "\e[1;31m";
        case ValueType::Error:
            return "\e[1;41;37m";
        default:
            return "\e[0m";
        }
    }

    std::string to_color_suffix() const
    {
        return "\e[0m";
    }

private:
    ValueType value_;
};

}  // namespace log
}  // namespace hera
}  // namespace wayz