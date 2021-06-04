///
/// @file replayer.cpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Implementation of class Replayer
/// @version 0.2
/// @date 2019-12-25
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include "tool.hpp"

#include <mutex>
#include <semaphore.h>

#include "common/include/logger/logger.hpp"
#include "device/include/include.hpp"

namespace wayz {
namespace hera {
namespace storage {

Tool::Tool(const Config& config) :
    read_thread_(nullptr),
    running_(false),
    progess_size_(0),
    total_size_(0),
    storage_(nullptr),
    config_(config)
{
    auto filesize = file::get_file_size(config_.filename);
    if (filesize > 0) {
        log::info << "Storage: File '" << config_.filename << "' sized " << filesize << log::endl;
    } else {
        log::error << "Storage: File '" << config_.filename << "' do not exist or sized 0" << log::endl;
        return;
    }
    total_size_ = filesize;

    storage_ = storage::StorageManager::open(config_.filename,
                                             true,
                                             config_.print_extra || config_.rebuild,
                                             config_.print_logs || config_.rebuild,
                                             false);
    if (storage_->header != nullptr) {
        log::flush();
        std::cout << *storage_->header << std::endl;
        if (config_.rebuild) {
            log::info << "Storage: Start reading '" << config_.filename << "'" << log::endl;
            running_ = true;
            read_thread_ = new std::thread(&Tool::read_thread_function, this);
        } else if (config_.trim) {
            log::info << "Storage: Start trimming '" << config_.filename << "'" << log::endl;
            running_ = true;
            read_thread_ = new std::thread(&Tool::trim_thread_function, this);
        }
    } else {
        log::error << "Storage: File '" << config_.filename << "' can not open or is invalid" << log::endl;
    }
}

/// Wait for thread to join, and close bag file
///
Tool::~Tool()
{
    if (read_thread_) {
        read_thread_->join();
    }
    log::info << "Storage: Finished" << log::endl;
}

void Tool::read_thread_function()
{
    decltype(storage_->read()) data = nullptr;

    auto new_header = *storage_->header;
    new_header.indices.resize(0);

    std::fill(new_header.device_message_nums.begin(), new_header.device_message_nums.end(), 0);
    std::fill(new_header.device_data_sizes.begin(), new_header.device_data_sizes.end(), 0);

    uint64_t indexed_time = 0;
    while ((data = storage_->read())) {
        progess_size_ += data->get_length();
        auto id = data->get_device_id();
        new_header.device_data_sizes[id] += data->get_length();
        new_header.device_message_nums[id]++;
        if (new_header.timestamp_end < data->get_timestamp_receive_ns()) {
            new_header.timestamp_end = data->get_timestamp_receive_ns();
        }

        static constexpr uint64_t IndexInterval = 1 * time::OneSecond;
        if (data->get_timestamp_receive_ns() > indexed_time + IndexInterval) {
            indexed_time = data->get_timestamp_receive_ns();
            new_header.indices.push_back(
                    {.ts = indexed_time, .offset = (int64_t)storage_->tellg() - (int64_t)data->get_length()});
        }
    }

    running_ = false;
    storage_.reset();

    log::info << "Storage: Reading finished, rebuilt header as below" << log::endl;

    log::info << new_header << log::endl;

    std::ofstream file;
    file.open(config_.filename, std::ios::binary | std::ios::out | std::ios::in);
    if (file.is_open()) {
        file.seekp(0, std::ios::beg);
        new_header.write_to(file);
        log::info << "Storage: New header written to file" << log::endl;
        file.close();
    } else {
        log::error << "Storage: Can not open '" << config_.filename << "' to write" << log::endl;
    }
}

void Tool::trim_thread_function()
{
    decltype(storage_->read()) data = nullptr;

    if (storage_->header->timestamp_start == 0) {
        log::error << "Trim: Invalid start time of source hera file, rebuild it first!" << log::endl;
        return;
    }
    if (storage_->header->timestamp_end == 0) {
        log::error << "Trim: Invalid end time of source hera file, rebuild it first!" << log::endl;
        return;
    }

    uint64_t start_time = storage_->header->timestamp_start + config_.start_time * time::OneSecond;
    uint64_t end_time = storage_->header->timestamp_start + (config_.start_time + config_.duration) * time::OneSecond;
    if (config_.duration == 0) {
        end_time = storage_->header->timestamp_end;
    }
    if (end_time > storage_->header->timestamp_end) {
        end_time = storage_->header->timestamp_end;
    }

    log::debug << "Start = " << start_time << ", End = " << end_time << log::endl;

    std::ofstream outfs(config_.outfilename, std::ios::binary);
    if (!outfs.is_open()) {
        log::error << "Trim: Can not open " << config_.outfilename << log::endl;
        return;
    } else {
        log::info << "Trim: Opened " << config_.outfilename << " for writing" << log::endl;
    }

    auto new_header = *storage_->header;

    new_header.timestamp_start = start_time;
    new_header.timestamp_end = end_time;
    std::fill(new_header.device_message_nums.begin(), new_header.device_message_nums.end(), 0);
    std::fill(new_header.device_data_sizes.begin(), new_header.device_data_sizes.end(), 0);
    new_header.indices.clear();
    new_header.write_to(outfs);

    storage_->seek(start_time);
    total_size_ -= storage_->tellg();

    uint64_t indexed_time = 0;
    while ((data = storage_->read())) {
        progess_size_ += data->get_length();
        if (data->get_timestamp_receive_ns() < end_time && data->get_timestamp_receive_ns() > start_time) {
            auto id = data->get_device_id();
            new_header.device_data_sizes[id] += data->get_length();
            new_header.device_message_nums[id]++;

            static constexpr uint64_t IndexInterval = 1 * time::OneSecond;
            if (data->get_timestamp_receive_ns() > indexed_time + IndexInterval) {
                indexed_time = data->get_timestamp_receive_ns();
                new_header.indices.push_back({.ts = indexed_time, .offset = (int64_t)storage_->tellg()});
            }

            data->write_to(outfs);
        }
    }

    outfs.seekp(0, std::ios::beg);
    new_header.write_to(outfs);
    log::info << "Trim: New header written to file" << log::endl;
    outfs.close();

    running_ = false;

    log::info << "Trim: Trimming finished" << log::endl;
}

}  // namespace storage
}  // namespace hera
}  // namespace wayz