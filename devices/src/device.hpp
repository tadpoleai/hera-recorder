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

#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "common/hera_errno.h"
#include "common/ipc/ipc_queue.hpp"
#include "common/logger/logger.hpp"
#include "device_data.hpp"
#include "storage.hpp"

namespace wayz {
namespace hera {

class Device;
class DeviceFactory;

///
/// @brief A unique pointer to Device
///
using DevicePtr = std::unique_ptr<Device>;

///
/// @brief A Map from DeviceParameterType(enum) to ParameterValue(string)
///
/// Used to in derived classes of Devices
///
using ParametersMap = std::map<DeviceParameterType, std::string>;

///
/// @brief A Map from ParameterType(string) to Value(string)
///
using OutParametersMap = std::map<std::string, std::string>;

/// DeviceId use uint32_t
using DeviceIdType = uint32_t;

///
/// @brief Abstract base class of all devices
///
/// Provides common interfaces and realizes common operations
///
class Device {
private:
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
    /// @param folder parent folder for all devices
    /// @param essential_parameter_types essential parameter types for a specific vendor_type
    /// @param read_mode whether device operates in read mode
    Device(DeviceIdType id,
           const std::string& vendor_type,
           const std::string& name,
           const std::string& folder,
           bool read_mode,
           const size_t history_depth,
           std::initializer_list<DeviceParameterType>&& essential_parameter_types);
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
    HeraErrno record(bool value);

    ///
    /// @todo Forward sensor data by socket
    ///
    // HeraErrno forward(bool value);

    ///
    /// @brief Set or adjust a single parameter
    ///
    /// @param type valid parameter type in string
    /// @see DeviceParameterType in device_type.hpp
    /// @param value paramaeter value
    /// @return HeraErrno operation result
    /// @note Essential parameters should be set before start()
    /// @note Some parameter is immutable after start()
    HeraErrno parameter(const std::string& type, const std::string& value);

    ///
    /// @brief Read a SensorData from device
    ///
    /// @return SensorDataPtr a shared pointer to sensor data, if succeed, otherwise nullptr
    /// @note In read mode, this method is the only valid operation in read mode, it returns
    /// data from storaged data
    SensorDataPtr read();

    ///
    /// @brief Get history Sensor from device
    ///
    /// @return std::vector<SensorDataPtr> a vector of history sensor data
    ///
    /// @note Only valid in normal (record) mode
    std::vector<SensorDataPtr> history();

    ///
    /// @brief Get the vendor type
    ///
    /// @return std::string vendor type in category/vendor
    inline std::string get_type() const noexcept
    {
        return type_;
    }

    ///
    /// @brief Get the device id
    ///
    /// @return DeviceIdType device id
    inline DeviceIdType get_id() const noexcept
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
    /// @brief Get current storage data size
    ///
    /// @return uint64_t data size in bytes
    inline uint64_t get_volume() const noexcept
    {
        return storage_->get_volume();
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
    /// @brief Get the parameters
    ///
    /// @return OutParametersType parameters in map<string, string>
    OutParametersMap get_parameters() const;

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

    ParametersMap parameters_;  ///< Parameters set by define_parameter()
    uint32_t sequence_;         ///< Data sequence, increased by 1 when a valid data comes

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
        return HeraErrno::UnimplementedDriver;
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
    /// @return StorageDataPtr A shared pointer to storage data if a valid data was fetched
    /// successfully, otherwise, A nullptr.
    /// @note Caller must check if return value is nullptr
    /// @note This function should be implemented by derived class.
    /// @note This function will called by fetch_thread_function()
    /// @note This function shoule be a synchronized function, not asynchronized.
    /// If derived device only provides asynchronized interface, await it inside this function.
    /// @note This function must return within a certain timeout, even if no data comes,
    /// otherwise the main thread will be blocked during deconstruction of device.
    virtual StorageDataPtr fetch()
    {
        return nullptr;
    }

    ///
    /// @brief Adjust a single parameter after start()/connect()
    ///
    /// @param type valid parameter type in enum
    /// @see DeviceParameterType in device_type.hpp
    /// @param value paramaeter value
    /// @return HeraErrno operation result
    /// @note Some parameter is immutable during operation
    /// @note This function should be implemented by derived class.
    /// @note This function will be called by parameter()
    virtual HeraErrno adjust_parameter(DeviceParameterType type, const std::string& value)
    {
        return HeraErrno::UnimplementedDriver;
    }

    ///
    /// @brief Convert from storage data to sensor data
    ///
    /// @param storage_data storage data to convert
    /// @return SensorDataPtr shared pointer to converted sensor data if convertion succeed,
    /// otherwise return SensorData::broken_data()
    /// @note This function should be implemented by derived class.
    /// @note Caller must check if return value is broken_data
    /// @note Do not return nullptr in derived implementions
    virtual SensorDataPtr convert(StorageDataPtr& storage_data)
    {
        return SensorData::broken_data();
    }

private:
    ///
    /// @brief Check whether essential parameters of a specific device are all settled
    ///
    /// @return true all essential parameters all settled
    /// @return false some of essential parameters not settled
    bool check_parameter();

    ///
    /// @brief function of fetching thread
    ///
    void fetch_thread_function();

    ///
    /// @brief function of forwarding thread
    ///
    void forward_thread_function();

private:
    const DeviceIdType id_;     ///< device id
    const std::string type_;    ///< device vendor type
    const std::string name_;    ///< device name
    const std::string folder_;  ///< parent folder for all devices

    ///
    /// @brief Device is in read mode
    ///
    /// This determines whether to read data from a storage,
    /// instead of fetching from physical device.
    /// And in this mode, only read() is allowed
    /// @see read()
    const bool read_mode_;

    const size_t history_depth_;  ///< history depth of storage data

    volatile DeviceStatus status_;  ///< device status
    HeraErrno hera_errno_;          ///< device error code
    std::string reason_;            ///< extra error reason

    volatile bool is_record_;    ///< if device is recording
    std::thread* thread_fetch_;  ///< fetching thread
    Storage* storage_;           ///< storager for this device

    const bool is_forward_;                   ///< if device configured to forward
    std::thread* thread_forward_;             ///< forwarding thread
    ThreadQueue<StorageData> forward_queue_;  ///< StorageData(device raw data) queue, ready for forwarding
    decltype(ipc::IPCQueue<SensorData>::create()) ipc_queue_;  ///< IPC queue for sensor data forwarding

    const std::vector<DeviceParameterType> essential_parameter_types_;  ///< essential parameters types
};

}  // namespace hera
}  // namespace wayz