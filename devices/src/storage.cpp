///
/// @file storage.cpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Implementation of class Storage
/// @version 0.1
/// @date 2019-11-06
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include "storage.hpp"

#include <sstream>

namespace wayz {
namespace hera {

/// Construct a new Storage:: Storage object
/// initialize the writing thread if not in read_mode
Storage::Storage(std::string&& folder, const bool read_mode, const size_t history_depth) :
    folder_(std::forward<std::string>(folder)),
    file_number_counter_(0),
    file_size_counter_(0),
    total_file_size_counter_(0),
    read_mode_(read_mode),
    thread_(nullptr),
    thread_running_(false),
    thread_queue_(0, history_depth)
{
    if (!read_mode_) {
        thread_ = new std::thread(&Storage::write_thread_function, this);
        thread_running_ = true;
    }
}

/// Set thread_running_ to false to make thread joinable,
/// then flush and close opened storage files
Storage::~Storage()
{
    thread_running_ = false;
    if (thread_ != nullptr) {
        thread_->join();
        delete thread_;
    }
    if (out_file_.is_open()) {
        out_file_.flush();
        out_file_.close();
    }
    if (in_file_.is_open()) {
        in_file_.close();
    }
}

/// This public interface push a storage data into thread-safe queue
///
void Storage::write(StorageDataPtr&& data, const bool only_history)
{
    thread_queue_.push(std::forward<StorageDataPtr>(data), only_history);
}

/// Read data from in_file first,
/// if failed, try to open next file and try to read again
StorageDataPtr Storage::read()
{
    if (!in_file_.is_open()) {
        if (!open_new_file()) {
            return nullptr;
        }
    }

    auto data = StorageData::read_from(in_file_);
    if (data == nullptr) {
        if (!open_new_file()) {
            return nullptr;
        }
        data = StorageData::read_from(in_file_);
    }
    total_file_size_counter_ += data->get_length();
    return data;
}

std::vector<StorageDataPtr> Storage::history() {
    if (!read_mode_) {
        return thread_queue_.history();
    } else {
        return std::vector<StorageDataPtr>();
    }
}

/// Use system call mkdir
/// to create a folder
bool Storage::create_folder()
{
    int ret = system(("mkdir -p '" + folder_ + "'").c_str());
    if (ret == 0) {
        return true;
    }
    return false;
}

/// Create a new storage file under the storage folder
/// with a filename <file_number_counter_>{FileNameWidth_}.bin
/// @note File opened previously, will be flushed and closed
/// @note File_number_counter_ with increse by one after this operation
bool Storage::open_new_file()
{
    if (read_mode_) {
        if (in_file_.is_open()) {
            in_file_.close();
        }
    } else {
        if (out_file_.is_open()) {
            out_file_.flush();
            out_file_.close();
        }
    }

    std::ostringstream filename;
    filename << folder_ << '/';
    filename.fill('0');
    filename.width(FileNameWidth_);
    filename << file_number_counter_++;
    filename << ".bin";

    if (read_mode_) {
        in_file_.open(filename.str(), std::ios::in | std::ios::binary);
        if (in_file_.is_open()) {
            return true;
        } else {
            return false;
        }
    } else {
        out_file_.open(filename.str(), std::ios::out | std::ios::binary);
        if (out_file_.is_open()) {
            return true;
        } else {
            return false;
        }
    }
}

void Storage::write_thread_function()
{
    /// Create folder and open a new file first
    /// @todo log if operation error
    if (!create_folder()) {
        return;
    }
    if (!open_new_file()) {
        return;
    }

    thread_local bool fulfilled = false;

    /// Thread wait until thread_running_ is false and queue is empty,
    /// to ensure every data in memory written to storage file
    while (thread_running_ || fulfilled) {
        auto data = thread_queue_.wait_pop();
        if (data != nullptr) {
            fulfilled = true;
            auto length = data->write_to(out_file_);
            file_size_counter_ += length;
            total_file_size_counter_ += length;
            if (file_size_counter_ > FileMaxSize_) {
                open_new_file();
                file_size_counter_ = 0;
            }
        } else {
            fulfilled = false;
        }
    }
    out_file_.close();
    out_file_.flush();
}

}  // namespace hera
}  // namespace wayz