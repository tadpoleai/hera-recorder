//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include "serial_port_sentence.hpp"

#include <common/logger/logger.hpp>

namespace wayz {
namespace hera {

SerialPortSentence::SerialPortSentence(const std::string& kernel, const SerialConfig& serial_config) :
    port_(nullptr),
    port_opened_(false),
    thread_run_(false),
    thread_fetch_(nullptr),
    buffer_inited_(false)
{
    port_ = new SerialPort();
    port_opened_ = port_->open_port(kernel, serial_config);

    if (port_opened_) {
        buffer_.reserve(BufferReverseSize_);
        thread_run_ = true;
        thread_fetch_ = new std::thread(&SerialPortSentence::fetch_thread_function, this);
    }
}

SerialPortSentence::~SerialPortSentence()
{
    thread_run_ = false;
    if (thread_fetch_ != nullptr) {
        thread_fetch_->join();
    }
    if (port_ != nullptr) {
        port_->close_port();
    }
}

bool SerialPortSentence::is_opened() const
{
    return port_opened_;
}

ThreadQueue<SerialData>* SerialPortSentence::get_queue_handler() const
{
    return (ThreadQueue<SerialData>*)(&queue_);
}

void SerialPortSentence::fetch_thread_function()
{
    while (thread_run_) {
        // Read new inmode chars from port
        auto new_chars = port_->read_port();

        // Insert them into buffer
        for (auto&& c : new_chars) {
            if (__glibc_unlikely(!buffer_inited_)) {
                if (c == SentenceDivider_) {
                    buffer_inited_ = true;
                }
            } else {
                buffer_.emplace_back(c);
                if (__glibc_unlikely(c == SentenceDivider_)) {
                    auto sentence = std::make_shared<SerialData>(std::move(buffer_));
                    buffer_.clear();
                    buffer_.reserve(BufferReverseSize_);
                    queue_.emplace(std::move(sentence));
                }
            }
        }
    }
}

}  // namespace hera
}  // namespace wayz
