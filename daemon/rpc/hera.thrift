//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

namespace cpp wayz.hera.daemon 

struct Parameter {
    1: required string type;
    2: required string value;
}

struct DeviceData {
    1: required i32 id;
    2: required string type;
    3: required string name;
    5: required bool valid;
    10: required bool isJpeg;
    20: required i32 sequence;
    30: required i32 timeSecond;
    31: required i32 timeNanosecond;
    40: required double frequency;
    50: required i32 dataSizeKB;
    60: required binary data;
}

struct DeviceStatus {
    1: required i32 id;
    2: required string type;
    3: required string name;
    5: required bool forward;
    8: required list<Parameter> essentialParameters;
    9: required list<Parameter> optionalParameters;
    20: required i32 error;
    21: required string reason;
}

struct Device {
    1: required string type;
    2: required string name;
    3: required list<Parameter> essentialParameters;
    4: required list<Parameter> optionalParameters;
    5: required bool forward;
}

struct Profile {
    1: required string name;
    2: required string author;
    10: required list<Device> devices;
}

struct DeviceTypeMeta {
    1: required string name;
    2: required list<string> essentialParameterTypes;
    3: required list<string> optionalParameterTypes;
}

struct Meta {
    1: required list<DeviceTypeMeta> deviceTypeMetas;
}

struct Status {
    1: required bool started;
    2: required bool recording;
    3: required string storageName;
    6: required i32 diskUsedSpaceKB;
    7: required i32 diskTotalSpaceKB;
    10: required list<DeviceStatus> devices;
    20: required list<Profile> profiles;
    21: required i32 profileIndex;
    30: required Meta meta;
}

struct Result {
    1: required i32 error;
    2: required string reason;
    10: required Status status;
}

struct ResultData {
    1: required i32 error;
    2: required string reason;
    10: required list<DeviceData> deviceDatas;
    20: required bool slamResultValid;
    21: required binary slamResult;
    30: required i32 startTimeSec;
    31: required i32 nowTimeSec;
}

service Service
{
    Result get();
    Result start(1:string storageName, 2:bool useSlam);
    Result stop();
    Result record(1:bool on);
    Result adjustParameters(1:i32 id, 2:list<Parameter> parameters);
    Result updateProfiles(1:list<Profile> profiles, 2:i32 profileIndex);

    ResultData getData();
}
