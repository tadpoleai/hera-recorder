///
/// @file extracter.cpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief
/// @date 2020-10-22
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include "extracter.hpp"

#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>

std::string generate_random_string(int length)
{
    char tmp;
    std::string buffer;

    std::random_device rd;
    std::default_random_engine random(rd());

    for (int i = 0; i < length; i++) {
        tmp = random() % 36;
        if (tmp < 10) {
            tmp += '0';
        } else {
            tmp -= 10;
            tmp += 'A';
        }
        buffer += tmp;
    }
    return buffer;
}

/// Scan the recorded data,
/// Open the bag file for writing, and init the threads
Extracter::Extracter(const std::string& src_file,
                     const std::string& out_folder,
                     int32_t start_time,
                     int32_t duration,
                     bool sticher_flag,
                     const std::string& hugin_project,
                     const std::string& ramdisk_path) :
    out_folder_(out_folder),
    hugin_project_(hugin_project),
    ramdisk_path_(ramdisk_path),
    ramdisk_random_folder_(""),
    sticher_flag_(sticher_flag),
    running_(false),
    read_thread_(nullptr),
    param_start_time_sec_(start_time),
    param_duration_sec_(duration),
    progress_(0),
    total_duration_(0),
    storage_(storage::StorageManager::open(src_file, true, false, false))
{
    if (storage_->header == nullptr) {
        log::error << "Extracter: Can not open hera record file " << src_file << log::endl;
        return;
    }

    log::info << "Extracter: Printing info\n" << *storage_->header << log::endl;

    total_duration_ = storage_->header->timestamp_end - storage_->header->timestamp_start;

    if (param_start_time_sec_ > 0) {
        log::info << "Extracter: Start Time = " << time::Duration(param_start_time_sec_ * time::OneSecond) << log::endl;
    }
    if ((param_duration_sec_ > 0) && (param_duration_sec_ * time::OneSecond < total_duration_)) {
        log::info << "Extracter: Duration = " << time::Duration(param_duration_sec_ * time::OneSecond) << log::endl;
        total_duration_ = (param_start_time_sec_ + param_duration_sec_) * time::OneSecond;
    }

    int32_t id_iter = 0;
    for (auto& device_name : storage_->header->device_names) {
        std::array<std::string, 3> tokens;
        std::stringstream device_name_ss(std::string(device_name.begin(), device_name.end()));

        for (auto&& token : tokens) {
            if (!getline(device_name_ss, token, '/')) {
                log::error << "Extracter: Can not determine topic name for " << device_name << log::endl;
                return;
            }
        }

        if (tokens[0] == "camera") {
            log::debug << "Extracter: Adding Camera " << device_name << log::endl;
            map_to_camera_id_.emplace_back(id_iter++);
            camera_names_.push_back(tokens[0] + "_" + tokens[2]);
        } else {
            map_to_camera_id_.emplace_back(-1);
        }
    }
    camera_image_buffers.resize(id_iter);

    struct stat info;
    if (stat(out_folder_.c_str(), &info) == 0 && info.st_mode & S_IFDIR) {
        log::warn << "Extracter: Output folder " << out_folder_
                  << " already exists, consider change output name or remove it" << log::endl;
        return;
    }

    auto res = ::mkdir(out_folder_.c_str(), 0775);
    if (res != 0) {
        log::error << "Extracter: Can not create output folder " << out_folder_ << log::endl;
        return;
    }

    if (!sticher_flag_) {
        running_ = true;
        read_thread_ = new std::thread(&Extracter::read_thread_function, this);
        return;
    }

    static constexpr auto RandomRetry = 10;
    static constexpr auto RandomStringLength = 24;
    bool ramdisk_ok = false;
    for (size_t i = 0; i < RandomRetry; ++i) {
        ramdisk_random_folder_ = ramdisk_path_ + "/" + generate_random_string(RandomStringLength);
        auto res = ::mkdir(ramdisk_random_folder_.c_str(), 0775);
        if (res == 0) {
            ramdisk_ok = true;
            break;
        }
    }
    if (!ramdisk_ok) {
        ramdisk_random_folder_ = "";
        log::error << "Extracter: Can not create temp folder in ramdisk " << ramdisk_path_ << log::endl;
        return;
    }

    try {
        std::ifstream src(hugin_project_, std::ios::binary);
        if (!src.is_open()) {
            log::error << "Extracter: Can not read hugin project, given " << hugin_project_ << log::endl;
            return;
        }
        std::ofstream dst(ramdisk_random_folder_ + "/" + "org.pto", std::ios::binary);
        dst << src.rdbuf();
    } catch (std::exception& e) {
        log::error << "Extracter: Can not parse hugin project, given " << hugin_project_ << log::endl;
        return;
    }

    running_ = true;
    read_thread_ = new std::thread(&Extracter::read_thread_function, this);
}
/// Wait for thread to join, and close bag file
///
Extracter::~Extracter()
{
    if (read_thread_) {
        read_thread_->join();
    }

    if (!ramdisk_random_folder_.empty()) {
        rmdir(ramdisk_random_folder_.c_str());
    }
}

