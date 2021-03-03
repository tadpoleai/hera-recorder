/// @file device_factory.cpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Class Factory
/// @version 0.1
/// @date 2019-12-25
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include "factory.hpp"

#include <algorithm>
#include <dlfcn.h>
#include <mutex>

#include "common/include/logger/logger.hpp"
#include "common/include/utils/folder_content.hpp"

namespace wayz {
namespace hera {
namespace device {

std::vector<Factory::DeviceHandle> Factory::device_handles;

bool Factory::is_loaded = false;

std::mutex Factory::load_mutex;

void Factory::load_plugins(const bool load_driver, const std::string& plugins_path)
{
    std::unique_lock<std::mutex> _(load_mutex);

    if (is_loaded) {
        return;
    } else {
        is_loaded = true;
    }

    log::debug << "Factory: Registering Plugins " << (load_driver ? "with" : "without") << " drivers" << log::endl;

    auto load_path = plugins_path;
    if (load_driver) {
        load_path += "/driver";
    } else {
        load_path += "/base";
    }
    auto fs = file::get_folder_content(load_path);

    for (auto&& file : fs.files) {
        auto dll = ::dlopen(file.fullname.c_str(), RTLD_NOW);
        if (!dll) {
            log::warn << "Factory: Can not load library '" << file.fullname << "', since " << ::dlerror() << log::endl;
            continue;
        }

        void* exports = ::dlsym(dll, "exports");
        if (!exports) {
            log::warn << "Factory: Can not read library '" << file.basename << "', since " << ::dlerror() << log::endl;
            continue;
        }

        typedef DeviceHandle (*exportsType)();
        auto device_handle = (reinterpret_cast<exportsType>(exports))();
        device_handles.emplace_back(device_handle);
    }

    for (const auto& device_handle : device_handles) {
        log::debug << "Factory: Registered " << device_handle.type_name << ", built " << device_handle.version
                   << log::endl;
    }
};

std::vector<std::string> Factory::plugin_types()
{
    if (!is_loaded) {
        load_plugins();
    }

    std::vector<std::string> ret;
    for (const auto& device_handle : device_handles) {
        ret.push_back(device_handle.type_name);
    }
    return ret;
}

bool Factory::check_type(const std::string& vendor_type)
{
    if (!is_loaded) {
        load_plugins();
    }

    const auto Types = plugin_types();
    return std::find(Types.begin(), Types.end(), vendor_type) != Types.end();
}

std::string Factory::plugin_description(const std::string& vendor_type)
{
    if (!is_loaded) {
        load_plugins();
    }

    if (!check_type(vendor_type)) {
        log::warn << "Factory::type_parameters: Unknown vendor type: " << vendor_type << log::endl;
        return R"({"comment":"", "label": "", "parameters": []})";
    }

    for (const auto& device_handle : device_handles) {
        if (vendor_type == device_handle.type_name) {
            return device_handle.description;
        }
    }

    return R"({"comment":"", "label": "", "parameters": []})";
}

DevicePtr Factory::create(const uint32_t id,
                          const std::string& vendor_type,
                          const std::string& name,
                          const bool forward,
                          ipc::IPCQueue<data::SensorData>* const ipc_queue,
                          storage::StorageManager* const storage)
{
    if (!is_loaded) {
        load_plugins();
    }

    for (const auto& device_handle : device_handles) {
        if (vendor_type == device_handle.type_name) {
            if (device_handle.create) {
                return DevicePtr(device_handle.create(id, vendor_type, name, forward, ipc_queue, storage));
            } else {
                log::warn << "Factory::create: driver is not loaded" << log::endl;
            }
        }
    }

    log::warn << "Factory::create: Unknown vendor type: " << vendor_type << log::endl;
    return nullptr;
}

data::SensorDataPtr Factory::convert(const data::DeviceDataPtr& data, const ParametersInterface* parameters)
{
    if (!is_loaded) {
        load_plugins();
    }

    if (data == nullptr) {
        log::warn << "Factory::convert: Nullptr device data" << log::endl;
        return data::SensorData::broken_data();
    }

    auto vendor_type = data->get_vendor_type();

    for (const auto& device_handle : device_handles) {
        if (vendor_type == device_handle.type) {
            return device_handle.convert(data, parameters);
            break;
        }
    }

    log::warn << "Factory::convert: Unknown vendor type :" << static_cast<uint16_t>(data->get_vendor_type())
              << log::endl;
    return data::SensorData::broken_data();
}

}  // namespace device
}  // namespace hera
}  // namespace wayz