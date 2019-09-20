//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//
#ifndef __data_message_sensor_and_data_types_hpp__
#define __data_message_sensor_and_data_types_hpp__
#include <cstdint>

#include <common/third_party/enum.h>

#include "data_dummy.hpp"
#include "data_imu.hpp"
#include "data_lidar.hpp"
#include "sensor_data.hpp"

namespace wayz {

enum class SensorStatus : int32_t { Error = 0, Uninited = 1, Inited = 2, Terminated = 3 };

BETTER_ENUM(SensorType, int32_t, Dummy = 1, Imu = 2, Gps = 3, Camera = 4, Lidar = 5)

BETTER_ENUM(SensorDataType,
            int32_t,
            Dummy = 0x100,

            Imu = 0x200,

            Gprmc = 0x300,

            Gpgga = 0x301,

            ImageRaw = 0x400,
            ImageCompressed = 0x401,
            ImageColor = 0x402,
            ImageMono = 0x403,

            LidarVelodyneScan = 0x500)


BETTER_ENUM(SensorParameterType,
            int32_t,
            DummyRate = 0,
            DummyValue = 1,

            IpAddress = 100,
            DataPort = 101,
            TelemetryPort = 102,
            BaudRate = 103,

            Exposure = 200,
            WhiteBalanceRed = 201,
            WhiteBalanceBlue = 202,
            FrameRate = 203)

}  // namespace wayz
#endif