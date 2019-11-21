///
/// @file device.cpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Implementation of class Device
/// @version 0.1
/// @date 2019-11-06
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include "device.hpp"

namespace wayz {
namespace hera {

Device::Device(DeviceIdType id,
               const std::string& vendor_type,
               const std::string& name,
               const std::string& folder,
               bool read_mode,
               std::initializer_list<DeviceParameterType>&& essential_parameter_types) :
    sequence_(0),
    id_(id),
    type_(vendor_type),
    name_(name),
    folder_(folder),
    read_mode_(read_mode),
    status_(DeviceStatus::BeforeConnect),
    hera_errno_(HeraErrno::OK),
    is_record_(false),
    storage_(nullptr),
    thread_fetch_(nullptr),
    essential_parameter_types_(std::move(essential_parameter_types))
{}

Device::~Device() {}

/// Start to connect to device.
/// If device is in DeviceStatus::BeforeConnect,
/// this function will check essential parameters first,
/// then call abstract function connect(),
/// after that create a storage object.
/// If device is in other status, return corresponding error code
HeraErrno Device::start()
{
    if (read_mode_) {
        return HeraErrno::DeviceInReadMode;
    }

    switch (status_) {
    case DeviceStatus::BeforeConnect: {
        if (!check_parameter()) {
            return HeraErrno::InsufficientParameters;
        }
        auto result = connect();
        if (result != HeraErrno::OK) {
            return result;
        }
        status_ = DeviceStatus::Connected;
        storage_ = new Storage(folder_ + "/" + type_ + "/" + name_);
        thread_fetch_ = new std::thread(&Device::fetch_thread_function, this);
        return HeraErrno::OK;
    }

    case DeviceStatus::Connected:
        return HeraErrno::DeviceAlreadyConnected;

    case DeviceStatus::Terminated:
        return HeraErrno::DeviceAlreadyClosed;

    case DeviceStatus::Error:
    default:
        return HeraErrno::InStatusError;
    }
}

/// Stop all resources and thread in this device.
/// If device is not in DeviceStatus::Terminated,
/// then set status to DeviceStatus::Terminated,
/// and then stop recording and join fetching thread,
/// after that, close storage
void Device::stop()
{
    if (status_ != DeviceStatus::Terminated) {
        status_ = DeviceStatus::Terminated;
        is_record_ = false;
        if (thread_fetch_ != nullptr) {
            thread_fetch_->join();
            delete thread_fetch_;
            thread_fetch_ = nullptr;
        }
        if (storage_ != nullptr) {
            delete storage_;
            storage_ = nullptr;
        }
        if (!read_mode_) {
            disconnect();
        }
    }
}

/// Change is_record_.
/// If device status is DeviceStatus::Connected,
/// set new value of is_record_,
/// otherwise, return corresponding error code
HeraErrno Device::record(bool value)
{
    switch (status_) {
    case DeviceStatus::BeforeConnect:
        return HeraErrno::DeviceNotReady;

    case DeviceStatus::Connected:
        is_record_ = value;
        return HeraErrno::OK;

    case DeviceStatus::Terminated:
        return HeraErrno::DeviceAlreadyClosed;

    case DeviceStatus::Error:
    default:
        return HeraErrno::InStatusError;
    }
}

/// Check if argument type(string) in valid,
/// and then add it to protected member parameters_,
/// in additional, call adjust_parameter() if device is connected
HeraErrno Device::parameter(const std::string& type, const std::string& value)
{
    auto status = status_;
    if (status == DeviceStatus::Error) {
        return HeraErrno::InStatusError;
    }
    if (status == DeviceStatus::Terminated) {
        return HeraErrno::DeviceAlreadyClosed;
    }
    auto parameter_type = DeviceParameterType::_from_string_nocase_nothrow(type.c_str());
    if (!parameter_type) {
        return HeraErrno::InvalidParameterType;
    } else {
        auto result = HeraErrno::OK;
        if (status == DeviceStatus::Connected) {
            result = adjust_parameter(parameter_type.value(), value);
        }
        if (result == OK) {
            parameters_[parameter_type.value()] = value;
        }
        return result;
    }
}

/// Read a StorageData from storage object and then
/// calls convert() to convert it into SensorData
SensorDataPtr Device::read()
{
    if (!read_mode_) {
        return nullptr;
    }

    if (storage_ == nullptr) {
        storage_ = new Storage(folder_ + "/" + type_ + "/" + name_, true);
    }

    auto storage_data = storage_->read();
    if (storage_data == nullptr) {
        return nullptr;
    } else {
        return convert(std::move(storage_data));
    }
}

/// Convert parameters_ from enum:string map
/// to string:string map
OutParametersMap Device::get_parameters() const
{
    OutParametersMap parameters;
    for (const auto& parameter : parameters_) {
        parameters[parameter.first._to_string()] = parameter.second;
    }
    return parameters;
}


/// Set status to DeviceStatus::Error, and set reason
///
HeraErrno Device::handle_error(HeraErrno e, std::string&& reason)
{
    status_ = DeviceStatus::Error;
    hera_errno_ = e;
    reason_ = std::forward<std::string>(reason);
    return e;
}

/// Check if every parameter_type in essential_parameter_types
/// is settled with define_parameter() by testing parameters
bool Device::check_parameter()
{
    for (const auto& type : essential_parameter_types_) {
        if (!parameters_.count(type)) {
            return false;
        }
    }
    return true;
}

/// Check the status, if in DeviceStatus::Connected
/// repeatly call abstract function fetch(),
/// if it returns a non-nullptr value, pass it to storage
void Device::fetch_thread_function()
{
    while (status_ == DeviceStatus::Connected) {
        auto data = fetch();
        if (data != nullptr) {
            storage_->write(std::move(data));
        }
    }
}

}  // namespace hera
}  // namespace wayz