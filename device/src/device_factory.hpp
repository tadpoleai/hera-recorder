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
    /// @param storage pointer to global storage for all devices
    /// @param read_mode operate in read mode or not
    /// @return DevicePtr an unique pointer to Device
    ///
    /// Check vendor_type and create corresponding object
    ///
    static DevicePtr create(const uint32_t id,
                            const std::string& vendor_type,
                            const std::string& name,
                            storage::StorageManager* const storage);

    ///
    /// @brief Convert storaged device data to sensor data
    ///
    /// @param data device data to convert
    /// @return SensorDataPtr converted data if succeed, otherwise broken_data()
    ///
    static data::SensorDataPtr convert(data::DeviceDataPtr& data);
};

}  // namespace device
}  // namespace hera
}  // namespace wayz