//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#pragma once

#include <cstdint>
#include <string>
#include <thread>
#include <vector>

#include <common/utils/thread_queue.hpp>

#include "serial_port.hpp"

namespace wayz {
namespace hera {

using SerialData = std::vector<uint8_t>;

class SerialPortSentence {
public:
    SerialPortSentence(const std::string& kernel, const SerialConfig& serial_config);
    SerialPortSentence(const SerialPortSentence&) = delete;
    SerialPortSentence& operator=(const SerialPortSentence&) = delete;
    ~SerialPortSentence();

    ThreadQueue<SerialData>* get_queue_handler() const;
    bool is_opened() const;

private:
    void fetch_thread_function();

    SerialPort* port_;
    bool port_opened_;
    volatile bool thread_run_;
    std::thread* thread_fetch_;

    static constexpr uint8_t SentenceDivider_ = '\n';

    static constexpr size_t BufferReverseSize_ = 64;
    SerialData buffer_;
    bool buffer_inited_;

    ThreadQueue<SerialData> queue_;
};


}  // namespace hera
}  // namespace wayz
