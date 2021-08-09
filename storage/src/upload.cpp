///
/// @file upload.cpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief Base and Factory of class upload::Manager
/// @date 2020-05-13
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include "upload.hpp"

#include <algorithm>
#include <dlfcn.h>
#include <mutex>

#include "common/include/logger/logger.hpp"


namespace wayz {
namespace hera {
namespace storage {
namespace upload {

std::vector<Transmission::PluginEntry> Transmission::plugin_entries_;
bool Transmission::is_loaded_ = false;
std::mutex Transmission::load_mutex_;
std::mutex Transmission::upload_mutex_;

void Transmission::load_plugins(const std::string& plugins_path)
{
    std::unique_lock<std::mutex> _(load_mutex_);

    if (is_loaded_) {
        return;
    } else {
        is_loaded_ = true;
    }

    log::debug << "Transmission: Registering Upload Plugins" << log::endl;

    auto load_path = plugins_path + "/upload";
    auto fs = file::get_folder_content(load_path);

    for (auto&& file : fs.files) {
        auto dll = ::dlopen(file.fullname.c_str(), RTLD_NOW);
        if (!dll) {
            log::warn << "Transmission: Can not load library '" << file.fullname << "', since " << ::dlerror()
                      << log::endl;
            continue;
        }

        void* exports = ::dlsym(dll, "exports");
        if (!exports) {
            log::warn << "Transmission: Can not read library '" << file.basename << "', since " << ::dlerror()
                      << log::endl;
            continue;
        }

        typedef PluginEntry (*exportsType)();
        auto plugin_entry = (reinterpret_cast<exportsType>(exports))();
        plugin_entries_.emplace_back(plugin_entry);
    }

    for (const auto& plugin_entry : plugin_entries_) {
        log::debug << "Transmission: Registered " << plugin_entry.name << log::endl;
    }
};

std::unique_ptr<Transmission> Transmission::create(const Config& config)
{
    if (!is_loaded_) {
        load_plugins();
    }

    for (const auto& plugin : plugin_entries_) {
        if (config.protocol == plugin.name) {
            log::debug << "Transmission: New " << plugin.name << " Upload Created" << log::endl;
            return std::unique_ptr<Transmission>(plugin.ctor(config));
        }
    }

    log::warn << "Transmission: Can not create " << config.protocol << log::endl;

    auto ret = new Transmission();
    ret->config_ = config;
    ret->set_error("No such protocol: '" + config.protocol + "'");

    return std::unique_ptr<Transmission>(ret);
}

void Transmission::set_error(const std::string& reason)
{
    status_.stage = Stage::Error;
    status_.error_reason += reason + " \n ";

    log::error << "Transmission: Error, " << reason << log::endl;
}

}  // namespace upload
}  // namespace storage
}  // namespace hera
}  // namespace wayz
