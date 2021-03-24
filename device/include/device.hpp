///
/// @file device.hpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Header of class Device
/// @version 0.1
/// @date 2019-11-06
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include <atomic>
#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

#ifdef HERA_COMPILE_IN_REPO
#include "common/include/hera_errno.h"
#include "common/include/ipc/ipc_queue.hpp"
#include "common/include/logger/logger.hpp"
#include "storage/include/storage.hpp"
#else
#include <hera/common/hera_errno.h>
#include <hera/common/ipc/ipc_queue.hpp>
#include <hera/common/logger/logger.hpp>
#include <hera/storage/storage.hpp>
#endif

#include "device_data.hpp"
#include "display_data.hpp"
#include "parameter.hpp"
#include "sensor_data.hpp"

namespace wayz {
namespace hera {
namespace device {

class Device;

class Factory;

///
/// @brief A unique pointer to Device
///
using DevicePtr = std::unique_ptr<Device>;

///
/// @brief A Map from ParameterType(string) to Value(string)
///
using ParametersMap = std::map<std::string, std::string>;

///
/// @brief Abstract base class of all devices
///
/// Provides common interfaces and realizes common operations
///
class Device {
public:
    ///
    /// @brief Indicating a Device's status
    ///
    enum class DeviceStatus : int32_t {
        BeforeConnect = 0,  ///< Constructed, ready for start()->connect() calling
        Connected = 1,      ///< Started and No error occured
        Terminated = 2,     ///< After stop()->disconnect() called, ready for deconstruction
        Error = 3           ///< Some error occured
    };

public:
    ///
    /// @brief Construct a new Device object
    ///
    /// @param id device_id, from 0
    /// @param vendor_type valid vendor_type from DeviceVendorType,
    /// in slash/case, i.e, category/vendor,
    /// e.g., lidar/velodyne, camera/flir
    /// @param name device name,
    /// @param forward whether to realtime forward data by ipc
    /// @param ipc_queue ipc queue for forwarding
    /// @param storage pointer to global storage for all devices
    /// @param essential_parameter_types essential parameter types for a specific vendor_type
    Device(const uint32_t id,
           const std::string& vendor_type,
           const std::string& name,
           const bool forward,
           ipc::IPCQueue<data::SensorData>* const ipc_queue,
           storage::StorageManager* const storage,
           const size_t history_depth,
           ParametersInterface* const parameter);
    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;

    ///
    /// @brief Destroy the Device object
    ///
    /// @note Derived Dtor should call stop()
    /// since abstract function disconnect(), which will be called in stop(),
    /// need a v-table that will be destroyed before ~Device()
    virtual ~Device();

    ///
    /// @brief Connect and start fetching data
    ///
    /// @return HeraErrno operation result
    HeraErrno start();

    ///
    /// @brief Stop fetching data and disconnect
    ///
    /// @note This should be called in derived class's dtor,
    /// since abstract function disconnect(), which called in stop(),
    /// need a v-table that would be destroyed before ~Device()
    void stop();

    ///
    /// @brief Start record or pause record
    ///
    /// @param value true to start record
    /// @param value false to pause record
    /// @return HeraErrno operation result
    HeraErrno record(const bool value);

    ///
    /// @todo Forward sensor data by socket
    ///
    // HeraErrno forward(bool value);

    ///
    /// @brief Set or adjust a single parameter
    ///
    /// @param type valid parameter type in string
    /// @param value paramaeter value
    /// @return HeraErrno operation result
    /// @note Essential parameters should be set before start()
    /// @note Some parameter is immutable after start()
    HeraErrno parameter(const std::string& type, const std::string& value);

    ///
    /// @brief Get the vendor type
    ///
    /// @return std::string vendor type in category/vendor
    inline std::string get_vendor_type() const noexcept
    {
        return vendor_type_;
    }

    ///
    /// @brief Get the device id
    ///
    /// @return uint32_t device id
    inline uint32_t get_id() const noexcept
    {
        return id_;
    }

    ///
    /// @brief Get the device name object
    ///
    /// @return std::string device name
    inline std::string get_name() const noexcept
    {
        return name_;
    }

    ///
    /// @brief Get current device data size
    ///
    /// @return uint64_t data size in bytes
    inline uint64_t get_volume() const
    {
        return storage_->get_volume(id_);
    }

    ///
    /// @brief Get the current status of device
    ///
    /// @return DeviceStatus device status
    inline DeviceStatus get_status() const noexcept
    {
        return status_;
    }

    ///
    /// @brief Get the current error code
    ///
    /// @return HeraErrno error code
    inline HeraErrno get_errno() const noexcept
    {
        return hera_errno_;
    }

    ///
    /// @brief Get the extra reason of error
    ///
    /// @return std::string reason of error
    inline std::string get_reason() const noexcept
    {
        return reason_;
    }

    ///
    /// @brief Get if device is recording
    ///
    /// @return true device is recording
    /// @return false device is not recording
    inline bool get_record() const noexcept
    {
        return is_record_;
    }

    ///
    /// @brief Get if device is forwarding sensor data (using IPC)
    ///
    /// @return true device is forwarding sensor data
    /// @return false device is not forwarding sensor data
    ///
    inline bool get_forward() const noexcept
    {
        return is_forward_;
    }

    ///
    /// @brief Get device's data sequence
    ///
    /// @return sequence
    ///
    inline auto get_sequence() const noexcept
    {
        return sequence_;
    }

    ///
    /// @brief Get device's health
    ///
    /// @return std::string health message
    std::string get_health() const;

