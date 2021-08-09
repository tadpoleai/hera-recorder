///
/// @file plugin_common.hpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief Headers for plugins (for plugin_entry.cpp)
/// @date 2020-06-18
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include "base/device_types_impl.hpp"
#include "device.hpp"
#include "factory.hpp"
#include "sensor_data_types.hpp"

#define HERA_PLUGIN_DEFINE_START(history_depth_param)                                                           \
    class DevicePlugin final : public Device {                                                                  \
    public:                                                                                                     \
        DevicePlugin(const uint32_t id,                                                                         \
                     const std::string& vendor_type,                                                            \
                     const std::string& name,                                                                   \
                     const bool forward,                                                                        \
                     ipc::IPCQueue<data::SensorData>* const ipc_queue,                                          \
                     storage::StorageManager* const storage) :                                                  \
            Device(id, vendor_type, name, forward, ipc_queue, storage, history_depth_param, &local_parameters_) \
        {}                                                                                                      \
                                                                                                                \
        DevicePlugin(const DevicePlugin&) = delete;                                                             \
        DevicePlugin& operator=(const DevicePlugin&) = delete;                                                  \
                                                                                                                \
        virtual ~DevicePlugin()                                                                                 \
        {                                                                                                       \
            stop();                                                                                             \
        }                                                                                                       \
                                                                                                                \
        data::SensorDataPtr convert(const data::DeviceDataPtr& storage_data) override                           \
        {                                                                                                       \
            return do_convert(storage_data, &local_parameters_);                                                \
        }                                                                                                       \
                                                                                                                \
        static data::SensorDataPtr do_convert(const data::DeviceDataPtr& storage_data,                          \
                                              const ParametersInterface* parameters);                           \
                                                                                                                \
    private:                                                                                                    \
        LocalParameters local_parameters_;                                                                      \
                                                                                                                \
    private:

#define HERA_PLUGIN_DEFINE_FUNCTIONS              \
    virtual HeraErrno connect() override;         \
    virtual void disconnect() override;           \
    virtual data::DeviceDataPtr fetch() override; \
    virtual HeraErrno adjust_parameter(const std::string& type, const std::string& value) override;

#define HERA_PLUGIN_DEFINE_END \
    }                          \
    ;

///
/// @brief Use this macro in a device's cpp file to export an plugin for hera device
///
#define HERA_PLUGIN_EXPORT(type_enum_param, type_name_param)                         \
    extern "C" {                                                                     \
    Device* static_create(const uint32_t id,                                         \
                          const std::string& vendor_type,                            \
                          const std::string& name,                                   \
                          const bool forward,                                        \
                          ipc::IPCQueue<data::SensorData>* const ipc_queue,          \
                          storage::StorageManager* const storage)                    \
    {                                                                                \
        return new DevicePlugin(id, vendor_type, name, forward, ipc_queue, storage); \
    }                                                                                \
                                                                                     \
    ParametersInterface* static_create_param()                                       \
    {                                                                                \
        return new LocalParameters();                                                 \
    }                                                                                \
                                                                                     \
    Factory::DeviceHandle exports()                                                  \
    {                                                                                \
        return {.type = DeviceVendorType::type_enum_param,                           \
                .type_name = type_name_param,                                        \
                .version = __DATE__,                                                 \
                .create = static_create,                                             \
                .convert = &DevicePlugin::do_convert,                                \
                .create_param = static_create_param,                                 \
                .description = LocalParameters::static_description,                  \
                .param_plain_rules = LocalParameters::static_plain_rules};            \
    }                                                                                \
    }
