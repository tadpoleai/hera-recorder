//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#pragma once

#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>

namespace wayz {
namespace tron {

enum class LoggerLevel : int32_t {
    Debug = 0,
    Info,
    Warn,
    Error,
};

class Logger final {
public:
    static bool create(const std::string& log_folder);
    ~Logger();

public:
    Logger& operator<<(Logger& (*op)(Logger&))
    {
        return (*op)(*this);
    }
    template<typename T>
    Logger& operator<<(const T& t)
    {
        file_ << t;
        if (extra_file_.is_open()) {
            extra_file_ << t;
        }
        std::cout << t;
        return *this;
    }
    static inline Logger& debug()
    {
        return start(LoggerLevel::Debug);
    }
    static inline Logger& info()
    {
        return start(LoggerLevel::Info);
    }
    static inline Logger& warn()
    {
        return start(LoggerLevel::Warn);
    }
    static inline Logger& error()
    {
        return start(LoggerLevel::Error);
    }
    static inline Logger& endl(Logger& that)
    {
        that.file_.put('\n');
        if (that.extra_file_.is_open()) {
            that.extra_file_.put('\n');
        }
        std::cout << "\n";
        that.end_line();
        return that;
    }

    static bool open_extra(const std::string& filepath);
    static void close_extra();

private:
    explicit Logger(const std::string& log_folder);
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    static Logger& start(LoggerLevel level);
    bool open_file(bool determine_line_count = false);
    void end_line();

    static std::unique_ptr<Logger> instance_;
    static std::mutex mutex_;

    static constexpr size_t FileNameWidth_ = 4;
    static const int32_t FileLineMax_ = 3000;
    bool created_;
    std::string log_folder_;
    int64_t file_number_counter_;
    int32_t file_line_counter_;
    std::mutex write_mutex_;
    std::ofstream file_;
    std::ofstream extra_file_;
};

}  // namespace tron
}  // namespace wayz
