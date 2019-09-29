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
    4:required string status;
    5:required bool is_forward;
    7:required i32 volume;
    8:required map<string, string> parameters;
    20:required i32 error;
    21:required string reason;
}

struct Status {
    10:required bool can_start;
    11:required bool can_stop;
    12:required bool is_error;
    20:required bool can_record;
    21:required bool can_pause;
    22:required bool is_record;
    30:required string storage_folder;
    40:required list<DeviceInformation> devices;
}

struct Result {
    1:required i32 error;
    2:required string reason;
    10:required Status status;
}

service TronService
{
    Result get_status();

    Result start(1:list<DeviceInitializer> device_initializers, 2:string storage_folder);
    Result stop();
    Result record_or_pause(1:bool is_record);
    Result adjust_device_parameters(1:i32 device_id, 2:map<string, string> parameters);
}