//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#pragma once

#include <iostream>

#ifndef LOG_LINE
#define LOG_LINE std::cout << "file: " << __FILE__ << " , line: " << __LINE__ << std::endl;
#endif

#include <cstdint>
namespace wayz {
namespace hera {

enum HeraErrno {
    Success = 0,
    OK = Success,

    InStatusError = 10,
    ConnectionFailed = 20,
    DeviceInReadMode = 30,
    ErrorReadProfiles = 40,

    InvalidDeviceType = 100,
    InvalidDeviceName = 101,
    InvalidDeviceId = 102,
    InvalidControlCommand = 103,
    EmptyDeviceList = 104,
    DevicesAlreadyCreated = 105,
    UnimplementedDriver = 106,

    CanNotConnectDevices = 202,
    CanNotOpenEthernetDevice = 203,
    CanNotOpenUsbDevice = 204,
    CanNotOpenTtyDevice = 205,
    CanNotOpenSmbusDevice = 206,
    CanNotOpenCamera = 207,
    CanNotOpenCanDevice = 208,

    InvalidParameterType = 300,
    InsufficientParameters = 301,
    UnimplementedParameter = 302,
    InvalidParameterValue = 303,
    ImmutableParameter = 304,

    CanNotConnectSynchronizer = 400,
    GnssTimeShiftOutOfRange = 401,
    CameraTimeShiftOutOfRange = 402,
    LidarTimeShiftOutOfRange = 403,

    StorageFileNoSet = 500,
    StorageFileAlreadySet = 501,
    CanNotCreateFolder = 502,
    CanNotOpenFile = 503,
    WriteError = 504,
    InvalidStorageFileName = 505,

    DeviceNotReady = 600,
    DeviceAlreadyConnected = 601,
    DeviceAlreadyClosed = 602,

    NoNewData = 700,

    Reserved = 9999,
};

}  // namespace hera
}  // namespace wayz