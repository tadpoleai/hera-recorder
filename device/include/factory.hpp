/// @file device_factory.hpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Class Factory
/// @version 0.1
/// @date 2019-11-06
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include <functional>
#include <mutex>
#include <string>

#include "device.hpp"

namespace wayz {
namespace hera {
namespace device {

///
/// @brief Factory of class Device
///
class Factory {
public:
    Factory() = delete;

    ///
    /// @brief Load device plugins(dynamic libraries)
    ///
    /// @param load_driver load libraries compiled with physicial driver or not
    /// @param plugins_path path to dynamic libraries
    static void load_plugins(const bool load_driver = false,
                             const std::string& plugins_path = "/usr/local/lib/hera/plugin");

    ///
    /// @brief Get meta info of valid types
    ///
    /// @return std::vector<std::string> const name of valid device types
    ///
    static std::vector<std::string> plugin_types();

    ///
    /// @brief Get meta info of specific type's parameters
    ///
    /// @param vendor_type vendor_type in string
    /// @return rules of parameter in json
    /// parameters
    ///
    static std::string plugin_parameter_rules(const std::string& vendor_type);

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
    static data::SensorDataPtr convert(const data::DeviceDataPtr& data, const ParametersInterface* parameters);

public:
    using CreateFunction = std::function<Device*(const uint32_t id,
                                                 const std::string& vendor_type,
                                                 const std::string& name,
                                                 const bool forward,
                                                 ipc::IPCQueue<data::SensorData>* const ipc_queue,
                                                 storage::StorageManager* const storage)>;
    using ConvertFunction =
            std::function<data::SensorDataPtr(const data::DeviceDataPtr&, const ParametersInterface* parameters)>;

    struct DeviceHandle {
        DeviceVendorType type;
        std::string type_name;
        std::string version;
        CreateFunction create;
        ConvertFunction convert;
        nlohmann::json rules;
    };

private:
    static std::vector<DeviceHandle> device_handles;  ///< Registered device handles

    static std::mutex load_mutex;
    static bool is_loaded;
};

}  // namespace device
}  // namespace hera
}  // namespace wayz