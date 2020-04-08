//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include "serial_transport.hpp"

#include <common/include/logger/logger.hpp>

#include "serial_port.hpp"

namespace wayz {
namespace hera {
namespace device {
namespace utils {

SerialTransport* SerialTransport::instance_ = nullptr;
int32_t SerialTransport::reference_count_ = 0;
std::mutex SerialTransport::mutex_;

SerialTransport* SerialTransport::create(const std::string& kernel, const SerialConfig& serial_config)
{
    mutex_.lock();
    if (instance_ == nullptr) {
        instance_ = new SerialTransport(kernel, serial_config);
    }
    reference_count_++;
    mutex_.unlock();
    return instance_;
}

void SerialTransport::free()
{
    mutex_.lock();
    if (--reference_count_ == 0) {
        delete instance_;
        instance_ = nullptr;
    }
    mutex_.unlock();
}

SerialTransport::SerialTransport(const std::string& kernel, const SerialConfig& serial_config) :
    port_(nullptr),
    port_opened_(false),
    decoder_(nullptr),
    thread_run_(false),
    thread_fetch_(nullptr)
{
    port_ = new SerialPort();
    port_opened_ = port_->open_port(kernel, serial_config);

    for (size_t i = 0; i <= MaxMsgType_; ++i) {
        queue_registered[i] = false;
    }

    if (port_opened_) {
        decoder_ = new SerialTransportDecoder();
        thread_run_ = true;
        thread_fetch_ = new std::thread(&SerialTransport::fetch_thread_function, this);

        log::debug << "SerialTransport: Start thread" << log::endl;
    }
}

SerialTransport::~SerialTransport()
{
    log::debug << "SerialTransport: Stop thread" << log::endl;

    thread_run_ = false;
    if (thread_fetch_ != nullptr) {
        thread_fetch_->join();
    }
    if (port_ != nullptr) {
        port_->close_port();
    }
    if (decoder_) {
        delete decoder_;
    }
}

bool SerialTransport::is_opened() const
{
    return port_opened_;
}

common::ThreadQueue<SerialData>* SerialTransport::get_queue_handler(const int32_t msg_type)
{
    if (msg_type < 0 || msg_type > (int32_t)MaxMsgType_) {
        return nullptr;
    }

    common::ThreadQueue<SerialData>* result = nullptr;
    mutex_queue_handler_.lock();
    if (!queue_registered[msg_type]) {
        queue_registered[msg_type] = true;
        result = &(queue_[msg_type]);
    }
    mutex_queue_handler_.unlock();

    return result;
}

void SerialTransport::fetch_thread_function()
{
    while (thread_run_) {
        decoder_->insert(port_->read_port());
        SerialRawMsg* msg;
        size_t read_size;
        while ((read_size = decoder_->get_data(&msg)) != 0) {
            if (queue_registered[msg->msg_type]) {
                auto data = std::make_shared<std::vector<uint8_t>>(msg->msg_length);
                memcpy((data->data()), msg->msg_rawdata, msg->msg_length);
                queue_[msg->msg_type].emplace(std::move(data));
            }
        }
    }
}

void SerialTransportDecoder::insert(const std::vector<uint8_t>& in_buffer)
{
    for (auto ch : in_buffer) {
        raw_buff_[buff_write_index_] = ch;
        buff_write_index_ = next_index(buff_write_index_);
    }
}

size_t SerialTransportDecoder::copy_from_buff()
{
    size_t copied_len = 0;
    uint8_t* ptr = decode_buff_;
    size_t buff_end_index = next_index(buff_read_index_);
    for (size_t i = buff_start_index_; i != buff_end_index;) {
        *ptr++ = raw_buff_[i];
        copied_len++;
        i = next_index(i);
    }
    return copied_len;
}

size_t SerialTransportDecoder::get_data(SerialRawMsg** out_buff)
{
    while (buff_read_index_ != buff_write_index_) {
        uint8_t ch = raw_buff_[buff_read_index_];
        if (!start_detected_)  // Detect 0xF0 - 0xF7
        {
            if (ch >= uint8_t(0xF0) && ch <= uint8_t(0xF7)) {
                start_detected_ = true;
                buff_start_index_ = buff_read_index_;
            }
        } else {
            if (ch == uint8_t(0xFF))  // End Byte 0xFF detected
            {
                start_detected_ = false;
                size_t copied_len = copy_from_buff();
                if (size_t msg_len = decode(copied_len)) {
                    *out_buff = (SerialRawMsg*)decode_buff_;
                    return msg_len;
                } else {
                    *out_buff = nullptr;
                    return 0;
                }
            }
        }
        buff_read_index_ = next_index(buff_read_index_);
    }
    *out_buff = nullptr;
    return 0;
}

size_t SerialTransportDecoder::decode(size_t encoded_len)
{
    decode_buff_[0] &= uint8_t(0x0F);
    uint8_t msg_length = decode_buff_[1];
    if (msg_length > 240) {
        return 0;
    }

    uint32_t aligned_package_num = (msg_length + 2) / 3;
    uint32_t aligned_length = 4 * aligned_package_num + 3;

    if (encoded_len != aligned_length) {
        return 0;
    }

    uint8_t* src_ptr = &decode_buff_[2];
    uint8_t* dest_ptr = &decode_buff_[2];
    for (uint32_t iter = 0; iter < aligned_package_num; ++iter) {
        char d0_low = *(src_ptr++);
        char d1_low = *(src_ptr++);
        char d2_low = *(src_ptr++);
        char s3 = *(src_ptr++);
        *(dest_ptr++) = d0_low + ((s3 & uint8_t(0b00000011)) << 6);
        *(dest_ptr++) = d1_low + ((s3 & uint8_t(0b00001100)) << 4);
        *(dest_ptr++) = d2_low + ((s3 & uint8_t(0b00110000)) << 2);
    }

    return msg_length;
}

}  // namespace utils
}  // namespace device
}  // namespace hera
}  // namespace wayz