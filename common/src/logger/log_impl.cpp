//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include "logger/log_impl.hpp"

#include <exception>
#include <regex>
#include <stdexcept>

namespace wayz {
namespace hera {
namespace log {
namespace impl {

std::unique_ptr<Logger> Logger::instance_(new Logger());

bool Logger::sleep_before_exiting_ = false;

void Logger::onlyprint()
{
    instance_->inited_ = true;
}

void Logger::set_sleep_before_exiting(bool value)
{
    sleep_before_exiting_ = value;
}

void Logger::clear_line()
{
    instance_->clear_line_ = true;
}

void Logger::flush()
{
    while (!instance_->flushed_)
        ;
}

bool Logger::init(const std::string& file)
{
    instance_->inited_ = true;
    char buffer[80];
    auto t = time::Timestamp::now().tv_sec;
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

bool Logger::open_aux(std::vector<LogString>* aux_vector)
{
    if (aux_vector != nullptr) {
        if (std::find(instance_->aux_vector_vector_.begin(), instance_->aux_vector_vector_.end(), aux_vector) ==
            instance_->aux_vector_vector_.end()) {
            instance_->aux_vector_vector_.push_back(aux_vector);
            return true;
        }
    }

    return false;
}

void Logger::close_aux(std::vector<LogString>* aux_vector)
{
    if (aux_vector != nullptr) {
        auto pos = std::find(instance_->aux_vector_vector_.begin(), instance_->aux_vector_vector_.end(), aux_vector);

        if (pos != instance_->aux_vector_vector_.end()) {
            instance_->aux_vector_vector_.erase(pos);
        }
    }
}

LogStringStream Logger::create_string(LogLevel level)
{
    if (level >= instance_->level_) {
        auto result = LogStringStream(&instance_->queue_, true, level, time::Timestamp::now());
        return result;
    }
    return LogStringStream(nullptr, false, level, 0);
}

Logger::Logger() :
    inited_(false),
    aux_vector_vector_(),
    level_(LogLevel::Debug),
    queue_(0, 0, 5ms),
    running_(true),
    flushed_(true),
    thread_(&Logger::write_thread_function, this)
{
    register_back_trace();
}

Logger::~Logger()
{
    running_ = false;
    thread_.join();
    file_.close();
}

void Logger::write_thread_function()
{
    decltype(queue_.wait_pop()) data = nullptr;
    while (running_ || data) {
        data = queue_.wait_pop();
        if (data != nullptr) {
            flushed_ = false;
            write(std::move(*data));
        } else {
            flushed_ = true;
        }
    };
}

void Logger::write(LogString&& data)
{
    auto str = format(data);
    if (!inited_) {
        inited_ = true;
        init("default_log");
    }

    if (clear_line_) {
        std::cout << "\r";
    }
    std::cout << data.level.to_color_prefix() << str << data.level.to_color_suffix() << std::endl;
    if (file_.is_open()) {
        file_ << str << std::endl;
    }
    for (auto& aux_vector_ptr : aux_vector_vector_) {
        aux_vector_ptr->emplace_back(std::move(data));
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