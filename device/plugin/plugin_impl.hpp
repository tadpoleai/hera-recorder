///
/// @file plugin_impl.hpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief Headers for plugins
/// @date 2020-06-18
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include "base/device_types_impl.hpp"
#include "factory.hpp"
#include "sensor_data_types.hpp"

///
/// @brief Use this macro in a device's cpp file to export an plugin for hera device
///
#define HERA_DEVICE_DRIVER_EXPORT_PLUGIN(type_enum_param, type_name_param, class_name_param)                 \
    extern "C" {                                                                                             \
    DevicePtr static_create(const uint32_t id,                                                               \
                            const std::string& vendor_type,                                                  \
                            const std::string& name,                                                         \
                            const bool forward,                                                              \
                            ipc::IPCQueue<data::SensorData>* const ipc_queue,                                \
                            storage::StorageManager* const storage)                                          \
    {                                                                                                        \
        return std::make_unique<class_name_param>(id, vendor_type, name, forward, ipc_queue, storage);       \
    }                                                                                                        \
                                                                                                             \
    Factory::DeviceHandle exports = {.type = DeviceVendorType::type_enum_param,                              \
                                     .type_name = type_name_param,                                           \
                                     .version = __DATE__,                                                    \
                                     .create = static_create,                                                \
                                     .convert = &class_name_param::do_convert,                               \
                                     .essential_parameter_types = class_name_param::EssentialParameterTypes, \
                                     .optional_parameter_types = class_name_param::OptionalParameterTypes};  \
    }
