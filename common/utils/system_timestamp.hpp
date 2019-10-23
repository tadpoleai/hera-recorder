//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#pragma once

#include <cstdint>
#include <ctime>
#include <iostream>

namespace wayz {
namespace tron {

class Timestamp;

class Duration final {
public:
    Duration(int64_t duration_ns);
    operator int64_t() const;
    static const Duration OneSecond;

private:
    int64_t duration_ns_;
};

class Timestamp final : public timespec {
    friend std::ostream& operator<<(std::ostream& os, const Timestamp& ts);

public:
    static Timestamp now();
    Timestamp(int64_t ts_ns);
    void set(int64_t ts_ns);
    operator int64_t() const;

private:
    Timestamp() = default;
};

}  // namespace tron
}  // namespace wayz
