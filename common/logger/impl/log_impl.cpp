//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include "log_impl.hpp"

#include <regex>

namespace wayz {
namespace hera {
namespace log {
namespace impl {

std::unique_ptr<Logger> Logger::instance_(new Logger());

void Logger::onlyprint()
{
    instance_->inited_ = true;
}

bool Logger::init(const std::string& file)
{
    instance_->inited_ = true;
    char buffer[80];
    auto t = Timestamp::now().tv_sec;
    auto timeinfo = std::localtime(&t);
    std::strftime(buffer, 80, "%Y%m%d_%H%M%S", timeinfo);
    auto file_with_time = file + "_" + buffer + ".log";
    instance_->file_.close();
    instance_->file_.open(file_with_time);
    std::cout << "Log created to " << file_with_time << std::endl;
    return instance_->file_.is_open();
}

void Logger::set_level(LogLevel level)
{
    instance_->level_ = level;
}

bool Logger::open_aux(const std::string& file)
{
    instance_->aux_file_.close();
    instance_->aux_file_.open(file);
    return instance_->aux_file_.is_open();
}

void Logger::close_aux()
{
    instance_->aux_file_.close();
}

LogStringStream Logger::create_string(LogLevel level)
{
    if (level >= instance_->level_) {
        auto result = LogStringStream(&instance_->queue_, true, level, Timestamp::now());
        return result;
    }
    return LogStringStream(nullptr, false, level, 0);
}

Logger::Logger() :
    inited_(false),
    level_(LogLevel::Debug),
    queue_(0, 0, 80ms),
    running_(true),
    thread_(&Logger::write_thread_function, this)
{
    register_back_trace();
}

Logger::~Logger()
{
    running_ = false;
    thread_.join();
    file_.close();
    aux_file_.close();
}

void Logger::write_thread_function()
{
    decltype(queue_.wait_pop()) data = nullptr;
    while (running_ || data) {
        data = queue_.wait_pop();
        if (data != nullptr) {
            write(*data);
        }
    };
}

void Logger::write(const LogString& data)
{
    auto str = format(data);
    if (!inited_) {
        inited_ = true;
        init("default_log");
    }
    std::cout << data.level.to_color_prefix() << str << data.level.to_color_suffix() << std::endl;
    if (file_.is_open()) {
        file_ << str << std::endl;
    }
    if (aux_file_.is_open()) {
        aux_file_ << str << std::endl;
    }
}

std::string Logger::format(const LogString& data)
{
    std::stringstream ss;
    ss << data.ts << " | " << data.level.to_string() << " | " << data.str;
    return ss.str();
}
}  // namespace impl
}  // namespace log
}  // namespace hera
}  // namespace wayz