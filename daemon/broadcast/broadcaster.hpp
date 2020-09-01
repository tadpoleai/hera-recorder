///
/// @file broadcast.hpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief Broadcast self's ip and info by UDP
/// @date 2020-05-27
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include <atomic>
#include <string>
#include <thread>
#include <vector>

namespace wayz {
namespace hera {
namespace daemon {

class Broadcaster final {
public:
    ///
    /// @brief Construct a new Broadcast object
    ///
    /// @param ifs List of names of interfaces to specify
    /// @param whilelist_mode [true] Use ifs as whitelist, or blacklist
    ///
    /// @param protocol protocol of upload server, only for upload server
    /// @param parameter parameter for protocol of upload server, only for upload server
    /// @param is_upload_server [false] for hera-daemon, [true] for upload server
    ///
    Broadcaster(const std::string& name,
                const std::vector<std::string>& ifs,
                const bool whilelist_mode,
                const std::string& protocol = "",
                const std::string& parameter = "",
                const bool is_upload_server = false);

    Broadcaster(const Broadcaster&) = delete;
    Broadcaster& operator=(const Broadcaster&) = delete;

    ~Broadcaster();

private:
    void thread_function();  ///< Thread function of broadcasting

private:
    const std::string name_;       ///< Name of daemon
    const std::string version_;    ///< Version info of daemon
    const std::string protocol_;   ///< Protocol of upload server, only for upload server
    const std::string parameter_;  ///< Parameter for protocol of upload server, only for upload server

    const std::vector<std::string> ifs_;  ///< Interface name for broadcasting
    const bool whitelist_mode_;           ///< Use ifs as whitelist, or blacklist
    const uint16_t port_;                 ///< Port of broadcasting
    const bool is_upload_server_;         ///<[false] for hera-daemon, [true] for upload server

    std::atomic<bool> running_;  ///< Running flag to control thread
    std::thread* thread_;        ///< Thread of broadcasting
};

}  // namespace daemon
}  // namespace hera
}  // namespace wayz