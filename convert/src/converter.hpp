///
/// @file converter.hpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Header of class Converter
/// @version 0.1
/// @date 2019-11-13
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <semaphore.h>
#include <thread>
#include <vector>

#include "common/include/utils/remapper.hpp"
#include "common/include/utils/time.hpp"
#include "direct_bag/direct_bag.h"
#include "ros_message.hpp"
#include "storage/include/storage.hpp"

namespace wayz {
namespace hera {
namespace convert {

///
/// @brief Convert storage folder to ROS Bag
///
class Converter {
public:
    ///
    /// @brief Construct a new Converter object
    ///
    /// @param src_filename source file name of recorded data
    /// @param bagfile output bag file name
    /// @param remapper a remapper for remapping frame_id and topic_name, must not be nullptr
    /// @param parameter_tuple_list list of parameters override map, tuple<DeviceName/Category/ID, ParamType,
    /// ParamValue>
    /// @param start_time [sec] if non-zero, convert data from that time only
    /// @param duration [sec] if non-zero, convert for a certain duration only
    Converter(const std::string& src_filename,
              const std::string& bagfile,
              common::RemapperPtr&& remapper,
              const std::vector<std::tuple<std::string, std::string, std::string>>& parameter_tuple_list,
              const int32_t start_time,
              const int32_t duration);

    ///
    /// @brief Destroy the Converter object
    ///
    ~Converter();

    ///
    /// @brief Get whether converter is running
    ///
    /// @return true converter is running
    /// @return false conversion over
    inline bool running() const noexcept
    {
        return bool(running_);
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
    /// @brief Get member total duration_
    ///
    inline time::Duration total_duration() const noexcept
    {
        return total_duration_;
    }

private:
    ///
    /// @brief thread function for reading hera record
    ///
    void read_thread_function();

    ///
    /// @brief thread function for writing bag
    ///
    void bag_thread_function();

    ///
    /// @brief Publish a message
    ///
    /// @param in_message a pointer to ROSMessage to publish
    /// @note This operation called by read thread using semaphore to invoke global bag access
    /// @note This operation move a ROSMessage to member 'message', and then bag thread can receive and write
    void publish(ROSMessagePtr&& in_message);

    ///
    /// @brief receive a message
    ///
    /// @return a pointer to ROSMessage
    /// @note This operation called by bag thread using semaphore to invoke global bag access
    /// @note This operation move a ROSMessage from member 'message', and then read thread can publish a new
    /// one
    ROSMessagePtr receive();

private:
    const std::string bagfile_;           ///< outfile name
    std::atomic<bool> running_;           ///< indicating whether conversion is running
    std::thread* read_thread_;            ///< thread handler of reading hera record
    std::thread* bag_thread_;             ///< thread handler of writing bag
    rosbag_direct_write::DirectBag bag_;  ///< output ROS bag handler
    common::RemapperPtr remapper_;        ///< the remapper for remapping frame_id and topic_name

    const int32_t param_start_time_sec_;       ///< [sec] if non-zero, convert data from that time(start time) only
    const int32_t param_duration_sec_;         ///< [sec] if non-zero, convert for a certain duration only
    std::atomic<int64_t> progress_;            ///< duration of data replaying now
    int64_t total_duration_;                   ///< total duration of data replaying now
    storage::StorageManagerPtr storage_;       ///< storage manager
    std::vector<std::string> topic_prefixes_;  ///< topic prefixes of sensors, i.e., "/lidar/top/"
    std::vector<std::string> frame_ids_;       ///< (remapped) frame ids of sensors

    ///
    /// @brief Semaphore for publish / conversion
    ///
    /// Wait this semaphore before a new message can be published,
    sem_t* publish_sem_;

    ///
    /// @brief Semaphore for bag receiving / bagfile writing
    ///
    /// Wait this semaphore before message can be read from member 'message'
    sem_t* receive_sem_;

    ///
    /// @brief Published message
    ///
    /// @note Both R/W access to it should protected by semaphore
    ROSMessagePtr message;
};

}  // namespace convert
}  // namespace hera
}  // namespace wayz
