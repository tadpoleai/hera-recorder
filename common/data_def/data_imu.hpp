//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#pragma once

#include <cstdint>
namespace wayz {
namespace tron {

struct DataImu {
    double angular_velocity[3];
    double linear_acceleration[3];
    double magnetic_field[3];
};

}  // namespace tron
}  // namespace wayz