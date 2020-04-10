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

#include "common/include/logger/logger.hpp"

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

std::vector<DeviceFactory::DeviceHandle> DeviceFactory::device_handles;

int DeviceFactory::register_type(DeviceFactory::DeviceHandle&& device_handle)
{
    device_handles.emplace_back(device_handle);
    return 0;
}

std::vector<std::string> DeviceFactory::types()
{
    std::vector<std::string> ret;
    for (const auto& device_handle : device_handles) {
        if (device_handle.implemented) {
            ret.push_back(device_handle.type_name);
        }
    }
    return ret;
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

    for (const auto& device_handle : device_handles) {
        if (vendor_type == device_handle.type_name) {
            essential = device_handle.essential_parameter_types;
            optional = device_handle.optional_parameter_types;
            break;
        }
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
    for (const auto& device_handle : device_handles) {
        if (vendor_type == device_handle.type_name) {
            return (*device_handle.create)(id, vendor_type, name, forward, ipc_queue, storage);
        }
    }

    log::warn << "DeviceFactory::create: Unknown vendor type: " << vendor_type << log::endl;
    return nullptr;
}
#endif

data::SensorDataPtr DeviceFactory::convert(data::DeviceDataPtr& data)
{
    if (data == nullptr) {
        log::warn << "DeviceFactory::convert: Nullptr device data" << log::endl;
        return data::SensorData::broken_data();
    }

    auto vendor_type = data->get_vendor_type();

    for (const auto& device_handle : device_handles) {
        if (vendor_type == device_handle.type) {
            return (*device_handle.do_convert)(data);
            break;
        }
    }

    log::warn << "DeviceFactory::convert: Unknown vendor type :" << static_cast<uint16_t>(data->get_vendor_type())
              << log::endl;
    return data::SensorData::broken_data();
}

}  // namespace device
}  // namespace hera
}  // namespace wayz