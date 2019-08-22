//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#ifndef __sensor_base_hpp__
#define __sensor_base_hpp__
#include <cstdint>
#include <fstream>
#include <string>
#include <thread>
#include <utility>

#include <common/data_message/sensor_data.hpp>
#include <common/utils/threadsafe_queue.hpp>


namespace wayz {

using Dictionary = std::pair<std::string, std::string>;

class SensorBase {
public:
    enum struct SensorStatus { error, uninited, terminated, inited, recording, paused };

    SensorBase(const std::string& storage_path, const std::string& sensor_name);
    SensorBase(const SensorBase&) = delete;
    SensorBase& operator=(const SensorBase&) = delete;
    ~SensorBase();

    // Common Functions
    void start_saving();
    void pause_saving();
    void connect_sensor();
    void connect_sensor(const std::vector<Dictionary>& sensor_parameters);
    void disconnect_sensor();
    void start_realtime_forwarding();
    void pause_realtime_forwarding();

    // Sensor Dependent Functions
    virtual bool do_connect_sensor(const std::vector<Dictionary>& sensor_parameters) = 0;
    virtual void do_disconnect_sensor() = 0;
    virtual SensorData* do_sensor_fetch() = 0;
    virtual bool get_sensor_alive();
    virtual bool set_sensor_parameters(const std::vector<Dictionary>& sensor_parameters);

private:
    bool create_storage_folder(const std::string& storage_path, const std::string& sensor_name);
    void create_and_open_storage_file();
    void push_one_data(SensorData* sensor_data_ptr);
    void wait_pop_save_one_data(bool if_save);
    void sensor_storage_thread_function();
    void sensor_fetch_thread_function();

    mutable SensorStatus sensor_status_;
    // mutable bool sensor_realtime_forwarding_;
    std::thread* sensor_fetch_thread_;
    std::thread* sensor_storage_thread_;

    std::string sensor_storage_path_;
    int64_t ofstream_num_count_;
    int64_t ofstream_current_bytecount_;
    //static const int64_t OfstreamMaxSizeByte = 0x7FFFFFFFL;
    static const int64_t OfstreamMaxSizeByte = 0x100L;
    static const int64_t OfstreamFileNameWidth = 4;
    std::ofstream sensor_storage_ofstream_;

    ThreadsafeQueue<SensorData*> sensor_data_queue_;
};

}  // namespace wayz
#endif