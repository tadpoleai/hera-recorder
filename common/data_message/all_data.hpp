//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//
#ifndef __data_message_all_data_hpp__
#define __data_message_all_data_hpp__
#include <cstdint>

#include <common/third_party/enum.h>

#include "data_dummy.hpp"
#include "data_imu.hpp"
#include "sensor_data.hpp"

namespace wayz {

BETTER_ENUM(SensorDataType,
            int32_t,
            dummy = 0x00,
            imu = 0x10,
            gprmc = 0x20,
            image_raw = 0x30,
            image_compressed = 0x31,
            image_color = 0x32,
            image_mono = 0x33,
            pointcloud = 0x40)

BETTER_ENUM(SensorType, int32_t, dummy = 0, imu = 1, gps = 2, camera = 3, lidar = 4)

}  // namespace wayz
#endif