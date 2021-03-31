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

Tool::Tool(const std::string& filename,
           const bool print_extra,
           const bool print_logs,
           const bool rebuild,
           const bool reindex,
           const std::string& outfilename) :
    read_thread_(nullptr),
    running_(false),
    progess_size_(0),
    total_size_(0),
    storage_(nullptr),
    rebuild_(rebuild),
    reindex_(reindex),
    filename_(filename),
    outfilename_(outfilename)
{
    auto filesize = file::get_file_size(filename_);
    if (filesize > 0) {
        log::info << "Storage: File '" << filename_ << "' sized " << filesize << log::endl;
    } else {
        log::error << "Storage: File '" << filename_ << "' do not exist or sized 0" << log::endl;
        return;
    }
    total_size_ = filesize;

    storage_ = storage::StorageManager::open(filename_, true, print_extra, print_logs);
    if (storage_->header != nullptr) {
        log::flush();
        std::cout << *storage_->header << std::endl;
        if (rebuild_ || reindex_) {
            log::info << "Storage: Start reading '" << filename_ << "'" << log::endl;
            read_thread_ = new std::thread(&Tool::read_thread_function, this);
            running_ = true;
        }
    } else {
        log::error << "Storage: File '" << filename_ << "' can not open or is invalid" << log::endl;
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

    std::fill(new_header.device_message_nums.begin(), new_header.device_message_nums.end(), 0);
    std::fill(new_header.device_data_sizes.begin(), new_header.device_data_sizes.end(), 0);

    while ((data = storage_->read())) {
        // if (reindex_) {
        //     auto sensor_data = device::Factory::convert(data);
        // }
        progess_size_ += data->get_length();
        auto id = data->get_device_id();
        new_header.device_data_sizes[id] += data->get_length();
        new_header.device_message_nums[id]++;
        if (new_header.timestamp_end < data->get_timestamp_receive_ns()) {
            new_header.timestamp_end = data->get_timestamp_receive_ns();
        }
    }

    running_ = false;
    storage_.reset();

    log::info << "Storage: Reading finished, rebuilt header as below" << log::endl;

    log::info << new_header << log::endl;

    std::ofstream file;
    file.open(filename_, std::ios::binary | std::ios::out | std::ios::in);
    if (file.is_open()) {
        file.seekp(0, std::ios::beg);
        new_header.write_to(file);
        log::info << "Storage: New header written to file" << log::endl;
        file.close();
    } else {
        log::error << "Storage: Can not open '" << filename_ << "' to write" << log::endl;
    }
}

}  // namespace storage
}  // namespace hera
}  // namespace wayz