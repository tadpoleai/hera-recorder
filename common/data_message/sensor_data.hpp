//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#ifndef __data_message_sensor_data_hpp__
#define __data_message_sensor_data_hpp__
#include <cstdint>

namespace wayz {

struct SensorRawData {
    int32_t length;
    int16_t sensorType;
    int16_t sensorDataType;
    int32_t sequence;
    int64_t timestampReceiveNs;
    uint8_t rawdataBuf[0];
};

struct SensorData {
    int32_t length;
    int16_t sensorType;
    int16_t sensorDataType;
    int32_t sequence;
    int64_t timestampReceiveNs;
    int64_t timestampIntrinsicNs;
    uint8_t dataBuf[0];
};

}  // namespace wayz
#endif