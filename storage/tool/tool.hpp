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
    struct Config {
        std::string filename;     ///< filename of input storage data
        std::string outfilename;  ///< filename of output storage data (after trim)
        bool rebuild{false};      ///< flag to rebuild broken header
        bool trim{false};         ///< flag to trim data
        bool print_extra{false};
        bool print_logs{false};
        bool isverbose{false};
        bool isquiet{false};
        double start_time{0};
        double duration{0};
    };

public:
    Tool(const Config& config);

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
    /// @brief thread function for read messsage from storage (for rebuilding)
    ///
    void read_thread_function();

    ///
    /// @brief thread function for read messsage from storage (for trimming)
    ///
    void trim_thread_function();

private:
    std::thread* read_thread_;            ///< thread handler of reading
    std::atomic<bool> running_;           ///< indicating whether conversion is running
    std::atomic<int64_t> progess_size_;   ///< progess of reading (in bytes) now
    int64_t total_size_;                  ///< total size of data reading now
    storage::StorageManagerPtr storage_;  ///< storage manager

    Config config_;
};

}  // namespace storage
}  // namespace hera
}  // namespace wayz