//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#pragma once

#include <atomic>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <common/utils/thread_queue.hpp>

#include "serial_port.hpp"

namespace wayz {
namespace hera {
namespace device {
namespace utils {

using SerialData = std::vector<uint8_t>;

#pragma pack(push, 1)

struct SerialRawMsg {
    uint8_t msg_type;
    uint8_t msg_length;
    uint8_t msg_rawdata[0];
};

#pragma pack(pop)

class SerialTransportDecoder {
public:
    void insert(const std::vector<uint8_t>& in_buffer);
    size_t get_data(SerialRawMsg** out_buff);

private:
    inline size_t next_index(size_t index)
    {
        if (++index >= BuffSize_) {
            index = 0;
        }
        return index;
    }
    size_t copy_from_buff();
    size_t decode(size_t len);

    static const size_t BuffSize_ = 65536;
    uint8_t raw_buff_[BuffSize_];
    static const size_t DecodeBuffSize_ = 240;
    uint8_t decode_buff_[DecodeBuffSize_];
    size_t buff_write_index_ = 0;
    size_t buff_read_index_ = 0;
    size_t buff_start_index_ = 0;
    bool start_detected_ = false;
};

class SerialTransport {
public:
    static SerialTransport* create(const std::string& kernel, const SerialConfig& serial_config);
    common::ThreadQueue<SerialData>* get_queue_handler(const int32_t msg_type);
    bool is_opened() const;
    void free();

private:
    SerialTransport(const std::string& kernel, const SerialConfig& serial_config);
    SerialTransport(const SerialTransport&) = delete;
    SerialTransport& operator=(const SerialTransport&) = delete;
    ~SerialTransport();

    void fetch_thread_function();

    static SerialTransport* instance_;
    static std::mutex mutex_;
    static int32_t reference_count_;

    SerialPort* port_;
    bool port_opened_;
    SerialTransportDecoder* decoder_;
    std::atomic<bool> thread_run_;
    std::thread* thread_fetch_;

    static const size_t MaxMsgType_ = 7;
    std::mutex mutex_queue_handler_;
    std::atomic<bool> queue_registered[MaxMsgType_ + 1];
    common::ThreadQueue<SerialData> queue_[MaxMsgType_ + 1];
};


}  // namespace utils
}  // namespace device
}  // namespace hera
}  // namespace wayz