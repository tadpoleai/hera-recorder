//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#pragma once

#include <cstdint>
#include <memory>
#include <sstream>
#include <string>

#include "common/utils/thread_queue.hpp"
#include "common/utils/timestamp.hpp"
#include "log_level.hpp"

namespace wayz {
namespace hera {
namespace log {
namespace impl {

class LogEndl final {};

class LogStartl final {
public:
    LogStartl(LogLevel level) : level(level) {}

public:
    LogLevel level;
};

class LogString;
using LogQueue = ThreadQueue<LogString, false>;

class LogString final {
public:
    LogString(LogLevel level, const Timestamp& ts, std::string&& str) :
        level(level),
        ts(ts),
        str(std::move(str))
    {}
    LogLevel level;
    Timestamp ts;
    std::string str;
};

class LogStringStream final : public std::stringstream {
public:
    LogStringStream(LogQueue* queue, bool valid, LogLevel level, Timestamp&& ts) :
        std::stringstream(std::ios_base::out),
        valid_(valid),
        level_(level),
        ts_(std::forward<Timestamp>(ts)),
        queue_(queue)
    {}

    LogStringStream(LogStringStream&& rhs) noexcept :
        std::stringstream(std::ios_base::out),
        valid_(std::move(rhs.valid_)),
        level_(std::move(rhs.level_)),
        ts_(std::move(rhs.ts_)),
        queue_(std::move(rhs.queue_))
    {
        this->swap(rhs);
    }

    inline void operator<<(const LogEndl& endl)
    {
        if (valid_ && queue_ != nullptr) {
            queue_->push(std::make_unique<LogString>(level_, ts_, str()));
        }
    }

    template<class T>
    inline LogStringStream& operator<<(T&& rhs)
    {
        if (valid_ && queue_ != nullptr) {
            *(static_cast<std::stringstream*>(this)) << rhs;
            return *this;
        } else {
            return *this;
        }
    }

private:
    bool valid_;
    LogLevel level_;
    Timestamp ts_;
    LogQueue* queue_;
};

}  // namespace impl
}  // namespace log
}  // namespace hera
}  // namespace wayz