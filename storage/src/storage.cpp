///
/// @file storage.cpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Implementation of class Storage
/// @version 0.2
/// @date 2019-12-24
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include "storage.hpp"

#include <sstream>

#include "common/include/logger/logger.hpp"
#include "unistd.h"

namespace wayz {
namespace hera {
namespace storage {

StorageManagerPtr StorageManager::open(const std::string& filename,
                                       const bool read_mode,
                                       const bool is_extra,
                                       const bool is_logs,
                                       const bool read_strict)
{
    return StorageManagerPtr(new StorageManager(filename, read_mode, is_extra, is_logs, read_strict));
}

StorageManager::StorageManager(const std::string& filename,
                               const bool read_mode,
                               const bool is_extra,
                               const bool is_logs,
                               const bool read_strict) :
    header(nullptr),
    filename_(filename),
    file_size_counter_(0),
    read_mode_(read_mode),
    read_strict_(read_strict),
    out_file_opened_(false),
    add_device_finished_(false),
    thread_(nullptr),
    thread_running_(false),
    prefetch_data_ready_(false),
    prefetch_ended_(false),
    read_header_timestamp_(UINT64_MAX)
{
    if (read_mode) {
        in_file_.open(filename, std::ios::binary);
        if (in_file_.is_open()) {
            header = StorageDataHeader::read_from(in_file_, is_extra, is_logs);
        } else {
            log::error << "StorageManager: Can not open " << filename << log::endl;
            return;
        }

        if (read_strict) {
            log::warn << "StorageManager: Opening '" << filename << "' in timestamp strict aligned mode" << log::endl;
            log::warn << "StorageManager: This cosumes more CPU and memory" << log::endl;

            thread_running_ = true;
            thread_ = new std::thread(&StorageManager::prefetch_thread_function, this);
        }
    } else {
        header = StorageDataHeaderPtr(new StorageDataHeader(4));
        log::open_aux(&header->logs);
        thread_running_ = true;
        thread_ = new std::thread(&StorageManager::write_thread_function, this);
    }
}

void StorageManager::close()
{
    log::debug << "StorageManager: Close" << log::endl;
    if (thread_ != nullptr) {
        thread_running_ = false;
        thread_->join();
        log::debug << "StorageManager: Joined Thread " << log::endl;
        delete thread_;
        thread_ = nullptr;
        if (header != nullptr) {
            log::debug << "StorageManager: Updating Header" << log::endl;
            header->timestamp_end = time::Timestamp::now();
        }
    }

    log::flush();
    log::close_aux();

    if (!read_mode_ && out_file_opened_) {
        out_file_.flush();
        log::flush();
        if (header != nullptr) {
            out_file_.seekp(0, std::ios::beg);
            header->write_to(out_file_);
        }
        log::debug << "StorageManager: closing file" << log::endl;
        out_file_.close();
        out_file_opened_ = false;
    }
}

StorageManager::~StorageManager()
{
    close();
}

bool StorageManager::add_device(const std::string& full_name, const size_t history_depth)
{
    if (read_mode_ || add_device_finished_ || !header) {
        log::warn << "Storage: Can not add device" << log::endl;
        return false;
    }
    log::debug << "Storage: Add device " << full_name << log::endl;
    header->device_message_nums.push_back(0);
    header->device_names.push_back(full_name);
    header->device_data_sizes.push_back(0);
    data_array_.emplace_back(
            std::move(std::make_unique<common::ThreadQueue<device::data::DeviceData>>(0, history_depth)));
    return true;
}

void StorageManager::finish_add_device()
{
    if (!read_mode_ && !add_device_finished_) {
        log::info << "Storage: Finished add devices" << log::endl;
        add_device_finished_ = true;
    }
}

bool StorageManager::add_data(const uint32_t device_id, device::data::DeviceDataPtr& data, const bool if_write_data)
{
    if (read_mode_ || !add_device_finished_ || device_id >= data_array_.size()) {
        auto logger = log::warn << "Storage: Can not add data";
        if (read_mode_) {
            logger << " due to read mode";
        }
        if (!add_device_finished_) {
            logger << " due to finish_add_device not called yet";
        }
        if (device_id >= data_array_.size()) {
            logger << " due to device id '" << device_id << "' is out of range";
        }
        logger << log::endl;
        return false;
    }

    return data_array_[device_id]->push(data, !if_write_data);
}

std::vector<DeviceDataPtr> StorageManager::history(const uint32_t device_id) const
{
    if (read_mode_ || !add_device_finished_ || device_id >= data_array_.size()) {
        return std::vector<DeviceDataPtr>();
    }

    return data_array_[device_id]->history();
}

uint64_t StorageManager::get_volume(const uint32_t device_id) const
{
    if (read_mode_ || !add_device_finished_ || device_id >= data_array_.size() || !header) {
        return 0;
    }

    return header->device_data_sizes[device_id];
}

void StorageManager::write_thread_function()
{
    thread_local bool fulfilled = false;

    while (thread_running_ || fulfilled) {
        fulfilled = false;
        for (uint32_t i = 0; i < data_array_.size(); i++) {
            auto data = data_array_[i]->pop();
            if (data != nullptr) {
                fulfilled = true;
                if (!out_file_opened_) {
                    out_file_.open(filename_, std::ios::binary);
                    header->timestamp_start = data->get_timestamp_receive_ns();
                    header->write_to(out_file_);
                    out_file_opened_ = true;
                }
                auto length = data->write_to(out_file_);
                if (header != nullptr) {
                    header->device_data_sizes[i] += length;
                    header->device_message_nums[i]++;
                }
            }
        }
        if (!fulfilled) {
            usleep(TimeSleepUs);
        }
    }
}

}  // namespace storage
}  // namespace hera
}  // namespace wayz