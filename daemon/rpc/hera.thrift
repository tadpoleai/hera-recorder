//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

namespace cpp wayz.hera.daemon 

// Meta
struct DeviceRule {
    1: required string name;
    4: required string description;
}

struct Meta {
    1: required list<DeviceRule> deviceRules;
    2: required string daemonVersion;
}

// AcquisitionSetting
struct Parameter {
    1: required string type;
    2: required string value;
}

struct Device {
    1: required string type;
    2: required string name;
    3: required list<Parameter> parameters;
    5: required bool forward;
}

struct Profile {
    1: required string name;
    2: required string author;
    10: required list<Device> devices;
}

struct OperatorInfo {
    1: required string operatorName;
    2: required string place;
    3: required bool slam;
}

struct AcquisitionSetting {
    1: required i32 profileIndex;
    2: required list<Profile> profiles;
    3: required OperatorInfo operatorInfo;
}

// AcquisitionStatus
struct AcquisitionStatus {
    1: required i32 error;
    2: required string reason;
    10: required bool started;
    11: required bool recording;
    30: required string storageFileName;
}

// Data
struct SingleDisplayData {
    1: required binary textData;
    2: required binary jpegData;
}

struct DeviceData {
    1: required i32 id;
    2: required string type;
    3: required string name;

    5: required bool forward;

    20: required string health;
    21: required string statusMessage;

    30: required i32 sequence;
    40: required double frequency;
    50: required double dataSize;

    60: required list<SingleDisplayData> dispData;

    70: required string status;
}

struct DataStatus {
    1: required i32 error;
    2: required string reason;
    10: required list<DeviceData> deviceDatas;
    20: required bool slamResultValid;
    21: required binary slamResult;
    30: required i32 startTimeSec;
    31: required i32 nowTimeSec;
}

// AdjustParameter
struct DeviceAndParameters {
    1: required i32 id;
    2: required string type;
    3: required string name;
    4: required string parametersJson;
}

// Storage
struct StorageSingleDevice {
    1: required string typeName;
    2: required i32 messageNum;
    3: required double dataSize;
}

struct StorageRecordFile {
    1: required string name;
    2: required double size;
    10: required string startTime;
    11: required string endTime;
    20: required list<StorageSingleDevice> devices;
}

struct StorageStatus {
    1: required list<StorageRecordFile> storageRecordFiles;
}

// DiskUsageSpace
struct DiskUsageStatus {
    1: required double diskUsedSpace;
    2: required double diskTotalSpace;
}

// Uploads
enum UploadOperationType {
    Start = 0,
    Complete = 1,
    Retry = 2,
    Abort = 3
}

struct UploadRequest {
    1: required string name;
    2: required string remote;
    3: required string extraPath;
    4: required UploadOperationType operationType;
    10: required bool compress;
}

struct UploadProcess {
    1: required bool running;
    2: required bool errored;
    3: required string reason;

    10: UploadRequest request;

    20: required double totalSize;
    21: required double processedSize;
    22: required string speed;
    23: required string eta;
}

struct LocalDisk {
    1: required string name;
    2: required DiskUsageStatus diskUsageStatus;
}

// Log
struct LogMessage {
    1: required i32 index;

    10: required i32 level;
    11: required i32 tsSec;
    12: required i32 tsNsec;

    20: required string message;
}

service Service
{
    // Meta
    Meta getMeta();

    // AcquisitionSetting
    AcquisitionSetting getSetting();
    AcquisitionSetting setProfiles(1: list<Profile> profiles);
    AcquisitionSetting selectProfile(1: i32 profileIndex);
    AcquisitionSetting setOperatorInfo(1: OperatorInfo operatorInfo)

    // AcquisitionControl
    AcquisitionStatus getStatus();
    AcquisitionStatus start();
    AcquisitionStatus stop();
    AcquisitionStatus setRecord(1: bool on);

    // Data
    DataStatus getData();
    DataStatus selectDetailDevice(1: i32 deviceIndex);
    DataStatus clearDetailDevice();

    // Adjust
    list<DeviceAndParameters> getDeviceAndParameterses();
    list<DeviceAndParameters> adjustDeviceParameter(1: i32 deviceIndex, 2: string type, 3: string value);

    // DiskUsage
    DiskUsageStatus getDiskUsageStatus();

    // Storage
    list<StorageRecordFile> getStorage();
    list<StorageRecordFile> deleteStorage(1: list<string> names);

    // Upload
    list<string> getUploadServers();
    list<LocalDisk> getLocalDisks();
    list<string> getLocalDiskFolders(1: list<string> path);
    list<UploadProcess> getUploadProcesses();
    list<UploadProcess> requestUpload(1: list<UploadRequest> requests);

    // Log
    list<LogMessage> latestLogs();
}
