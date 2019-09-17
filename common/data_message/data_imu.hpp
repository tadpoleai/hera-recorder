//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#ifndef __data_message_data_imu_hpp__
#define __data_message_data_imu_hpp__
#include <cstdint>

namespace wayz {

struct DataImu {
    double AngularVelocity[3];
    double LinearAcceleration[3];
    double MagneticField[3];
};

}  // namespace wayz
#endif