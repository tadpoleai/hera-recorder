///
/// @file storage.hpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Header of class Storage
/// @version 0.1
/// @date 2019-11-06
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include <cstdint>
#include <fstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "common/utils/thread_queue.hpp"
#include "device_data.hpp"

namespace wayz {
namespace hera {

///
/// @brief A shared pointer to storage data
///
using StorageDataPtr = std::shared_ptr<StorageData>;

///
/// @brief Manage storage of hera devices' raw data
///
class Storage final {

public:
    ///
    /// @brief Construct a new Storage object
    ///
    /// @param folder folder of this storage
    /// @param read_mode true if to read out from storage, otherwise write
    Storage(std::string&& folder, bool read_mode = false);
    Storage(const Storage&) = delete;
    Storage& operator=(const Storage&) = delete;
    ~Storage();

    ///
    /// @brief Write storage data into storage
    ///
    /// @param data a shared pointer to storage data
    void write(StorageDataPtr&& data);

    ///
    /// @brief Read storage data from storage
    ///
    /// @return StorageDataPtr shared pointer to storage data, if read succeed, otherwise nullptr
    StorageDataPtr read();

    ///
    /// @brief Get total size of data written / read
    ///
    /// @return size_t size of data written, in bytes, if write mode
    /// @return size_t size of data read, in bytes, if read mode
    /// @note size is in KiB, not bytes
    inline uint64_t get_volume() const noexcept
    {
        return total_file_size_counter_;
    }

private:
    ///
    /// @brief Create the folder for this storage object
    ///
    /// @return true succeed
    /// @return false failed
    bool create_folder();

    ///
    /// @brief Open a new file to write
    ///
    /// @return true succeed
    /// @return false failed
    bool open_new_file();

    ///
    /// @brief function of writing thread
    ///
    void write_thread_function();

private:
    static constexpr size_t FileMaxSize_ = 1'000'000'000;  ///< Max size a single storage file
    static constexpr size_t FileNameWidth_ = 4;            ///< Storage file name width
    std::string folder_;                                   ///< Storage folder name
    size_t file_number_counter_;      ///< Storage file count of current writing
    size_t file_size_counter_;        ///< Size of current storage file, in bytes
    size_t total_file_size_counter_;  ///< Size of all this storage, in bytes, if write mode
                                      ///, other wise read storage data size, if read mode

    bool read_mode_;                ///< indicating if storage is in read mode
    std::ofstream out_file_;        ///< Current output file, used in write mode
    std::thread* thread_;           ///< Writing thread handler, used in write mode
    volatile bool thread_running_;  ///< A volatile variable indicating if storage is operating,
                                    /// used in write mode
    ThreadQueue<StorageData> thread_queue_;  ///< Thread-safe queue for buffering storage data,
                                             /// used in write mode
    std::ifstream in_file_;                  ///< Current input file, used in read mode
};

}  // namespace hera
}  // namespace wayz