    ///
    /// @brief Get device's status message
    ///
    /// @return status message in string(non-data message to display)
    inline auto get_status_message() const noexcept
    {
        return status_message_;
    }

    ///
    /// @brief Update and get the disp data object
    ///
    /// @param is_detail_disp_data show detail
    /// @return updated disp_data
    ///
    inline auto update_get_disp_data(const bool is_detail_disp_data = false)
    {
        disp_data_->update_from(history(), is_detail_disp_data);
        return disp_data_;
    }

    ///
    /// @brief Get the parameters
    ///
    /// @return ParametersType parameters in map<string, string>
    nlohmann::json get_parameters_json() const
    {
        if (parameters_) {
            return parameters_->dump();
        } else {
            return nlohmann::json::object();
        }
    }

protected:
    ///
    /// @brief Error handling function called by derived classes
    ///
    /// @param e error code
    /// @param reason optional extra reason
    /// @return HeraErrno error code
    /// @note This function will set status to DeviceStatus::Error
    /// @note Typical usage is return handler_error(...) in connect()
    HeraErrno handle_error(HeraErrno e, std::string&& reason = "");

    ///
    /// @brief Replace status message
    ///
    /// @param value
    void set_status_message(std::string&& value)
    {
        status_message_ = std::forward<std::string>(value);
    }

protected:
    ///
    /// @brief Connect to derived device
    ///
    /// @return HeraErrno operation result
    /// @note This function should be implemented by derived class.
    /// @note This function will called by start();
    /// @see start()
    virtual HeraErrno connect()
    {
        return handle_error(HeraErrno::UnimplementedDriver, "Driver is not loaded");
    }

    ///
    /// @brief Disconnect from derived device
    ///
    /// @return HeraErrno operation result
    /// @note This function should be implemented by derived class.
    /// @note This function will called by stop();
    virtual void disconnect() {}

    ///
    /// @brief Fetch from d device
    ///
    /// @return DeviceDataPtr A shared pointer to device data if a valid data was fetched
    /// successfully, otherwise, A nullptr.
    /// @note Caller must check if return value is nullptr
    /// @note This function should be implemented by derived class.
    /// @note This function will called by fetch_thread_function()
    /// @note This function shoule be a synchronized function, not asynchronized.
    /// If derived device only provides asynchronized interface, await it inside this function.
    /// @note This function must return within a certain timeout, even if no data comes,
    /// otherwise the main thread will be blocked during deconstruction of device.
    virtual data::DeviceDataPtr fetch()
    {
        return nullptr;
    }

    ///
    /// @brief Adjust a single parameter after start()/connect()
    ///
    /// @param type valid parameter type in enum
    /// @param value paramaeter value
    /// @return HeraErrno operation result
    /// @note Some parameter is immutable during operation
    /// @note This function should be implemented by derived class.
    /// @note This function will be called by parameter()
    virtual HeraErrno adjust_parameter(const std::string& type, const std::string& value)
    {
        return handle_error(HeraErrno::UnimplementedDriver, "Driver is not loaded");
    }

    ///
    /// @brief Convert from device data to sensor data
    ///
    /// @param storage_data device data to convert
    /// @return SensorDataPtr shared pointer to converted sensor data if convertion succeed,
    /// otherwise return SensorData::broken_data()
    /// @note This function should be implemented by derived class.
    /// @note Caller must check if return value is broken_data
    /// @note Do not return nullptr in derived implementions
    virtual data::SensorDataPtr convert(const data::DeviceDataPtr& storage_data)
    {
        return data::SensorData::broken_data();
    }

protected:
    ///
    /// @brief Clear history of display data
    ///
    void clear_display_history();

private:
    ///
    /// @brief Check whether parameters of a specific device are all settled
    ///
    /// @return true all parameters all settled
    /// @return false some of parameters not settled
    bool check_parameter();

    ///
    /// @brief function of fetching thread
    ///
    void fetch_thread_function();

    ///
    /// @brief function of forwarding thread
    ///
    void forward_thread_function();

    ///
    /// @brief Get history Sensor from device
    ///
    /// @return std::vector<SensorDataPtr> a vector of history sensor data
    ///
    std::vector<data::SensorDataPtr> history();

protected:
    const uint32_t id_;              ///< device id
    const std::string vendor_type_;  ///< device vendor type
    const std::string name_;         ///< device name

    ParametersInterface* const parameters_;
    uint32_t sequence_;  ///< Data sequence, increased by 1 when a valid data comes

private:
    const size_t history_depth_;  ///< history depth of device data

    std::atomic<DeviceStatus> status_;  ///< device status
    HeraErrno hera_errno_;              ///< device error code
    std::string reason_;                ///< extra error reason

    time::Timestamp last_data_recvtime_;
    std::string status_message_;  ///< non-data message

    std::atomic<bool> is_record_;             ///< if device is recording
    std::thread* thread_fetch_;               ///< fetching thread
    storage::StorageManager* const storage_;  ///< global storage manager

    const bool is_forward_;                                ///< if device configured to forward
    std::thread* thread_forward_;                          ///< forwarding thread
    common::ThreadQueue<data::DeviceData> forward_queue_;  ///< DeviceData(device raw data) queue, ready for forwarding
    ipc::IPCQueue<data::SensorData>* ipc_queue_;           ///< IPC queue for sensor data forwarding

    const std::shared_ptr<data::DisplayData> disp_data_;  ///< Pointer to display data;
};

}  // namespace device
}  // namespace hera
}  // namespace wayz