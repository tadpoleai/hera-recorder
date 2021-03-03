//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#pragma once

#include "log_impl.hpp"

namespace wayz {
namespace hera {
namespace log {

const auto debug = impl::LogStartl(LogLevel::Debug);
const auto info = impl::LogStartl(LogLevel::Info);
const auto warn = impl::LogStartl(LogLevel::Warn);
const auto error = impl::LogStartl(LogLevel::Error);
const auto endl = impl::LogEndl();

inline void onlyprint()
{
    return impl::Logger::onlyprint();
}

inline void clear_line()
{
    return impl::Logger::clear_line();
}

inline void ignore_signal(int signo)
{
    return impl::Logger::ignore_signal(signo);
}

inline void flush()
{
    return impl::Logger::flush();
}

inline bool init(const std::string& file)
{
    return impl::Logger::init(file);
}

inline void set_level(LogLevel level)
{
    return impl::Logger::set_level(level);
}

inline void set_sleep_before_exiting(bool value)
{
    impl::Logger::set_sleep_before_exiting(value);
}

inline bool open_aux(const std::string& aux_file)
{
    return impl::Logger::open_aux(aux_file);
}

inline bool open_aux(std::vector<impl::LogString>* aux_vector)
{
    return impl::Logger::open_aux(aux_vector);
}

inline void close_aux(std::vector<impl::LogString>* aux_vector)
{
    return impl::Logger::close_aux(aux_vector);
}

}  // namespace log
}  // namespace hera
}  // namespace wayz