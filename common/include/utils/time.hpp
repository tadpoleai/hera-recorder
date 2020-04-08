//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#pragma once

#include <cstdint>
#include <ctime>
#include <iostream>

namespace wayz {
namespace hera {
namespace time {

class Duration;
class Timestamp;

class Duration final {
    friend std::ostream& operator<<(std::ostream& os, const Duration& duration);

public:
    constexpr Duration(int64_t duration_ns) : duration_ns_(duration_ns) {}
    constexpr operator int64_t() const noexcept
    {
        return duration_ns_;
    }

    std::string to_str_second() const;

private:
    int64_t duration_ns_;
};

constexpr Duration OneSecond = 1'000'000'000LL;
constexpr Duration OneMinute = OneSecond * 60;
constexpr Duration OneHour = OneMinute * 60;
constexpr Duration OneDay = OneHour * 24;

class Timestamp final : public timespec {
    friend std::ostream& operator<<(std::ostream& os, const Timestamp& ts);

public:
    static Timestamp now();

    Timestamp() = default;
    Timestamp(uint64_t ts_ns);
    Timestamp(const Timestamp&) = default;
    Timestamp& operator=(const Timestamp&) = default;
    Timestamp(Timestamp&&) = default;

    operator uint64_t() const noexcept;
    Duration operator-(const Timestamp& rhs) const noexcept;

    std::string to_datetime() const;
};

}  // namespace time
}  // namespace hera
}  // namespace wayz
