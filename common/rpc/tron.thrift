//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

struct DeviceInitializer {
    1:required string type;
    2:required string name;
    3:required map<string, string> parameter;
}

struct DeviceInformation {
    1:required i32 id;
    2:required string type;
    3:required string name;
    4:required string status;
    5:required bool is_record;
    6:required bool is_forward;
    7:required i32 volume;
    20:required i32 error;
    21:optional string reason;
}

struct Result {
    1:required i32 error;
    2:optional string reason;
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
    list<DeviceInformation> get_informations();
    Result set_storage(1:string folder);
    Result adjust_device_parameters(1:i32 device_id, 2:map<string, string> parameters);
    Result control(1:ControlCommand command, 2:bool to_all, 3: i32 device_id);
}