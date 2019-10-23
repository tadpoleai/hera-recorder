//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include "logger.hpp"

#include <algorithm>
#include <iostream>
#include <regex>
#include <sstream>

#include "../utils/get_folder_content.hpp"
#include "../utils/system_timestamp.hpp"

namespace wayz {
namespace tron {

std::unique_ptr<Logger> Logger::instance_;
std::mutex Logger::mutex_;

bool Logger::create(const std::string& log_folder)
{
    std::unique_lock<std::mutex> _(mutex_);
    if (instance_ == nullptr) {
        instance_ = std::unique_ptr<Logger>(new Logger(log_folder));
        return instance_->created_;
    }
    return false;
}

Logger& Logger::start(LoggerLevel level)
{
    if (instance_) {
        instance_->write_mutex_.lock();
        auto now = Timestamp::now();
        std::string level_str;
        switch (level) {
        case LoggerLevel::Debug:
            level_str = "DEBUG";
            std::cout << "\033[37m";
            break;
        case LoggerLevel::Info:
            level_str = "INFO ";
            std::cout << "\033[32m";
            break;
        case LoggerLevel::Warn:
            level_str = "WARN";
            std::cout << "\033[1m\033[33m";
            break;
        case LoggerLevel::Error:
            level_str = "ERROR";
            std::cout << "\033[1m\033[31m";
            break;
        }
        instance_->file_ << "[" << now << " " << level_str << "] ";
        if (instance_->extra_file_.is_open()) {
            instance_->extra_file_ << "[" << now << " " << level_str << "] ";
        }
        std::cout << "[" << now << " " << level_str << "] ";

        return *instance_;
    }
    throw("Fatal: No Log Instance, Exit!");
    exit(1);
}

void Logger::end_line()
{
    if (++file_line_counter_ >= FileLineMax_) {
        open_file();
    }
    write_mutex_.unlock();
}

Logger::Logger(const std::string& log_folder) :
    created_(false),
    log_folder_(log_folder),
    file_number_counter_(0)
{
    int ret = system(("mkdir -p '" + log_folder_ + "'").c_str());
    if (ret) {
        return;
    }
    auto content = get_folder_content(log_folder_);
    if (!content.opened) {
        return;
    }

    std::regex pattern(R"(^.*/\d{)" + std::to_string(FileNameWidth_) + R"(}\.log$)");
    for (auto& file : content.files) {
        if (std::regex_match(file, pattern)) {
            constexpr size_t ExtensionWidth = sizeof(".log") - 1;
            auto file_count = std::stoi(
                    file.substr(file.length() - FileNameWidth_ - ExtensionWidth, FileNameWidth_));
            if (file_number_counter_ < file_count) {
                file_number_counter_ = file_count;
            }
        }
    }

    created_ = open_file(true);
}

Logger::~Logger()
{
    if (file_.is_open()) {
        file_.flush();
        file_.close();
    }
    close_extra();
}

bool Logger::open_file(bool determine_line_count)
{
    if (file_.is_open()) {
        file_.flush();
        file_.close();
    }

    std::ostringstream filename;
    filename << log_folder_ + "/";
    filename.fill('0');
    filename.width(FileNameWidth_);
    filename << file_number_counter_++;
    filename << ".log";

    file_line_counter_ = 0;
    if (determine_line_count) {
        try {
            std::ifstream in_file;
            in_file.open(filename.str(), std::ios::in);
            if (!in_file.fail()) {
                file_line_counter_ = std::count(std::istreambuf_iterator<char>(in_file),
                                                std::istreambuf_iterator<char>(),
                                                '\n');
            }
            in_file.close();
        } catch (...) {
        }
    }

    file_.open(filename.str(), std::ios::out | std::ios::app);
    if (file_.is_open()) {
        return true;
    }
    return false;
}

bool Logger::open_extra(const std::string& filepath)
{
    close_extra();
    instance_->extra_file_.open(filepath, std::ios::out);
    return instance_->extra_file_.is_open();
}
void Logger::close_extra()
{
    if (instance_->extra_file_.is_open()) {
        instance_->extra_file_.flush();
        instance_->extra_file_.close();
    }
}


}  // namespace tron
}  // namespace wayz