void Extracter::read_thread_function()
{
    decltype(storage_->read()) data = nullptr;

    int64_t from_time = 0;
    if (param_start_time_sec_ > 0) {
        from_time = param_start_time_sec_ * time::OneSecond;
    }

    int64_t until_time = 0x7FFF'FFFF'FFFF'FFFFLL;
    if (param_duration_sec_ > 0) {
        until_time = from_time + param_duration_sec_ * time::OneSecond;
    }

    while (running_ && (data = storage_->read())) {
        progress_ = data->get_timestamp_receive_ns() - storage_->header->timestamp_start;
        if (progress_ < from_time) {
            continue;
        }

        if (progress_ > until_time) {
            break;
        }

        auto device_id = data->get_device_id();
        auto camera_id = map_to_camera_id_[device_id];
        if (camera_id >= 0) {
            auto sensor_data = device::Factory::convert(data, {});
            if (sensor_data->sensor_data_type == device::SensorDataType::CompressedImage) {
                add_image(camera_id, sensor_data);
            }
        }
    }

    running_ = false;
}

void Extracter::add_image(int32_t camera_id, device::data::SensorDataPtr& camera_image)
{
    static constexpr int64_t MaxTimestampError = 0.02 * time::OneSecond;
    int64_t timestamp = camera_image->timestamp_intrinsic_ns;

    std::vector<int32_t> matched_other_camera_seq(camera_image_buffers.size(), -1);
    bool all_found = true;

    for (size_t camera_iter = 0; camera_iter < camera_image_buffers.size(); ++camera_iter) {
        if (camera_iter == (size_t)camera_id) {
            continue;
        }

        const auto image_buffer = camera_image_buffers[camera_iter];
        bool found = false;
        for (size_t image_iter = 0; image_iter < image_buffer.size(); ++image_iter) {
            int64_t timestamp_other = image_buffer[image_iter]->timestamp_intrinsic_ns;
            if (abs(timestamp - timestamp_other) < MaxTimestampError) {
                matched_other_camera_seq[camera_iter] = image_iter;
                found = true;
                break;
            }
        }

        if (!found) {
            all_found = false;
            break;
        }
    }

    if (all_found) {
        std::vector<device::data::SensorDataPtr> synced_image_set;
        for (size_t camera_iter = 0; camera_iter < camera_image_buffers.size(); ++camera_iter) {
            auto& buffer = camera_image_buffers[camera_iter];
            if (camera_iter == (size_t)camera_id) {
                synced_image_set.push_back(camera_image);
                buffer.clear();
            } else {
                auto seq = matched_other_camera_seq[camera_iter];
                synced_image_set.push_back(camera_image_buffers[camera_iter][seq]);
                buffer.erase(std::begin(buffer), std::begin(buffer) + seq);
            }
        }
        if (sticher_flag_) {
            sticher_images(synced_image_set);
        } else {
            write_extracted_images(synced_image_set);
        }
    } else {
        camera_image_buffers[camera_id].emplace_back(camera_image);
    }
}
