//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#pragma once

#include <cstdint>
#include <vector>

namespace wayz {
namespace tron {

struct LaserPoint
{
    float x;
    float y;
    float z;
    int8_t ring;
    int8_t intensity;
};

struct DataLidar
{
    int8_t sensor_type;
    int32_t point_number;
    LaserPoint points[0];
};


}  // namespace tron
}  // namespace wayz