//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#pragma once

#include <cstdint>
#include <vector>

namespace wayz {
namespace tron {

enum class LidarType : int8_t {
    VelodyneHDL32E = 0x21,
    VelodyneVLP16C = 0x22,
    VelodyneVLP32C = 0x28
};

struct LidarPoint {
    float x;
    float y;
    float z;
    float channel;
    float intensity;
};

struct DataLidar {
    LidarType lidar_type;
    int32_t point_number;
    LidarPoint lidar_points[0];
};


}  // namespace tron
}  // namespace wayz