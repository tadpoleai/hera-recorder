//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#pragma once

#include <iostream>
#define LOG_LINE std::cout<< "file: " << __FILE__ << " , line: " << __LINE__ <<std::endl;
//#define LOG_LINE  

#include <cstdint>
namespace wayz {
namespace tron {

enum TronErrno {
    Success = 0,
    InStatusError = 10,
    ConnectionFailed = 20,

    InvalidDeviceType = 100,
    InvalidDeviceName = 101,
    InvalidDeviceId = 102,
    InvalidControlCommand = 103,
    EmptyDeviceList = 104,
    DevicesAlreadyCreated = 105,

    CanNotOpenEthernetDevice = 203,
    CanNotOpenUsbDevice = 204,
    CanNotOpenTtyDevice = 205,
    CanNotOpenSmbusDevice = 206,

    InvalidParameterType = 300,
    InsufficientParameters = 301,
    UnimplementedParameter = 302,
    InvalidParameterValue = 303,

    CanNotConnectSynchronizer = 400,
    GpsTimeShiftOutOfRange = 401,
    CameraTimeShiftOutOfRange = 402,
    LidarTimeShiftOutOfRange = 403,

    StorageFolderNoSet = 500,
    StorageFolderAlreadySet = 501,
    CanNotCreateFolder = 502,
    CanNotOpenFile = 503,
    WriteError = 504,

    DeviceNotReady = 600,
    DeviceAlreadyConnected = 601,
    DeviceAlreadyClosed = 602,

    NoNewData = 700,

    Reserved = 9999,
};

}  // namespace tron
}  // namespace wayz