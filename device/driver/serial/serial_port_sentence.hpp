//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#pragma once

#include <atomic>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

#include <common/include/utils/thread_queue.hpp>

#include "serial_port.hpp"

namespace wayz {
namespace hera {
namespace device {
namespace driver {

using SerialData = std::vector<uint8_t>;

class SerialPortSentence {
public:
    SerialPortSentence(const std::string& kernel, const SerialConfig& serial_config);
    SerialPortSentence(const SerialPortSentence&) = delete;
    SerialPortSentence& operator=(const SerialPortSentence&) = delete;
    ~SerialPortSentence();

    common::ThreadQueue<SerialData>* get_queue_handler() const;
    bool is_opened() const;

private:
    void fetch_thread_function();

    SerialPort* port_;
    bool port_opened_;
    std::atomic<bool> thread_run_;
    std::thread* thread_fetch_;

    static constexpr uint8_t SentenceDivider_ = '\n';

    static constexpr size_t BufferReverseSize_ = 64;
    SerialData buffer_;
    bool buffer_inited_;

    common::ThreadQueue<SerialData> queue_;
};


}  // namespace driver
}  // namespace device
}  // namespace hera
}  // namespace wayz
