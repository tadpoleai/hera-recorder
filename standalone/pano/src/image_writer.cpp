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

void Extracter::write_extracted_images(std::vector<device::data::SensorDataPtr>& synced_images)
{
    static uint32_t synced_images_counter = 0;

    auto counter_high = synced_images_counter / 1000;
    auto counter_low = synced_images_counter % 1000;

    synced_images_counter++;

    std::stringstream path;
    path << out_folder_ << "/" << std::setw(4) << std::setfill('0') << counter_high;
    if (counter_low == 0) {
        ::mkdir(path.str().c_str(), 0775);
    }
    path << "/" << std::setw(4) << std::setfill('0') << counter_low;
    auto path_str = path.str();
    ::mkdir(path_str.c_str(), 0775);

    for (size_t i = 0; i < synced_images.size(); ++i) {
        const device::data::CompressedImage* image = (device::data::CompressedImage*)synced_images[i].get();
        auto filename = path_str + "/" + camera_names_[i] + ".jpg";

        std::ofstream file(filename.c_str(), std::ios::out | std::ios::binary);
        file.write((const char*)image->image_data, image->image_data_size);
        file.close();
    }
}