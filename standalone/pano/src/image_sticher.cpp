///
/// @file extracter.cpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief
/// @date 2020-10-22
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include <iomanip>
#include <iostream>
#include <sstream>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>

#include "extracter.hpp"

void Extracter::sticher_images(std::vector<device::data::SensorDataPtr>& synced_images)
{
    static uint32_t synced_images_counter = 0;
    auto counter_high = synced_images_counter / 10000;
    auto counter_low = synced_images_counter % 10000;
    synced_images_counter++;

    std::stringstream result_path;
    result_path << out_folder_ << "/" << std::setw(5) << std::setfill('0') << counter_high;
    if (counter_low == 0) {
        ::mkdir(result_path.str().c_str(), 0775);
    }
    result_path << "/" << std::setw(5) << std::setfill('0') << counter_low << ".jpg";
    auto result_path_str = result_path.str();

    if (synced_images_counter % 30) {
        return;
    }

    // Write Image out
    for (size_t i = 0; i < synced_images.size(); ++i) {
        const device::data::CompressedImage* image = (device::data::CompressedImage*)synced_images[i].get();
        auto filename = ramdisk_random_folder_ + "/" + camera_names_[i] + ".jpg";

        std::ofstream file(filename.c_str(), std::ios::out | std::ios::binary);
        file.write((const char*)image->image_data, image->image_data_size);
        file.close();
    }

    auto project_file = ramdisk_random_folder_ + "/org.pto";
    auto reoptimized_project_file = ramdisk_random_folder_ + "/opt.pto";

    // // Call Autooptimizer
    // if (synced_images_counter % 200 == 0) {
    //     std::string cmd = "autooptimiser -m -o " + reoptimized_project_file + " " + project_file;
    //     log::info << cmd << log::endl;
    //     auto ret = system(cmd.c_str());
    //     if (ret) {
    //         log::warn << "Call Autooptimiser failed" << log::endl;
    //     }
    // }

    // Call nona
    std::string cmd = "nona -m TIFF_m " + project_file;
    cmd += " -o " + ramdisk_random_folder_ + "/mapped";
    log::info << cmd << log::endl;
    auto ret = system(cmd.c_str());
    if (ret) {
        log::warn << "Call nona failed" << log::endl;
    }

    // Call enblend
    cmd = "enblend -o ";
    cmd += result_path_str;
    cmd += " ";
    cmd += ramdisk_random_folder_ + "/mapped*.tif";
    log::info << cmd << log::endl;
    ret = system(cmd.c_str());
    if (ret) {
        log::warn << "Call enblend failed" << log::endl;
    }
}