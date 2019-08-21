//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#ifndef __sensor_base_hpp__
#define __sensor_base_hpp__
#include <cstdint>
#include <map>
#include <queue>
#include <string>
#include <thread>
#include <mutex>

#include "../../../common/data_message/sensor_data.hpp"

namespace wayz {

class SensorBase {
public:
    using Dictionary = std::map<std::string, std::string>;
    enum struct SensorStatus { stopped, inited, recording, paused };

    SensorBase(const std::string& storage_path, const std::string& sensor_name);
    SensorBase(const SensorBase&) = delete;
    SensorBase& operator=(const SensorBase&) = delete;
    ~SensorBase();

    // Common Functions
    void start_saving();
    void pause_saving();
    void start_realtime_forwarding();
    void pause_realtime_forwarding();

    // Sensor Dependent Functions
    virtual void connect_sensor(std::vector<Dictionary> sensor_parameters) = 0;
    virtual void disconnect_sensor() = 0;
    virtual bool get_sensor_alive() = 0;
    virtual bool set_sensor_parameters(std::vector<Dictionary> sensor_parameters) = 0;

private:
    mutable SensorStatus sensor_status_;
    mutable bool sensor_realtime_forwarding_;
    std::string sensor_storage_path_;
    std::thread* sensor_fetch_thread_;
    std::thread* sensor_storage_thread_;

    bool create_storage(const std::string& storage_path, const std::string& sensor_name);
    bool push_one_data(const std::shared_ptr<SensorData> sensor_data_ptr);
    bool pop_and_save_one_data();
    virtual void sensor_fetch_thread_function() = 0;
    void sensor_storage_thread_function();
};

}  // namespace wayz
#endif