//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#ifndef __data_message_data_imu_hpp__
#define __data_message_data_imu_hpp__
#include <cstdint>

namespace wayz {

struct DataImu {
    float acceleration[3];
    float angular_velocity[3];
    float magnetic_field[3];
};

}  // namespace wayz
#endif