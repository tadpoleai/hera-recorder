//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//
#include "system_timestamp.hpp"

#include <ctime>

namespace wayz {
namespace tron {

static constexpr int64_t OneSecondToNs = (int64_t)(1000000000L);

Duration::Duration(int64_t duration_ns) : duration_ns_(duration_ns) {}
Duration::operator int64_t() const
{
    return duration_ns_;
}
const Duration Duration::OneSecond = Duration(OneSecondToNs);

Timestamp Timestamp::now()
{
    auto result = Timestamp();
    clock_gettime(CLOCK_REALTIME, (timespec*)&result);
    return result;
}
Timestamp::Timestamp(int64_t ts_ns)
{
    set(ts_ns);
}
void Timestamp::set(int64_t ts_ns)
{
    tv_sec = ts_ns / OneSecondToNs;
    tv_nsec = ts_ns % OneSecondToNs;
}
Timestamp::operator int64_t() const
{
    return OneSecondToNs * (int64_t)(tv_sec) + (int64_t)(tv_nsec);
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

}  // namespace tron
}  // namespace wayz