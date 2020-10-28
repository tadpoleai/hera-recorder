///
/// @file extracter.cpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief
/// @date 2020-10-22
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include <hera/device/include.hpp>

using namespace wayz::hera;

class Extracter final {
public:
    ///
    /// @brief Construct a new Extracter object
    ///
    /// @param src_file source file name of recorded data
    /// @param out_folder output folder
    /// @param start_time [sec] if non-zero, convert data from that time only
    /// @param duration [sec] if non-zero, convert for a certain duration only
    Extracter(const std::string& src_file,
              const std::string& out_folder,
              int32_t start_time,
              int32_t duration,
              bool sticher_flag,
              const std::string& hugin_project,
              const std::string& ramdisk_path);

    ~Extracter();

    ///
    /// @brief Get whether extracter is running
    ///
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

    void add_image(int32_t camera_id, device::data::SensorDataPtr& camera_image);

    void write_extracted_images(std::vector<device::data::SensorDataPtr>& synced_images);

    void sticher_images(std::vector<device::data::SensorDataPtr>& synced_images);

private:
    const std::string out_folder_;  ///< output folder
    const std::string hugin_project_;
    const std::string ramdisk_path_;
    std::string ramdisk_random_folder_;
    const bool sticher_flag_;
    std::atomic<bool> running_;  ///< indicating whether conversion is running
    std::thread* read_thread_;   ///< thread handler of reading hera record

    const int32_t param_start_time_sec_;  ///< [sec] if non-zero, convert data from that time(start time) only
    const int32_t param_duration_sec_;    ///< [sec] if non-zero, convert for a certain duration only
    std::atomic<int64_t> progress_;       ///< duration of data replaying now
    int64_t total_duration_;              ///< total duration of data replaying now
    storage::StorageManagerPtr storage_;  ///< storage manager

private:
    std::vector<int32_t> map_to_camera_id_;  ///< map from device id to camera id, -1 for non-camera
    std::vector<std::string> camera_names_;  ///< camera names
    std::vector<std::vector<device::data::SensorDataPtr>> camera_image_buffers;
};
