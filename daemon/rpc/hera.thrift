//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

namespace cpp wayz.hera.daemon 

struct Parameter {
    1: required string type;
    2: required string value;
}

struct SingleDisplayData {
    1: required binary textData;
    2: required binary jpegData;
}

struct DeviceData {
    1: required i32 id;
    2: required string type;
    3: required string name;

    20: required i32 sequence;
    40: required double frequency;
    50: required i32 dataSizeKB;

    60: required list<SingleDisplayData> dispData;
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

struct OperatorInfo {
    1: required string storagePath;
    2: required string operatorName;
    3: required string place;
    4: required bool slam;
}

struct Status {
    1: required bool started;
    2: required bool recording;
    3: required OperatorInfo operatorInfo;
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

struct DeviceStorage {
    1: required string typeName;
    2: required i32 messageNum;
    3: required i32 dataSizeKB;
}

struct StorageRecordFile {
    1: required string name;
    2: required i32 sizeKB;
    10: required string startTime;
    11: required string endTime;
    20: required list<DeviceStorage> devices;
}

struct StorageInfo {
    1: required list<StorageRecordFile> storageRecordFiles;

    10: required i32 diskUsedSpaceKB;
    11: required i32 diskTotalSpaceKB;
}

struct UploadRequest {
    1: required string name;
    2: required string remote;
    10: required bool compress;
}

struct UploadProcess {
    1: required bool running;
    2: required bool errored;
    3: required string reason;

    10: UploadRequest request;

    20: required i32 totalSizeKB;
    21: required i32 processedSizeKB;
    22: required string speed;
    23: required string eta;
}

struct UploadInfo {
    1: required list<UploadProcess> uploadProcesses;
    10: required list<string> remoteServers;
}

enum UploadOperationType {
    Start = 0,
    Complete = 1,
    Retry = 2,
    Abort = 3
}

service Service
{
    Result get();
    Result start(1: OperatorInfo operatorInfo);
    Result stop();
    Result record(1: bool on);
    Result adjustParameters(1: i32 id, 2: list<Parameter> parameters);
    Result updateProfiles(1: list<Profile> profiles, 2: i32 profileIndex);

    ResultData getData();

    StorageInfo getStorage();
    StorageInfo deleteStorage(1: string name);

    UploadInfo getUploadInfo();
    UploadInfo operateUpload(1: UploadOperationType op, 2: UploadRequest request);
}
