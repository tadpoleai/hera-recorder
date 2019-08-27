//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include "sensor_base.hpp"

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace wayz {

SensorBase::SensorBase(const std::string& sensor_name) :
    sensor_status_(SensorStatus::uninited),
    // sensor_realtime_forwarding_(false),
    sensor_fetch_thread_(nullptr),
    sensor_storage_thread_(nullptr),
    sensor_name_(sensor_name),
    sensor_storage_path_set_(false),
    ofstream_num_count_(0),
    ofstream_current_bytecount_(0)
{
    sensor_storage_thread_ = new std::thread(&SensorBase::sensor_storage_thread_function, this);
    sensor_fetch_thread_ = new std::thread(&SensorBase::sensor_fetch_thread_function, this);
}
SensorBase::~SensorBase()
{
    disconnect_sensor();
}

void SensorBase::start_saving()
{
    auto sensor_status = sensor_status_;
    if (sensor_status == SensorStatus::inited) {
        if (sensor_storage_path_set_) {
            create_storage_folder();
            sensor_status_ = SensorStatus::recording;
        }
    }
    if (sensor_status == SensorStatus::paused) {
        sensor_status_ = SensorStatus::recording;
    }
}
void SensorBase::pause_saving()
{
    auto sensor_status = sensor_status_;
    if (sensor_status == SensorStatus::recording) {
        sensor_status_ = SensorStatus::paused;
    }
}
void SensorBase::set_storage_folder(const std::string& storage_folder)
{
    if (!sensor_storage_path_set_) {
        sensor_storage_path_ = storage_folder + "/" + sensor_name_ + "/";
        sensor_storage_path_set_ = true;
    }
}
void SensorBase::start_realtime_forwarding() {}
void SensorBase::pause_realtime_forwarding() {}

bool SensorBase::create_storage_folder()
{
    int ret = system(("mkdir -p '" + sensor_storage_path_ + "'").c_str());
    if (ret == 0) {
        create_and_open_storage_file();
        return true;
    }
    return false;
}
void SensorBase::create_and_open_storage_file()
{
    sensor_storage_ofstream_.close();

    std::ostringstream current_filename_stream;
    current_filename_stream << sensor_storage_path_;
    current_filename_stream.fill('0');
    current_filename_stream.width(OfstreamFileNameWidth);
    current_filename_stream << ofstream_num_count_++;
    current_filename_stream << ".bin";

    sensor_storage_ofstream_.open(current_filename_stream.str(), std::ios::out | std::ios::binary);
    ofstream_current_bytecount_ = 0;
}
void SensorBase::push_one_data(SensorData* sensor_data_ptr)
{
    if (sensor_data_ptr != nullptr) {
        sensor_data_queue_.push(sensor_data_ptr);
    }
}
void SensorBase::wait_pop_save_one_data(bool if_save)
{
    SensorData* sensor_data = sensor_data_queue_.wait_and_pop();
    if (if_save) {
        if (ofstream_current_bytecount_ + sensor_data->length > OfstreamMaxSizeByte) {
            create_and_open_storage_file();
        }
        sensor_storage_ofstream_.write(reinterpret_cast<const char*>(sensor_data),
                                       sensor_data->length);
        ofstream_current_bytecount_ += sensor_data->length;
    }
    delete[] sensor_data;
}

void SensorBase::sensor_storage_thread_function()
{
    while (true) {
        auto sensor_status = sensor_status_;
        if (sensor_status == SensorStatus::error || sensor_status == SensorStatus::terminated) {
            break;
        }
        if (sensor_status == SensorStatus::uninited) {
        } else {
            bool if_save = (sensor_status == SensorStatus::recording);
            wait_pop_save_one_data(if_save);
        }
    }
}
void SensorBase::sensor_fetch_thread_function()
{
    while (true) {
        auto sensor_status = sensor_status_;
        if (sensor_status == SensorStatus::error || sensor_status == SensorStatus::terminated) {
            break;
        }
        if (sensor_status == SensorStatus::inited || sensor_status == SensorStatus::recording ||
            sensor_status == SensorStatus::paused) {
            SensorData* sensor_data = do_sensor_fetch();
            push_one_data(sensor_data);
        }
    }
}

void SensorBase::connect_sensor()
{
    if (sensor_status_ == SensorStatus::uninited) {
        if (do_connect_sensor()) {
            sensor_status_ = SensorStatus::inited;
        } else {
            sensor_status_ = SensorStatus::error;
        }
    }
}
void SensorBase::disconnect_sensor()
{
    auto sensor_status = sensor_status_;
    if (sensor_status != SensorStatus::terminated) {
        sensor_status_ = SensorStatus::terminated;
        do_disconnect_sensor();
        sensor_fetch_thread_->join();
        sensor_storage_thread_->join();
        delete sensor_fetch_thread_;
        delete sensor_storage_thread_;
        sensor_data_queue_.clear_and_delete();
        sensor_storage_ofstream_.close();
    }
}

std::string SensorBase::get_sensor_name() const
{
    return sensor_name_;
}
std::string SensorBase::get_sensor_status() const
{
    auto sensor_status = sensor_status_;
    switch (sensor_status) {
    case SensorStatus::error:
        return "error";
    case SensorStatus::uninited:
        return "uninited";
    case SensorStatus::inited:
        return "inited";
    case SensorStatus::recording:
        return "recording";
    case SensorStatus::paused:
        return "paused";
    case SensorStatus::terminated:
        return "terminated";
    }
}
bool SensorBase::get_sensor_alive() const
{
    return true;
}

}  // namespace wayz