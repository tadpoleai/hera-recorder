//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//
#ifndef __data_message_all_data_hpp__
#define __data_message_all_data_hpp__
#include <cstdint>

#include "data_dummy.hpp"
#include "data_imu.hpp"
#include "sensor_data.hpp"

namespace wayz {

enum struct SensorDataType : int32_t {
    dummy = 0,
    ins,
    gprmc,
    image_raw,
    image_compressed,
    image_color,
    image_mono,
    pointcloud
};

enum struct SensorType : int32_t { dummy = 0, imu, gps, camera, lidar };

}  // namespace wayz
#endif