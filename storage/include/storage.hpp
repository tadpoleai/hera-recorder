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
/// @brief A shared pointer to device data
///
using DeviceDataPtr = std::shared_ptr<device::data::DeviceData>;

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
    ///
    /// @return StorageManagerPtr the unique pointer to StorageManager
    static StorageManagerPtr open(const std::string& filename, const bool read_mode);

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

    ///
    /// @brief Get history data (write mode)
    ///
    /// @param device_id device id of device
    /// @return std::vector<DeviceDataPtr> a vector of latest history data
    /// @return empty vector if device id out of range
    std::vector<device::data::DeviceDataPtr> history(const uint32_t device_id) const;

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
    StorageManager(const std::string& filename, const bool read_mode);

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
    static constexpr auto TimeSleepUs = 500;  ///< Time to sleep on write thread when no data

public:
    StorageDataHeaderPtr header;  ///< Header of storage

private:
    std::string filename_;      ///< Storage folder name
    size_t file_size_counter_;  ///< Size of current storage file, in bytes

    bool read_mode_;          ///< indicating if storage is in read mode
    std::ofstream out_file_;  ///< Current output file, used in write mode
    bool out_file_opened_;    ///< output file (write mode) opened
    std::ifstream in_file_;   ///< Current input file, used in read mode

    bool add_device_finished_;  ///< Finish_add_device called

    std::thread* thread_;               ///< Writing thread handler, used in write mode
    std::atomic<bool> thread_running_;  ///< An atomic variable indicating if storage is operating,
                                        /// used in write mode

    std::vector<std::unique_ptr<common::ThreadQueue<device::data::DeviceData>>> data_array_;  ///< Device datas
};

}  // namespace storage
}  // namespace hera
}  // namespace wayz