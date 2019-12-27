/// @file device_factory.cpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Class DeviceFactory
/// @version 0.1
/// @date 2019-12-25
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include "device_factory.hpp"

#include "common/logger/logger.hpp"

// Device Vendors
#include "camera/flir/flir.hpp"
#include "dummy/foobar/foobar.hpp"
#include "gnss/serialsync/serialsync.hpp"
#include "imu/aceinna/aceinna.hpp"
#include "lidar/velodyne/velodyne.hpp"

namespace wayz {
namespace hera {
namespace device {

bool DeviceFactory::check_type(const std::string& vendor_type)
{
    if (vendor_type.compare("dummy/foobar") == 0 ||    // Foobar
        vendor_type.compare("imu/aceinna") == 0 ||     // Aceinna
        vendor_type.compare("lidar/velodyne") == 0 ||  // Velodyne
        vendor_type.compare("camera/flir") == 0 ||     // Flir
        vendor_type.compare("gnss/serialsync") == 0    // SerialSync
    ) {
        return true;
    }
    return false;
}

DevicePtr DeviceFactory::create(const uint32_t id,
                                const std::string& vendor_type,
                                const std::string& name,
                                storage::StorageManager* const storage)
{
    if (vendor_type.compare("dummy/foobar") == 0) {
        return std::make_unique<dummy::foobar::Foobar>(id, vendor_type, name, storage);
    } else if (vendor_type.compare("imu/aceinna") == 0) {
        return std::make_unique<imu::aceinna::Aceinna>(id, vendor_type, name, storage);
    } else if (vendor_type.compare("lidar/velodyne") == 0) {
        return std::make_unique<lidar::velodyne::Velodyne>(id, vendor_type, name, storage);
    } else if (vendor_type.compare("camera/flir") == 0) {
        return std::make_unique<camera::flir::Flir>(id, vendor_type, name, storage);
    } else if (vendor_type.compare("gnss/serialsync") == 0) {
        return std::make_unique<gnss::serialsync::Serialsync>(id, vendor_type, name, storage);
    }

    log::warn << "DeviceFactory::create: Unknown vendor type : " << vendor_type << log::endl;
    return nullptr;
}

data::SensorDataPtr DeviceFactory::convert(data::DeviceDataPtr& data)
{
    if (data == nullptr) {
        log::warn << "DeviceFactory::convert: Nullptr device data" << log::endl;
        return data::SensorData::broken_data();
    }

    switch (data->get_vendor_type()) {
    case DeviceVendorType::DummyFoobar:
        return dummy::foobar::Foobar::do_convert(data);
    case DeviceVendorType::ImuAceinna:
        return imu::aceinna::Aceinna::do_convert(data);
    case DeviceVendorType::GnssSerialsync:
        return gnss::serialsync::Serialsync::do_convert(data);
    case DeviceVendorType::CameraFlir:
        return camera::flir::Flir::do_convert(data);
    case DeviceVendorType::LidarVelodyne:
        return lidar::velodyne::Velodyne::do_convert(data);
    default:
        log::warn << "DeviceFactory::convert: Unknown vendor type :" << static_cast<uint16_t>(data->get_vendor_type())
                  << log::endl;
        return data::SensorData::broken_data();
    }
}


}  // namespace device
}  // namespace hera
}  // namespace wayz