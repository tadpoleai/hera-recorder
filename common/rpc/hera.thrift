//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

struct DeviceInitializer {
    1:required string type;
    2:required string name;
    3:required map<string, string> parameters;
}

struct DeviceInformation {
    1:required i32 id;
    2:required string type;
    3:required string name;
    4:required i32 status;
    5:required bool forward;
    7:required i32 volume;
    8:required map<string, string> parameters;
    20:required i32 error;
    21:required string reason;
}

struct Status {
    1:required bool inited;
    2:required bool record;
    3:required string folder;
    10:required list<DeviceInformation> devices;
}

struct Result {
    1:required i32 error;
    2:required string reason;
    10:required Status status;
}

struct ProfileResult {
    1:required i32 error;
    2:required string reason;
    10:required string profiles;
}

service AcquisitionManager
{
    Result get();
    Result start(1:list<DeviceInitializer> devices, 2:string storage_folder);
    Result stop();
    Result record(1:bool on);
    Result adjust(1:i32 id, 2:map<string, string> parameters);

    ProfileResult getProfile();
    ProfileResult updateProfile(1:string json);
}
