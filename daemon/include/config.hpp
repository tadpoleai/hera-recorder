///
/// @file config.hpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief Config of daemon
/// @date 2021-03-11
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include <map>
#include <string>
#include <vector>

#include "common/include/logger/logger.hpp"

namespace wayz {
namespace hera {
namespace daemon {

///
/// @brief Config of daemon
///
class Config final {
public:
    ///
    /// @brief Struct of remote server to upload storage file
    ///
    struct UploadServer {
        std::string remark;       ///< Remark(name) in client
        std::string protocol;     ///< Protocol
        std::string destination;  ///< Destination of upload protocol, i.e. ip address or remark of ssh
    };

public:
    static Config read_config(const std::string& config_path);

    bool write_config(const std::string& config_path);

public:
    std::string name{"DefaultName"};  ///< Host name

    std::string data_directory{"/var/hera/data"};        ///< Directory to store record files
    std::string setting_file{"/var/hera/setting.json"};  ///< Setting file( profiles, etc. )

    std::string device_plugin_directory{"/usr/local/lib/hera/plugin"};  ///< Directory to search and load plugins
    std::string upload_plugin_directory{"/usr/local/lib/hera/plugin"};  ///< Directory to search and load plugins

    std::string log_prefix{"/var/hera/logs/hera-daemon"};  ///< Prefix for a new log file
    log::LogLevel log_level{log::LogLevel::Debug};         ///< Only log level above which

    std::string listen_address{"0.0.0.0"};  ///< Host address to listen for daemon
    int32_t listen_port{10093};             ///< Listen port, usually 10093 for release, and 10094 for debug

    bool heartbeat_mode{false};  ///< Heartbeat use include mode
    std::vector<std::string> heartbeat_interfaces;
    double heartbeat_period{5};  ///< Period [seconds]

    bool upload_dynamic{false};  ///< Dynamically find servers (by listening to server heartbeat)
    std::vector<UploadServer> upload_servers;
    std::string localdisk_mountpoint{"/media"};
};


}  // namespace daemon
}  // namespace hera
}  // namespace wayz
