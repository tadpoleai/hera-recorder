///
/// @file aligned_converter.hpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Header of class AlignedConverter
/// @version 0.1
/// @date 2019-11-13
///
/// @copyright Copyright (c) 2019
///

#pragma once
#include <memory>
#include <mutex>
#include <semaphore.h>
#include <thread>
#include <vector>

#include "common/utils/get_folder_content.hpp"
#include "common/utils/remapper.hpp"
#include "direct_bag/direct_bag.h"
#include "processer.hpp"
#include "ros_message.hpp"

namespace wayz {
namespace hera {
namespace convert {

///
/// @brief Convert storage folder to ROS Bag
///
/// Scan storage folder and detect recorded device data,
/// calls processer and align their messages by timestamp,
/// then write messages to bag file
///
class AlignedConverter {
public:
    ///
    /// @brief Construct a new Aligned Converter object
    ///
    /// @param folder folder name of recorded data
    /// @param bagfile output bag file name
    /// @param remapper a remapper for remapping frame_id and topic_name
    AlignedConverter(const std::string& folder, const std::string& bagfile, RemapperPtr&& remapper);

    ///
    /// @brief Destroy the Aligned Converter object
    ///
    ~AlignedConverter();

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
    /// @brief thread function for read messsage from processers and writing bag
    ///
    void write_thread_function();

private:
    std::thread* write_thread_;             ///< thread handler of writing
    std::vector<ProcesserPtr> processers_;  ///< array of processers
    volatile bool running_;                 ///< indicating whether conversion is running
    FileSize total_size_;                   ///< file size of total source data, in bytes

    rosbag_direct_write::DirectBag bag_;  ///< output ROS bag handler
    RemapperPtr remapper_;                ///< the remapper for remapping frame_id and topic_name
};

}  // namespace convert
}  // namespace hera
}  // namespace wayz