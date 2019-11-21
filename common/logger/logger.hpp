//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#pragma once

#include "impl/log_impl.hpp"

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

inline bool init(const std::string& file)
{
    return impl::Logger::init(file);
}

inline void set_level(LogLevel level)
{
    return impl::Logger::set_level(level);
}

inline bool open_aux(const std::string& file)
{
    return impl::Logger::open_aux(file);
}

inline void close_aux()
{
    return impl::Logger::close_aux();
}

}  // namespace log
}  // namespace hera
}  // namespace wayz