//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//
#include "timestamp.hpp"

#include <ctime>

namespace wayz {
namespace hera {

static constexpr int64_t OneSecondToNs = 1'000'000'000;
const Duration Duration::OneSecond = Duration(OneSecondToNs);
const Duration Duration::OneMinute = Duration(60 * OneSecondToNs);

Duration::Duration(int64_t duration_ns) : duration_ns_(duration_ns) {}
Duration::operator int64_t() const
{
    return duration_ns_;
}

Timestamp Timestamp::now()
{
    auto result = Timestamp();
    clock_gettime(CLOCK_REALTIME, (timespec*)&result);
    return result;
}

Timestamp::Timestamp(uint64_t ts_ns)
{
    set(ts_ns);
}

void Timestamp::set(uint64_t ts_ns)
{
    tv_sec = ts_ns / OneSecondToNs;
    tv_nsec = ts_ns % OneSecondToNs;
}

Timestamp::operator uint64_t() const
{
    return OneSecondToNs * (uint64_t)(tv_sec) + (uint64_t)(tv_nsec);
}

Duration Timestamp::operator-(const Timestamp& rhs) const
{
    return Duration(int64_t(*this) - int64_t(rhs));
}

std::ostream& operator<<(std::ostream& os, const Timestamp& ts)
{
    std::tm* timeinfo;
    char buffer[80];
    timeinfo = std::localtime(&ts.tv_sec);
    std::strftime(buffer, 80, "%F %T ", timeinfo);
    os << buffer << timeinfo->tm_zone << " ";
    os.fill('0');
    os.width(9);
    os << ts.tv_sec;
    os << '.';
    os.fill('0');
    os.width(9);
    os << ts.tv_nsec;
    return os;
}

std::string Duration::to_str_second() const
{
    std::string result;
    int64_t sec = double(duration_ns_) / double(Duration::OneSecond);
    if (sec < 0) {
        sec = -sec;
        result += '-';
    }
    if (sec / 86400ULL) {
        result += std::to_string(sec / 86400ULL) + "d";
        sec %= 86400ULL;
    }
    if (sec / 3600) {
        result += std::to_string(sec / 3600) + "h";
        sec %= 3600;
    }
    if (sec / 60) {
        result += std::to_string(sec / 60) + "m";
        sec %= 60;
    }
    result += std::to_string(sec) + "s";
    return result;
}

std::ostream& operator<<(std::ostream& os, const Duration& duration)
{
    int64_t ns = int64_t(duration);
    if (ns < 0) {
        ns = -ns;
        os << '-';
    }
    if (ns < 1000LL) {
        os << ns << "ns";
    } else if (ns < 1000000LL) {
        os << ns / 1000.0 << "μs";
    } else if (ns < Duration::OneSecond) {
        os << ns / 1000000.0 << "ms";
    } else if (ns < Duration::OneMinute) {
        os << ns / double(Duration::OneSecond) << "sec";
    } else {
        os << ns / int64_t(Duration::OneMinute) << "min"
           << (ns % int64_t(Duration::OneMinute)) / double(Duration::OneSecond) << "sec";
    }
    return os;
}

}  // namespace hera
}  // namespace wayz