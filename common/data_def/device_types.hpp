//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//
#pragma once

#include <cstdint>

#include <common/third_party/enum.h>

#include "data_dummy.hpp"
#include "data_imu.hpp"
#include "device_data.hpp"
#include "data_lidar.hpp"

namespace wayz {
namespace tron {

enum class DeviceStatus : int32_t { Error = 0, Uninited = 1, Inited = 2, Terminated = 3 };

BETTER_ENUM(DeviceType, int32_t, Dummy = 1, Imu = 2, Gps = 3, Camera = 4, Lidar = 5)

BETTER_ENUM(DeviceDataType,
            int32_t,
            Dummy = 0x100,

            Imu = 0x200,

            Gprmc = 0x300,

            Gpgga = 0x301,

            ImageRaw = 0x400,
            ImageJpeg = 0x401,

            LidarVelodyneScan = 0x500)


BETTER_ENUM(DeviceParameterType,
            int32_t,
            DummyRate = 0,
            DummyValue = 1,

            IpAddress = 100,
            DataPort = 101,
            TelemetryPort = 102,

            Kernel = 200,
            BaudRate = 201,
            SerialMsgType = 202,

            Exposure = 400,
            WhiteBalanceRed = 401,
            WhiteBalanceBlue = 402,
            FrameRate = 403)
            
}  // namespace tron
}  // namespace wayz