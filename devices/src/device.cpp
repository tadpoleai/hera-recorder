//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include "device.hpp"

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <thread>
#include <unistd.h>
#include <utility>
#include <vector>

namespace wayz {
namespace tron {

Device::Device(int32_t id, const std::string& name) :
    sequence_(0),
    id_(id),
    name_(name),
    is_storage_path_set_(false),
    file_number_counter_(0),
    file_size_counter_(0),
    total_file_size_counter_(0),
    status_(DeviceStatus::Uninited),
    last_errno_(TronErrno::Success),
    last_data_timestamp_ns_(0),
    is_record_(false),
    is_forward_(false)
{
    thread_fetch_ = new std::thread(&Device::fetch_thread_function, this);
    thread_storage_ = new std::thread(&Device::storage_thread_function, this);
    thread_forward_ = new std::thread(&Device::forward_thread_function, this);
}
Device::~Device() {}

// Control
TronErrno Device::start()
{
    auto status = status_;
    if (status == DeviceStatus::Error) {
        return TronErrno::InStatusError;
    }
    if (status == DeviceStatus::Uninited) {
        TronErrno e = connect();
        if (e == TronErrno::Success) {
            status_ = DeviceStatus::Inited;
            return TronErrno::Success;
        } else {
            return set_error_and_die(e);
        }
    }
    return TronErrno::DeviceAlreadyConnected;
}
TronErrno Device::stop()
{
    auto status = status_;
    if (status != DeviceStatus::Terminated) {
        status_ = DeviceStatus::Terminated;
        is_record_ = false;
        is_forward_ = false;
        thread_fetch_->join();
        thread_storage_->join();
        thread_forward_->join();
        delete thread_fetch_;
        delete thread_storage_;
        delete thread_forward_;
        file_.flush();
        file_.close();
        return TronErrno::Success;
    }
    return TronErrno::DeviceAlreadyClosed;
}
TronErrno Device::start_record()
{
    auto status = status_;
    if (status == DeviceStatus::Error) {
        return TronErrno::InStatusError;
    }
    if (status == DeviceStatus::Inited) {
        if (!is_storage_path_set_) {
            return TronErrno::StorageFolderNoSet;
        } else {
            is_record_ = true;
            return TronErrno::Success;
        }
    }
    return TronErrno::DeviceNotReady;
}
TronErrno Device::pause_record()
{
    auto status = status_;
    if (status == DeviceStatus::Error) {
        return TronErrno::InStatusError;
    }
    if (status == DeviceStatus::Inited) {
        is_record_ = false;
        return TronErrno::Success;
    }
    return TronErrno::DeviceNotReady;
}
TronErrno Device::enable_forward()
{
    auto status = status_;
    if (status == DeviceStatus::Error) {
        return TronErrno::InStatusError;
    }
    if (status == DeviceStatus::Inited) {
        is_forward_ = true;
        return TronErrno::Success;
    }
    return TronErrno::DeviceNotReady;
}
TronErrno Device::disable_forward()
{
    auto status = status_;
    if (status == DeviceStatus::Error) {
        return TronErrno::InStatusError;
    }
    if (status == DeviceStatus::Inited) {
        is_forward_ = false;
        return TronErrno::Success;
    }
    return TronErrno::DeviceNotReady;
}

// Configure
TronErrno Device::set_storage(const std::string& folder)
{
    auto status = status_;
    if (status == DeviceStatus::Error) {
        return TronErrno::InStatusError;
    }
    if (!is_storage_path_set_) {
        storage_root_ = folder;
        storage_path_ = folder + "/" + get_type()._to_string() + "/" + name_ + "/";
        TronErrno e = create_storage_folder();
        if (e == TronErrno::Success) {
            is_storage_path_set_ = true;
        }
        return e;
    }
    return TronErrno::StorageFolderAlreadySet;
}
TronErrno Device::set_parameter(const std::string& type, const std::string& value)
{
    printf("Set Parameter\n");
    auto status = status_;
    if (status == DeviceStatus::Error) {
        return TronErrno::InStatusError;
    }
    auto parameter_type = DeviceParameterType::_from_string_nocase_nothrow(type.c_str());
    if (!parameter_type) {
        return TronErrno::InvalidParameterType;
    } else {
        parameters_[parameter_type.value()] = value;
        return TronErrno::Success;
    }
}
TronErrno Device::adjust_parameter(const std::string& type, const std::string& value)
{
    auto status = status_;
    if (status == DeviceStatus::Error) {
        return TronErrno::InStatusError;
    }
    auto parameter_type = DeviceParameterType::_from_string_nocase_nothrow(type.c_str());
    if (!parameter_type) {
        return TronErrno::InvalidParameterType;
    } else {
        parameters_[parameter_type.value()] = value;
        return do_adjust_parameter(parameter_type.value(), value);
    }
}

// Status
int32_t Device::get_id() const
{
    return id_;
}
std::string Device::get_name() const
{
    return name_;
}
std::map<std::string, std::string> Device::get_parameters() const
{
    std::map<std::string, std::string> parameters;
    for (const auto& parameter : parameters_) {
        parameters[parameter.first._to_string()] = parameter.second;
    }
    return parameters;
}
int64_t Device::get_volume() const
{
    return total_file_size_counter_;
}
std::string Device::get_status() const
{
    switch (status_) {
    case DeviceStatus::Uninited:
        return "Uninited";
    case DeviceStatus::Inited:
        return "Inited";
    case DeviceStatus::Terminated:
        return "Terminated";
    default:
        return "Error";
    }
    return "Error";
}
TronErrno Device::get_errno() const
{
    if (last_errno_ == TronErrno::Success) {
        if (check_new_data()) {
            return TronErrno::Success;
        } else {
            return TronErrno::NoNewData;
        }
    }
    return last_errno_;
}
std::string Device::get_reason() const
{
    return last_errno_reason_;
}
bool Device::get_is_record() const
{
    return is_record_;
}
bool Device::get_is_forward() const
{
    return is_forward_;
}
bool Device::get_is_storage_set() const
{
    return is_storage_path_set_;
}
std::string Device::get_storage_folder() const
{
    return storage_root_;
}

// Protected
TronErrno Device::set_error_and_die(TronErrno e, const std::string& reason)
{
    status_ = DeviceStatus::Error;
    last_errno_ = e;
    if (!reason.empty()) {
        last_errno_reason_ = reason;
    }
    return e;
}

// Private
TronErrno Device::create_storage_folder()
{
    auto status = status_;
    if (status == DeviceStatus::Error) {
        return TronErrno::InStatusError;
    }
    int ret = system(("mkdir -p '" + storage_path_ + "'").c_str());
    if (ret == 0) {
        return TronErrno::Success;
    }
    return set_error_and_die(TronErrno::CanNotCreateFolder);
}
TronErrno Device::open_new_storage_file()
{
    auto status = status_;
    if (status == DeviceStatus::Error) {
        return TronErrno::InStatusError;
    }
    if (file_.is_open()) {
        file_.flush();
        file_.close();
    }

    std::ostringstream filename;
    filename << storage_path_;
    filename.fill('0');
    filename.width(FileNameWidth);
    filename << file_number_counter_++;
    filename << ".bin";

    file_.open(filename.str(), std::ios::out | std::ios::binary);

    if (file_.is_open()) {
        file_size_counter_ = 0;
        return TronErrno::Success;
    } else {
        set_error_and_die(TronErrno::CanNotOpenFile);
    }

    return TronErrno::Success;
}

// Threads
void Device::fetch_thread_function()
{
    while (true) {
        auto status = status_;
        if (status == DeviceStatus::Error || status == DeviceStatus::Terminated) {
            break;
        }
        if (status == DeviceStatus::Inited) {
            auto rawdata = fetch();
            if (rawdata != nullptr) {
                last_data_timestamp_ns_ = rawdata->timestamp_receive_ns;
                if (is_record_) {
                    queue_storage_.push(rawdata);
                }
                if (is_forward_) {
                    queue_forward_.push(rawdata);
                }
            } else {
                usleep(TimeSleepNotDataUs_);
            }
        }
    }
}
void Device::storage_thread_function()
{
    while (true) {
        auto status = status_;
        if (status == DeviceStatus::Error || status == DeviceStatus::Terminated) {
            break;
        }
        if (status == DeviceStatus::Inited && is_storage_path_set_) {
            if (!queue_storage_.empty()) {
                auto rawdata = queue_storage_.wait_and_pop();
                if (file_number_counter_ == 0 && file_size_counter_ == 0) {
                    open_new_storage_file();
                } else if ((file_size_counter_ != 0) &&
                           (file_size_counter_ + rawdata->length > FileMaxSize_)) {
                    open_new_storage_file();
                }
                file_.write(reinterpret_cast<char*>(rawdata.get()), rawdata->length);
                // file_.flush();
                file_size_counter_ += rawdata->length;
                total_file_size_counter_ += rawdata->length;
            } else {
                usleep(TimeSleepNotDataUs_);
            }
        }
    }
}
void Device::forward_thread_function()
{
    while (true) {
        auto status = status_;
        if (status == DeviceStatus::Error || status == DeviceStatus::Terminated) {
            break;
        }
        if (status == DeviceStatus::Inited) {
            if (!queue_forward_.empty()) {
                auto rawdata = queue_forward_.wait_and_pop();
                auto data = convert(rawdata);
                // TODO
                // Send Data via Socket
            } else {
                usleep(TimeSleepNotDataUs_);
            }
        }
    }
}

// Utils
bool Device::check_new_data() const
{
    return (get_system_timestamp() - last_data_timestamp_ns_ < MaxDataDurationNs_);
}

}  // namespace tron
}  // namespace wayz