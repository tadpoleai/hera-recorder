/// @file device_factory.cpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Class DeviceFactory
/// @version 0.1
/// @date 2019-12-25
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include "device_factory.hpp"

#include <algorithm>

#include "common/logger/logger.hpp"

// Device Vendors
#include "camera/flir/flir.hpp"
#include "camera/s32vmipi/s32vmipi.hpp"
#include "dummy/foobar/foobar.hpp"
#include "gnss/serialsync/serialsync.hpp"
#include "imu/aceinna/aceinna.hpp"
#include "lidar/velodyne/velodyne.hpp"

namespace wayz {
namespace hera {
namespace device {

std::vector<std::string> DeviceFactory::types()
{
    return {"dummy/foobar", "imu/aceinna", "lidar/velodyne", "camera/flir", "camera/s32vmipi", "gnss/serialsync"};
}

bool DeviceFactory::check_type(const std::string& vendor_type)
{
    const auto Types = types();
    return std::find(Types.begin(), Types.end(), vendor_type) != Types.end();
}

std::pair<std::vector<std::string>, std::vector<std::string>> DeviceFactory::parameter_types(
        const std::string& vendor_type)
{
    if (!check_type(vendor_type)) {
        return {};
        log::warn << "DeviceFactory::type_parameters: Unknown vendor type: " << vendor_type << log::endl;
    }

    std::vector<DeviceParameterType> essential;
    std::vector<DeviceParameterType> optional;

    decltype(parameter_types(std::string())) ret;

    if (vendor_type.compare("dummy/foobar") == 0) {
        essential = dummy::foobar::Foobar::EssentialParameterTypes;
        optional = dummy::foobar::Foobar::OptionalParameterTypes;
    } else if (vendor_type.compare("imu/aceinna") == 0) {
        essential = imu::aceinna::Aceinna::EssentialParameterTypes;
        optional = imu::aceinna::Aceinna::OptionalParameterTypes;
    } else if (vendor_type.compare("lidar/velodyne") == 0) {
        essential = lidar::velodyne::Velodyne::EssentialParameterTypes;
        optional = lidar::velodyne::Velodyne::OptionalParameterTypes;
    } else if (vendor_type.compare("camera/flir") == 0) {
        essential = camera::flir::Flir::EssentialParameterTypes;
        optional = camera::flir::Flir::OptionalParameterTypes;
    } else if (vendor_type.compare("camera/s32vmipi") == 0) {
        essential = camera::s32vmipi::S32VMipi::EssentialParameterTypes;
        optional = camera::s32vmipi::S32VMipi::OptionalParameterTypes;
    } else if (vendor_type.compare("gnss/serialsync") == 0) {
        essential = gnss::serialsync::Serialsync::EssentialParameterTypes;
        optional = gnss::serialsync::Serialsync::OptionalParameterTypes;
    }

    for (const auto& type : essential) {
        ret.first.emplace_back(type._to_string());
    }
    for (const auto& type : optional) {
        ret.second.emplace_back(type._to_string());
    }
    return ret;
}

#ifdef WITH_DRIVER
DevicePtr DeviceFactory::create(const uint32_t id,
                                const std::string& vendor_type,
                                const std::string& name,
                                const bool forward,
                                ipc::IPCQueue<data::SensorData>* const ipc_queue,
                                storage::StorageManager* const storage)
{
    if (vendor_type.compare("dummy/foobar") == 0) {
        return std::make_unique<dummy::foobar::Foobar>(id, vendor_type, name, forward, ipc_queue, storage);
    } else if (vendor_type.compare("imu/aceinna") == 0) {
        return std::make_unique<imu::aceinna::Aceinna>(id, vendor_type, name, forward, ipc_queue, storage);
    } else if (vendor_type.compare("lidar/velodyne") == 0) {
        return std::make_unique<lidar::velodyne::Velodyne>(id, vendor_type, name, forward, ipc_queue, storage);
    } else if (vendor_type.compare("camera/flir") == 0) {
        return std::make_unique<camera::flir::Flir>(id, vendor_type, name, forward, ipc_queue, storage);
    } else if (vendor_type.compare("camera/s32vmipi") == 0) {
        return std::make_unique<camera::s32vmipi::S32VMipi>(id, vendor_type, name, forward, ipc_queue, storage);
    } else if (vendor_type.compare("gnss/serialsync") == 0) {
        return std::make_unique<gnss::serialsync::Serialsync>(id, vendor_type, name, forward, ipc_queue, storage);
    }

    log::warn << "DeviceFactory::create: Unknown vendor type : " << vendor_type << log::endl;
    return nullptr;
}
#endif

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
    case DeviceVendorType::CameraS32VMipi:
        return camera::s32vmipi::S32VMipi::do_convert(data);
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