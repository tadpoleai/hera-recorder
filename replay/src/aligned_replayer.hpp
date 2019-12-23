///
/// @file aligned_replayer.hpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Header of class AlignedReplayer
/// @version 0.1
/// @date 2019-12-19
///
/// @copyright Copyright (c) 2019
///

#pragma once
#include <memory>
#include <mutex>
#include <semaphore.h>
#include <thread>
#include <vector>

#include "common/ipc/ipc_queue.hpp"
#include "common/utils/get_folder_content.hpp"
#include "processer.hpp"

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
class AlignedReplayer {
public:
    ///
    /// @brief Construct a new Aligned Replayer object
    ///
    /// @param folder folder name of recorded data
    ///
    AlignedReplayer(const std::string& folder, const double replay_rate = 1.0);

    ///
    /// @brief Destroy the Aligned Converter object
    ///
    ///
    ~AlignedReplayer();

    ///
    /// @brief Get file size of converted source data, in bytes
    ///
    /// @return FileSize file size of converted source data, in bytes
    FileSize converted_size() const;

    ///
    /// @brief Get file size of total source data, in bytes
    ///
    /// @return FileSize file size of total source data, in bytes
    FileSize total_size() const noexcept;

    ///
    /// @brief Get whether converter is running
    ///
    /// @return true converter is running
    /// @return false conversion over
    bool running() const noexcept;

private:
    ///
    /// @brief thread function for read messsage from processers and writing ipc queue
    ///
    void write_thread_function();

private:
    std::thread* write_thread_;                                           ///< thread handler of writing
    const double replay_rate_;                                            ///< time ratio of replaying
    std::vector<ProcesserPtr> processers_;                                ///< array of processers
    std::vector<std::unique_ptr<ipc::IPCQueue<SensorData>>> ipc_queues_;  ///< array of ipc_queues_;
    volatile bool running_;                                               ///< indicating whether conversion is running
    FileSize total_size_;                                                 ///< file size of total source data, in bytes
};

}  // namespace replay
}  // namespace hera
}  // namespace wayz