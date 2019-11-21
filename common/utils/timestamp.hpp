//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#pragma once

#include <cstdint>
#include <ctime>
#include <iostream>

namespace wayz {
namespace hera {

class Duration;
class Timestamp;

class Duration final {
    friend std::ostream& operator<<(std::ostream& os, const Duration& duration);

public:
    Duration(int64_t duration_ns);
    operator int64_t() const;
    std::string to_str_second() const;
    static const Duration OneSecond;
    static const Duration OneMinute;

private:
    int64_t duration_ns_;
};

class Timestamp final : public timespec {
    friend std::ostream& operator<<(std::ostream& os, const Timestamp& ts);

public:
    static Timestamp now();

    Timestamp(uint64_t ts_ns);
    Timestamp(const Timestamp&) = default;
    Timestamp& operator=(const Timestamp&) = default;
    Timestamp(Timestamp&&) = default;

    void set(uint64_t ts_ns);
    operator uint64_t() const;
    Duration operator-(const Timestamp& rhs) const;

private:
    Timestamp() = default;
};

}  // namespace hera
}  // namespace wayz
