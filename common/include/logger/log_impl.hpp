//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#pragma once

#include <atomic>
#include <exception>
#include <execinfo.h>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <set>
#include <signal.h>
#include <sstream>
#include <thread>

#include "log_level.hpp"
#include "log_string.hpp"

namespace wayz {
namespace hera {
namespace log {
namespace impl {

class Logger;

class Logger {
public:
    static void onlyprint();
    static void ignore_signal(int signo);
    static void flush();
    static bool init(const std::string& file);
    static void set_level(LogLevel level);
    static void stop();
    static bool open_aux(const std::string& aux_file);
    static bool open_aux(std::vector<LogString>* aux_vector);
    static void close_aux();
    static LogStringStream create_string(LogLevel level);
    static std::string format(const LogString& data);

private:
    static std::unique_ptr<Logger> instance_;
    static std::set<int> signal_to_ignore_user_;

private:
    Logger();

public:
    ~Logger();

private:
    void write_thread_function();
    void write(LogString&& data);


    void register_back_trace();
    static void singal_handler(int signo);
    static void back_trace(int signo);

private:
    bool inited_;

    std::ofstream file_;
    std::vector<LogString>* aux_vector_;

    LogLevel level_;
    LogQueue queue_;

    std::atomic<bool> running_;
    std::atomic<bool> flushed_;
    std::thread thread_;
};

template<class T>
LogStringStream operator<<(const LogStartl startl, T&& rhs)
{
    auto result = Logger::create_string(startl.level);
    result << rhs;
    return result;
}

}  // namespace impl
}  // namespace log
}  // namespace hera
}  // namespace wayz