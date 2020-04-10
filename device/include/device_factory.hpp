/// @file device_factory.hpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Class DeviceFactory
/// @version 0.1
/// @date 2019-11-06
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include <string>

#include "device.hpp"

namespace wayz {
namespace hera {
namespace device {

///
/// @brief Factory of class Device
///
class DeviceFactory {
public:
    DeviceFactory() = delete;

    ///
    /// @brief Get meta info of valid types
    ///
    /// @return std::vector<std::string> const name of valid device types
    ///
    static std::vector<std::string> types();

    ///
    /// @brief Get meta info of specific type's parameters
    ///
    /// @param vendor_type vendor_type in string
    /// @return std::pair<std::vector<std::string>, std::vector<std::string>> pair of essential parameters and valid
    /// parameters
    ///
    static std::pair<std::vector<std::string>, std::vector<std::string>> parameter_types(
            const std::string& vendor_type);

    ///
    /// @brief Check vendor_type
    ///
    /// @param vendor_type vendor_type in string
    /// @see DeviceVendorType
    /// @return true argument vendor_type is valid
    /// @return false argument vendor_type is invalid
    ///
    /// Check whether argument vendor_type is valid
    ///
    static bool check_type(const std::string& vendor_type);

    ///
    /// @brief Creating particular derived object of Device
    ///
    /// @param id device id
    /// @param vendor_type vendor_type in string
    /// @see DeviceVendorType
    /// @param name device name
    /// @param forward whether to realtime forward data by ipc
    /// @param ipc_queue ipc queue for forwarding
    /// @param storage pointer to global storage for all devices
    /// @param read_mode operate in read mode or not
    /// @return DevicePtr an unique pointer to Device
    ///
    /// Check vendor_type and create corresponding object
    ///
    static DevicePtr create(const uint32_t id,
                            const std::string& vendor_type,
                            const std::string& name,
                            const bool forward,
                            ipc::IPCQueue<data::SensorData>* const ipc_queue,
                            storage::StorageManager* const storage);

    ///
    /// @brief Convert storaged device data to sensor data
    ///
    /// @param data device data to convert
    /// @return SensorDataPtr converted data if succeed, otherwise broken_data()
    ///
    static data::SensorDataPtr convert(data::DeviceDataPtr& data);

public:
    struct DeviceHandle {
        DeviceVendorType type;
        std::string type_name;
        DevicePtr (*create)(const uint32_t,
                            const std::string&,
                            const std::string&,
                            const bool,
                            ipc::IPCQueue<data::SensorData>* const,
                            storage::StorageManager* const);         ///< Function pointer to create
        data::SensorDataPtr (*do_convert)(data::DeviceDataPtr&);     ///< Function pointer to do_convert
        std::vector<DeviceParameterType> essential_parameter_types;  ///< Essential Parameters for device
        std::vector<DeviceParameterType> optional_parameter_types;   ///< Optional Parameters for device
        bool implemented;                                            ///< Is driver implemented or not
    };

    ///
    /// @brief To register a device
    ///
    /// @note This should be called in vendor device's cpp
    ///
    /// @param device_handle
    /// @return int no use
    ///
    static int register_type(DeviceHandle&& device_handle);

private:
    static std::vector<DeviceHandle> device_handles;  ///< Registered device handles
};

}  // namespace device
}  // namespace hera
}  // namespace wayz