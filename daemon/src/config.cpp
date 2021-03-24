///
/// @file daemon.cpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Main process of Acquistion Daemon
/// @version 0.1
/// @date 2019-11-08
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include "config.hpp"

#include <libconfig.h++>

namespace wayz {
namespace hera {
namespace daemon {

Config Config::read_config(const std::string& config_path)
{
    Config ret;
    libconfig::Config config;

    try {
        config.readFile(config_path.c_str());
    } catch (libconfig::FileIOException& e) {
        std::cout << "Error: can not read file '" << config_path << "', use default" << std::endl;
        ret.write_config(config_path);
        return ret;
    } catch (libconfig::ParseException& e) {
        std::cout << "Error: can not read file '" << config_path << "', since line " << e.getLine() << ", errored "
                  << e.getError() << ", use default" << std::endl;
        return ret;
    } catch (std::exception& e) {
        std::cout << "Error: can not read file '" << config_path << "', since " << e.what() << ", use default"
                  << std::endl;
        return ret;
    }

    if (!config.lookupValue("name", ret.name)) {
        std::cout << "Error: can not parse "
                  << "[name]"
                  << " from " << config_path << std::endl;
    } else {
        std::cout << "Config: name = " << ret.name << std::endl;
    }

    if (!config.lookupValue("data_directory", ret.data_directory)) {
        std::cout << "Error: can not parse "
                  << "[data_directory]"
                  << " from " << config_path << std::endl;
    } else {
        std::cout << "Config: data_directory = " << ret.data_directory << std::endl;
    }

    if (!config.lookupValue("setting_file", ret.setting_file)) {
        std::cout << "Error: can not parse "
                  << "[setting_file]"
                  << " from " << config_path << std::endl;
    } else {
        std::cout << "Config: setting_file = " << ret.setting_file << std::endl;
    }

    if (!config.lookupValue("plugin_directory/device", ret.device_plugin_directory)) {
        std::cout << "Error: can not parse "
                  << "[plugin_directory/device]"
                  << " from " << config_path << std::endl;
    } else {
        std::cout << "Config: device_plugin_directory = " << ret.device_plugin_directory << std::endl;
    }

    if (!config.lookupValue("plugin_directory/upload", ret.upload_plugin_directory)) {
        std::cout << "Error: can not parse "
                  << "[plugin_directory/upload]"
                  << " from " << config_path << std::endl;
    } else {
        std::cout << "Config: upload_plugin_directory = " << ret.upload_plugin_directory << std::endl;
    }

    if (!config.lookupValue("log/prefix", ret.log_prefix)) {
        std::cout << "Error: can not parse "
                  << "[log/prefix]"
                  << " from " << config_path << std::endl;
    } else {
        std::cout << "Config: log/prefix = " << ret.log_prefix << std::endl;
    }

    std::string log_level_str;
    if (!config.lookupValue("log/level", log_level_str)) {
        std::cout << "Error: can not parse "
                  << "[log/level]"
                  << " from " << config_path << std::endl;
    } else {
        std::cout << "Config: log_level = " << log_level_str << std::endl;
    }
    if (log_level_str == "debug") {
        ret.log_level = log::LogLevel::Debug;
    } else if (log_level_str == "info") {
        ret.log_level = log::LogLevel::Info;
    } else if (log_level_str == "warn") {
        ret.log_level = log::LogLevel::Warn;
    } else if (log_level_str == "error") {
        ret.log_level = log::LogLevel::Error;
    } else {
        std::cout << "Warn: can not parse " << log_level_str << " to log level, use default = debug" << std::endl;
    }

    if (!config.lookupValue("listen/address", ret.listen_address)) {
        std::cout << "Error: can not parse "
                  << "[listen/address]"
                  << " from " << config_path << std::endl;
    } else {
        std::cout << "Config: listen_addresss = " << ret.listen_address << std::endl;
    }

    if (!config.lookupValue("listen/port", ret.listen_port)) {
        std::cout << "Error: can not parse "
                  << "[listen/port]"
                  << " from " << config_path << std::endl;
    } else {
        std::cout << "Config: listen_port = " << ret.listen_port << std::endl;
    }

    if (!config.lookupValue("heartbeat/mode", ret.heartbeat_mode)) {
        std::cout << "Error: can not parse "
                  << "[heartbeat/mode]"
                  << " from " << config_path << std::endl;
    } else {
        std::cout << "Config: heartbeat_mode = " << (ret.heartbeat_mode ? "include" : "exclude") << std::endl;
    }

    try {
        auto& interfaces = config.lookup("heartbeat/interfaces");
        for (auto& entry : interfaces) {
            std::string entry_str = entry;
            std::cout << "Config: Interface " << entry_str << std::endl;
            ret.heartbeat_interfaces.emplace_back(std::move(entry_str));
        }
    } catch (...) {
        std::cout << "Error: can not parse "
                  << "[heartbeat/interfaces]"
                  << " from " << config_path << std::endl;
    }

    if (!config.lookupValue("heartbeat/period", ret.heartbeat_period)) {
        std::cout << "Error: can not parse "
                  << "[heartbeat/period]"
                  << " from " << config_path << std::endl;
    } else {
        std::cout << "Config: heartbeat_period = " << ret.heartbeat_period << std::endl;
    }

    if (!config.lookupValue("upload/dynamic", ret.upload_dynamic)) {
        std::cout << "Error: can not parse "
                  << "[upload/dynamic]"
                  << " from " << config_path << std::endl;
    } else {
        std::cout << "Config: upload_dynamic = " << (ret.upload_dynamic ? "true" : "false") << std::endl;
    }

    try {
        auto& servers = config.lookup("upload/servers");
        for (auto& entry : servers) {
            auto server = Config::UploadServer{.remark = entry["remark"],
                                               .protocol = entry["protocol"],
                                               .destination = entry["destination"]};

            std::cout << "Config: UploadServer " << server.remark << ", " << server.protocol << std::endl;

            ret.upload_servers.emplace_back(std::move(server));
        }
    } catch (...) {
        std::cout << "Error: can not parse "
                  << "[upload/servers]"
                  << " from " << config_path << std::endl;
    }

    if (!config.lookupValue("upload/localdisk_mountpoint", ret.localdisk_mountpoint)) {
        std::cout << "Error: can not parse "
                  << "[upload/localdisk_mountpoint]"
                  << " from " << config_path << std::endl;
    } else {
        std::cout << "Config: localdisk_mountpoint = " << ret.localdisk_mountpoint << std::endl;
    }

    return ret;
}

bool Config::write_config(const std::string& config_path)
{
    libconfig::Config config;

    config.setOptions(0x11);

    auto& setting = config.getRoot();
    setting.add("name", libconfig::Setting::TypeString) = name;
    setting.add("data_directory", libconfig::Setting::TypeString) = data_directory;
    setting.add("setting_file", libconfig::Setting::TypeString) = setting_file;

    auto& plugin_directory = setting.add("plugin_directory", libconfig::Setting::TypeGroup);
    plugin_directory.add("device", libconfig::Setting::TypeString) = device_plugin_directory;
    plugin_directory.add("upload", libconfig::Setting::TypeString) = upload_plugin_directory;

    auto& log = setting.add("log", libconfig::Setting::TypeGroup);
    log.add("prefix", libconfig::Setting::TypeString) = log_prefix;
    log.add("level", libconfig::Setting::TypeString) = log_level.to_config_string();

    auto& listen = setting.add("listen", libconfig::Setting::TypeGroup);
    listen.add("address", libconfig::Setting::TypeString) = listen_address;
    listen.add("port", libconfig::Setting::TypeInt) = listen_port;

    auto& heartbeat = setting.add("heartbeat", libconfig::Setting::TypeGroup);
    heartbeat.add("mode", libconfig::Setting::TypeBoolean) = heartbeat_mode;
    auto& interfaces = heartbeat.add("interfaces", libconfig::Setting::TypeArray);
    for (auto& i : heartbeat_interfaces) {
        interfaces.add(libconfig::Setting::TypeString) = i;
    }
    heartbeat.add("period", libconfig::Setting::TypeFloat) = heartbeat_period;

    auto& upload = setting.add("upload", libconfig::Setting::TypeGroup);
    upload.add("dynamic", libconfig::Setting::TypeBoolean) = upload_dynamic;
    auto& servers = upload.add("servers", libconfig::Setting::TypeArray);
    for (auto& i : upload_servers) {
        auto& server = servers.add(libconfig::Setting::TypeGroup);
        server.add("remark", libconfig::Setting::TypeString) = i.remark;
        server.add("protocol", libconfig::Setting::TypeString) = i.protocol;
        server.add("destination", libconfig::Setting::TypeString) = i.destination;
    }

    config.writeFile(config_path.c_str());

    return true;
}

}  // namespace daemon
}  // namespace hera
}  // namespace wayz
