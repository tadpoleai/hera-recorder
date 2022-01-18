///
/// @file plugin_common.hpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief Headers for plugins (for plugin_entry.cpp)
/// @date 2020-06-18
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include "device.hpp"
#include "factory.hpp"
#include "sensor_data_types.hpp"

#define HERA_PLUGIN_DEFINE_START(type_name_param, type_vendor_type_val_param, history_depth_param)              \
    static constexpr std::string_view StaticPluginName{type_name_param};                                        \
    static constexpr DeviceVendorType StaticPluginVendorTypeVal{type_vendor_type_val_param};                    \
                                                                                                                \
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
                                                                                                                \
    private:


#define HERA_PLUGIN_DEFINE_FUNCTIONS              \
    virtual HeraErrno connect() override;         \
    virtual void disconnect() override;           \
    virtual data::DeviceDataPtr fetch() override; \
    virtual HeraErrno adjust_parameter(const std::string& type, const std::string& value) override;

///
/// @brief Use this macro in a device's cpp file to export an plugin for hera device
///
#define HERA_PLUGIN_DEFINE_END                                                       \
    }                                                                                \
    ;                                                                                \
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
        return new LocalParameters();                                                \
    }                                                                                \
                                                                                     \
    Factory::DeviceHandle exports()                                                  \
    {                                                                                \
        return {.type = StaticPluginVendorTypeVal,                                   \
                .type_name = std::string(StaticPluginName),                          \
                .version = __DATE__,                                                 \
                .create = static_create,                                             \
                .convert = &DevicePlugin::do_convert,                                \
                .create_param = static_create_param,                                 \
                .description = LocalParameters::static_description,                  \
                .param_plain_rules = LocalParameters::static_plain_rules};           \
    }                                                                                \
    }

#define HERA_PLUGIN_DATA_DEFINE_START(data_type_name_param, data_type_num)                             \
    _Pragma("pack(push, 1)");                                                                          \
                                                                                                       \
    class data_type_name_param final : public data::DeviceData {                                       \
    public:                                                                                            \
        data_type_name_param() = delete;                                                               \
                                                                                                       \
        static constexpr DeviceDataType TypeVal{data_type_num};                                        \
                                                                                                       \
        static auto create(uint32_t length, uint32_t id, uint32_t sequence)                            \
        {                                                                                              \
            return data::DeviceData::create(length, id, StaticPluginVendorTypeVal, TypeVal, sequence); \
        }                                                                                              \
                                                                                                       \
    public:

#define HERA_PLUGIN_DATA_DEFINE_END \
    }                               \
    ;                               \
    _Pragma("pack(pop)")
