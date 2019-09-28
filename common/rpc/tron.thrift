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

struct ResultInformation {
    1:required i32 error;
    2:required string reason;
    10:required bool can_create;
    11:required bool can_start;
    12:required bool can_stop;
    13:required bool is_error;
    20:required bool can_record;
    22:required bool can_pause;
    23:required bool is_record;
    30:required bool can_set_storage;
    31:required bool is_storage_set;
    32:required string storage_folder;
    40:required list<DeviceInformation> devices;
}

struct Result {
    1:required i32 error;
    2:required string reason;
}

enum ControlCommand {
    Start = 0,
    Stop = 1,
    StartRecord = 2,
    PauseRecord = 3,
    EnableForward = 4,
    DisableForward = 5,
    Reset = 10
}

service TronService
{
    Result create_devices(1:list<DeviceInitializer> device_initializers);
    ResultInformation get_information();
    Result set_storage(1:string folder);
    Result adjust_device_parameters(1:i32 device_id, 2:map<string, string> parameters);
    Result control(1:ControlCommand command);
}