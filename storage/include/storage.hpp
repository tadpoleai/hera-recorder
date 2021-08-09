///
/// @file storage.hpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Header of class StorageManager
/// @version 0.2
/// @date 2019-12-24
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include <atomic>
#include <cstdint>
#include <fstream>
#include <memory>
#include <queue>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#ifdef HERA_COMPILE_IN_REPO
#include "common/include/utils/thread_queue.hpp"
#include "device/include/device_data.hpp"
#else
#include <hera/common/utils/thread_queue.hpp>
#include <hera/device/device_data.hpp>
#endif

#include "storage_data_header.hpp"

namespace wayz {
namespace hera {
namespace storage {

class StorageManager;

///
/// @brief An unique pointer to StorageManager
///
using StorageManagerPtr = std::unique_ptr<StorageManager>;

///
/// @brief Manage storage of hera devices' raw data
///
class StorageManager final {

public:
    ///
    /// @brief Open a storage
    ///
    /// @param filename filename of storage data
    /// @param read_mode true if to read out from storage, otherwise write
    /// @param is_extra bool write/read extra information
    /// @param is_logs bool write/read extra information
    /// @param read_strict bool read deviceData in timestamp re-indexed mode
    ///
    /// @return StorageManagerPtr the unique pointer to StorageManager
    static StorageManagerPtr open(const std::string& filename,
                                  const bool read_mode,
                                  const bool is_extra = true,
                                  const bool is_logs = true,
                                  const bool read_strict = false);

public:
    ///
    /// @brief Close a storage
    ///
    void close();

public:
    ///
    /// @brief Destroy the Storage Manager object
    ///
    ~StorageManager();

    StorageManager(const StorageManager&) = delete;
    StorageManager& operator=(const StorageManager&) = delete;

    ///
    /// @brief Add device to write (write mode)
    ///
    /// @param full_name full name of device
    /// @param history_depth history depth of data
    /// @return true succeed
    /// @return false failed
    ///
    bool add_device(const std::string& full_name, const size_t history_depth);

    ///
    /// @brief Finish adding device to write (write mode)
    ///
    void finish_add_device();

    ///
    /// @brief Add data to storage (write mode)
    ///
    /// @param device_id device id of device
    /// @param data device data to add
    /// @param if_write_data whether to store this data to storage or just add
    /// @return true succeed
    /// @return false failed
    ///
    bool add_data(const uint32_t device_id, device::data::DeviceDataPtr& data, const bool if_write_data);

    ///
    /// @brief Read device data from storage
    ///
    device::data::DeviceDataPtr read();

    device::data::DeviceDataPtrWithIndex read_data_and_index();

    device::data::DeviceDataPtr read_data_by_index(int64_t);

    ///
    /// @brief Seek data header to specific timestamp
    ///
    /// @param ts timestamp to seek to
    /// @return true / false, successfully seek or not
    ///
    /// @note This function should be used before read() in strict mode
    /// @note This function may cosume a lot of time
    bool seek(time::Timestamp ts);

    ///
    /// @brief Tellg (read mode)
    ///
    /// @return int64_t tellg() of input storage file
    int64_t tellg()
    {
        if (read_mode_) {
            return in_file_.tellg();
        } else {
            return 0;
        }
    }

    ///
    /// @brief Get history data (write mode)
    ///
    /// @param device_id device id of device
    /// @return std::vector<DeviceDataPtr> a vector of latest history data
    /// @return empty vector if device id out of range
    std::vector<device::data::DeviceDataPtr> history(const uint32_t device_id) const;

    ///
    /// @brief Clear history data (write mode)
    ///
    /// @param device_id device id of device
    void clear_history(const uint32_t device_id);

    ///
    /// @brief Get total size of data written
    ///
    /// @param device_id device id of device
    /// @return size_t size of data written, in bytes, if write mode
    /// @return 0 if device id out of range
    uint64_t get_volume(const uint32_t device_id) const;

private:
    ///
    /// @brief Construct a new Storage Manager object
    ///
    StorageManager(const std::string& filename,
                   const bool read_mode,
                   const bool is_extra = true,
                   const bool is_logs = true,
                   const bool read_aligned = false);

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

    ///
    /// @brief function of prefetching data (read mode, strict)
    ///
    void prefetch_thread_function();

private:
    static constexpr auto TimeSleepUs = 500;  ///< Time to sleep on write thread when no data

public:
    StorageDataHeaderPtr header;  ///< Header of storage

private:
    std::string filename_;      ///< Storage folder name
    size_t file_size_counter_;  ///< Size of current storage file, in bytes

    bool read_mode_;    ///< Indicating if storage is in read mode
    bool read_strict_;  ///< Read sensor data messages aligned by timestamp

    std::ofstream out_file_;  ///< Current output file, used in write mode
    bool out_file_opened_;    ///< output file (write mode) opened
    std::ifstream in_file_;   ///< Current input file, used in read mode

    bool add_device_finished_;  ///< Finish_add_device called, used in write mode

    std::thread* thread_;  ///< Writing thread handler, used in write mode. or Prefetch thread handler, used in
                           ///< read_strict mode
    std::atomic<bool> thread_running_;  ///< An atomic variable indicating if storage is operating,
                                        /// used in write, or read_strict mode

    ///
    /// @brief Data array used in write mode
    ///
    std::vector<std::unique_ptr<common::ThreadQueue<device::data::DeviceData>>> data_array_;

    ///
    /// @brief Data array used in read_strict mode
    ///
    std::vector<std::queue<device::data::DeviceDataPtrWithIndex>> data_array_prefetch_;
    mutable std::mutex mutex_prefetch_;            ///< mutex for operations in read_strict mode
    mutable std::condition_variable cv_prefetch_;  ///< condition_variable for operations in read_strict mode
    bool prefetch_data_ready_;  ///< either, data is ready to read in data_array_prefetch, or prefetch has ended up
    bool prefetch_ended_;       ///< if prefetch has ended up
    uint64_t read_header_timestamp_;
};

}  // namespace storage
}  // namespace hera
}  // namespace wayz