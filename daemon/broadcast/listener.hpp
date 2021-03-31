///
/// @file broadcast.hpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief Broadcast self's ip and info by UDP
/// @date 2020-05-27
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include <atomic>
#include <functional>
#include <string>
#include <thread>

#include "message.hpp"

namespace wayz {
namespace hera {
namespace daemon {

class Listener final {
public:
    using UploadServerInfoCallback = void (*)(const UploadServerInfoPacket& packet);

public:
    using BroadcastCallback = void (*)(const BroadcastPacket& packet);

public:
    ///
    /// @brief Construct a new Listener object
    ///
    /// @param is_upload_server [false] for listening hera-daemon, [true] for listening upload server
    ///
    Listener(const bool is_upload_server, void* callback);

    Listener(const Listener&) = delete;
    Listener& operator=(const Listener&) = delete;

    ~Listener();

private:
    void thread_function();  ///< Thread function of broadcasting

private:
    const uint16_t port_;          ///< Port of broadcasting
    const bool is_upload_server_;  ///<[false] for hera-daemon, [true] for upload server

    std::atomic<bool> running_;  ///< Running flag to control thread
    std::thread* thread_;        ///< Thread of broadcasting

    void* callback_;
};

}  // namespace daemon
}  // namespace hera
}  // namespace wayz