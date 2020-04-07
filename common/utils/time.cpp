//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//
#include "time.hpp"

#include <ctime>

namespace wayz {
namespace hera {
namespace time {

static constexpr int64_t OneSecondToNs = 1'000'000'000;

Timestamp Timestamp::now()
{
    auto result = Timestamp();
    clock_gettime(CLOCK_REALTIME, (timespec*)&result);
    return result;
}

Timestamp::Timestamp(uint64_t ts_ns)
{
    tv_sec = ts_ns / OneSecondToNs;
    tv_nsec = ts_ns % OneSecondToNs;
}

Timestamp::operator uint64_t() const noexcept
{
    return OneSecondToNs * (uint64_t)(tv_sec) + (uint64_t)(tv_nsec);
}

Duration Timestamp::operator-(const Timestamp& rhs) const noexcept
{
    return Duration(int64_t(*this) - int64_t(rhs));
}

std::string Timestamp::to_datetime() const
{
    std::tm* timeinfo;
    char buffer[80];
    timeinfo = std::localtime(&tv_sec);
    std::strftime(buffer, 80, "%Y%m%d%H%M%S", timeinfo);
    return std::string(buffer);
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
    int64_t sec = duration_ns_ / OneSecondToNs;
    if (sec < 0) {
        sec = -sec;
        result += '-';
    }
    if (sec / 86400ULL) {
        result += std::to_string(sec / 86400ULL) + "d ";
        sec %= 86400ULL;
    }
    if (sec / 3600) {
        result += std::to_string(sec / 3600) + "h ";
        sec %= 3600;
    }
    if (sec / 60) {
        result += std::to_string(sec / 60) + "m ";
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
    } else if (ns < OneSecond) {
        os << ns / 1000000.0 << "ms";
    } else if (ns < OneMinute) {
        os << ns / double(OneSecond) << "s";
    } else if (ns < OneHour) {
        os << ns / int64_t(OneMinute) << "m " << ns % int64_t(OneMinute) / double(OneSecond) << "s";
    } else if (ns < OneDay) {
        os << ns / int64_t(OneHour) << "h " << (ns % int64_t(OneHour)) / int64_t(OneMinute) << "m "
           << (ns % int64_t(OneMinute)) / double(OneSecond) << "s";
    } else {
        os << ns / int64_t(OneDay) << "d " << ns % int64_t(OneHour) / int64_t(OneHour) << "h "
           << (ns % int64_t(OneHour)) / int64_t(OneMinute) << "m " << (ns % int64_t(OneMinute)) / double(OneSecond)
           << "s";
    }

    return os;
}

}  // namespace time
}  // namespace hera
}  // namespace wayz