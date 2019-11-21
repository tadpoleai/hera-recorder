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

// Device Base
#include "device.hpp"

// Device Vendors
#include "camera/flir/flir.hpp"
#include "dummy/foobar/foobar.hpp"
#include "imu/aceinna/aceinna.hpp"
#include "lidar/velodyne/velodyne.hpp"

namespace wayz {
namespace hera {

///
/// @brief Factory of class Device
///
class DeviceFactory {
public:
    DeviceFactory() = delete;

    ///
    /// @brief Creating particular derived object of Device
    ///
    /// @param id device id
    /// @param vendor_type vendor_type in string
    /// @see DeviceVendorType
    /// @param name device name
    /// @param folder parent folder of acquisition
    /// @param read_mode operate in read mode or not
    /// @return DevicePtr an unique pointer to Device
    ///
    /// Check vendor_type and create corresponding object
    ///
    static DevicePtr create(DeviceIdType id,
                            const std::string& vendor_type,
                            const std::string& name,
                            const std::string& folder,
                            bool read_mode = false)
    {
        if (vendor_type.compare("dummy/foobar") == 0) {
            return std::make_unique<dummy::Foobar>(id, vendor_type, name, folder, read_mode);
        } else if (vendor_type.compare("imu/aceinna") == 0) {
            return std::make_unique<imu::Aceinna>(id, vendor_type, name, folder, read_mode);
        } else if (vendor_type.compare("lidar/velodyne") == 0) {
            return std::make_unique<lidar::Velodyne>(id, vendor_type, name, folder, read_mode);
        } else if (vendor_type.compare("camera/flir") == 0) {
            return std::make_unique<camera::Flir>(id, vendor_type, name, folder, read_mode);
        }
        return nullptr;
    }

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
    static bool check_type(const std::string& vendor_type)
    {
        if (vendor_type.compare("dummy/foobar") == 0 ||    // Foobar
            vendor_type.compare("imu/aceinna") == 0 ||     // Aceinna
            vendor_type.compare("lidar/velodyne") == 0 ||  // Velodyne
            vendor_type.compare("camera/flir") == 0        // Flir
        ) {
            return true;
        }
        return false;
    }
};

}  // namespace hera
}  // namespace wayz