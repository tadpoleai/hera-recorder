//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#ifndef __data_message_sensor_data_hpp__
#define __data_message_sensor_data_hpp__
#include <cstdint>

namespace wayz {

struct SensorData {
    int32_t length;
    int32_t sensor_type;
    int32_t sensor_datatype;
    int32_t is_timestamp_synced;
    int64_t timestamp_us;
    int64_t timestamp_recv_us;
    char rawdata[0];
};

}  // namespace wayz
#endif