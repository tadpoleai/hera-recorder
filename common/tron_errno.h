//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#ifndef __tron_error_hpp__
#define __tron_error_hpp__
#include <cstdint>
namespace wayz {

#define LOG_LINE std::cout << "FILE: " << __FILE__ << " LINE: " << __LINE__ << std::endl

enum TronErrno {
    Success = 0,
    InStatusError = 10,

    InvalidSensorType = 100,
    InvalidSensorName = 101,
    InvalidSensorId = 102,

    CanNotOpenEthernetSensor = 203,
    CanNotOpenUsbSensor = 204,
    CanNotOpenTtySensor = 205,
    CanNotOpenSmbusSensor = 206,

    InvalidParameterType = 300,
    InsufficientParameters = 301,
    UnimplementedParameter = 302,
    InvalidParameterValue = 303,

    CanNotConnectSynchronizer = 400,
    GpsTimeShiftOutOfRange = 401,
    CameraTimeShiftOutOfRange = 402,
    LidarTimeShiftOutOfRange = 403,

    StorageFolderAlreadySet = 500,
    CanNotCreateFolder = 501,
    CanNotOpenFile = 502,
    WriteError = 503,

    SensorNotReady = 600,
    SensorAlreadyConnected = 601,
    SensorAlreadyClosed = 602,

    NoNewData = 700,

    Reserved = 9999,
};

}  // namespace wayz
#endif