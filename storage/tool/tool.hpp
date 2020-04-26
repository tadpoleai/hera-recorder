///
/// @file tool.hpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief
/// @date 2020-04-26
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///
///

#pragma once

#include <atomic>
#include <memory>
#include <thread>
#include <vector>

#include "common/include/utils/folder_content.hpp"
#include "storage/include/storage.hpp"

namespace wayz {
namespace hera {
namespace storage {

///
/// @brief Storage Tool
///
/// reindex or refix broken header of hera-storage
///
class Tool {
public:
    ///
    /// @brief Construct a new Tool object
    ///
    /// @param filename filename of input storage data
    /// @param rebuild flag to rebuild broken header
    /// @param reindex flag to reindex out-of-ordered timestamp
    /// @param outfilename filename of output storage data (after reindexing)
    ///
    Tool(const std::string& filename, const bool rebuild, const bool reindex, const std::string& outfilename);

    ///
    /// @brief Destroy the Tool object
    ///
    ///
    ~Tool();

    ///
    /// @brief Return if thread is running
    ///
    ///
    inline bool running() const noexcept
    {
        return running_;
    }

    ///
    /// @brief Return progess_size
    ///
    ///
    inline int64_t progess_size() const noexcept
    {
        return progess_size_;
    }

    ///
    /// @brief Return total_size_
    ///
    ///
    inline int64_t total_size() const noexcept
    {
        return total_size_;
    }

private:
    ///
    /// @brief thread function for read messsage from storage
    ///
    void read_thread_function();

private:
    std::thread* read_thread_;            ///< thread handler of reading
    std::atomic<bool> running_;           ///< indicating whether conversion is running
    std::atomic<int64_t> progess_size_;   ///< progess of reading (in bytes) now
    int64_t total_size_;                  ///< total size of data reading now
    storage::StorageManagerPtr storage_;  ///< storage manager

    const bool rebuild_;
    const bool reindex_;
    const std::string filename_;
    const std::string outfilename_;
};

}  // namespace storage
}  // namespace hera
}  // namespace wayz