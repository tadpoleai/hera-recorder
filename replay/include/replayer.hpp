///
/// @file replayer.hpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Header of class Replayer
/// @version 0.2
/// @date 2019-12-25
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include <atomic>
#include <memory>
#include <thread>
#include <vector>

#include "common/include/ipc/ipc_queue.hpp"
#include "common/include/utils/folder_content.hpp"
#include "storage/include/storage.hpp"

namespace wayz {
namespace hera {
namespace replay {

///
/// @brief Replay recorded data to IPC queue
///
/// Scan storage folder and detect recorded device data,
/// calls processer and align their messages by timestamp,
/// then write messages to ipc queue
///
class Replayer {
public:
    ///
    /// @brief Construct a new Aligned Replayer object
    ///
    /// @param filename source file name of recorded data
    ///
    Replayer(const std::string& filename,
             const double replay_rate = 1.0,
             const std::string& config_filename = "",
             const uint64_t start_time = 0.0);

    ///
    /// @brief Destroy the Aligned Converter object
    ///
    ///
    ~Replayer();

    ///
    /// @brief Get whether converter is running
    ///
    /// @return true converter is running
    /// @return false conversion over
    inline bool running() const noexcept
    {
        return running_;
    }

    ///
    /// @brief Switch paused
    ///
    inline void pause() noexcept
    {
        paused_ = !paused_;
    }

    ///
    /// @brief Get whether converter is paused
    ///
    inline bool is_paused() const noexcept
    {
        return paused_;
    }

    ///
    /// @brief Force stop
    ///
    inline void stop() noexcept
    {
        running_ = false;
    }

    ///
    /// @brief Get member progress_
    ///
    inline time::Duration progress() const noexcept
    {
        return int64_t(progress_);
    }

    ///
    /// @brief Get total duration of replay
    ///
    inline time::Duration total_duration() const noexcept
    {
        return total_duration_;
    }

private:
    ///
    /// @brief thread function for read messsage from processers and writing ipc queue
    ///
    void replay_thread_function();

private:
    std::thread* replay_thread_;     ///< thread handler of replaying
    const double replay_rate_;       ///< time ratio of replaying
    std::atomic<bool> running_;      ///< indicating whether conversion is running(not terminated) (true even paused)
    std::atomic<bool> paused_;       ///< indicating whether conversion is paused
    std::atomic<int64_t> progress_;  ///< duration of data replaying now
    uint64_t param_start_time_;      ///< skip data time less than this
    int64_t total_duration_;         ///< total duration of data replaying now
    storage::StorageManagerPtr storage_;  ///< storage manager

    std::vector<ipc::IPCQueue<device::data::SensorData>*>
            ipc_ptrs_;  ///< pointer to which ipc queues to use for every device
    std::vector<std::unique_ptr<ipc::IPCQueue<device::data::SensorData>>>
            ipc_queues_;  ///< list of ipc queue for data forwarding;
};

}  // namespace replay
}  // namespace hera
}  // namespace wayz