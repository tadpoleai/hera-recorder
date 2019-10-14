//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#pragma once

#include <cstdint>

namespace wayz {
namespace tron {

struct DeviceRawData {
    int32_t length;
    int16_t device_type;
    int16_t device_data_type;
    int64_t sequence;
    int64_t timestamp_receive_ns;
    uint8_t rawdata_buf[0];
};

struct SensorData {
    int32_t length;
    int16_t device_type;
    int16_t device_data_type;
    int64_t sequence;
    int64_t timestamp_receive_ns;
    int64_t timestamp_intrinsic_ns;
    uint8_t data_buf[0];
};

}  // namespace tron
}  // namespace wayz