//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#pragma once

#include <cstdint>
#include <fstream>
#include <map>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <common/data_def/device_types.hpp>
#include <common/third_party/enum.h>
#include <common/tron_errno.h>
#include <common/utils/system_timestamp.hpp>
#include <common/utils/threadsafe_queue.hpp>

namespace wayz {
namespace tron {

class Device {
public:
    Device(int32_t id, const std::string& name);
    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;
    virtual ~Device();

    // Control
    TronErrno start();
    TronErrno stop();
    TronErrno start_record();
    TronErrno pause_record();
    TronErrno enable_forward();
    TronErrno disable_forward();

    // Configure
    TronErrno set_storage(const std::string& folder);
    TronErrno set_parameter(const std::string& type, const std::string& value);
    TronErrno adjust_parameter(const std::string& type, const std::string& value);

    // Status
    virtual DeviceType get_type() const = 0;
    int32_t get_id() const;
    std::string get_name() const;
    std::map<std::string, std::string> get_parameters() const;
    int64_t get_volume() const;
    std::string get_status() const;
    TronErrno get_errno() const;
    std::string get_reason() const;
    bool get_is_record() const;
    bool get_is_forward() const;

protected:
    TronErrno set_error_and_die(TronErrno e, const std::string& reason = "");
    std::map<DeviceParameterType, std::string> parameters_;
    int32_t sequence_;

    // Device Dependent Functions
    virtual TronErrno do_connect() = 0;
    virtual void do_disconnect() = 0;
    virtual std::shared_ptr<DeviceRawData> do_fetch() = 0;
    virtual std::shared_ptr<SensorData> do_convert(
            const std::shared_ptr<DeviceRawData>& rawdata) = 0;
    virtual TronErrno do_adjust_parameter(DeviceParameterType type, const std::string& value) = 0;

private:
    TronErrno create_storage_folder();
    TronErrno open_new_storage_file();

    void fetch_thread_function();
    void storage_thread_function();
    void forward_thread_function();
    bool check_new_data() const;

    int32_t id_;
    std::string name_;
    std::string storage_path_;
    mutable bool is_storage_path_set_;
    mutable int64_t file_number_counter_;
    mutable int64_t file_size_counter_;
    mutable int64_t total_file_size_counter_;
    static const int64_t FileMaxSize_ = 0x7FFFFFFFL;
    static const int64_t FileNameWidth = 4;
    std::ofstream file_;

    volatile DeviceStatus status_;
    mutable TronErrno last_errno_;
    mutable std::string last_errno_reason_;
    mutable TimestampNs last_data_timestamp_ns_;
    static const DurationNs MaxDataDurationNs_ = 3 * OneSecondToNs;
    mutable bool is_record_;
    mutable bool is_forward_;

    std::thread* thread_fetch_;
    std::thread* thread_storage_;
    std::thread* thread_forward_;
    ThreadsafeQueue<std::shared_ptr<DeviceRawData>> queue_storage_;
    ThreadsafeQueue<std::shared_ptr<DeviceRawData>> queue_forward_;
};

}  // namespace tron
}  // namespace wayz